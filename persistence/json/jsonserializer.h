#ifndef PERSISTENCE_JSON_JSONSERIALIZER_H
#define PERSISTENCE_JSON_JSONSERIALIZER_H

#include "persistence/op/operations.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include "json.hpp"

namespace persistence
{
  namespace json
  {
    class JsonSerializer
    {
    public:
      static nlohmann::json serialize(const hotel::Hotel& item);
      static nlohmann::json serialize(const hotel::RoomCategory& item);
      static nlohmann::json serialize(const hotel::HotelRoom& item);
      static nlohmann::json serialize(const hotel::Reservation& item);
      static nlohmann::json serialize(const hotel::ReservationAtom& item);
      static nlohmann::json serialize(const hotel::Person& item);

      static nlohmann::json serialize(const op::Operation& operation);
      static nlohmann::json serialize(const op::EraseAllData& operation);
      static nlohmann::json serialize(const op::StoreNewHotel& operation);
      static nlohmann::json serialize(const op::StoreNewReservation& operation);
      static nlohmann::json serialize(const op::StoreNewPerson& operation);
      static nlohmann::json serialize(const op::UpdateHotel& operation);
      static nlohmann::json serialize(const op::UpdateReservation& operation);
      static nlohmann::json serialize(const op::DeleteReservation& operation);

      static hotel::Hotel deserializeHotel(const nlohmann::json &json);
      static hotel::RoomCategory deserializeRoomCategory(const nlohmann::json &json);
      static hotel::HotelRoom deserializeHotelRoom(const nlohmann::json &json);
      static hotel::Reservation deserializeReservation(const nlohmann::json &json);
      static hotel::ReservationAtom deserializeReservationAtom(const nlohmann::json &json);
      static hotel::Person deserializePerson(const nlohmann::json &json);
    private:
      static void setCommonPersistentObjectFields(const hotel::PersistentObject &obj, nlohmann::json &json);
      static void deserializeCommonPersistenceObjectFields(hotel::PersistentObject &obj, const nlohmann::json &json);
    };

  } // namespace json
} // namespace persistence

#endif // PERSISTENCE_JSON_JSONSERIALIZER_H
