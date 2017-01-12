#ifndef HOTEL_OBSERVABLECOLLECTION_H
#define HOTEL_OBSERVABLECOLLECTION_H

#include <set>
#include <vector>

namespace hotel
{
  template <class T>
  class ObservableCollection;

  template <class T>
  class CollectionObserver
  {
  public:
    CollectionObserver() : _observedCollection(nullptr) {}
    virtual ~CollectionObserver()
    {
      auto collection = _observedCollection;
      _observedCollection = nullptr;
      collection->removeObserver(this);
    }

    // Update methods called by the collection when its contents change
    virtual void itemsAdded(const std::vector<T>& reservations) = 0;
    virtual void itemsRemoved(const std::vector<T>& reservations) = 0;

    // Update methods called when changing/setting the collection
    virtual void allItemsRemoved() = 0;

  protected:
    friend class ObservableCollection<T>;
    const ObservableCollection<T>* observedCollection() const { return _observedCollection; }

  private:
    //! This method is called from ObservableCollection when addObserver is called
    void setObservedCollection(ObservableCollection<T>* observedCollection)
    {
      if (_observedCollection != nullptr && observedCollection == nullptr)
        allItemsRemoved();
      _observedCollection = observedCollection;
    }

    ObservableCollection<T>* _observedCollection;
  };

  /**
   * @brief the ObservableCollection enable observers to get notified of changes to the collection
   */
  template <class T>
  class ObservableCollection
  {
  public:
    ObservableCollection& operator=(const ObservableCollection& that) = delete;
    ObservableCollection& operator=(ObservableCollection&& that) = default;
    virtual ~ObservableCollection()
    {
      for (auto observer : _observers)
        observer->setObservedCollection(nullptr);
    }

    void addObserver(CollectionObserver<T>* observer)
    {
      // It is a violation of the contract to add a collection observer which is actively observing a collection
      assert(observer->observedCollection() == nullptr);
      observer->setObservedCollection(this);
      _observers.insert(observer);
    }

    void removeObserver(CollectionObserver<T>* observer)
    {
      auto it = _observers.find(observer);
      if (it != _observers.end())
      {
        _observers.erase(it);
        (*it)->setObservedCollection(nullptr);
      }
    }

    bool hasObservers() const { return !_observers.empty(); }

    template <class Func>
    void foreachObserver(Func f)
    {
      for (auto& observer : _observers)
        f(*observer);
    }

  private:
    //! List fo observers which are notified of changes
    std::set<CollectionObserver<T>*> _observers;
  };

} // namespace hotel

#endif // HOTEL_OBSERVABLECOLLECTION_H
