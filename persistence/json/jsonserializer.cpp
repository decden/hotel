#include "persistence/json/jsonserializer.h"

#include <sstream>

namespace persistence
{
  namespace json
  {
    JsonSerializer::JsonSerializer() {}

    nlohmann::json JsonSerializer::serializeHotelCollection(const hotel::HotelCollection &hotelCollection)
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

    nlohmann::json JsonSerializer::serializePlanning(const hotel::PlanningBoard &planning)
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
            {"roomId", atom.roomId()},
            {"from", boost::gregorian::to_iso_extended_string(atom.dateRange().begin())},
            {"to", boost::gregorian::to_iso_extended_string(atom.dateRange().end())}
          };
          reservationJson["atoms"].push_back(atomJson);
        }

        resultJson.push_back(reservationJson);
      }

      return resultJson;
    }

    nlohmann::json JsonSerializer::serialize(const hotel::Hotel &item)
    {
      nlohmann::json obj;
      obj["name"] = item.name();

      obj["categories"] = nlohmann::json::array();
      for (auto& category : item.categories())
        obj["categories"].push_back(JsonSerializer::serialize(*category));

      obj["rooms"] = nlohmann::json::array();
      for (auto& room : item.rooms())
        obj["rooms"].push_back(JsonSerializer::serialize(*room));

      setCommonPersistentObjectFields(item, obj);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const hotel::RoomCategory &item)
    {
      nlohmann::json obj = {
        {"shortCode", item.shortCode()},
        {"name", item.name()}
      };
      setCommonPersistentObjectFields(item, obj);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const hotel::HotelRoom &item)
    {
      nlohmann::json obj = {
        {"categoryId", item.category()->id()},
        {"name", item.name()}
      };
      setCommonPersistentObjectFields(item ,obj);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const hotel::Reservation &item)
    {
      return {{"Reservation", "Reservation"}};
    }

    nlohmann::json JsonSerializer::serialize(const hotel::Person &item)
    {
      return {{"Person", "Person"}};
    }

    nlohmann::json JsonSerializer::serialize(const persistence::op::Operation &operation)
    {
      return boost::apply_visitor([](auto &operation){
        return JsonSerializer::serialize(operation);
      }, operation);
    }

    nlohmann::json JsonSerializer::serialize(const op::EraseAllData &)
    {
      nlohmann::json obj;
      obj["op"] = "erase_all_data";
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const op::StoreNewHotel &operation)
    {
      nlohmann::json obj;
      obj["op"] = "store_new_hotel";
      obj["o"] = JsonSerializer::serialize(*operation.newHotel);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const op::StoreNewReservation &operation)
    {
      nlohmann::json obj;
      obj["op"] = "store_new_reservation";
      obj["o"] = JsonSerializer::serialize(*operation.newReservation);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const op::StoreNewPerson &operation)
    {
      nlohmann::json obj;
      obj["op"] = "store_new_person";
      obj["o"] = JsonSerializer::serialize(*operation.newPerson);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const op::UpdateHotel &operation)
    {
      nlohmann::json obj;
      obj["op"] = "update_hotel";
      obj["o"] = JsonSerializer::serialize(*operation.updatedHotel);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const op::UpdateReservation &operation)
    {
      nlohmann::json obj;
      obj["op"] = "update_reservation";
      obj["o"] = JsonSerializer::serialize(*operation.updatedReservation);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const op::DeleteReservation &operation)
    {
      nlohmann::json obj;
      obj["op"] = "delete_reservation";
      obj["o"] = operation.reservationId;
      return obj;
    }

    void JsonSerializer::setCommonPersistentObjectFields(const hotel::PersistentObject &obj, nlohmann::json &json)
    {
      json["id"] = obj.id();
      json["rev"] = obj.revision();
    }

  } // namespace json
} // namespace persistence
