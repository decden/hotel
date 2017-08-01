#ifndef PERSISTENCE_DATASTREAM_H
#define PERSISTENCE_DATASTREAM_H

#include "persistence/datastreamobserver.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

namespace persistence
{
  /**
   * @brief The StreamableType enum holds all possible native data types a steam can have
   */
  enum class StreamableType { NullStream, Hotel, Reservation };

  /**
   * @brief Writable backend for data stream
   */
  class DataStream
  {
  public:
    DataStream(StreamableType streamType)
        : _streamId(0), _streamType(streamType), _isInitialized(false), _observer(nullptr)
    {}
    virtual ~DataStream() {}

    void connect(int streamId, DataStreamObserver* observer)
    {
      assert(_streamId == 0);
      assert(_observer == nullptr);
      _streamId = streamId;
      _observer = observer;
    }

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
    virtual void addItems(StreamableItems newItems)
    {
      std::lock_guard<std::mutex> lock(_pendingOperationsMutex);
      _pendingOperations.push_back(ItemsAdded{std::move(newItems)});
    }
    virtual void removeItems(std::vector<int> itemsToRemove)
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

  /**
   * @brief The SingleIdDataStream class is a datastream which contains only an object with a given id.
   * The fact that only one ID can be provided means that the stream will always "hold" at most 1 element.
   */
  class SingleIdDataStream : public DataStream
  {
  public:
    SingleIdDataStream(StreamableType streamType, int idFilter)
        : DataStream(streamType), _idFilter(idFilter)
    {}

    virtual void addItems(StreamableItems newItems) override
    {
      auto filteredList = boost::apply_visitor([id=_idFilter](auto& list) -> StreamableItems {
        typename std::remove_reference<decltype(list)>::type result;
        for (auto& elem : list)
          if (elem.id() == id)
            result.push_back(elem);
        return StreamableItems{result};
      }, newItems);

      DataStream::addItems(filteredList);
    }
    virtual void removeItems(std::vector<int> itemsToRemove) override
    {
      if (std::any_of(itemsToRemove.begin(), itemsToRemove.end(), [id=_idFilter](int x) { return x == id; }))
        DataStream::removeItems({_idFilter});
    }

  private:
    int _idFilter;
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
