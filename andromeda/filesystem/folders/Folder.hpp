#ifndef LIBA2_FOLDER_H_
#define LIBA2_FOLDER_H_

#include <string>

#include "../BaseFolder.hpp"
#include "../../Utilities.hpp"

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

private:

    Debug debug;

    std::string id;
};

#endif