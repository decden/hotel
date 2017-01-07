#ifndef PERSISTENCE_OP_RESULTS_H
#define PERSISTENCE_OP_RESULTS_H

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/person.h"
#include "hotel/planning.h"
#include "hotel/reservation.h"

#include "boost/variant.hpp"

#include <memory>

namespace persistence
{
  namespace op
  {
    struct NoResult { };

    //! Result of erasing all data from the database
    struct EraseAllDataResult { };

    //! Result of initial loading operation
    struct LoadInitialDataResult
    {
      std::unique_ptr<hotel::HotelCollection> hotels;
      std::unique_ptr<hotel::PlanningBoard> planning;
    };

    struct StoreNewHotelResult { std::unique_ptr<hotel::Hotel> storedHotel; };
    struct StoreNewReservationResult { std::unique_ptr<hotel::Reservation> storedReservation; };
    struct StoreNewPersonResult { std::unique_ptr<hotel::Person> storedPerson; };

    struct DeleteReservationResult { int deletedReservationId; };

    // Define a union type of all known operation results
    typedef boost::variant<op::NoResult,
                           op::EraseAllDataResult,
                           op::LoadInitialDataResult,
                           op::StoreNewHotelResult,
                           op::StoreNewReservationResult,
                           op::StoreNewPersonResult,
                           op::DeleteReservationResult>
            OperationResult;
    typedef std::vector<OperationResult> OperationResults;

  } // namespace op
} // namespace persistence

#endif // PERSISTENCE_OP_RESULTS_H
