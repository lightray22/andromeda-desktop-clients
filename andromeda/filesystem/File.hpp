
#ifndef LIBA2_FILE_H_
#define LIBA2_FILE_H_

#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Item.hpp"
#include "Utilities.hpp"
#include "FSConfig.hpp"

class Backend;
class Folder;

class File : public Item
{
public:

    /** Exception indicating that the filesystem does not support writing */
    class WriteTypeException : public Exception { public:
        WriteTypeException() : Exception("Write Type Unsupported") {}; };

    virtual ~File() { }

    virtual void Refresh(const nlohmann::json& data) override;

    virtual Type GetType() const override { return Type::FILE; }

    File(Backend& backend, const nlohmann::json& data, Folder& parent);

    /**
     * Read data from the file
     * @param buffer pointer to buffer to fill
     * @param offset byte offset in file to read
     * @param length number of bytes to read
     */
    virtual size_t ReadBytes(std::byte* buffer, const size_t offset, size_t length) final;

    /**
     * Writes data to a file
     * @param buffer buffer with data to write
     * @param offset byte offset in file to write
     * @param length number of bytes to write
     */
    virtual void WriteBytes(const std::byte* buffer, const size_t offset, const size_t length) final;

    /** Set the file size to the given value */
    virtual void Truncate(size_t size) final;

    virtual void FlushCache() override;

protected:

    virtual void SubDelete() override;

    virtual void SubRename(const std::string& name, bool overwrite) override;

    virtual void SubMove(Folder& parent, bool overwrite) override;

private:

    /** Checks the FS and account limits for the allowed write mode */
    virtual FSConfig::WriteMode GetWriteMode() const;

    size_t pageSize;

    struct Page
    {
        Page(size_t pageSize) : data(pageSize){}
        typedef std::vector<std::byte> Data; Data data;
        bool dirty = false;
    };

    typedef std::map<size_t, Page> PageMap; PageMap pages;

    /** 
     * Returns a reference to a data page
     * @param index index of the page to load
     * @param minsize minimum size of the page for writing
     */
    virtual Page& GetPage(const size_t index, const size_t minsize = 0) final;

    /** Reads data from the given page index */
    virtual void ReadPage(std::byte* buffer, const size_t index, const size_t offset, const size_t length) final;

    /** Writes data to the given page index */
    virtual void WritePage(const std::byte* buffer, const size_t index, const size_t offset, const size_t length) final;

    /* file size as far as the backend knows */
    size_t backendSize;

    /** true if the file was deleted */
    bool deleted = false;

    Debug debug;
};

#endif
