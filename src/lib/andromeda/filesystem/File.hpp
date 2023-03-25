
#ifndef LIBA2_FILE_H_
#define LIBA2_FILE_H_

#include <functional>
#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Item.hpp"
#include "FSConfig.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/ScopeLocked.hpp"
#include "andromeda/SharedMutex.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class Folder;

namespace Filedata { class PageManager; class PageBackend; }

/** 
 * An Andromeda file that can be read/written
 * THREADING - get locks in advance and pass to functions
 */
class File : public Item
{
public:

    /** Exception indicating that the read goes out of bounds */
    class ReadBoundsException : public Exception { public:
        ReadBoundsException() : Exception("Read Out of Range") {}; };

    /** Exception indicating that the filesystem does not support writing */
    class WriteTypeException : public Exception { public:
        WriteTypeException() : Exception("Write Type Unsupported") {}; };

    typedef Andromeda::ScopeLocked<File> ScopeLocked;
    /** Tries to lock mScopeMutex, returns a ref that is maybe locked */
    ScopeLocked TryLockScope() { return ScopeLocked(*this, mScopeMutex); }

    virtual ~File();

    virtual void Refresh(const nlohmann::json& data, const SharedLockW& itemLock) override;

    virtual Type GetType() const override { return Type::FILE; }

    /** Returns the total file size */
    virtual uint64_t GetSize(const SharedLock& itemLock) const final;

    /**
     * @brief Construct a File using backend data
     * @param backend backend reference
     * @param data JSON data from backend
     * @param parent reference to parent folder
     */
    File(Backend::BackendImpl& backend, const nlohmann::json& data, Folder& parent);

    /** Function to create the file on the backend and return its JSON */
    typedef std::function<nlohmann::json(const std::string& name)> CreateFunc;
    /** Function to upload the file on the backend and return its JSON */
    typedef std::function<nlohmann::json(const std::string& name, const std::string& data)> UploadFunc;

    /**
     * @brief Construct a new file in memory only to be created on the backend when flushed
     * @param backend backend reference
     * @param parent reference to parent folder
     * @param name name of the new file
     * @param fsConfig reference to fs config
     * @param createFunc function to create on the backend
     * @param uploadFunc function to upload on the backend
     */
    File(Backend::BackendImpl& backend, Folder& parent, 
        const std::string& name, const FSConfig& fsConfig,
        const CreateFunc& createFunc, const UploadFunc& uploadFunc);

    /** Returns true iff the file exists on the backend (false if waiting for flush) */
    virtual bool ExistsOnBackend(const SharedLock& itemLock) const;

    /**
     * Read data from the file
     * @param buffer pointer to buffer to fill
     * @param offset byte offset in file to read
     * @param length max number of bytes to read
     * @return the number of bytes read (may be < length if EOF)
     */
    virtual size_t ReadBytesMax(char* buffer, const uint64_t offset, const size_t maxLength, const SharedLock& itemLock) final;

    /**
     * Read data from the file
     * @param buffer pointer to buffer to fill
     * @param offset byte offset in file to read
     * @param length exact number of bytes to read
     * @return the number of bytes read (may be < length if EOF)
     */
    virtual void ReadBytes(char* buffer, const uint64_t offset, const size_t length, const SharedLock& itemLock) final;

    /**
     * Writes data to a file
     * @param buffer buffer with data to write
     * @param offset byte offset in file to write
     * @param length number of bytes to write
     */
    virtual void WriteBytes(const char* buffer, const uint64_t offset, const size_t length, const SharedLockW& itemLock) final;

    /** Set the file size to the given value */
    virtual void Truncate(uint64_t newSize, const SharedLockW& itemLock) final;

    virtual void FlushCache(const SharedLockW& itemLock, bool nothrow = false) override;

protected:

    virtual void SubDelete(const DeleteLock& deleteLock) override;

    virtual void SubRename(const std::string& newName, const SharedLockW& itemLock, bool overwrite) override;

    virtual void SubMove(const std::string& parentID, const SharedLockW& itemLock, bool overwrite) override;

private:

    /** Checks the FS and account limits for the allowed write mode */
    virtual FSConfig::WriteMode GetWriteMode() const final;

    /** Returns the page size calculated from the backend.pageSize and fsConfig.chunkSize */
    virtual size_t CalcPageSize() const;

    std::unique_ptr<Filedata::PageManager> mPageManager;
    std::unique_ptr<Filedata::PageBackend> mPageBackend;

    /** true if the file was deleted */
    bool mDeleted { false };

    mutable Debug mDebug;
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_FILE_H_
