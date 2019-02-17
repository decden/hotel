#ifndef PERSISTENCE_DATASTREAMOBSERVER_H
#define PERSISTENCE_DATASTREAMOBSERVER_H

#include "hotel/hotel.h"
#include "hotel/reservation.h"

#include <algorithm>
#include <vector>
#include <variant>

namespace persistence
{
  typedef std::variant<std::vector<hotel::Hotel>, std::vector<hotel::Reservation>> StreamableItems;

  /**
   * @brief The DataStreamObserver class is the baseclass for all classes who want to listen to datastreams
   *
   * @note In contrast to DataStreamObserverTyped, this class is only losely typed. In fact addItems will receive as
   *       input a variant of all supported data types. Only one data type is supported for streams however. You should
   *       always use DataStreamObserverTyped for your own classes.
   */
  class DataStreamObserver
  {
  public:
    virtual ~DataStreamObserver() {}

    virtual void addItems(const StreamableItems& items) = 0;
    virtual void updateItems(const StreamableItems& items) = 0;
    virtual void removeItems(const std::vector<int>& ids) = 0;
    virtual void clear() = 0;
    virtual void initialized() = 0;
  };

  template <class T>
  class DataStreamObserverTyped : public DataStreamObserver
  {
  public:
    virtual ~DataStreamObserverTyped() {}

    virtual void addItems(const StreamableItems& items) final override { addItems(std::get<std::vector<T>>(items)); }
    virtual void addItems(const std::vector<T>& items) = 0;
    virtual void updateItems(const StreamableItems& items) final override { updateItems(std::get<std::vector<T>>(items)); }
    virtual void updateItems(const std::vector<T>& items) = 0;
  };

  /**
   * @brief Simple implementation of DataStreamObserver which stores all elements into a vector
   */
  template <class T>
  class VectorDataStreamObserver : public DataStreamObserverTyped<T>
  {
  public:
    /**
     * @brief items returns the list of all items in the stream
     * @return a reference to the internally held vector with all of the items in the stream.
     */
    const std::vector<T>& items() const { return _dataItems; }

    virtual void addItems(const std::vector<T>& items) override
    {
      std::copy(items.begin(), items.end(), std::back_inserter(_dataItems));
    }
    virtual void updateItems(const std::vector<T>& items) override
    {
      for (auto& updatedItem : items)
      {
        auto it = std::find_if(_dataItems.begin(), _dataItems.end(), [&updatedItem](const T& item) {
          return item.id() == updatedItem.id();
        });
        if (it != _dataItems.end())
          *it = updatedItem;
      }
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
}

#endif // PERSISTENCE_DATASTREAMOBSERVER_H
