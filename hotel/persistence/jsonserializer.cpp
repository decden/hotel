#include "hotel/persistence/jsonserializer.h"

#include <sstream>

namespace hotel
{
  namespace persistence
  {
    JsonSerializer::JsonSerializer() {}

    nlohmann::json JsonSerializer::serializeHotelCollection(const HotelCollection &hotelCollection)
    {
      using json = nlohmann::json;
      json resultJson = json::array();

      for (auto& hotel : hotelCollection.hotels())
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

        resultJson.push_back(hotelJson);
      }

      return resultJson;
    }

    nlohmann::json JsonSerializer::serializePlanning(const PlanningBoard &planning)
    {
      using json = nlohmann::json;
      json resultJson;
      resultJson = json::array();

      for (auto& reservation : planning.reservations())
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
            {"from", boost::gregorian::to_iso_extended_string(atom->dateRange().begin())},
            {"to", boost::gregorian::to_iso_extended_string(atom->dateRange().end())}
          };
          reservationJson["atoms"].push_back(atomJson);
        }

        resultJson.push_back(reservationJson);
      }

      return resultJson;
    }

  } // namespace persistence
} // namespace hotel
