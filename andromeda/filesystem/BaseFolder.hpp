#ifndef LIBA2_BASEFOLDER_H_
#define LIBA2_BASEFOLDER_H_

#include "Item.hpp"

/** A folder in the filesystem */
class BaseFolder : public Item
{
protected:

    using Item::Item;
};

#endif