#ifndef LIBA2_TEMPPATH_H_
#define LIBA2_TEMPPATH_H_

#include <filesystem>
#include <random>
#include <string>

#include "Utilities.hpp"

namespace Andromeda {

/** Auto-deleting temporary file path */
class TempPath
{
public:

    /** Create a temp path with the given suffix - does NOT create the file! */
    explicit TempPath(const std::string& suffix) :
        mPath(std::filesystem::temp_directory_path().string()+"/a2_"
            +Utilities::Random(16)+"_"+suffix) { }

    virtual ~TempPath()
    {
        if (std::filesystem::exists(mPath))
            std::filesystem::remove(mPath);
    }
    DELETE_COPY(TempPath)
    DELETE_MOVE(TempPath)

    /** returns the temporary path generated */
    [[nodiscard]] const std::string& Get() const { return mPath; }

private:
    // path to the created file
    const std::string mPath;
};

} // namespace Andromeda

#endif // LIBA2_TEMPPATH_H_
