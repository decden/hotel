#ifndef GUI_PLANNINGWIDGET_H
#define GUI_PLANNINGWIDGET_H

#include "gui/planningwidget/context.h"
#include "gui/planningwidget/datebarwidget.h"
#include "gui/planningwidget/planningboardlayout.h"
#include "gui/planningwidget/planningboardwidget.h"
#include "gui/planningwidget/roomlistwidget.h"

#include "persistence/datasource.h"
#include "persistence/sqlite/sqlitestorage.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include <QObject>
#include <QScrollBar>
#include <QWidget>

#include <boost/date_time.hpp>
#include <boost/signals2.hpp>

#include <memory>
#include <vector>

namespace gui
{
  class PlanningWidget;

  template <class T>
  class DataStreamObserverAdapter : public persistence::DataStreamObserver<T>
  {
  public:
    void connect(persistence::DataSource& dataSource) { _streamHandle = dataSource.connectToStream(this); }

    // DataStreamObserver<T> interface
    virtual void addItems(const std::vector<T>& items) override { itemsAddedSignal(items); }
    virtual void removeItems(const std::vector<int>& ids) override { itemsRemovedSignal(ids); }
    virtual void clear() override { allItemsRemovedSignal(); }

    // Public signals
    boost::signals2::signal<void(const std::vector<T>&)> itemsAddedSignal;
    boost::signals2::signal<void(const std::vector<int>&)> itemsRemovedSignal;
    boost::signals2::signal<void()> allItemsRemovedSignal;

    persistence::UniqueDataStreamHandle<T> _streamHandle;
  };

  /**
   * @brief The PlanningWidget class displays a list of reservations
   *
   * In a nutshell, the planning widget is composed of three main parts: A list of rooms, a date bar and a planning
   * board containing all of the reservations.
   */
  class PlanningWidget : public QWidget
  {
    Q_OBJECT
  public:
    PlanningWidget(persistence::DataSource& dataSource);

    void registerTool(const std::string& toolName, std::unique_ptr<gui::planningwidget::Tool> tool);
    void activateTool(const std::string& toolName);

  public slots:
    void setPivotDate(boost::gregorian::date pivotDate);

  signals:
    void pivotDateChanged(boost::gregorian::date pivotDate);

  protected:
    virtual void keyPressEvent(QKeyEvent* event) override;

  private:
    // Event dispatching classes
    DataStreamObserverAdapter<hotel::Hotel> _hotelsStream;
    DataStreamObserverAdapter<hotel::Reservation> _reservationsStream;

    void reservationsAdded(const std::vector<hotel::Reservation>& reservations);
    void reservationsRemoved(const std::vector<int>& ids);
    void allReservationsRemoved();
    void hotelsAdded(const std::vector<hotel::Hotel>& hotels);
    void hotelsRemoved(const std::vector<int>& ids);
    void allHotelsRemoved();

    // Widgets
    QScrollBar* _verticalScrollbar;
    QScrollBar* _horizontalScrollbar;
    planningwidget::PlanningBoardWidget* _planningBoard;
    planningwidget::RoomListWidget* _roomList;
    planningwidget::DateBarWidget* _dateBar;

    // Shared widget state
    planningwidget::Context _context;

    void updateDateRange();

    // Triggers a layout update of all the widgets
    void updateLayout();
  };

} // namespace gui

#endif // GUI_PLANNINGWIDGET_H
