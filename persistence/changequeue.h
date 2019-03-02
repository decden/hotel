
#ifndef PERSISTENCE_RESULTINTEGRATOR_H
#define PERSISTENCE_RESULTINTEGRATOR_H

#include "persistence/datastream.h"
#include "persistence/taskresult.h"
#include "persistence/op/operations.h"

#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include "boost/signals2.hpp"

#include <vector>
#include <mutex>
#include <queue>

namespace persistence
{
  struct DataStreamDifferential
  {
    int streamId;
    DataStreamChange change;
  };

  /**
   * @brief The ChangeList class holds a list of changes to tasks and streams
   */
  struct ChangeList
  {
    std::vector<DataStreamDifferential> streamChanges;
  };

  /**
   * @brief The ChangeQueue class collects changes from a backend and makes them available to the backend.
   */
  class ChangeQueue
  {
  public:
    ChangeQueue() = default;
    ~ChangeQueue() = default;

    // Methods used by data source:

    /**
     * @brief addStream adds a stream for which changes will be tracked.
     *
     * Without adding a stream, any collected change, referencing the stream by id will not be applied. The backend is
     * responsible for adding the stream to the change queue.
     *
     * @param dataStream
     */
    void addStream(std::shared_ptr<DataStream> dataStream);

    /**
     * @brief hasUninitializedStreams returns whethere there are still streams for which the initial data has not yet been set.
     * @return true if at least one stream has not yet received its initial data.
     */
    bool hasUninitializedStreams() const;

    // Methods for applying pending changes in the main thread

    void applyStreamChanges();

    // Methods used by backend

    void addChanges(ChangeList list);
    void addStreamChange(int streamId, DataStreamChange change);

    boost::signals2::connection connectToStreamChangesAvailableSignal(boost::signals2::slot<void()> slot);

  private:
    std::vector<std::shared_ptr<DataStream>> _dataStreams;

    std::mutex _streamChangesMutex;
    std::mutex _completedTasksMutex;
    ChangeList _changeList;

    boost::signals2::signal<void()> _streamChangesAvailableSignal;
  };

} // namespace persistence

#endif // PERSISTENCE_RESULTINTEGRATOR_H
