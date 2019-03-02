#include "persistence/sqlite/sqlitebackend.h"

#include "persistence/changequeue.h"

#include <cassert>

namespace persistence
{
  namespace detail
  {
    class DefaultDataStreamHandler : public DataStreamHandler
    {
    public:
      virtual ~DefaultDataStreamHandler() = default;
      virtual void initialize(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, sqlite::SqliteStorage& storage) override
      {
        switch(stream.streamType())
        {
        case StreamableType::NullStream: return;
        case StreamableType::Hotel: return initializeTyped<hotel::Hotel>(stream, changeQueue, storage);
        case StreamableType::Reservation: return initializeTyped<hotel::Reservation>(stream, changeQueue, storage);
        }
      }

      virtual void addItems(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, const StreamableItems& items) override
      {
        changeQueue.push_back({stream.streamId(), DataStreamItemsAdded{items}});
      }

      virtual void updateItems(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, const StreamableItems& items) override
      {
        changeQueue.push_back({stream.streamId(), DataStreamItemsUpdated{items}});
      }

      virtual void removeItems(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, const std::vector<int> ids) override
      {
        changeQueue.push_back({stream.streamId(), DataStreamItemsRemoved{ids}});
      }

      virtual void clear(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue) override
      {
        changeQueue.push_back({stream.streamId(), DataStreamCleared{}});
      }

    private:
      template <class T>
      void initializeTyped(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, sqlite::SqliteStorage& storage)
      {
        auto items = storage.loadAll<T>();
        changeQueue.push_back({stream.streamId(), DataStreamItemsAdded{std::move(items)}});
      }
    };


    class SingleIdDataStreamHandler : public DataStreamHandler
    {
    public:
      virtual ~SingleIdDataStreamHandler() {}
      virtual void initialize(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, sqlite::SqliteStorage& storage) override
      {
        switch(stream.streamType())
        {
        case StreamableType::NullStream: return;
        case StreamableType::Hotel: return initializeTyped<hotel::Hotel>(stream, changeQueue, storage);
        case StreamableType::Reservation: return initializeTyped<hotel::Reservation>(stream, changeQueue, storage);
        }
      }

      virtual void addItems(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, const StreamableItems& items) override
      {
        int id = stream.streamOptions()["id"];
        auto filteredItems = filter(items, id);
        bool isEmpty = std::visit([](const auto& items) { return items.empty(); }, filteredItems);
        if (!isEmpty)
          changeQueue.push_back({stream.streamId(), DataStreamItemsAdded{filteredItems}});
      }

      virtual void updateItems(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, const StreamableItems& items) override
      {
        int id = stream.streamOptions()["id"];
        auto filteredItems = filter(items, id);
        bool isEmpty = std::visit([](const auto& items) { return items.empty(); }, filteredItems);
        if (!isEmpty)
          changeQueue.push_back({stream.streamId(), DataStreamItemsUpdated{filteredItems}});
      }

      virtual void removeItems(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, const std::vector<int> ids) override
      {
        int id = stream.streamOptions()["id"];
        if (std::any_of(ids.begin(), ids.end(), [id](int item) { return item == id; }))
          changeQueue.push_back({stream.streamId(), DataStreamItemsRemoved{{id}}});
      }

      virtual void clear(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue) override
      {
        changeQueue.push_back({stream.streamId(), DataStreamCleared{}});
      }

    private:
      StreamableItems filter(const StreamableItems& items, int id)
      {
        return std::visit([id](const auto& items) -> StreamableItems {
          typename std::remove_const<typename std::remove_reference<decltype(items)>::type>::type filteredItems;
          std::copy_if(items.begin(), items.end(), std::back_inserter(filteredItems), [id](const auto& item) { return item.id() == id; });
          return filteredItems;
        }, items);
      }

      template <class T>
      void initializeTyped(DataStream& stream, std::vector<DataStreamDifferential>& changeQueue, sqlite::SqliteStorage& storage)
      {
        auto id = stream.streamOptions()["id"];
        auto item = storage.loadById<T>(id);
        if (item != std::nullopt)
        {
          std::vector<T> items({*item});
          changeQueue.push_back({stream.streamId(), DataStreamItemsAdded{std::move(items)}});
        }
      }
    };


    DataStreamManager::DataStreamManager()
    {
      _streamHandlers[HandlerKey{StreamableType::NullStream, ""}] = std::make_unique<DefaultDataStreamHandler>();
      _streamHandlers[HandlerKey{StreamableType::Hotel, ""}] = std::make_unique<DefaultDataStreamHandler>();
      _streamHandlers[HandlerKey{StreamableType::Hotel, "hotel.by_id"}] = std::make_unique<SingleIdDataStreamHandler>();
      _streamHandlers[HandlerKey{StreamableType::Reservation, ""}] = std::make_unique<DefaultDataStreamHandler>();
      _streamHandlers[HandlerKey{StreamableType::Reservation, "reservation.by_id"}] = std::make_unique<SingleIdDataStreamHandler>();
    }

    void DataStreamManager::addNewStream(const std::shared_ptr<DataStream>& stream)
    {
      std::lock_guard<std::mutex> lock(_streamMutex);
      _uninitializedStreams.push_back(stream);
    }

    void DataStreamManager::removeStream(const std::shared_ptr<DataStream> &stream)
    {
      std::lock_guard<std::mutex> lock(_streamMutex);
      _uninitializedStreams.erase(std::remove(_uninitializedStreams.begin(), _uninitializedStreams.end(), stream), _uninitializedStreams.end());
      _activeStreams.erase(std::remove(_activeStreams.begin(), _activeStreams.end(), stream), _activeStreams.end());
    }

    void DataStreamManager::initialize(ChangeQueue &changeQueue, sqlite::SqliteStorage &storage)
    {
      // Copy the uninitialized streams to the active list while holding the lock.
      // The actual initialization is done while not holding the lock, since it may take a long time.
      std::vector<std::shared_ptr<DataStream>> uninitializedStreams;
      std::unique_lock<std::mutex> lock(_streamMutex);
      std::swap(uninitializedStreams, _uninitializedStreams);
      std::copy(uninitializedStreams.begin(), uninitializedStreams.end(), std::back_inserter(_activeStreams));
      lock.unlock();

      for (auto& uninitializedStream : uninitializedStreams)
      {
        auto streamPtr = uninitializedStream.get();
        auto streamHandler = findHandler(*streamPtr);
        std::vector<DataStreamDifferential> changes;
        if (streamHandler)
          streamHandler->initialize(*streamPtr, changes, storage);
        else
          std::cerr << "Cannot initialize stream, because there is no handler registered" << std::endl;
        changes.push_back({streamPtr->streamId(), DataStreamInitialized{}});
        for (auto& change : changes)
          changeQueue.addStreamChange(change.streamId, std::move(change.change));
      }
    }

    template<class T, class Func>
    void DataStreamManager::foreachStream(Func func)
    {
      foreachStream(DataStream::GetStreamTypeFor<T>(), func);
    }

    template<class Func>
    void DataStreamManager::foreachStream(StreamableType type, Func func)
    {
      std::unique_lock<std::mutex> lock(_streamMutex);
      for (auto& activeStream : _activeStreams)
      {
        auto handler = findHandler(*activeStream);
        assert(handler != nullptr);
        if (activeStream->streamType() == type && handler != nullptr)
          func(*activeStream, *handler);
      }
    }

    void DataStreamManager::addItems(std::vector<DataStreamDifferential>& changeQueue, StreamableType type, const StreamableItems items)
    {
      foreachStream(type, [&changeQueue, &items](DataStream& stream, DataStreamHandler& handler) {
        handler.addItems(stream, changeQueue, items);
      });
    }

    void DataStreamManager::updateItems(std::vector<DataStreamDifferential>& changeQueue, StreamableType type, const StreamableItems items)
    {
      foreachStream(type, [&changeQueue, &items](DataStream& stream, DataStreamHandler& handler) {
        handler.updateItems(stream, changeQueue, items);
      });
    }

    void DataStreamManager::removeItems(std::vector<DataStreamDifferential>& changeQueue, StreamableType type, std::vector<int> ids)
    {
      foreachStream(type, [&changeQueue, &ids](DataStream& stream, DataStreamHandler& handler) {
        handler.removeItems(stream, changeQueue, ids);
      });
    }

    void DataStreamManager::clear(std::vector<DataStreamDifferential>& changeQueue, StreamableType type)
    {
      foreachStream(type, [&changeQueue](DataStream& stream, DataStreamHandler& handler) {
        handler.clear(stream, changeQueue);
      });
    }

    DataStreamHandler *DataStreamManager::findHandler(const DataStream &stream)
    {
      auto it = _streamHandlers.find({stream.streamType(), stream.streamEndpoint()});
      return (it != _streamHandlers.end()) ? it->second.get() : nullptr;
    }

  } // namespace detail

  namespace sqlite
  {
    SqliteBackend::SqliteBackend(const std::string& databasePath)
      : _storage(databasePath), _nextOperationId(1), _nextStreamId(1), _backendThread(), _quitBackendThread(false),
        _workAvailableCondition(), _queueMutex(), _operationsQueue()
    {
      start();
    }

    SqliteBackend::~SqliteBackend()
    {
      stopAndJoin();
    }

    fas::Future<std::vector<TaskResult>> SqliteBackend::queueOperations(op::Operations operations)
    {
      auto [future, promise] = fas::makePromise<std::vector<TaskResult>>();

      auto pair = QueuedOperation{std::move(operations), std::move(promise)};
      {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _operationsQueue.push_back(std::move(pair));
      }
      _workAvailableCondition.notify_one();

      return std::move(future);
    }

    void SqliteBackend::start()
    {
      assert(!_backendThread.joinable());
      _backendThread = std::thread([this]() { this->threadMain(); });
    }

    void SqliteBackend::stopAndJoin()
    {
      if (_backendThread.joinable())
      {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _quitBackendThread = true;
        _workAvailableCondition.notify_all();
        lock.unlock();

        _backendThread.join();
      }
    }

    persistence::UniqueDataStreamHandle SqliteBackend::createStream(DataStreamObserver *observer, StreamableType type, const std::string &service, const nlohmann::json &options)
    {
      std::unique_lock<std::mutex> lock(_queueMutex);

      auto sharedState = std::make_shared<DataStream>(type, service, options);

      sharedState->connect(_nextStreamId++, observer);
      _changeQueue.addStream(sharedState);
      _dataStreams.addNewStream(sharedState);
      lock.unlock();

      _workAvailableCondition.notify_one();

      return persistence::UniqueDataStreamHandle(this, sharedState);
    }

    void SqliteBackend::removeStream(std::shared_ptr<DataStream> stream)
    {
      _dataStreams.removeStream(stream);
    }

    void SqliteBackend::threadMain()
    {
      while (!_quitBackendThread)
      {
        // Get the tasks we are going to process (This is the only part which is guarded by the mutex)
        std::unique_lock<std::mutex> lock(_queueMutex);
        std::vector<QueuedOperation> newTasks;
        std::swap(newTasks, _operationsQueue);
        const bool hasUninitializedStreams = _dataStreams.hasUninitializedStreams();
        // Sleep until there is work to do
        if (!_quitBackendThread && newTasks.empty() && !hasUninitializedStreams)
          _workAvailableCondition.wait(lock);
        lock.unlock();

        // Initialize new data streams
        _dataStreams.initialize(_changeQueue, _storage);

        // Process tasks
        for (auto& operationsMessage : newTasks)
        {
          _storage.beginTransaction();
          ChangeList transactionChanges;
          std::vector<persistence::TaskResult> results;
          bool rollback = false;
          for (auto& operation : operationsMessage.first)
          {
            auto result = std::visit([this, &transactionChanges](auto& op) { return this->executeOperation(op, transactionChanges.streamChanges); }, operation);
            bool succeeded = result.status != TaskResultStatus::Error;
            results.push_back(std::move(result));

            if (!succeeded)
            {
              rollback = true;
              break;
            }
          }

          if (rollback)
          {
            _storage.rollbackTransaction();
          }
          else
          {
            _storage.commitTransaction();
            _changeQueue.addChanges(std::move(transactionChanges));
          }
          operationsMessage.second.resolve(std::move(results));
        }
      }
    }

    TaskResult SqliteBackend::executeOperation(op::EraseAllData&, std::vector<DataStreamDifferential>& streamChanges)
    {
      _storage.deleteAll();

      _dataStreams.clear(streamChanges, StreamableType::Reservation);
      _dataStreams.clear(streamChanges, StreamableType::Hotel);

      return TaskResult{TaskResultStatus::Successful, {}};
    }

    TaskResult SqliteBackend::executeOperation(op::StoreNew& op, std::vector<DataStreamDifferential>& streamChanges)
    {
      bool isNull = std::visit([](const auto& item) { return item == nullptr; }, op.newItem);
      assert(!isNull );
      if (isNull )
        return TaskResult{TaskResultStatus::Successful, {{"message", "Trying to store empty item"}}};

      return std::visit([this, &streamChanges](const auto& newItem) {
        return this->executeStoreNew(*newItem, streamChanges);
      }, op.newItem);
    }

    TaskResult SqliteBackend::executeStoreNew(hotel::Hotel& hotel, std::vector<DataStreamDifferential>& streamChanges)
    {
      _storage.storeNewHotel(hotel);
      _dataStreams.addItems(streamChanges, StreamableType::Hotel, std::vector<hotel::Hotel>{{hotel}});
      return TaskResult{TaskResultStatus::Successful, {{"id", hotel.id()}}};
    }

    TaskResult SqliteBackend::executeStoreNew(hotel::Reservation& reservation, std::vector<DataStreamDifferential>& streamChanges)
    {
      _storage.storeNewReservationAndAtoms(reservation);
      _dataStreams.addItems(streamChanges, StreamableType::Reservation, std::vector<hotel::Reservation>{{reservation}});
      return TaskResult{TaskResultStatus::Successful, {{"id", reservation.id()}}};
    }

    TaskResult SqliteBackend::executeStoreNew(hotel::Person& person, std::vector<DataStreamDifferential>& streamChanges)
    {
      // TODO: Implement this
      std::cout << "STUB: This functionality has not yet been implemented..." << std::endl;
      return TaskResult{TaskResultStatus::Error, {{"message", "Not implemented yet!"}}};
    }

    TaskResult SqliteBackend::executeOperation(op::Update& op, std::vector<DataStreamDifferential>& streamChanges)
    {
      auto result = std::visit([this](const auto& item) {
        if (item == nullptr)
          return TaskResult{TaskResultStatus::Error, {{"message", "Trying to update empty item"}}};
        if (item->id() == 0)
          return TaskResult{TaskResultStatus::Error, {{"message", "Cannot update hotel without id"}}};
        if (!_storage.update(*item))
          return TaskResult{TaskResultStatus::Error, {{"message", "Could not update item (internal db error)"}}};

        return TaskResult{TaskResultStatus::Successful, {}};
      }, op.updatedItem);

      if (result.status == TaskResultStatus::Error)
        return result;

      // TODO: This should be implementable using
      std::visit([this, &streamChanges](const auto& updatedItem) {
        return this->executeUpdate(*updatedItem, streamChanges);
      }, op.updatedItem);

      return result;
    }

    void SqliteBackend::executeUpdate(const hotel::Hotel& hotel, std::vector<DataStreamDifferential> &streamChanges)
    {
      _dataStreams.updateItems(streamChanges, StreamableType::Hotel, std::vector<hotel::Hotel>{{hotel}});
    }

    void SqliteBackend::executeUpdate(const hotel::Reservation& reservation, std::vector<DataStreamDifferential> &streamChanges)
    {
      _dataStreams.updateItems(streamChanges, StreamableType::Reservation, std::vector<hotel::Reservation>{{reservation}});
    }

    void SqliteBackend::executeUpdate(const hotel::Person &person, std::vector<DataStreamDifferential> &streamChanges)
    {
      // TODO: Implement this
    }

    TaskResult SqliteBackend::executeOperation(op::Delete &op, std::vector<DataStreamDifferential> &streamChanges)
    {
      //if (op.type == persistence::op::StreamableType::Hotel)
      //  _storage.deleteById<hotel::Hotel>(op.id);
      if (op.type == persistence::op::StreamableType::Reservation)
      {
        _storage.deleteReservationById(op.id);
        _dataStreams.removeItems(streamChanges, StreamableType::Reservation, {op.id});
      }
      else
      {
        assert(false);
        return TaskResult{TaskResultStatus::Error, {{"message", "Unknown data type"}}};
      }

      return TaskResult{TaskResultStatus::Successful, {{"id", op.id}}};
    }

  } // namespace sqlite
} // namespace persistence
