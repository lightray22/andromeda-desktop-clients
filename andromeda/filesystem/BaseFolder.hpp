#ifndef LIBA2_BASEFOLDER_H_
#define LIBA2_BASEFOLDER_H_

#include <string>

#include "Item.hpp"
#include "Utilities.hpp"

class Backend;

/** A folder in the filesystem */
class BaseFolder : public Item
{
public:

    virtual const Type GetType() const override { return Type::FOLDER; };

    /** Load the item with the given relative path */
    Item& GetItemByPath(const std::string& path);

protected:

    BaseFolder(Backend& backend);

private:

    Debug debug;
};

#endif