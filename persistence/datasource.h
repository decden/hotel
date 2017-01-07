#ifndef PERSISTENCE_DATASOURCE_H
#define PERSISTENCE_DATASOURCE_H

#include "persistence/op/operations.h"
#include "persistence/op/results.h"
#include "persistence/sqlite/sqlitebackend.h"

#include "hotel/planning.h"

#include <memory>
#include <deque>

namespace persistence
{
  /**
   * @brief The DataSource class handles all writing and reading access to the data.
   */
  class DataSource
  {
  public:
    DataSource(const std::string& databaseFile);

    hotel::HotelCollection& hotels();
    const hotel::HotelCollection& hotels() const;
    hotel::PlanningBoard& planning();
    const hotel::PlanningBoard& planning() const;

    /**
     * @brief queueOperation queues a given operation to perform on the data
     * @param operation The operation to perform
     *
     * @note The operation might be performed immediately or queued for later execution
     */
    void queueOperation(op::Operation operation);

    /**
     * @brief queueOperations queues multiple operations
     * The operations are executed together under a transaction if possible.
     * @param operations List of operations to perform.
     */
    void queueOperations(std::vector<op::Operation> operations);

  private:
    void processQueue();
    void processIntegrationQueue();

    void integrateResult(op::NoResult& res);
    void integrateResult(op::EraseAllDataResult& res);
    void integrateResult(op::LoadInitialDataResult& res);
    void integrateResult(op::StoreNewReservationResult& res);
    void integrateResult(op::StoreNewHotelResult& res);
    void integrateResult(op::StoreNewPersonResult& res);

    // Backing store for the data
    persistence::sqlite::SqliteBackend _backend;

    hotel::PlanningBoard _planning;
    hotel::HotelCollection _hotels;

    std::deque<op::Operations> _operationsQueue;
    std::deque<op::OperationResults> _integrationQueue;
  };

} // namespace persistence

#endif // PERSISTENCE_DATASOURCE_H
