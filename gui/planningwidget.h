#ifndef GUI_PLANNINGWIDGET_H
#define GUI_PLANNINGWIDGET_H

#include "gui/planningwidget/datebarwidget.h"
#include "gui/planningwidget/planningboardlayout.h"
#include "gui/planningwidget/planningboardwidget.h"
#include "gui/planningwidget/roomlistwidget.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include "hotel/persistence/sqlitestorage.h"

#include <QScrollBar>
#include <QWidget>

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
  public:
    PlanningWidget(std::unique_ptr<hotel::persistence::SqliteStorage> storage);

  protected:
    virtual void keyPressEvent(QKeyEvent* event) override;

  private:
    // Layout objects, holding layout information for this widget
    planningwidget::PlanningBoardLayout _layout;
    QRect _sceneRect;

    // Widgets
    QScrollBar* _verticalScrollbar;
    QScrollBar* _horizontalScrollbar;
    planningwidget::PlanningBoardWidget* _planningBoard;
    planningwidget::RoomListWidget* _roomList;
    planningwidget::DateBarWidget* _dateBar;

    // Planning data
    std::unique_ptr<hotel::HotelCollection> _hotelsData;
    std::unique_ptr<hotel::PlanningBoard> _planningData;
  };

} // namespace gui

#endif // GUI_PLANNINGWIDGET_H
