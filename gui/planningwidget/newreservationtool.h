#ifndef GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H
#define GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H

#include "gui/planningwidget/context.h"
#include "gui/planningwidget/planningboardlayout.h"
#include "gui/planningwidget/tool.h"

#include <QGraphicsRectItem>
#include <QMouseEvent>

#include <boost/date_time.hpp>

#include <vector>

namespace gui
{
  namespace planningwidget
  {
    using namespace boost::gregorian;

    class ReservationGhostItem : public QGraphicsRectItem
    {
    public:
      ReservationGhostItem(const Context& context)
          : QGraphicsRectItem(), reservation("new"), atom(-1, date_period(date(), date())), _context(context)
      {
        setZValue(2);
        reservation.setStatus(hotel::Reservation::Temporary);
      }

      // QGraphicsRectItem interface
      virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

      void updateLayout()
      {
        auto rect = _context.layout().getAtomRect(atom.roomId(), atom.dateRange());
        setRect(rect);
      }

      hotel::Reservation reservation;
      hotel::ReservationAtom atom;
      boost::gregorian::date maximumDate;

    private:
      const Context& _context;
    };



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
      //! Information needed to draw on the planning widget
      Context* _context;

      //! The list of temporary reservation items in the scene
      std::vector<ReservationGhostItem*> _ghosts;
      ReservationGhostItem* _currentGhost;

    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H
