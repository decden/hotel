#ifndef GUI_DATASTREAMOBSERVERADAPTER_H
#define GUI_DATASTREAMOBSERVERADAPTER_H

#include "persistence/datasource.h"
#include "persistence/datastreamobserver.h"

#include <boost/signals2.hpp>

#include <memory>


namespace gui
{
  template <class T>
  class DataStreamObserverAdapter : public persistence::DataStreamObserverTyped<T>
  {
  public:
    void connect(persistence::DataSource& dataSource, const std::string& endpoint = "", const nlohmann::json& options = {})
    {
      _streamHandle = dataSource.connectToStream(this, endpoint, options);
    }

    // DataStreamObserver<T> interface
    virtual void addItems(const std::vector<T>& items) override { itemsAddedSignal(items); }
    virtual void updateItems(const std::vector<T>& items) override { itemsUpdatedSignal(items); }
    virtual void removeItems(const std::vector<int>& ids) override { itemsRemovedSignal(ids); }
    virtual void clear() override { allItemsRemovedSignal(); }
    virtual void initialized() override { initializedSignal(); }

    // Public signals
    boost::signals2::signal<void(const std::vector<T>&)> itemsAddedSignal;
    boost::signals2::signal<void(const std::vector<T>&)> itemsUpdatedSignal;
    boost::signals2::signal<void(const std::vector<int>&)> itemsRemovedSignal;
    boost::signals2::signal<void()> allItemsRemovedSignal;
    boost::signals2::signal<void()> initializedSignal;

    persistence::UniqueDataStreamHandle _streamHandle;
  };

} // namespace gui

#endif // GUI_DATASTREAMOBSERVERADAPTER_H
