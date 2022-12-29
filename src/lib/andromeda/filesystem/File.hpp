
#ifndef LIBA2_FILE_H_
#define LIBA2_FILE_H_

#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Item.hpp"
#include "FSConfig.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/SharedMutex.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class Folder;

namespace Filedata { class PageManager; }

/** An Andromeda file */
class File : public Item
{
public:

    /** Exception indicating that the read goes out of bounds */
    class ReadBoundsException : public Exception { public:
        ReadBoundsException() : Exception("Read Out of Range") {}; };

    /** Exception indicating that the filesystem does not support writing */
    class WriteTypeException : public Exception { public:
        WriteTypeException() : Exception("Write Type Unsupported") {}; };

    virtual ~File();

    virtual void Refresh(const nlohmann::json& data) override;

    virtual Type GetType() const override { return Type::FILE; }

    /** Returns the total file size */
    virtual uint64_t GetSize() const final;

    /**
     * @brief Construct a File using backend data
     * @param backend backend reference
     * @param data JSON data from backend
     * @param parent reference to parent folder
     */
    File(Backend::BackendImpl& backend, const nlohmann::json& data, Folder& parent);

    /**
     * Read data from the file
     * @param buffer pointer to buffer to fill
     * @param offset byte offset in file to read
     * @param length max number of bytes to read
     * @return the number of bytes read (may be < length if EOF)
     */
    virtual size_t ReadBytesMax(char* buffer, const uint64_t offset, const size_t maxLength) final;

    /**
     * Read data from the file
     * @param buffer pointer to buffer to fill
     * @param offset byte offset in file to read
     * @param length exact number of bytes to read
     * @return the number of bytes read (may be < length if EOF)
     */
    virtual void ReadBytes(char* buffer, const uint64_t offset, size_t length) final;
    virtual void ReadBytes(char* buffer, const uint64_t offset, size_t length, const Andromeda::SharedLockR& dataLock) final;

    /**
     * Writes data to a file
     * @param buffer buffer with data to write
     * @param offset byte offset in file to write
     * @param length number of bytes to write
     */
    virtual void WriteBytes(const char* buffer, const uint64_t offset, const size_t length) final;

    /** Set the file size to the given value */
    virtual void Truncate(uint64_t newSize) final;

    virtual void FlushCache(bool nothrow = false) override;

protected:

    virtual void SubDelete() override;

    virtual void SubRename(const std::string& newName, bool overwrite) override;

    virtual void SubMove(Folder& newParent, bool overwrite) override;

private:

    /** Checks the FS and account limits for the allowed write mode */
    virtual FSConfig::WriteMode GetWriteMode() const final;

    std::unique_ptr<Filedata::PageManager> mPageManager;

    /** true if the file was deleted */
    bool mDeleted { false };

    Debug mDebug;
};

} // namespace Filesystem
} // namespace Andromeda

#endif
