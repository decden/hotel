#include "persistence/json/jsonserializer.h"

#include <sstream>

namespace persistence
{
  namespace json
  {
    nlohmann::json JsonSerializer::serialize(const hotel::Hotel &item)
    {
      nlohmann::json obj = {
        {"name", item.name()},
        {"categories", nlohmann::json::array()},
        {"rooms", nlohmann::json::array()}
      };

      for (auto& category : item.categories())
        obj["categories"].push_back(JsonSerializer::serialize(*category));

      for (auto& room : item.rooms())
        obj["rooms"].push_back(JsonSerializer::serialize(*room));

      setCommonPersistentObjectFields(item, obj);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const hotel::RoomCategory &item)
    {
      nlohmann::json obj = {
        {"short_code", item.shortCode()},
        {"name", item.name()}
      };
      setCommonPersistentObjectFields(item, obj);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const hotel::HotelRoom &item)
    {
      nlohmann::json obj = {
        {"category_id", item.category()->shortCode()},
        {"name", item.name()}
      };
      setCommonPersistentObjectFields(item ,obj);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const hotel::Reservation &item)
    {
      nlohmann::json obj = {
        {"description", item.description()},
        {"status", (int)item.status()},
        {"atoms", nlohmann::json::array()}
      };

      for (auto& atom : item.atoms())
        obj["atoms"].push_back(JsonSerializer::serialize(atom));

      setCommonPersistentObjectFields(item, obj);
      return obj;
    }

    nlohmann::json JsonSerializer::serialize(const hotel::ReservationAtom &item)
    {
      nlohmann::json obj = {
        {"room_id", item.roomId()},
        {"from", boost::gregorian::to_iso_extended_string(item.dateRange().begin())},
        {"to", boost::gregorian::to_iso_extended_string(item.dateRange().end())}
      };
      setCommonPersistentObjectFields(item, obj);
      return obj;
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

    hotel::Hotel JsonSerializer::deserializeHotel(const nlohmann::json &json)
    {
      std::string name = json["name"];
      hotel::Hotel hotel(name);

      for (auto &category : json["categories"])
        hotel.addRoomCategory(std::make_unique<hotel::RoomCategory>(deserializeRoomCategory(category)));

      for (auto &room : json["rooms"])
        hotel.addRoom(std::make_unique<hotel::HotelRoom>(deserializeHotelRoom(room)), room["category_id"]);

      deserializeCommonPersistenceObjectFields(hotel, json);
      return hotel;
    }

    hotel::RoomCategory JsonSerializer::deserializeRoomCategory(const nlohmann::json &json)
    {
      hotel::RoomCategory category(json["short_code"], json["name"]);
      deserializeCommonPersistenceObjectFields(category, json);
      return category;
    }

    hotel::HotelRoom JsonSerializer::deserializeHotelRoom(const nlohmann::json &json)
    {
      std::string name = json["name"];
      hotel::HotelRoom room(name);
      deserializeCommonPersistenceObjectFields(room, json);
      return room;
    }

    hotel::Reservation JsonSerializer::deserializeReservation(const nlohmann::json &json)
    {
      std::string description = json["description"];
      int status = json["status"];
      hotel::Reservation reservation(description);
      reservation.setStatus((hotel::Reservation::ReservationStatus)status);
      for (auto &atom : json["atoms"])
        reservation.addAtom(deserializeReservationAtom(atom));
      deserializeCommonPersistenceObjectFields(reservation, json);
      return reservation;
    }

    hotel::ReservationAtom JsonSerializer::deserializeReservationAtom(const nlohmann::json &json)
    {
      auto fromDate = boost::gregorian::from_string(json["from"]);
      auto toDate = boost::gregorian::from_string(json["to"]);

      hotel::ReservationAtom atom(json["room_id"], boost::gregorian::date_period(fromDate, toDate));
      deserializeCommonPersistenceObjectFields(atom, json);
      return atom;
    }

    hotel::Person JsonSerializer::deserializePerson(const nlohmann::json &json)
    {
      // Not implemented...
      assert(false);
      return hotel::Person("", "");
    }

    void JsonSerializer::setCommonPersistentObjectFields(const hotel::PersistentObject &obj, nlohmann::json &json)
    {
      json["id"] = obj.id();
      json["rev"] = obj.revision();
    }

    void JsonSerializer::deserializeCommonPersistenceObjectFields(hotel::PersistentObject &obj, const nlohmann::json &json)
    {
      obj.setId(json["id"]);
      obj.setRevision(json["rev"]);
    }

  } // namespace json
} // namespace persistence
