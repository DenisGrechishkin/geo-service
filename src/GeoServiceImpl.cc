#include "GeoServiceImpl.h"

#include "reactors/GetCitiesReactor.h"
#include "reactors/GetRegionsReactor.h"
#include "search/SearchEngine.h"
#include "utils/ConfigConstants.h"
#include "utils/Configuration.h"

namespace geo
{

GeoServiceImpl::GeoServiceImpl(const Configuration& configuration)
   : m_overpassApiClient(configuration.GetString(sz_overpassEndpointKey))    // Initialize Overpass API client
   , m_nominatimApiClient(configuration.GetString(sz_nominatimEndpointKey))  // Initialize Nominatim API client
   , m_searchEngine(
        std::make_unique<SearchEngine>(m_overpassApiClient, m_nominatimApiClient))  // Initialize search engine
{
}

grpc::ServerUnaryReactor* GeoServiceImpl::GetCities(
   grpc::CallbackServerContext* context, const geoproto::CitiesRequest* request, geoproto::CitiesResponse* response)
{
   return new GetCitiesReactor(context, *request, *response, *m_searchEngine);
}

grpc::ServerUnaryReactor* GeoServiceImpl::GetRegions(
   grpc::CallbackServerContext* context, const geoproto::RegionsRequest* request, geoproto::RegionsResponse* response)
{
   return new GetRegionsReactor(context, *request, *response, *m_searchEngine);
}

grpc::ServerWriteReactor<geoproto::RegionsResponse>* GeoServiceImpl::GetRegionsStream(
   grpc::CallbackServerContext* context, const geoproto::RegionsRequest* request)
{
   return nullptr;  // gRPC sends grpc::StatusCode::UNIMPLEMENTED to Client
}

grpc::ServerUnaryReactor* GeoServiceImpl::GetWeather(
   grpc::CallbackServerContext* context, const geoproto::WeatherRequest* request, ::geoproto::WeatherResponse* response)
{
   return nullptr;  // gRPC sends grpc::StatusCode::UNIMPLEMENTED to Client
}

}  // namespace geo
