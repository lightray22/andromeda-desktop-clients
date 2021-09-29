
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

    File(Backend& backend, Folder& parent, const nlohmann::json& data);

protected:

    virtual void SubDelete() override;

    virtual void SubRename(const std::string& name, bool overwrite) override;

    virtual void SubMove(Folder& parent, bool overwrite) override;

private:

    Debug debug;
};

#endif
