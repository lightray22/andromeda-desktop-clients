#include "BaseFolder.hpp"
#include "Backend.hpp"

/*****************************************************/
BaseFolder::BaseFolder(Backend& backend) : 
    Item(backend), debug("BaseFolder",this)
{
    debug << __func__ << "()"; debug.Info();
};

/*****************************************************/
Item& BaseFolder::GetItemByPath(const std::string& path)
{
    this->debug << __func__ << "(path:" << path << ")"; this->debug.Info();

    if (path.empty()) return *this;

    const auto [item, subpath] = Utilities::split(path,"/");

    this->debug << __func__ << "... item:" << item << " subpath:" << subpath; this->debug.Details();
    
    // TODO implement maybe storing subitems if given in Initialize() (not always!)
    // TODO fetch subitem from map and pass request
}
