
#include <nlohmann/json.hpp>

#include "Item.hpp"
#include "Backend.hpp"

/*****************************************************/
Item::Item(Backend& backend) : 
    backend(backend), debug("Item",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
Item::Item(Backend& backend, const nlohmann::json& data) : Item(backend)
{    
    try
    {
        data.at("id").get_to(this->id);
        data.at("name").get_to(this->name);

        debug << __func__ << "... name:" << this->name; debug.Details();

        data.at("dates").at("created").get_to(this->created);
        data.at("dates").at("modified").get_to(this->modified);
        data.at("dates").at("accessed").get_to(this->accessed);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}
