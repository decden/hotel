#ifndef HOTEL_PERSISTENCE_JSONSERIALIZER_H
#define HOTEL_PERSISTENCE_JSONSERIALIZER_H

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include "json.hpp"

namespace hotel
{
  namespace persistence
  {

    class JsonSerializer
    {
    public:
      JsonSerializer();

      nlohmann::json serializeHotelCollection(const HotelCollection& hotelCollection);
      nlohmann::json serializePlanning(const PlanningBoard& planning);
    };

  } // namespace persistence
} // namespace hotel

#endif // HOTEL_PERSISTENCE_JSONSERIALIZER_H
