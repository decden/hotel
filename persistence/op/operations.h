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

    typedef boost::variant<std::unique_ptr<hotel::Hotel>,
                           std::unique_ptr<hotel::Reservation>,
                           std::unique_ptr<hotel::Person>> StreamableTypePtr;

    enum class StreamableType { Hotel, Reservation, Person };

    template <class T>
    StreamableType getStreamableType();
    StreamableType getStreamableType(const StreamableTypePtr& ptr);

    struct StoreNew { StreamableTypePtr newItem; };
    struct Update { StreamableTypePtr updatedItem; };
    struct Delete { StreamableType type; int id; };

    // Define a union type of all known operations
    typedef boost::variant<op::EraseAllData,
                           // crud operations
                           op::StoreNew,
                           op::Update,
                           op::Delete>
            Operation;
    typedef std::vector<Operation> Operations;

  } // namespace op
} // namespace persistence

#endif // PERSISTENCE_OP_OPERATIONS_H
