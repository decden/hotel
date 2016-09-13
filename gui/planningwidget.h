#ifndef GUI_PLANNINGWIDGET_H
#define GUI_PLANNINGWIDGET_H

#include "gui/datebarwidget.h"
#include "gui/planningboardlayout.h"
#include "gui/planningboardwidget.h"
#include "gui/roomlistwidget.h"

#include "hotel/hotel.h"
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

  private:
    // Layout objects, holding layout information for this widget
    PlanningBoardLayout _layout;
    QRect _sceneRect;

    // Widgets
    QScrollBar* _verticalScrollbar;
    QScrollBar* _horizontalScrollbar;
    gui::PlanningBoardWidget* _planningBoard;
    gui::RoomListWidget* _roomList;
    gui::DateBarWidget* _dateBar;

    // Planning data
    std::vector<std::unique_ptr<hotel::Hotel>> _hotelsData;
    std::unique_ptr<hotel::PlanningBoard> _planningData;
  };

} // namespace gui

#endif // GUI_PLANNINGWIDGET_H
