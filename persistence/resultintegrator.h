#ifndef PERSISTENCE_RESULTINTEGRATOR_H
#define PERSISTENCE_RESULTINTEGRATOR_H

#include "persistence/datastream.h"

#include "persistence/op/operations.h"
#include "persistence/op/results.h"
#include "persistence/op/task.h"

#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include "boost/signals2.hpp"

#include <vector>
#include <mutex>
#include <queue>

namespace persistence
{
  /**
   * @brief The ResultIntegrator class collects results from the persitency backend and applies them locally.
   */
  class ResultIntegrator
  {
  public:
    ResultIntegrator() = default;
    ~ResultIntegrator() = default;

    hotel::HotelCollection& hotels();
    const hotel::HotelCollection& hotels() const;
    hotel::PlanningBoard& planning();
    const hotel::PlanningBoard& planning() const;

    void processIntegrationQueue();
    void addPendingOperation(op::Task<op::OperationResults> task);
    size_t pendingOperationsCount() const;

    template <class T>
    void addStream(std::shared_ptr<DataStream<T>> dataStream) { _dataStreams.push_back(std::move(dataStream)); }

  private:
    void integrateResult(op::NoResult& res);
    void integrateResult(op::EraseAllDataResult& res);
    void integrateResult(op::LoadInitialDataResult& res);
    void integrateResult(op::StoreNewReservationResult& res);
    void integrateResult(op::StoreNewHotelResult& res);
    void integrateResult(op::StoreNewPersonResult& res);
    void integrateResult(op::DeleteReservationResult& res);

    hotel::PlanningBoard _planning;
    hotel::HotelCollection _hotels;

    std::mutex _queueMutex;
    std::vector<op::Task<op::OperationResults>> _integrationQueue;
    std::vector<DataStreamVariant> _dataStreams;
  };

} // namespace persistence

#endif // PERSISTENCE_RESULTINTEGRATOR_H
