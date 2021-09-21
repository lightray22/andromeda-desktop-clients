
#ifndef LIBA2_FILE_H_
#define LIBA2_FILE_H_

#include "Item.hpp"
#include "Utilities.hpp"

class Backend;

class File : public Item
{
public:

    virtual const Type GetType() const override { return Type::FILE; };

protected:

    File(Backend& backend);

private:

    Debug debug;
};

#endif