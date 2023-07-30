
#include "MixedValue.hpp"
#include "ObjectDatabase.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
void ObjectDatabase::notifyModified(BaseObject& object)
{ 
    const UniqueLock lock(mMutex);

    if (!mModified.exists(&object))
        mModified.enqueue_back(&object, object);
}

/*****************************************************/
void ObjectDatabase::SaveObjects()
{
    const UniqueLockStore lock(mMutex, mAtomicOp);

    MDBG_INFO("() created:" << mCreated.size() << " modified:" << mModified.size());

    mDb.transaction([&]()
    {
        // insert new objects first for foreign keys
        for (decltype(mCreated)::value_type& pair : mCreated) pair.second->Save();

        // check mCreated to avoid saving created objects twice
        for (decltype(mModified)::value_type& pair : mModified) 
            if (!mCreated.exists(&pair.second)) pair.second.Save();
    });

    // all queries are done, update data structures
    while (!mCreated.empty()) InsertObject_Register(*mCreated.front().second, *mAtomicOp);
    while (!mModified.empty()) UpdateObject_Register(mModified.front().second, *mAtomicOp);
}

/*****************************************************/
std::string ObjectDatabase::GetClassTableName(const std::string& className)
{
    Utilities::StringList pieces { Utilities::explode(className,"\\") };
    pieces.erase(pieces.begin()); // no Andromeda_ prefix

    return std::string("a2obj_")+Utilities::tolower(Utilities::implode("_",pieces));
}

/*****************************************************/
void ObjectDatabase::DeleteObject(BaseObject& object)
{
    const UniqueLock lock(mMutex);
    MDBG_INFO("(object:" << static_cast<std::string>(object) << ")");

    if (mCreated.exists(&object))
    {
        MDBG_INFO("... deleting created!");
        object.NotifyDeleted();
        mModified.erase(&object);
        mCreated.erase(&object);
    }
    else
    {
        const std::string table { GetClassTableName(object.GetClassName()) };

        // DELETE FROM $table $query
        std::string qstr("DELETE FROM "); qstr+=table; qstr+=" WHERE id=:id"; // += for efficiency

        if (mDb.query(qstr, {{":id",object.ID()}}) != 1)
            throw DeleteFailedException(object.GetClassName());
        
        object.NotifyDeleted();
        mModified.erase(&object);

        const std::string objkey { object.ID()+":"+object.GetClassName() };
        mObjects.erase(objkey);
    }
}

/*****************************************************/
void ObjectDatabase::SaveObject(BaseObject& object, const BaseObject::FieldList& fields)
{
    UniqueLock lock; if (!mAtomicOp) lock = UniqueLock(mMutex);
    const UniqueLock& lockRef { mAtomicOp ? *mAtomicOp : lock };
    
    if (mCreated.exists(&object))
    {
        InsertObject_Query(object, fields, lockRef);
        if (!mAtomicOp) InsertObject_Register(object, lockRef);
    }
    else
    {
        UpdateObject_Query(object, fields, lockRef);
        if (!mAtomicOp) UpdateObject_Register(object, lockRef);
    }
}

/*****************************************************/
void ObjectDatabase::UpdateObject_Query(BaseObject& object, const BaseObject::FieldList& fields, const UniqueLock& lock)
{
    MDBG_INFO("(object:" << static_cast<std::string>(object) << ")");

    MixedParams data {{":id", object.ID()}};
    std::list<std::string> sets; size_t i { 0 };

    for (FieldTypes::BaseField* field : fields)
    {
        const std::string key { field->GetName() };
        const MixedValue val { field->GetDBValue() };
        
        if (val.is_null())
        {
            sets.emplace_back(key+"=NULL");
            MDBG_INFO("... " << key << " is NULL");
        }
        else if (field->UseDBIncrement())
        {
            const std::string istr { std::to_string(i) };
            std::string s(key); s+="="; s+=key; s+="+:d"; s+=istr; // += for efficiency
            sets.emplace_back(s); data.emplace(":d"+istr, val); ++i;
            MDBG_INFO("... " << key << "+=:d" << istr << "(" << val.ToString() << ")");
        }
        else
        {
            const std::string istr { std::to_string(i) };
            std::string s(key); s+="=:d"; s+=istr; // += for efficiency
            sets.emplace_back(s); data.emplace(":d"+istr, val); ++i;
            MDBG_INFO("... " << key << "=:d" << istr << "(" << val.ToString() << ")");
        }
    }

    if (sets.empty()) { MDBG_INFO("... nothing to do!"); return; }

    const std::string setstr { Utilities::implode(", ",sets) };
    const std::string table { GetClassTableName(object.GetClassName()) };

    // UPDATE $table SET $setstr WHERE id=:id
    std::string query("UPDATE "); query+=table; query+=" SET "; query+=setstr; query+=" WHERE id=:id"; // += for efficiency
    
    if (mDb.query(query, data) != 1)
        throw UpdateFailedException(object.GetClassName());
}

/*****************************************************/
void ObjectDatabase::UpdateObject_Register(BaseObject& object, const UniqueLock& lock)
{
    MDBG_INFO("(object:" << static_cast<std::string>(object) << ")");

    object.SetUnmodified();
    mModified.erase(&object);
}

/*****************************************************/
void ObjectDatabase::InsertObject_Query(BaseObject& object, const BaseObject::FieldList& fields, const UniqueLock& lock)
{
    MDBG_INFO("(object:" << static_cast<std::string>(object) << ")");

    std::list<std::string> columns;
    std::list<std::string> indexes;
    MixedParams data; size_t i { 0 };

    for (FieldTypes::BaseField* field : fields)
    {
        const std::string key { field->GetName() };
        const MixedValue val { field->GetDBValue() };

        columns.emplace_back(key);
        if (val.is_null())
        {
            indexes.emplace_back("NULL");
            MDBG_INFO("... " << key << " is NULL");
        }
        else
        {
            const std::string istr { std::to_string(i) };
            indexes.emplace_back(":d"+istr);
            data.emplace(":d"+istr, val); ++i;
            MDBG_INFO("... " << key << " = :d" << istr << "(" << val.ToString() << ")");
        }
    }

    const std::string colstr { Utilities::implode(",",columns) };
    const std::string idxstr { Utilities::implode(",",indexes) };
    const std::string table { GetClassTableName(object.GetClassName()) };

    // INSERT INTO $table ($colstr) VALUES ($idxstr)
    std::string query("INSERT INTO "); query+=table; query+=" ("; query+=colstr; query+=") VALUES ("; query+=idxstr; query+=")"; // += for efficiency

    if (mDb.query(query, data) != 1)
        throw InsertFailedException(object.GetClassName());
}

/*****************************************************/
void ObjectDatabase::InsertObject_Register(BaseObject& object, const UniqueLock& lock)
{
    MDBG_INFO("(object:" << static_cast<std::string>(object) << ")");

    object.SetUnmodified();
    mModified.erase(&object);

    const decltype(mCreated)::lookup_iterator it { mCreated.lookup(&object) };
    std::unique_ptr<BaseObject> objptr { std::move(it->second->second) };
    mCreated.erase(it);

    const std::string objkey { objptr->ID()+":"+objptr->GetClassName() };
    mObjects.emplace(objkey, std::move(objptr));
}

} // namespace Database
} // namespace Andromeda
