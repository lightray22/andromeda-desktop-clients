
#ifndef LIBA2_FILE_H_
#define LIBA2_FILE_H_

#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Item.hpp"
#include "FSConfig.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
class Backend;

namespace Filesystem {
class Folder;

/** An Andromeda file */
class File : public Item
{
public:

    /** Exception indicating that the filesystem does not support writing */
    class WriteTypeException : public Exception { public:
        WriteTypeException() : Exception("Write Type Unsupported") {}; };

    virtual ~File() { }

    virtual void Refresh(const nlohmann::json& data) override;

    virtual Type GetType() const override { return Type::FILE; }

    /**
     * @brief Construct a File using backend data
     * @param backend backend reference
     * @param data JSON data from backend
     * @param parent reference to parent folder
     */
    File(Backend& backend, const nlohmann::json& data, Folder& parent);

    /**
     * Read data from the file
     * @param buffer pointer to buffer to fill
     * @param offset byte offset in file to read
     * @param length max number of bytes to read
     * @return the number of bytes read (may be < length if EOF)
     */
    virtual size_t ReadBytes(std::byte* buffer, const uint64_t offset, size_t length) final;

    /**
     * Writes data to a file
     * @param buffer buffer with data to write
     * @param offset byte offset in file to write
     * @param length number of bytes to write
     */
    virtual void WriteBytes(const std::byte* buffer, const uint64_t offset, const size_t length) final;

    /** Set the file size to the given value */
    virtual void Truncate(uint64_t newSize) final;

    virtual void FlushCache(bool nothrow = false) override;

protected:

    virtual void SubDelete() override;

    virtual void SubRename(const std::string& newName, bool overwrite) override;

    virtual void SubMove(Folder& newParent, bool overwrite) override;

private:

    /** Checks the FS and account limits for the allowed write mode */
    virtual FSConfig::WriteMode GetWriteMode() const;

    size_t mPageSize;

    struct Page
    {
        explicit Page(size_t pageSize) : data(pageSize){}
        typedef std::vector<std::byte> Data; Data data;
        bool dirty { false };
    };

    typedef std::map<uint64_t, Page> PageMap; PageMap mPages;

    /** 
     * Returns a reference to a data page
     * @param index index of the page to load
     * @param minsize minimum size of the page for writing
     */
    virtual Page& GetPage(const uint64_t index, const size_t minsize = 0) final;

    /** Reads data from the given page index */
    virtual void ReadPage(std::byte* buffer, const uint64_t index, const size_t offset, const size_t length) final;

    /** Writes data to the given page index */
    virtual void WritePage(const std::byte* buffer, const uint64_t index, const size_t offset, const size_t length) final;

    /* file size as far as the backend knows */
    uint64_t mBackendSize;

    /** true if the file was deleted */
    bool mDeleted { false };

    Debug mDebug;
};

} // namespace Filesystem
} // namespace Andromeda

#endif
