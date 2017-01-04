#ifndef PERSISTENCE_OP_OPERATIONS_H
#define PERSISTENCE_OP_OPERATIONS_H

#include "hotel/reservation.h"
#include "hotel/person.h"

#include "boost/variant.hpp"

#include <memory>

namespace persistence
{
  namespace op
  {
    struct StoreNewReservation
    {
      std::unique_ptr<hotel::Reservation> newReservation;
    };

    struct StoreNewPerson
    {
      std::unique_ptr<hotel::Person> newPerson;
    };


    // Define a union type of all known operations
    typedef boost::variant<op::StoreNewReservation,
                           op::StoreNewPerson>
                           Operation;

  } // namespace op
} // namespace persistence

#endif // PERSISTENCE_OP_OPERATIONS_H
