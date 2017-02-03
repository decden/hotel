#ifndef PERSISTENCE_RESULTINTEGRATOR_H
#define PERSISTENCE_RESULTINTEGRATOR_H

#include "persistence/op/results.h"

#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include "boost/signals2.hpp"

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

    /**
     * @brief reportResult This method is called by the persistency backend, in order to report any available results.
     * @note This method will be called from the persistency backend thread!
     */
    void reportResult(op::OperationResultsMessage results);

    /**
     * @brief resultsAvailableSignal returns the signal which is triggered when new results are waiting to be integrated
     * @note The signal is not called on the main thread, but on the backend worker thread
     */
    boost::signals2::signal<void()>& resultsAvailableSignal();

    /**
     * @brief resultIntegratedSignal returns the signal which is fired right after a change has been integrated.
     * The argument of the emitted signal is the ID of the completed operation.
     * @note The signal is fired during the processIntegrationQueue() method.
     */
    boost::signals2::signal<void(int)>& resultIntegratedSignal();

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

    boost::signals2::signal<void()> _resultsAvailableSignal;
    boost::signals2::signal<void(int)> _resultIntegratedSignal;

    std::mutex _queueMutex;
    std::queue<op::OperationResultsMessage> _integrationQueue;
  };

} // namespace persistence

#endif // PERSISTENCE_RESULTINTEGRATOR_H
