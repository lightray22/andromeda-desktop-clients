#ifndef LIBA2_ITEM_H_
#define LIBA2_ITEM_H_

#include <string>

class Backend;

/** An abstract item in a filesystem */
class Item
{
public:

    /** Returns the FS name of this item */
    std::string GetName(){ return this->name; }

protected:

    /** @param backend reference to backend */
    Item(Backend& backend) : backend(backend){ };
    
    Backend& backend;

    std::string name;
};

#endif