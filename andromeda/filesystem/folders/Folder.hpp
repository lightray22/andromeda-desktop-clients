#ifndef LIBA2_FOLDER_H_
#define LIBA2_FOLDER_H_

#include <string>
#include <nlohmann/json_fwd.hpp>

#include "../BaseFolder.hpp"
#include "Utilities.hpp"

class Backend;

/** A regular Andromeda folder */
class Folder : public BaseFolder
{
public:

    /**
     * Load a folder from the backend with the given ID
     * @param backend reference to backend
     * @param id ID of folder to load
     */
    Folder(Backend& backend, const std::string& id);

protected: 

    Folder(Backend& backend);

    virtual void Initialize(const nlohmann::json& data);

    std::string id;

private:

    Debug debug;
};

#endif