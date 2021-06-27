#ifndef GUI_PLANNINGWIDGET_MOVERESERVATIONTOOL_H
#define GUI_PLANNINGWIDGET_MOVERESERVATIONTOOL_H

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
     * @brief The MoveReservationTool class is a tool allowing reservation atoms to be moved vertically on the board
     */
    class MoveReservationTool : public Tool
    {
    public:
      MoveReservationTool();

      void init(Context& context) override;

      void load() override;
      void unload() override;
      void updateLayout() override;

      void reservationAdded(const hotel::Reservation& item) override;

      void atomMousePressEvent(const hotel::Reservation& reservation, const hotel::ReservationAtom& atom,
                               const QPointF& position) override;
      void mouseReleaseEvent(QMouseEvent* event, const QPointF& position) override;
      void mouseMoveEvent(QMouseEvent* event, const QPointF& position) override;
      void keyPressEvent(QKeyEvent* event) override;

      Qt::CursorShape getReservationAtomCursor(const hotel::ReservationAtom&) const override
      {
        return Qt::SizeAllCursor;
      }


    private:
      //! Information needed to draw on the planning widget
      Context* _context;

      struct ReservationGhost
      {
        std::unique_ptr<PlanningBoardReservationItem> item;
        std::unique_ptr<hotel::Reservation> temporaryReservation;

        // Information needed for interaction
        hotel::Reservation::ReservationStatus originalStatus;
        int originalRoomId;
        int atomIndex;
      };

      //! The list of temporary reservation items in the scene
      std::optional<ReservationGhost> _currentGhost;
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H
