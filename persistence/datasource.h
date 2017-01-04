#ifndef PERSISTENCE_DATASOURCE_H
#define PERSISTENCE_DATASOURCE_H

#include "persistence/op/operations.h"

#include "hotel/persistence/sqlitestorage.h"
#include "hotel/planning.h"

#include <memory>

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

  private:
    // Backing store for the data
    hotel::persistence::SqliteStorage _storage;

    std::unique_ptr<hotel::PlanningBoard> _planning;
    std::unique_ptr<hotel::HotelCollection> _hotels;
  };

} // namespace persistence

#endif // PERSISTENCE_DATASOURCE_H
