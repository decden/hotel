#ifndef HOTEL_PERSISTENCE_JSONSERIALIZER_H
#define HOTEL_PERSISTENCE_JSONSERIALIZER_H

#include "hotel/hotel.h"
#include "hotel/planning.h"

namespace hotel
{
  namespace persistence
  {

    class JsonSerializer
    {
    public:
      JsonSerializer();

      std::string serializeHotels(const std::vector<std::unique_ptr<hotel::Hotel>>& hotels);
      std::string serializePlanning(const std::unique_ptr<hotel::PlanningBoard>& planning);
    };

  } // namespace persistence
} // namespace hotel

#endif // HOTEL_PERSISTENCE_JSONSERIALIZER_H
