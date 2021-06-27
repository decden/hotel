#include "gui/planningwidget/planningboardreservationitem.h"
#include "gui/planningwidget/tool.h"

#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QtGui/QPainter>

namespace gui
{
  namespace planningwidget
  {
    PlanningBoardAtomItem::PlanningBoardAtomItem(Context* context, const hotel::Reservation* reservation,
                                                 int atomIndex, QGraphicsItem* parent)
        : QGraphicsRectItem(parent), _context(context), _reservation(reservation), _atomIndex(atomIndex)
    {
      setFlag(QGraphicsItem::ItemIsSelectable);
      setAcceptHoverEvents(true);
    }

    void PlanningBoardAtomItem::updateLayout()
    {
      QRectF itemRect;

      auto atom = _reservation->atomAtIndex(_atomIndex);
      if (atom != nullptr)
        itemRect = _context->layout().getAtomRect(atom->roomId(), atom->dateRange());

      setRect(itemRect);
    }

    void PlanningBoardAtomItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/,
                                      QWidget* /*widget*/)
    {
      auto renderer = _context->appearance().reservationRenderer();
      auto atom = _reservation->atomAtIndex(_atomIndex);

      if (atom != nullptr)
        renderer->paintAtom(painter, *_context, *_reservation, *atom, rect(), isSelected());
    }

    QVariant PlanningBoardAtomItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
    {
      // Apply change locally
      auto newValue = QGraphicsRectItem::itemChange(change, value);

      // Propagate selection changes to the parent, i.e. the PlanningBoardReservationItem
      if (change == QGraphicsItem::ItemSelectedChange)
        dynamic_cast<PlanningBoardReservationItem*>(parentItem())->setReservationSelected(value.toBool());
      return newValue;
    }

    void PlanningBoardAtomItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* /*event*/)
    {
      _context->emitReservationDoubleClicked(*_reservation);
    }

    void PlanningBoardAtomItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
    {
      QGraphicsRectItem::mousePressEvent(event);

      auto tool = _context->activeTool();
      if (tool)
        tool->atomMousePressEvent(*_reservation, *_reservation->atomAtIndex(_atomIndex), event->scenePos());
    }
    void PlanningBoardAtomItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
    {
      QGraphicsRectItem::mouseReleaseEvent(event);

      auto tool = _context->activeTool();
      if (tool)
        tool->atomMouseReleaseEvent(*_reservation, *_reservation->atomAtIndex(_atomIndex), event->scenePos());
    }

    void PlanningBoardAtomItem::hoverEnterEvent(QGraphicsSceneHoverEvent* /*event*/)
    {
      auto cursor = Qt::PointingHandCursor;
      auto tool = _context->activeTool();
      if (tool)
      {
        auto atom = _reservation->atomAtIndex(_atomIndex);
        cursor = tool->getReservationAtomCursor(*atom);
      }
      setCursor(cursor);
    }

    PlanningBoardReservationItem::PlanningBoardReservationItem(Context* context, const hotel::Reservation* reservation,
                                                               QGraphicsItem* parent)
        : QGraphicsItem(parent), _context(context), _reservation(reservation), _isSelected(false),
          _isUpdatingSelection(false)
    {
      setAcceptHoverEvents(true);
      updateLayout();
    }

    PlanningBoardReservationItem::~PlanningBoardReservationItem()
    {
      if (_isSelected)
        _context->removeSelectedReservation(_reservation);
    }

    void PlanningBoardReservationItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/,
                                             QWidget* /*widget*/)
    {
      std::vector<QRectF> rects;
      rects.reserve(static_cast<std::size_t>(childItems().size()));
      for (auto child : childItems())
        rects.push_back(child->boundingRect());

      auto renderer = _context->appearance().reservationRenderer();
      renderer->paintReservationConnections(painter, *_context, rects,
                                            _isSelected || _reservation->status() == hotel::Reservation::Temporary);
    }

    QRectF PlanningBoardReservationItem::boundingRect() const { return childrenBoundingRect(); }

    void PlanningBoardReservationItem::setReservationSelected(bool select)
    {
      if (_reservation->status() == hotel::Reservation::Temporary)
        return;

      // Avoid reentrance
      if (_isUpdatingSelection)
        return;
      if (_isSelected == select)
        return;
      _isUpdatingSelection = true;
      _isSelected = select;

      // Add/remove selection to/from context
      if (select)
        _context->addSelectedReservation(_reservation);
      else
        _context->removeSelectedReservation(_reservation);

      // Update selection of the children
      for (auto child : childItems())
        child->setSelected(select);

      // If items are selected pop them in the foreground
      setZValue(select ? 1 : 0);

      _isUpdatingSelection = false;
    }

    void PlanningBoardReservationItem::updateLayout()
    {
      auto numberOfAtoms = _reservation->atoms().size();
      prepareGeometryChange();

      // Remove items if there are too many children
      while (childItems().size() > numberOfAtoms)
        delete childItems().last();

      // Add some, if there are too few items
      while (childItems().size() < numberOfAtoms)
      {
        auto item = new PlanningBoardAtomItem(_context, _reservation, childItems().size());
        item->setParentItem(this);
      }

      // Update the layout of each sub-item
      for (auto item : childItems())
      {
        assert(dynamic_cast<PlanningBoardAtomItem*>(item) != nullptr);
        auto atomItem = dynamic_cast<PlanningBoardAtomItem*>(item);
        atomItem->updateLayout();
      }
    }

  } // namespace planningwidget
} // namespace gui
