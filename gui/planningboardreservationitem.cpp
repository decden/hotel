#include "gui/planningboardreservationitem.h"

#include <QPainter>

namespace gui
{
  PlanningBoardAtomItem::PlanningBoardAtomItem(PlanningBoardLayout* layout, const hotel::ReservationAtom* atom,
                                               QGraphicsItem* parent)
      : QGraphicsRectItem(parent), _layout(layout), _atom(atom)
  {
    setFlag(QGraphicsItem::ItemIsSelectable);
    updateAppearance();
  }

  void PlanningBoardAtomItem::updateAppearance()
  {
    auto itemRect = _layout->getAtomRect(_atom->roomId(), _atom->_dateRange);
    setRect(itemRect);
  }

  void PlanningBoardAtomItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
  {
    auto itemRect = rect().adjusted(1, 1, 0, -1);
    const int cornerRadius = 5;
    auto itemColor = getItemColor();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(itemColor);
    painter->setPen(itemColor.darker(200));

    // Construct a rounded rectangle. Not all of the corners are rounded. Only the parts corresponding to the beginning
    // or end of a reservation are rounded
    auto borderRect = itemRect.adjusted(-0.5, -0.5, -0.5, -0.5);
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.addRoundedRect(borderRect, cornerRadius, cornerRadius, Qt::AbsoluteSize);
    if (!_atom->isFirst())
      path.addRect(borderRect.adjusted(0, 0, -cornerRadius, 0));
    if (!_atom->isLast())
      path.addRect(borderRect.adjusted(cornerRadius, 0, 0, 0));
    painter->drawPath(path.simplified());

    // Draw the text
    painter->setClipRect(itemRect);
    painter->setPen(getItemTextColor());
    painter->setFont(_layout->appearance().atomTextFont);
    painter->drawText(itemRect.adjusted(5, 2, -2, -2), Qt::AlignLeft | Qt::AlignVCenter, getDisplayedText());
    painter->restore();
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

  QString PlanningBoardAtomItem::getDisplayedText() const
  {
    auto description = QString::fromStdString(_atom->reservation()->description());
    description += QString(" (%1)").arg(_atom->reservation()->length());

    if (!_atom->isFirst())
      description = "\u25B8 " + description;
    return description;
  }

  QColor PlanningBoardAtomItem::getItemColor() const
  {
    using ReservationStatus = hotel::Reservation::ReservationStatus;

    auto& appearance = _layout->appearance();
    if (isSelected())
    {
      if (_atom->reservation()->status() == ReservationStatus::CheckedOut ||
          _atom->reservation()->status() == ReservationStatus::Archived)
        return appearance.atomArchivedSelectedColor;

      return appearance.atomSelectedColor;
    }
    else
    {
      if (_atom->reservation()->status() == ReservationStatus::Archived)
        return appearance.atomArchivedColor;

      if (_atom->reservation()->status() == ReservationStatus::CheckedOut)
        return appearance.atomCheckedOutColor;

      if (_atom->reservation()->status() == ReservationStatus::CheckedIn)
        return appearance.atomCheckedInColor;

      if (_atom->reservation()->status() == ReservationStatus::New)
        return appearance.atomUnconfirmedColor;

      return appearance.atomDefaultColor;
    }
  }

  QColor PlanningBoardAtomItem::getItemTextColor() const
  {
    auto& appearance = _layout->appearance();
    auto itemColor = getItemColor();
    if (itemColor.lightness() > 200)
      return appearance.atomDarkTextColor;
    else
      return appearance.atomLightTextColor;
  }

  PlanningBoardReservationItem::PlanningBoardReservationItem(PlanningBoardLayout* layout,
                                                             const hotel::Reservation* reservation,
                                                             QGraphicsItem* parent)
      : QGraphicsItem(parent), _layout(layout), _reservation(reservation), _isSelected(false),
        _isUpdatingSelection(false)
  {
    for (auto& atom : _reservation->atoms())
    {
      auto item = new PlanningBoardAtomItem(layout, atom.get());
      item->setParentItem(this);
    }
  }

  void PlanningBoardReservationItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
  {
    if (_isSelected)
    {
      // Draw the connection links between items
      auto& appearance = _layout->appearance();
      auto children = childItems();
      if (children.size() > 1)
      {
        for (auto i = 0; i < children.size() - 1; ++i)
        {
          auto previousBox = children[i]->boundingRect();
          auto nextBox = children[i + 1]->boundingRect();

          // Calculate the two points
          auto x1 = previousBox.right();
          auto y1 = previousBox.top() + previousBox.height() / 2;
          auto x2 = nextBox.left();
          auto y2 = nextBox.top() + nextBox.height() / 2;

          // Draw the two rectangular handles
          const int handleSize = appearance.atomConnectionHandleSize;
          const int linkOverhang = appearance.atomConnectionOverhang;
          const QColor& handleColor = appearance.selectionColor;
          painter->fillRect(QRect(x1 - handleSize, y1 - handleSize, handleSize * 2, handleSize * 2), handleColor);
          painter->fillRect(QRect(x2 - handleSize, y2 - handleSize, handleSize * 2, handleSize * 2), handleColor);

          // Draw the zig-yag line between handles
          QPointF points[] = {QPoint(x1, y1), QPoint(x1 + linkOverhang, y1), QPoint(x2 - linkOverhang, y2),
                              QPoint(x2, y2)};
          painter->save();
          painter->setRenderHint(QPainter::Antialiasing, true);
          painter->setPen(QPen(handleColor, 2));
          painter->drawPolyline(points, 4);
          painter->restore();
        }
      }
    }
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
} // namespace gui
