
#include "MixedValue.hpp"
#include "ObjectDatabase.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
void ObjectDatabase::commit()
{
    MDBG_INFO("()");
    mDb.commit();
}

/*****************************************************/
void ObjectDatabase::rollback()
{
    MDBG_INFO("()");
    mDb.rollback();
}

/*****************************************************/
void ObjectDatabase::notifyModified(BaseObject& object)
{ 
    const UniqueLock lock(mMutex);

    if (!mModified.exists(&object))
    {
        MDBG_INFO("... object:" << static_cast<std::string>(object));
        mModified.enqueue_front(&object, object);
    }
}

/*****************************************************/
void ObjectDatabase::SaveObjects()
{
    const UniqueLock lock(mMutex);
    MDBG_INFO("() created:" << mCreated.size() << " modified:" << mModified.size());

    // insert new objects first for foreign keys
    while (!mCreated.empty()) mCreated.front().second->Save();
    while (!mModified.empty()) mModified.front().second.Save();

    mCreated.clear();
    mModified.clear();
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
        MDBG_INFO("... was created!");
        object.NotifyDeleted();
        mModified.erase(&object);
        mCreated.erase(&object);
    }
    else
    {
        QueryBuilder query;
        query.Where(query.Equals("id",object.ID()));

        const std::string table { GetClassTableName(object.GetClassName()) };

        // DELETE FROM $table $query
        std::string qstr("DELETE FROM "); qstr+=table; qstr+=" "; qstr+=query.GetText(); // += for efficiency

        if (mDb.query(qstr, query.GetParams()) != 1)
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
    const UniqueLock lock(mMutex);
    
    if (mCreated.exists(&object))
        InsertObject(object, fields, lock);
    else UpdateObject(object, fields, lock);
}

/*****************************************************/
void ObjectDatabase::UpdateObject(BaseObject& object, const BaseObject::FieldList& fields, const UniqueLock& lock)
{
    MDBG_INFO("(object:" << static_cast<std::string>(object) << ")");

    MixedParams data {{":id", object.ID()}};
    std::list<std::string> sets; size_t i { 0 };

    for (FieldTypes::BaseField* field : fields)
    {
        const std::string key { field->GetName() };
        const MixedValue val { field->GetDBValue() };
        field->SetUnmodified();
        
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

    mModified.erase(&object);
}

/*****************************************************/
void ObjectDatabase::InsertObject(BaseObject& object, const BaseObject::FieldList& fields, const UniqueLock& lock)
{
    MDBG_INFO("(object:" << static_cast<std::string>(object) << ")");

    std::list<std::string> columns;
    std::list<std::string> indexes;
    MixedParams data; size_t i { 0 };

    for (FieldTypes::BaseField* field : fields)
    {
        const std::string key { field->GetName() };
        const MixedValue val { field->GetDBValue() };
        field->SetUnmodified();

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

    mModified.erase(&object);

    const decltype(mCreated)::lookup_iterator it { mCreated.lookup(&object) };
    std::unique_ptr<BaseObject> objptr { std::move(it->second->second) };
    mCreated.erase(it);

    const std::string objkey { objptr->ID()+":"+objptr->GetClassName() };
    mObjects.emplace(objkey, std::move(objptr));
}

} // namespace Database
} // namespace Andromeda
