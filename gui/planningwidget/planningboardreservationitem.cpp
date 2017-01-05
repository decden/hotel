#include "gui/planningwidget/planningboardreservationitem.h"

#include <QPainter>

namespace gui
{
  namespace planningwidget
  {
    PlanningBoardAtomItem::PlanningBoardAtomItem(const Context* context, const hotel::Reservation* reservation,
                                                 const hotel::ReservationAtom* atom, QGraphicsItem* parent)
        : QGraphicsRectItem(parent), _context(context), _reservation(reservation), _atom(atom)
    {
      setFlag(QGraphicsItem::ItemIsSelectable);
      updateLayout();
    }

    void PlanningBoardAtomItem::updateLayout()
    {
      auto itemRect = _context->layout().getAtomRect(_atom->roomId(), _atom->dateRange());
      setRect(itemRect);
    }

    void PlanningBoardAtomItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
      auto renderer = _context->appearance().reservationRenderer();
      renderer->paintAtom(painter, *_context, *_reservation, *_atom, rect(), isSelected());
    }

    QVariant PlanningBoardAtomItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
    {
      // Apply change locally
      auto newValue = QGraphicsRectItem::itemChange(change, value);

      // Propagate selection changes to the parent, i.e. the PlanningBoardReservationItem
      if (change == QGraphicsItem::ItemSelectedChange)
        static_cast<PlanningBoardReservationItem*>(parentItem())->setReservationSelected(value.toBool());
      return newValue;
    }

    PlanningBoardReservationItem::PlanningBoardReservationItem(const Context *context,
                                                               const hotel::Reservation* reservation,
                                                               QGraphicsItem* parent)
        : QGraphicsItem(parent), _context(context), _reservation(reservation), _isSelected(false),
          _isUpdatingSelection(false)
    {
      for (auto& atom : _reservation->atoms())
      {
        auto item = new PlanningBoardAtomItem(context, _reservation, atom.get());
        item->setParentItem(this);
      }
    }

    void PlanningBoardReservationItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
      std::vector<QRectF> rects;
      rects.reserve(childItems().size());
      for (auto child : childItems())
        rects.push_back(child->boundingRect());

      auto renderer = _context->appearance().reservationRenderer();
      renderer->paintReservationConnections(painter, *_context, rects, _isSelected);
    }

    QRectF PlanningBoardReservationItem::boundingRect() const { return childrenBoundingRect(); }

    void PlanningBoardReservationItem::setReservationSelected(bool select)
    {
      // Avoid reentrance
      if (_isUpdatingSelection)
        return;
      if (_isSelected == select)
        return;
      _isUpdatingSelection = true;
      _isSelected = select;

      // Update selection of the children
      for (auto child : childItems())
        child->setSelected(select);

      // If items are selected pop them in the foreground
      setZValue(select ? 1 : 0);

      _isUpdatingSelection = false;
    }

    void PlanningBoardReservationItem::updateLayout()
    {
      for (auto item : childItems())
      {
        auto atomItem = dynamic_cast<PlanningBoardAtomItem*>(item);
        if (atomItem != nullptr)
          atomItem->updateLayout();
      }
    }

  } // namespace planningwidget
} // namespace gui
