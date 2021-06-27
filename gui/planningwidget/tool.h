#ifndef GUI_PLANNINGWIDGET_TOOL_H
#define GUI_PLANNINGWIDGET_TOOL_H

#include "hotel/hotel.h"
#include "hotel/reservation.h"

#include <Qt>

class QContextMenuEvent;
class QGraphicsScene;
class QKeyEvent;
class QMouseEvent;
class QPointF;

namespace gui
{
  namespace planningwidget
  {
    class Context;

    /**
     * @brief The Tool class is the base class of all tools used to interact in special ways with the planning board
     */
    class Tool
    {
    public:
      Tool();
      virtual ~Tool() {}

      // Load and update routines

      /**
       * @brief init is called when the tool is added to the reservation widget
       * This method is used by the reservation widget to tell the tool the information needed to draw and paint on the
       * canvas. This method is called only once during the tool's lifetime
       */
      virtual void init(Context& context) = 0;

      /**
       * @brief load is called before the tool is activated
       */
      virtual void load() = 0;

      /**
       * @brief unload is called before the tool is deactivated.
       * In this method the tool should clear all of the temporary items it may have allocated and added to the scene.
       */
      virtual void unload() = 0;

      /**
       * @brief updateLayout is called whenever the layout of the board changes.
       * A layout change might be triggered e.g. by a reordering or filtering of the rows.
       */
      virtual void updateLayout() = 0;

      // Data modification callbacks

      virtual void reservationAdded(const hotel::Reservation&) {}
      virtual void reservationRemoved([[maybe_unused]] const int id) {}

      // Event handling routines

      virtual void mousePressEvent(QMouseEvent*, const QPointF&) {}
      virtual void mouseReleaseEvent(QMouseEvent*, const QPointF&) {}
      virtual void mouseMoveEvent(QMouseEvent*, const QPointF&) {}
      virtual void contextMenuEvent(QContextMenuEvent*) {}
      virtual void keyPressEvent(QKeyEvent*) {}

      virtual void atomMousePressEvent(const hotel::Reservation&, const hotel::ReservationAtom&, const QPointF&) {}
      virtual void atomMouseReleaseEvent(const hotel::Reservation&, const hotel::ReservationAtom&, const QPointF&) {}

      // Callbacks

      virtual Qt::CursorShape getReservationAtomCursor(const hotel::ReservationAtom&) const
      {
        return Qt::PointingHandCursor;
      }

    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGWIDGET_TOOL_H
