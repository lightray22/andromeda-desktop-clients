#include <utility>
#include <nlohmann/json.hpp>

#include "Folder.hpp"
#include "File.hpp"
#include "Backend.hpp"
#include "folders/PlainFolder.hpp"

/*****************************************************/
Folder::Folder(Backend& backend) : 
    Item(backend), debug("Folder",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
Folder::Folder(Backend& backend, const nlohmann::json& data) : 
    Item(backend, data), debug("Folder",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
Item& Folder::GetItemByPath(std::string path)
{
    this->debug << __func__ << "(path:" << path << ")"; this->debug.Info();

    if (path[0] == '/') path.erase(0,1);

    if (path.empty()) return *this;    

    Utilities::StringList parts = Utilities::explode(path,"/");

    Folder* parent = this; for (size_t i = 0; i < parts.size(); i++)
    {
        const ItemMap& items = parent->GetItems();
        ItemMap::const_iterator it = items.find(parts[i]);
        if (it == items.end()) throw NotFoundException();

        Item& item = *(it->second);

        if (i + 1 == parts.size()) return item;

        if (item.GetType() != Type::FOLDER) throw NotFolderException();

        parent = dynamic_cast<Folder*>(&item);
    }

    throw NotFoundException(); // should never get here
}

/*****************************************************/
File& Folder::GetFileByPath(const std::string& path)
{
    Item& item = GetItemByPath(path);

    if (item.GetType() != Type::FILE)
        throw NotFileException();

    return dynamic_cast<File&>(item);
}

/*****************************************************/
Folder& Folder::GetFolderByPath(const std::string& path)
{
    Item& item = GetItemByPath(path);

    if (item.GetType() != Type::FOLDER)
        throw NotFolderException();

    return dynamic_cast<Folder&>(item);
}

/*****************************************************/
const Folder::ItemMap& Folder::GetItems()
{
    if (!this->haveItems) LoadItems();

    this->haveItems = true;
    return this->itemMap;
}

/*****************************************************/
void Folder::LoadItemsFrom(const nlohmann::json& data)
{
    debug << __func__ << "()"; debug.Info();

    try
    {
        for (const nlohmann::json& el : data.at("files"))
        {
            std::unique_ptr<File> file(std::make_unique<File>(backend, *this, el));

            debug << __func__ << "... file:" << file->GetName(); debug.Details();

            this->itemMap[file->GetName()] = std::move(file);
        }

        for (const nlohmann::json& el : data.at("folders"))
        {
            std::unique_ptr<PlainFolder> folder(std::make_unique<PlainFolder>(backend, *this, el));

            debug << __func__ << "... folder:" << folder->GetName(); debug.Details();

            this->itemMap[folder->GetName()] = std::move(folder);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    this->haveItems = true;
}

/*****************************************************/
void Folder::CreateFile(const std::string& name)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    const ItemMap& items = GetItems(); // pre-populate items

    if (items.count(name)) throw DuplicateItemException();

    SubCreateFile(name);
}

/*****************************************************/
void Folder::CreateFolder(const std::string& name)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    const ItemMap& items = GetItems(); // pre-populate items

    if (items.count(name)) throw DuplicateItemException();

    SubCreateFolder(name);
}

/*****************************************************/
void Folder::DeleteItem(const std::string& name)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    GetItems(); ItemMap::const_iterator it = this->itemMap.find(name);
    if (it == this->itemMap.end()) throw NotFoundException();

    SubDeleteItem(*(it->second)); this->itemMap.erase(it);
}

/*****************************************************/
void Folder::RenameItem(const std::string& name0, const std::string& name1, bool overwrite)
{
    debug << __func__ << "(name0:" << name0 << " name1:" << name1 << ")"; debug.Info();

    GetItems(); ItemMap::const_iterator it = this->itemMap.find(name0);
    if (it == this->itemMap.end()) throw NotFoundException();

    ItemMap::const_iterator dup = this->itemMap.find(name1);
    if (!overwrite && dup != this->itemMap.end())
        throw DuplicateItemException();

    SubRenameItem(*(it->second), name1, overwrite);

    if (dup != this->itemMap.end()) this->itemMap.erase(dup);

    ItemMap::node_type node(this->itemMap.extract(it));
    node.key() = name1; this->itemMap.insert(std::move(node));
}

/*****************************************************/
void Folder::MoveItem(const std::string& name, Folder& parent, bool overwrite)
{
    debug << __func__ << "(name:" << name << " parent:" << parent.GetName() << ")"; debug.Info();

    GetItems(); ItemMap::const_iterator it = this->itemMap.find(name);
    if (it == this->itemMap.end()) throw NotFoundException();

    parent.GetItems(); if (!parent.CanReceiveItems()) throw ModifyException();

    ItemMap::const_iterator dup = parent.itemMap.find(name);    
    if (!overwrite && dup != parent.itemMap.end())
        throw DuplicateItemException();

    SubMoveItem(*(it->second), parent, overwrite);

    if (dup != parent.itemMap.end()) parent.itemMap.erase(dup);

    parent.itemMap.insert(this->itemMap.extract(it));
}

/*****************************************************/
void Folder::FlushCache()
{
    ItemMap::const_iterator it = itemMap.begin(); 
    
    for (; it != itemMap.end(); it++) it->second->FlushCache();
}