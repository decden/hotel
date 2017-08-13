#ifndef PERSISTENCE_OP_OPERATIONS_H
#define PERSISTENCE_OP_OPERATIONS_H

#include "hotel/hotel.h"
#include "hotel/person.h"
#include "hotel/reservation.h"

#include "boost/variant.hpp"

#include <memory>
#include <mutex>
#include <condition_variable>

namespace persistence
{
  namespace op
  {
    //! Operations which deletes everyghing present in the database
    struct EraseAllData { };

    struct StoreNewHotel { std::unique_ptr<hotel::Hotel> newHotel; };
    struct StoreNewReservation { std::unique_ptr<hotel::Reservation> newReservation; };
    struct StoreNewPerson { std::unique_ptr<hotel::Person> newPerson; };

    struct UpdateHotel { std::unique_ptr<hotel::Hotel> updatedHotel; };
    struct UpdateReservation { std::unique_ptr<hotel::Reservation> updatedReservation; };

    struct DeleteReservation { int reservationId; };

    // Define a union type of all known operations
    typedef boost::variant<op::EraseAllData,
                           op::StoreNewHotel,
                           op::StoreNewReservation,
                           op::StoreNewPerson,
                           op::UpdateHotel,
                           op::UpdateReservation,
                           op::DeleteReservation>
            Operation;
    typedef std::vector<Operation> Operations;

  } // namespace op
} // namespace persistence

#endif // PERSISTENCE_OP_OPERATIONS_H
