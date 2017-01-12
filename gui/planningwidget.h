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

#include <QScrollBar>
#include <QWidget>

#include <boost/date_time.hpp>

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
  class PlanningWidget : public QWidget, public hotel::PlanningBoardObserver
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

    // PlanningBoardObserver interface
    virtual void itemsAdded(const std::vector<const hotel::Reservation*>& reservations) override;
    virtual void itemsRemoved(const std::vector<const hotel::Reservation*>& reservations) override;
    virtual void allItemsRemoved() override;

  private:
    // Layout objects, holding layout information for this widget

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
