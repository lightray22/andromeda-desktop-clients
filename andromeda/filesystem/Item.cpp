
#include <nlohmann/json.hpp>

#include "Item.hpp"
#include "Backend.hpp"

/*****************************************************/
Item::Item(Backend& backend) : 
    backend(backend), debug("Item",this)
{
    debug << __func__ << "()"; debug.Info();
};

/*****************************************************/
void Item::Initialize(const nlohmann::json& data)
{
    debug << __func__ << "()"; debug.Info();
    
    try
    {
        data.at("name").get_to(this->name);

        data.at("dates").at("created").get_to(this->created);
        data.at("dates").at("modified").get_to(this->modified);
        data.at("dates").at("accessed").get_to(this->accessed);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}