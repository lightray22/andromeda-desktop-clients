
#ifndef LIBA2_FILESYSTEM_H_
#define LIBA2_FILESYSTEM_H_

#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

#include "PlainFolder.hpp"
#include "Utilities.hpp"

class Backend;

/** A root folder accessed by its filesystem ID */
class Filesystem : public PlainFolder
{
public:

    virtual ~Filesystem(){};

    /**
     * Load a filesystem from the backend with the given ID
     * @param backend reference to backend
     * @param fsid ID of filesystem to load
     */
    static std::unique_ptr<Filesystem> LoadByID(Backend& backend, const std::string& fsid);

    /**
     * Load a filesystem with the given backend data and parent
     * @param backend reference to backend
     * @param parent reference to parent
     * @param data pre-loaded JSON data
     */
    static std::unique_ptr<Filesystem> LoadFromData(Backend& backend, Folder& parent, const nlohmann::json& data);

    /** 
     * Construct with root folder JSON data
     * @param backend reference to backend
     * @param fsid filesystem ID
     * @param data root folder json data
     */
    Filesystem(Backend& backend, std::string fsid, const nlohmann::json& rdata);

    virtual void Refresh(const nlohmann::json& data) override { }

protected:

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubMove(Folder& parent, bool overwrite = false) override { throw ModifyException(); }

private:

    std::string fsid;

    Debug debug;
};

#endif
