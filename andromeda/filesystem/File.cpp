#include <nlohmann/json.hpp>

#include "File.hpp"
#include "Backend.hpp"

/*****************************************************/
File::File(Backend& backend, const nlohmann::json& data) : 
    Item(backend, data), debug("File",this)
{
    debug << __func__ << "()"; debug.Info();
    
    try
    {
        data.at("size").get_to(this->size);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}
