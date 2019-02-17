#ifndef GUI_PLANNINGBOARDRESERVATIONITEM_H
#define GUI_PLANNINGBOARDRESERVATIONITEM_H

#include "gui/planningwidget/context.h"
#include "gui/planningwidget/planningboardlayout.h"

#include "hotel/reservation.h"

#include <QtWidgets/QGraphicsItem>

namespace gui
{
  namespace planningwidget
  {

    /**
     * @brief The PlanningBoardAtomItem class is the graphical equivalent to a ReservationAtom object
     *
     * It displays a single reserved room over a well defined date range. Multiple instances are grouped together in a
     * PlanningBoardReservationItem object.
     *
     * @see PlanningBoardWidget
     * @see PlanningBoardReservationItem
     */
    class PlanningBoardAtomItem : public QGraphicsRectItem
    {
    public:
      PlanningBoardAtomItem(const Context* context, const hotel::Reservation* reservation, int atomIndex,
                            QGraphicsItem* parent = nullptr);

      // QGraphicsRectItem interface
      virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

      void updateLayout();

    protected:
      //! @brief Reacts on selection changes and propagates them to the parent PlanningBoardReservationItem
      virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

      virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    private:
      const Context* _context;
      const hotel::Reservation* _reservation;
      int _atomIndex;
    };

    /**
     * @brief The PlanningBoardReservationItem class is the graphical equivalent to a Reservation object
     *
     * It displays all of the reservation atoms. It also displays connecting lines between the reservations when they
     * are
     * selected, so that the user can visually follow change of rooms.
     *
     * @see PlanningBoardWidget
     * @see PlanningBoardAtomItem
     */
    class PlanningBoardReservationItem : public QGraphicsItem
    {
    public:
      PlanningBoardReservationItem(Context* context, const hotel::Reservation* reservation,
                                   QGraphicsItem* parent = nullptr);
      virtual ~PlanningBoardReservationItem();

      // QGraphicsItem interface
      virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
      virtual QRectF boundingRect() const override;

      const hotel::Reservation* reservation() const { return _reservation; }

      /**
       * @brief setReservationSelected marks this item and all of its children as selected
       */
      void setReservationSelected(bool select);
      void updateLayout();

    private:
      Context* _context;
      const hotel::Reservation* _reservation;
      bool _isSelected;
      /**
       * @brief Set to true during a setReservationSelected() call to avoid reentrance and infinite recursion.
       *
       * This is necessary, since the individual atoms can trigger the selection of the whole reservation, and the
       * reservation can trigger the selection of all of the atoms.
       */
      bool _isUpdatingSelection;
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGBOARDRESERVATIONITEM_H
