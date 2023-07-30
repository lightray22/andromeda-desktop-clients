#ifndef LIBA2_OBJECTDATABASE_H_
#define LIBA2_OBJECTDATABASE_H_

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "BaseObject.hpp"
#include "DatabaseException.hpp"
#include "MixedValue.hpp"
#include "QueryBuilder.hpp"
#include "SqliteDatabase.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/OrderedMap.hpp"
#include "andromeda/Utilities.hpp"
#include "fieldtypes/BaseField.hpp"

namespace Andromeda {
namespace Database {

/**
 * Provides the interfaces between BaseObject and the underlying database
 *
 * Basic functions include loading, caching, updating, creating, and deleting objects.
 * This class should only be used internally by BaseObjects.
 * 
 * Mostly ported from the server's PHP implementation, but simplified
 * without support for object inheritance or object caching by unique key
 * 
 * THREAD SAFE (INTERNAL LOCKS)
 */
class ObjectDatabase
{
private:
    mutable Debug mDebug;

public:

    /** Exception indicating that multiple objects were loaded for by-unique query */
    class MultipleUniqueKeyException : public DatabaseException { public:
        explicit MultipleUniqueKeyException(const std::string& className) : 
            DatabaseException("Multiple unique objects: "+className) {}; };

    /** Exception indicating that the row update failed */
    class UpdateFailedException : public DatabaseException { public:
        explicit UpdateFailedException(const std::string& className) : 
            DatabaseException("Object row update failed: "+className) {}; };

    /** Exception indicating that the row insert failed */
    class InsertFailedException : public DatabaseException { public:
        explicit InsertFailedException(const std::string& className) : 
            DatabaseException("Object row insert failed: "+className) {}; };

    /** Exception indicating that the row delete failed */
    class DeleteFailedException : public DatabaseException { public:
        explicit DeleteFailedException(const std::string& className) : 
            DatabaseException("Object row delete failed: "+className) {}; };

    explicit ObjectDatabase(SqliteDatabase& db) : 
        mDebug(__func__,this), mDb(db) { }
    
    virtual ~ObjectDatabase() = default;
    DELETE_COPY(ObjectDatabase)
    DELETE_MOVE(ObjectDatabase)

    /** Notify the DB that the given object needs to be updated */
    void notifyModified(BaseObject& object);

    /** 
     * Insert created objects, update all objects that notified us as needing it 
     * Done as an ATOMIC LOCKED TRANSACTION - if an exception is thrown, nothing is changed
     * @throws TransactionException if already in a transaction
     * @throws DatabaseException if any myriad of things go wrong
     */
    void SaveObjects();

    /** Return the number of loaded objects (not counting newly created) */
    inline size_t getLoadedCount() const { return mObjects.size(); }

    /** Return the database table name for a class */
    static std::string GetClassTableName(const std::string& className);

    /**
     * Counts objects using the given query (ignores limit/offset)
     * @tparam T type of object
     * @param query query used to match objects
     */
    template<class T>
    size_t CountObjectsByQuery(const QueryBuilder& query)
    {
        MDBG_INFO("(T:" << T::GetClassNameS() << ")"); // no locking required

        static const std::string prop { "COUNT(id)" };
        static const std::string table { GetClassTableName(T::GetClassNameS()) };
        const std::string querystr { "SELECT "+prop+" FROM "+table+" "+query.GetText() };

        SqliteDatabase::RowList rows;
        mDb.query(querystr, query.GetParams(), rows);
        return static_cast<size_t>(rows.front().at(prop).get<int>());
    }

    /**
     * Loads a list of objects matching the given query
     * @tparam T type of object
     * @param query query used to match objects
     * @return list of non-null pointers to objects
     */
    template<class T>
    std::list<T*> LoadObjectsByQuery(const QueryBuilder& query)
    {
        MDBG_INFO("(T:" << T::GetClassNameS() << ")"); // no locking required
        
        static const std::string table { GetClassTableName(T::GetClassNameS()) };
        const std::string querystr { "SELECT * FROM "+table+" "+query.GetText() };

        SqliteDatabase::RowList rows;
        mDb.query(querystr, query.GetParams(), rows);

        std::list<T*> objects;
        for (const SqliteDatabase::Row& row : rows)
            objects.emplace_back(&ConstructObject<T>(row));
        return objects;
    }

    /**
     * Immediately delete objects matching the given query
     * The objects will be loaded and have their NotifyDeleted() run
     * @tparam T type of object
     * @param query query used to match objects
     * @return size_t number of deleted objects
     */
    template<class T>
    size_t DeleteObjectsByQuery(const QueryBuilder& query)
    {
        MDBG_INFO("(T:" << T::GetClassNameS() << ")"); // no locking required
        
        // no RETURNING, just load the objects and delete individually
        // TODO FUTURE will be supported in sqlite 3.35

        const std::list<T*> objs { LoadObjectsByQuery<T>(query) };
        for (T* obj : objs) DeleteObject(*obj);
        return objs.size();
    }

    /**
     * Loads a unique object matching the given query
     * @see LoadObjectsByQuery()
     * @throws MultipleUniqueKeyException if > 1 object is found
     * @return possibly null pointer to an object
     */
    template<class T>
    T* TryLoadUniqueByQuery(const QueryBuilder& query)
    {
        MDBG_INFO("(T:" << T::GetClassNameS() << ")");
        
        const std::list<T*> objs { LoadObjectsByQuery<T>(query) };
        if (objs.size() > 1) throw MultipleUniqueKeyException(T::GetClassNameS()); // cppcheck-suppress knownConditionTrueFalse
        return !objs.empty() ? objs.front() : nullptr; // cppcheck-suppress knownConditionTrueFalse
    }

    /**
     * Deletes a unique object matching the given query
     * @see DeleteObjectsByQuery()
     * @throws MultipleUniqueKeyException if > 1 object is found
     * @return true if an object was found
     */
    template<class T>
    bool TryDeleteUniqueByQuery(const QueryBuilder& query)
    {
        MDBG_INFO("(T:" << T::GetClassNameS() << ")");
        
        const size_t count { DeleteObjectsByQuery<T>(query) };
        if (count > 1) throw MultipleUniqueKeyException(T::GetClassNameS());
        return count != 0;
    }

    /**
     * Creates a new object and registers it to be inserted
     * @tparam T type of object
     * @return T& ref to new object
     */
    template<class T>
    T& CreateObject()
    {
        const UniqueLock lock(mMutex);

        std::unique_ptr<T> objptr { std::make_unique<T>(*this, MixedParams()) };
        T& retobj { *objptr.get() };
        retobj.InitializeID();

        MDBG_INFO("(T:" << T::GetClassNameS() << " id:" << retobj.ID() << ")");

        mCreated.enqueue_back(&retobj, std::move(objptr));
        return retobj; // cppcheck-suppress returnReference
    }

    /** 
     * Immediately deletes a single object from the database 
     * @throws DeleteFailedException if nothing is deleted
     */
    void DeleteObject(BaseObject& object);

    /**
     * INSERTS or UPDATES the given object as necessary
     * @param fields list of modified fields to send
     * @throws UpdateFailedException if UPDATE does nothing
     * @throws InsertFailedException if INSERT does nothing
     */
    void SaveObject(BaseObject& object, const BaseObject::FieldList& fields);

private:

    using UniqueLock = std::unique_lock<std::recursive_mutex>;

    /** 
     * Sends an UPDATE for the given object
     * @param fields list of modified fields to send
     * @throws UpdateFailedException if UPDATE does nothing
    */
    void UpdateObject_Query(BaseObject& object, const BaseObject::FieldList& fields, const UniqueLock& lock);
    /** Updates data structures to indicate the object is updated */
    void UpdateObject_Register(BaseObject& object, const UniqueLock& lock);

    /** 
     * Sends an INSERT for the given object
     * @param fields list of modified fields to send
     * @throws InsertFailedException if INSERT does nothing
    */
    void InsertObject_Query(BaseObject& object, const BaseObject::FieldList& fields, const UniqueLock& lock);
    /** Updates data structures to indicate the object is inserted */
    void InsertObject_Register(BaseObject& object, const UniqueLock& lock);

    /** OrderedMap of objects that have notified us of being modified, indexed by pointer */
    OrderedMap<BaseObject*, BaseObject&> mModified;
    /** OrderedMap of objects that were newly created and not yet saved, indexed by pointer */
    OrderedMap<BaseObject*, std::unique_ptr<BaseObject>> mCreated;

    /** map of loaded objects (to prevent duplicates) indexed by ID:ClassName */
    std::unordered_map<std::string, std::unique_ptr<BaseObject>> mObjects;

    /**
     * Constructs a new object from a row if not already loaded
     * @tparam T type of object
     * @param row database row of fields
     * @return ref to loaded object
     */
    template<class T>
    T& ConstructObject(const SqliteDatabase::Row& row)
    {
        const UniqueLock lock(mMutex);

        std::string id; row.at("id").get_to(id);
        const std::string key { id+":"+T::GetClassNameS() };
        MDBG_INFO("(T:" << T::GetClassNameS() << " id:" << id << ")");

        const decltype(mObjects)::iterator it { mObjects.find(key) };
        if (it != mObjects.end()) // already loaded, don't replace it
            return *dynamic_cast<T*>(it->second.get());
        else
        {
            std::unique_ptr<T> objptr { std::make_unique<T>(*this, row) };
            T& retobj { *objptr.get() };

            mObjects.emplace(key, std::move(objptr));
            return retobj; // cppcheck-suppress returnReference
        }
    }

    SqliteDatabase& mDb;
    /** Primary mutex used to protect data structures */
    std::recursive_mutex mMutex;

    /** Struct used to get a UniqueLock and store a pointer to it */
    class UniqueLockStore
    {
    public:
        /** Locks the mutex and stores a pointer to the UniqueLock */
        UniqueLockStore(std::recursive_mutex& mutex, const UniqueLock*& lockPtrRef) :
            mUniqueLock(mutex), mLockPtrRef(lockPtrRef) { lockPtrRef = &mUniqueLock; }
        virtual ~UniqueLockStore() { mLockPtrRef = nullptr; }
        DELETE_COPY(UniqueLockStore)
        DELETE_MOVE(UniqueLockStore)
    private:
        const UniqueLock mUniqueLock;
        const UniqueLock*& mLockPtrRef;
    };
    /** Pointer to lock held if doing an atomic multiple save/delete */
    const UniqueLock* mAtomicOp { nullptr };
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_OBJECTDATABASE_H_
