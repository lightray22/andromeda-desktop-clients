
#ifndef LIBA2_FILE_H_
#define LIBA2_FILE_H_

#include <nlohmann/json_fwd.hpp>

#include "Item.hpp"
#include "Utilities.hpp"

class Backend;

class File : public Item
{
public:

	virtual ~File(){};

    virtual const Type GetType() const override { return Type::FILE; }

    File(Backend& backend, const nlohmann::json& data);

private:

    Debug debug;
};

#endif
