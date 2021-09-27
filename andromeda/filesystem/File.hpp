
#ifndef LIBA2_FILE_H_
#define LIBA2_FILE_H_

#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Item.hpp"
#include "Utilities.hpp"

class Backend;
class Folder;

class File : public Item
{
public:

	virtual ~File(){};

    virtual const Type GetType() const override { return Type::FILE; }

    /** Returns true if this item has a parent */
    virtual const bool HasParent() const override { return true; }

    /** Returns the parent folder */
    virtual Folder& GetParent() const override { return this->parent; }

    File(Backend& backend, Folder& parent, const nlohmann::json& data);

    virtual void Delete(bool internal = false) override;

    virtual void Rename(const std::string& name, bool overwrite = false, bool internal = false) override;

private:

    Folder& parent;

    Debug debug;
};

#endif
