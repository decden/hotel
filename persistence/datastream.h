#ifndef PERSISTENCE_DATASTREAM_H
#define PERSISTENCE_DATASTREAM_H

#include "hotel/hotel.h"
#include "hotel/reservation.h"

#include <boost/variant.hpp>

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

namespace persistence
{
  typedef boost::variant<std::vector<hotel::Hotel>,
                         std::vector<hotel::Reservation>> StreamableItems;
  enum class StreamableType { Hotel, Reservation };

  class DataStreamObserver
  {
  public:
    virtual ~DataStreamObserver() {}

    virtual void addItems(const StreamableItems& items) = 0;
    virtual void removeItems(const std::vector<int>& ids) = 0;
    virtual void clear() = 0;
    virtual void initialized() = 0;
  };

  template <class T>
  class DataStreamObserverTyped : public DataStreamObserver
  {
  public:
    virtual ~DataStreamObserverTyped() {}

    virtual void addItems(const StreamableItems& items) final override { addItems(boost::get<std::vector<T>>(items)); }
    virtual void addItems(const std::vector<T>& items) = 0;
  };

  template <class T>
  class VectorDataStreamObserver : public DataStreamObserverTyped<T>
  {
  public:
    const std::vector<T>& items() const { return _dataItems; }

    virtual void addItems(const std::vector<T>& items) override
    {
      std::copy(items.begin(), items.end(), std::back_inserter(_dataItems));
    }
    virtual void removeItems(const std::vector<int>& ids) override
    {
      _dataItems.erase(std::remove_if(_dataItems.begin(), _dataItems.end(),[&ids](const T& item) {
        return std::find(ids.begin(), ids.end(), item.id()) != ids.end();
      }), _dataItems.end());
    }
    virtual void clear() override { _dataItems.clear(); }
    virtual void initialized() override {}

  private:
    std::vector<T> _dataItems;
  };

  /**
   * @brief Writable backend for data stream
   */
  class DataStream
  {
  public:
    DataStream(int streamId, StreamableType streamType, DataStreamObserver* observer)
        : _streamId(streamId), _streamType(streamType), _isInitialized(false), _observer(observer)
    {}

    /**
     * @brief integrateChanges integrates all outstanding changes
     *
     * Calling this method will inform the observer of all the changes that have happened. Be sure to call this method
     * on the main thread. This method is usually called by the ResultIntegrator class.
     */
    void integrateChanges()
    {
      std::lock_guard<std::mutex> lock(_pendingOperationsMutex);
      if (_observer)
      {
        for (auto& operation : _pendingOperations)
          boost::apply_visitor([this](const auto& op) { return this->integrate(op); }, operation);
      }
      _pendingOperations.clear();
    }

    // Interface for backend
    void addItems(StreamableItems newItems)
    {
      std::lock_guard<std::mutex> lock(_pendingOperationsMutex);
      _pendingOperations.push_back(ItemsAdded{std::move(newItems)});
    }
    void removeItems(std::vector<int> itemsToRemove)
    {
      std::lock_guard<std::mutex> lock(_pendingOperationsMutex);
      _pendingOperations.push_back(ItemsRemoved{std::move(itemsToRemove)});
    }
    void clear()
    {
      std::lock_guard<std::mutex> lock(_pendingOperationsMutex);
      _pendingOperations.push_back(Cleared());
    }
    void setInitialized()
    {
      std::lock_guard<std::mutex> lock(_pendingOperationsMutex);
      _pendingOperations.push_back(Initialized());
    }

    //! Returns the unique ID of this stream
    int streamId() const { return _streamId; }
    //! Returns the datatype of this stream
    StreamableType streamType() const { return _streamType; }
    //! Returns true if there is still an observer listening on this stream
    bool isValid() const { return _observer != nullptr; }
    //! Returns true if the initial data for the observer has already been set
    bool isInitialized() const { return _isInitialized; }
    //! Dissociates the stream from the observer.
    void disconnect() { _observer = nullptr; }

    template <class T>
    static StreamableType GetStreamTypeFor();
  private:

    struct ItemsAdded { StreamableItems newItems; };
    struct ItemsRemoved { std::vector<int> removedItems; };
    struct Initialized { };
    struct Cleared {};
    typedef boost::variant<ItemsAdded, ItemsRemoved, Initialized, Cleared> StreamOperation;

    // Private help methods called by the integrateChanges() method. _observer is guaranteed to be not null when these
    // functions are called
    void integrate(const ItemsAdded& op) { _observer->addItems(op.newItems); }
    void integrate(const ItemsRemoved &op) { _observer->removeItems(op.removedItems); }
    void integrate(const Initialized&) { _isInitialized = true; _observer->initialized(); }
    void integrate(const Cleared&) { _observer->clear(); }

    int _streamId;
    StreamableType _streamType;
    bool _isInitialized;
    DataStreamObserver* _observer;

    std::mutex _pendingOperationsMutex;
    std::vector<StreamOperation> _pendingOperations;
  };

  class UniqueDataStreamHandle
  {
  public:
    UniqueDataStreamHandle()
        : _dataStream(nullptr)
    {}
    UniqueDataStreamHandle(std::shared_ptr<DataStream> dataStream)
        : _dataStream(dataStream)
    {}

    UniqueDataStreamHandle(const UniqueDataStreamHandle& that) = delete;
    UniqueDataStreamHandle& operator=(const UniqueDataStreamHandle& that) = delete;

    UniqueDataStreamHandle(UniqueDataStreamHandle&& that) = default;
    UniqueDataStreamHandle& operator=(UniqueDataStreamHandle&& that) = default;
    ~UniqueDataStreamHandle() { if (_dataStream) _dataStream->disconnect(); }

  private:
    std::shared_ptr<DataStream> _dataStream;
  };

}

#endif // PERSISTENCE_DATASTREAM_H
