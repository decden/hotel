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
    enum OperationResultStatus { Successful, Error };

    struct OperationResult {
      OperationResultStatus status;
      std::string message;
    };
    typedef std::vector<OperationResult> OperationResults;

    struct OperationResultsMessage {
      int uniqueId;
      OperationResults result;
    };

  } // namespace op
} // namespace persistence

#endif // PERSISTENCE_OP_RESULTS_H
