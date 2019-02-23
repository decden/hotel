#ifndef GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H
#define GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H

#include "gui/planningwidget/context.h"
#include "gui/planningwidget/planningboardlayout.h"
#include "gui/planningwidget/planningboardreservationitem.h"
#include "gui/planningwidget/tool.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QGraphicsRectItem>

#include <boost/date_time.hpp>

#include <optional>
#include <vector>

namespace gui
{
  namespace planningwidget
  {
    using namespace boost::gregorian;

    /**
     * @brief The NewReservationTool class is a tool allowing new reservations to be created on the board
     */
    class NewReservationTool : public Tool
    {
    public:
      NewReservationTool();

      virtual void init(Context& context) override;

      virtual void load() override;
      virtual void unload() override;
      virtual void updateLayout() override;

      virtual void reservationAdded(const hotel::Reservation& item) override;

      virtual void mousePressEvent(QMouseEvent *event, const QPointF &position) override;
      virtual void mouseReleaseEvent(QMouseEvent *event, const QPointF &position) override;
      virtual void mouseMoveEvent(QMouseEvent *event, const QPointF& position) override;
      virtual void keyPressEvent(QKeyEvent* event) override;

    private:
      boost::gregorian::date computeMaximumDate(int roomId, boost::gregorian::date fromDate);

      //! Information needed to draw on the planning widget
      Context* _context;

      struct ReservationGhost
      {
        std::unique_ptr<PlanningBoardReservationItem> item;
        std::unique_ptr<hotel::Reservation> temporaryReservation;

        // Information needed for interaction
        int startRoomId;
        boost::gregorian::date startDate;
      };

      //! The list of temporary reservation items in the scene
      std::vector<ReservationGhost> _ghosts;
      std::optional<ReservationGhost> _currentGhost;
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H
