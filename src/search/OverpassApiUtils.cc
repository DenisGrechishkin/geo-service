#include "OverpassApiUtils.h"

#include "../utils/JsonUtils.h"
#include "../utils/WebClient.h"
#include "ProtoTypes.h"

#include <rapidjson/document.h>

#include <format>

namespace
{

using namespace geo;

// Overpass API query format to find relations by name or English name.
constexpr const char* sz_requestByNameFormat =  //
   "[out:json];"
   "rel[\"name\"=\"{0}\"][\"boundary\"=\"administrative\"];"
   "out ids;";  // Return ids.

// Overpass API query format to find relations by coordinates.
constexpr const char* sz_requestByCoordinatesFormat =
   "[out:json];"
   "is_in({},{}) -> .areas;"  // Save "area" entities which contain a point with the given coordinates to .areas set.
   "("
   "rel(pivot.areas)[\"boundary\"=\"administrative\"];"
   "rel(pivot.areas)[\"place\"~\"^(city|town|state)$\"];"
   ");"         // Save "relation" entities with administrative boundary type or with city|town|state place
                // which define the outlines of the found "area" entities to the result set.
   "out ids;";  // Return ids.

}  // namespace

namespace geo::overpass
{
std::vector<CityDetail> LoadCityDetails(OsmId relationId, WebClient& client)
{
    std::vector<CityDetail> result;
    
    // overpass запрос для получения музеев и отелей в пределах города
    std::string query = std::format(
        "[out:json][timeout:180];"
        "rel({});"
        "map_to_area -> .city_area;"
        "("
        "  node[\"tourism\"=\"museum\"](area.city_area);"
        "  node[\"tourism\"=\"hotel\"](area.city_area);"
        ");"
        "out tags;",
        relationId);
    
    std::string response = client.Post(query);
    
    rapidjson::Document document;
    document.Parse(response.c_str());
    
    if (!document.IsObject() || !document.HasMember("elements"))
        return result;
    
    const auto& elements = document["elements"].GetArray();
    for (const auto& element : elements)
    {
        if (!element.IsObject() || !element.HasMember("type") || 
            std::string(element["type"].GetString()) != "node")
            continue;
            
        CityDetail detail;
        detail.latitude = element["lat"].GetDouble();
        detail.longitude = element["lon"].GetDouble();
        
        if (element.HasMember("tags"))
        {
            const auto& tags = element["tags"].GetObject();
            
            if (tags.HasMember("tourism"))
            {
                std::string tourism_type = tags["tourism"].GetString();
                if (tourism_type == "museum" || tourism_type == "hotel")
                {
                    detail.tourism_type = tourism_type;
                }
            }
            
            if (tags.HasMember("name"))
                detail.name = tags["name"].GetString();
                
            if (tags.HasMember("name:en"))
                detail.name_en = tags["name:en"].GetString();
        }
        
        if (!detail.tourism_type.empty())
            result.push_back(std::move(detail));
    }
    
    return result;
}

OsmIds ExtractRelationIds(const std::string& json)
{
   if (json.empty())
      return {};

   rapidjson::Document document;
   document.Parse(json.c_str());
   if (!document.IsObject())
      return {};

   OsmIds result;
   for (const auto& e : document["elements"].GetArray())
   {
      const auto& id = json::Get(e, "id");
      if (!id.IsNull() && json::GetString(json::Get(e, "type")) == "relation")
         result.emplace_back(json::GetInt64(id));
   }
   return result;
}

OsmIds LoadRelationIdsByName(WebClient& client, const std::string& name)
{
   const std::string request = std::format(sz_requestByNameFormat, name);
   const std::string response = client.Post(request);
   return ExtractRelationIds(response);
}

OsmIds LoadRelationIdsByLocation(WebClient& client, double latitude, double longitude)
{
   const std::string request = std::format(sz_requestByCoordinatesFormat, latitude, longitude);
   const std::string response = client.Post(request);
   return ExtractRelationIds(response);
}

}  // namespace geo::overpass