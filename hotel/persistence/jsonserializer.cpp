#include "hotel/persistence/jsonserializer.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <sstream>

namespace hotel {
namespace persistence {

JsonSerializer::JsonSerializer()
{
}

std::string JsonSerializer::serializeHotels(const std::vector<std::unique_ptr<Hotel>>& hotels)
{
  using boost::property_tree::ptree;

  ptree json;
  ptree hotelsJson;
  json.put("results", hotels.size());
  for(auto& hotel : hotels)
  {
    ptree hotelJson;
    hotelJson.put("id", hotel->id());
    hotelJson.put("name", hotel->name());

    ptree categoriesJson;
    for (auto& category : hotel->categories())
    {
      ptree categoryJson;
      categoryJson.put("id", category->id());
      categoryJson.put("shortCode", category->shortCode());
      categoryJson.put("name", category->name());
      categoriesJson.push_back(std::make_pair("", categoryJson));
    }
    hotelJson.add_child("categories", categoriesJson);

    ptree roomsJson;
    for (auto& room : hotel->rooms())
    {
      ptree roomJson;
      roomJson.put("id", room->id());
      roomJson.put("categoryId", room->category()->id());
      roomJson.put("name", room->name());
      roomsJson.push_back(std::make_pair("", roomJson));
    }
    hotelJson.add_child("rooms", roomsJson);

    hotelsJson.push_back(std::make_pair("", hotelJson));
  }
  json.add_child("hotels", hotelsJson);

  std::stringstream stream;
  boost::property_tree::write_json(stream, json);
  return stream.str();
}

std::string JsonSerializer::serializePlanning(const std::unique_ptr<PlanningBoard> &planning)
{
  using boost::property_tree::ptree;

  ptree json;
  ptree reservationsJson;
  json.put("results", planning->reservations().size());
  // TODO: Store room ids in planning!

  for(auto& reservation : planning->reservations())
  {
    ptree reservationJson;
    reservationJson.put("id", reservation->id());
    reservationJson.put("description", reservation->description());

    ptree atomsJson;
    for (auto& atom : reservation->atoms())
    {
      ptree atomJson;
      atomJson.put("id", atom->id());
      atomJson.put("roomId", atom->roomId());
      atomJson.put("dateFrom", boost::gregorian::to_iso_string(atom->dateRange().begin()));
      atomJson.put("dateTo", boost::gregorian::to_iso_string(atom->dateRange().end()));
      atomsJson.push_back(std::make_pair("", atomJson));
    }
    reservationJson.add_child("atoms", atomsJson);

    reservationsJson.push_back(std::make_pair("", reservationJson));
  }
  json.add_child("hotels", reservationsJson);

  std::stringstream stream;
  boost::property_tree::write_json(stream, json);
  return stream.str();
}

} // namespace persistence
} // namespace hotel
