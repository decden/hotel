#include "hotel/persistence/jsonserializer.h"

#include "json.hpp"

#include <sstream>

namespace hotel
{
  namespace persistence
  {
    JsonSerializer::JsonSerializer() {}

    std::string JsonSerializer::serializeHotels(const std::vector<std::unique_ptr<Hotel>>& hotels)
    {
      using json = nlohmann::json;
      json resultJson;
      resultJson["data"] = {
        {"count", hotels.size()},
        {"hotels", json::array()}
      };

      for (auto& hotel : hotels)
      {
        json hotelJson = {
          {"id", hotel->id()},
          {"name", hotel->name()}
        };

        hotelJson["categories"] = json::array();
        for (auto& category : hotel->categories())
        {
          json categoryJson = {
            {"id", category->id()},
            {"shortCode", category->shortCode()},
            {"name", category->name()}
          };
          hotelJson["categories"].push_back(categoryJson);
        }

        hotelJson["rooms"] = json::array();
        for (auto& room : hotel->rooms())
        {
          json roomJson = {
            {"id", room->id()},
            {"categoryId", room->category()->id()},
            {"name", room->name()}
          };
          hotelJson["rooms"].push_back(roomJson);
        }

        resultJson["result"]["hotels"].push_back(hotelJson);
      }

      return resultJson.dump();
    }

    std::string JsonSerializer::serializePlanning(const std::unique_ptr<PlanningBoard>& planning)
    {
      using json = nlohmann::json;
      json resultJson;
      resultJson["data"] = {
        {"count", planning->reservations().size()},
        {"reservations", json::array()}
      };

      // TODO: Store room ids in planning!

      for (auto& reservation : planning->reservations())
      {
        json reservationJson = {
          {"id", reservation->id()},
          {"description", reservation->description()},
          {"atoms", json::array()}
        };

        for (auto& atom : reservation->atoms())
        {
          json atomJson = {
            {"roomId", atom->roomId()},
            {"from", boost::gregorian::to_iso_string(atom->dateRange().begin())},
            {"to", boost::gregorian::to_iso_string(atom->dateRange().end())}
          };
          reservationJson["atoms"].push_back(atomJson);
        }

        resultJson["reservations"].push_back(reservationJson);
      }

      return resultJson.dump();
    }

  } // namespace persistence
} // namespace hotel
