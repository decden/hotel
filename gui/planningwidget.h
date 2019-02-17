#ifndef GUI_PLANNINGWIDGET_H
#define GUI_PLANNINGWIDGET_H

#include "gui/planningwidget/context.h"
#include "gui/planningwidget/datebarwidget.h"
#include "gui/planningwidget/planningboardlayout.h"
#include "gui/planningwidget/planningboardwidget.h"
#include "gui/planningwidget/roomlistwidget.h"
#include "gui/datastreamobserveradapter.h"

#include "persistence/sqlite/sqlitestorage.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include <QtCore/QObject>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QWidget>

#include <boost/date_time.hpp>
#include <boost/signals2.hpp>

#include <memory>
#include <vector>

namespace gui
{
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
    PlanningWidget(persistence::Backend& backend);

    void registerTool(const std::string& toolName, std::unique_ptr<gui::planningwidget::Tool> tool);
    void activateTool(const std::string& toolName);

  public slots:
    void setPivotDate(boost::gregorian::date pivotDate);

  signals:
    void pivotDateChanged(boost::gregorian::date pivotDate);
    void reservationDoubleClicked(const hotel::Reservation& reservation);

  protected:
    virtual void keyPressEvent(QKeyEvent* event) override;

  private:
    void emitReservationDoubleClicked(const hotel::Reservation& reservation) { emit reservationDoubleClicked(reservation); }

    // Event dispatching classes
    DataStreamObserverAdapter<hotel::Hotel> _hotelsStream;
    DataStreamObserverAdapter<hotel::Reservation> _reservationsStream;

    void reservationsAdded(const std::vector<hotel::Reservation>& reservations);
    void reservationsUpdated(const std::vector<hotel::Reservation>& reservations);
    void reservationsRemoved(const std::vector<int>& ids);
    void allReservationsRemoved();
    void hotelsAdded(const std::vector<hotel::Hotel>& hotels);
    void hotelsUpdated(const std::vector<hotel::Hotel>& hotels);
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
