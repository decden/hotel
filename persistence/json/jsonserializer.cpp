#include "persistence/json/jsonserializer.h"

#include <sstream>

namespace persistence
{
  namespace json
  {
    template <> nlohmann::json serialize(const hotel::PersistentObject& item)
    {
      nlohmann::json obj;
      obj["id"] = item.id();
      obj["rev"] = item.revision();
      return obj;
    } 
    void deserializePersistentObject(hotel::PersistentObject &item, const nlohmann::json &json)
    {
      item.setId(json["id"]);
      item.setRevision(json["rev"]);
    }

    template <> nlohmann::json serialize(const hotel::Hotel& item)
    {
      nlohmann::json obj = serialize<hotel::PersistentObject>(item);
      obj["name"] = item.name();
      obj["categories"] = nlohmann::json::array();
      obj["rooms"] = nlohmann::json::array();

      for (auto& category : item.categories())
        obj["categories"].push_back(serialize(*category));

      for (auto& room : item.rooms())
        obj["rooms"].push_back(serialize(*room));

      return obj;
    }
    template <> hotel::Hotel deserialize(const nlohmann::json& json)
    {
      std::string name = json["name"];
      hotel::Hotel hotel(name);
      deserializePersistentObject(hotel, json);

      for (auto &category : json["categories"])
        hotel.addRoomCategory(std::make_unique<hotel::RoomCategory>(deserialize<hotel::RoomCategory>(category)));

      for (auto &room : json["rooms"])
        hotel.addRoom(std::make_unique<hotel::HotelRoom>(deserialize<hotel::HotelRoom>(room)), room["category_id"]);

      return hotel;
    }

    template <> nlohmann::json serialize(const hotel::RoomCategory& item)
    {
      nlohmann::json obj = serialize<hotel::PersistentObject>(item);
      obj["short_code"] = item.shortCode();
      obj["name"] = item.name();
      return obj;
    }
    template <> hotel::RoomCategory deserialize(const nlohmann::json& json)
    {
      hotel::RoomCategory category(json["short_code"], json["name"]);
      deserializePersistentObject(category, json);
      return category;
    }


    template <> nlohmann::json serialize(const hotel::HotelRoom& item)
    {
      nlohmann::json obj = serialize<hotel::PersistentObject>(item);
      obj["category_id"] = item.category()->shortCode();
      obj["name"] = item.name();
      return obj;
    }
    template <> hotel::HotelRoom deserialize(const nlohmann::json& json)
    {
      std::string name = json["name"];
      hotel::HotelRoom room(name);
      deserializePersistentObject(room, json);
      // TODO: It would be nice if we could completely deserialize a room here (category is missing)
      return room;
    }

    template <> nlohmann::json serialize(const hotel::Reservation& item)
    {
      nlohmann::json obj = serialize<hotel::PersistentObject>(item);
      obj["description"] = item.description();
      obj["status"] = serialize(item.status());
      obj["adults"] = item.numberOfAdults();
      obj["children"] = item.numberOfChildren();
      obj["atoms"] = nlohmann::json::array();

      for (auto& atom : item.atoms())
        obj["atoms"].push_back(serialize(atom));

      return obj;
    }
    template <> hotel::Reservation deserialize(const nlohmann::json& json)
    {
      std::string description = json["description"];
      hotel::Reservation reservation(description);
      deserializePersistentObject(reservation, json);
      reservation.setStatus(deserialize<hotel::Reservation::ReservationStatus>(json["status"]));
      reservation.setNumberOfAdults(json["adults"]);
      reservation.setNumberOfChildren(json["children"]);
      for (auto &atom : json["atoms"])
        reservation.addAtom(deserialize<hotel::ReservationAtom>(atom));
      deserializePersistentObject(reservation, json);
      return reservation;
    }

    template <> nlohmann::json serialize(const hotel::ReservationAtom& item)
    {
      nlohmann::json obj = serialize<hotel::PersistentObject>(item);
      obj["room_id"] = item.roomId();
      obj["from"] = boost::gregorian::to_iso_extended_string(item.dateRange().begin());
      obj["to"] = boost::gregorian::to_iso_extended_string(item.dateRange().end());
      return obj;
    }
    template <> hotel::ReservationAtom deserialize(const nlohmann::json& json)
    {
      auto fromDate = boost::gregorian::from_string(json["from"]);
      auto toDate = boost::gregorian::from_string(json["to"]);

      hotel::ReservationAtom atom(json["room_id"], boost::gregorian::date_period(fromDate, toDate));
      deserializePersistentObject(atom, json);
      return atom;
    }

    template <> nlohmann::json serialize(const hotel::Reservation::ReservationStatus &item)
    {
      if (item == hotel::Reservation::Unknown) return "unknown";
      if (item == hotel::Reservation::Temporary) return "temporary";
      if (item == hotel::Reservation::New) return "new";
      if (item == hotel::Reservation::Confirmed) return "confirmed";
      if (item == hotel::Reservation::CheckedIn) return "checked_in";
      if (item == hotel::Reservation::CheckedOut) return "checked_out";
      if (item == hotel::Reservation::Archived) return "archived";
      assert(false /* cannot serialize enum value */);
      return "unknown";
    }
    template <> hotel::Reservation::ReservationStatus deserialize(const nlohmann::json& json)
    {
      std::string str = json;
      if (str == "unknown") return hotel::Reservation::Unknown;
      if (str == "temporary") return hotel::Reservation::Temporary;
      if (str == "new") return hotel::Reservation::New;
      if (str == "confirmed") return hotel::Reservation::Confirmed;
      if (str == "checked_in") return hotel::Reservation::CheckedIn;
      if (str == "checked_out") return hotel::Reservation::CheckedOut;
      if (str == "archived") return hotel::Reservation::Archived;
      assert(false /* cannot deserialize enum value */);
      return hotel::Reservation::Unknown;
    }

    template <> nlohmann::json serialize(const hotel::Person& item)
    {
      return {{"Person", "Person"}};
    }


    template <> nlohmann::json serialize(const op::Operation& operation)
    {
      return boost::apply_visitor([](auto &operation){
        return serialize(operation);
      }, operation);
    }

    template <> nlohmann::json serialize(const op::EraseAllData& operation)
    {
      nlohmann::json obj;
      obj["op"] = "erase_all_data";
      return obj;
    }

    template <> nlohmann::json serialize(const op::StoreNewHotel& operation)
    {
      nlohmann::json obj;
      obj["op"] = "store_new_hotel";
      obj["o"] = serialize(*operation.newHotel);
      return obj;
    }

    template <> nlohmann::json serialize(const op::StoreNewReservation& operation)
    {
      nlohmann::json obj;
      obj["op"] = "store_new_reservation";
      obj["o"] = serialize(*operation.newReservation);
      return obj;
    }

    template <> nlohmann::json serialize(const op::StoreNewPerson& operation)
    {
      nlohmann::json obj;
      obj["op"] = "store_new_person";
      obj["o"] = serialize(*operation.newPerson);
      return obj;
    }

    template <> nlohmann::json serialize(const op::UpdateHotel& operation)
    {
      nlohmann::json obj;
      obj["op"] = "update_hotel";
      obj["o"] = serialize(*operation.updatedHotel);
      return obj;
    }

    template <> nlohmann::json serialize(const op::UpdateReservation& operation)
    {
      nlohmann::json obj;
      obj["op"] = "update_reservation";
      obj["o"] = serialize(*operation.updatedReservation);
      return obj;
    }

    template <> nlohmann::json serialize(const op::DeleteReservation& operation)
    {
      nlohmann::json obj;
      obj["op"] = "delete_reservation";
      obj["o"] = operation.reservationId;
      return obj;
    }

  } // namespace json
} // namespace persistence
