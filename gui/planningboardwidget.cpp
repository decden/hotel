#include "planningboardwidget.h"

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
    auto& appearance = _layout->appearance();
    auto today = boost::gregorian::day_clock::local_day();
    if (isSelected())
    {
      if (_atom->reservation()->dateRange().is_before(today))
        return appearance.atomArchivedSelectedColor;

      return appearance.atomSelectedColor;
    }
    else
    {
      if (_atom->reservation()->dateRange().is_before(today))
        return appearance.atomArchivedColor;

      if (_atom->reservation()->dateRange().contains(today))
        return appearance.atomCheckedInColor;

      if (_atom->reservation()->id() % 30 == 0)
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

  PlanningBoardWidget::PlanningBoardWidget(std::unique_ptr<hotel::persistence::SqliteStorage> storage)
      : QGraphicsView(), _storage(std::move(storage))
  {
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setCacheMode(QGraphicsView::CacheBackground);
    setScene(&_scene);

    _hotels = _storage->loadHotels();
    std::vector<int> roomIds;
    for (auto& hotel : _hotels)
      for (auto& room : hotel->rooms())
        roomIds.push_back(room->id());
    _planning = _storage->loadPlanning(roomIds);
    _layout.updateRoomGeometry(_hotels);

    // Add the reservations
    addReservations(_planning->reservations());

    // Compute the size of the scen
    auto dateRange = _planning->getPlanningExtent();
    auto left = _layout.getDatePositionX(dateRange.begin()) - 10;
    auto right = _layout.getDatePositionX(dateRange.end()) + 10;
    _scene.setSceneRect(QRectF(left, 0, right - left, _layout.getHeight()));
  }

  void PlanningBoardWidget::drawBackground(QPainter* painter, const QRectF& rect)
  {
    auto& appearance = _layout.appearance();
    painter->fillRect(rect, appearance.widgetBackground);

    // Draw the horizontal alternating rows
    bool isRowEven = true;
    for (auto row : _layout.rowGeometries())
    {
      if (row.rowType() == PlanningBoardRowGeometry::RoomRow)
      {
        auto background = (isRowEven) ? appearance.boardEvenRowColor : appearance.boardOddRowColor;
        painter->fillRect(QRect(rect.left(), row.top(), rect.width(), row.height()), background);
      }
      isRowEven = !isRowEven;
    }

    // Get the horizontal date range
    auto leftDatePos = _layout.getNearestDatePosition(rect.left() - _layout.dateColumnWidth());

    // Draw the vertical day lines
    auto posX = leftDatePos.second;
    auto dayOfWeek = leftDatePos.first.day_of_week();
    painter->setPen(appearance.boardWeekdayColumnColor);
    while (posX < rect.right() + _layout.dateColumnWidth())
    {
      if (dayOfWeek == boost::gregorian::Sunday)
      {
        int w = appearance.boardSundayColumnWidth;
        painter->fillRect(QRectF(posX - w / 2, rect.top(), w, rect.height()), QColor(appearance.boardSundayColumnColor));
      }
      else if (dayOfWeek == boost::gregorian::Saturday)
      {
        int w = appearance.boardSaturdayColumnWidth;
        painter->fillRect(QRectF(posX - w / 2, rect.top(), w, rect.height()), QColor(appearance.boardSaturdayColumnColor));
      }
      else
        painter->drawLine(posX-1, rect.top(), posX-1, rect.bottom());
      posX += _layout.dateColumnWidth();
      dayOfWeek = (dayOfWeek + 1) % 7;
    }

    // Fill in the separator rows (we do not want to have vertical lines in there)
    for (auto row : _layout.rowGeometries())
    {
      if (row.rowType() == PlanningBoardRowGeometry::SeparatorRow)
      {
        auto background = QColor(0x353F41);
        painter->fillRect(QRect(rect.left(), row.top(), rect.width(), row.height()-1), background);
        painter->setPen(appearance.boardEvenRowColor);
        painter->drawLine(rect.left(), row.bottom()-1, rect.width(), row.bottom()-1);
      }
    }
  }

  void PlanningBoardWidget::addReservations(const std::vector<std::unique_ptr<hotel::Reservation>>& reservations)
  {
    for (auto& reservation : reservations)
    {
      auto item = new PlanningBoardReservationItem(&_layout, reservation.get());
      _scene.addItem(item);
    }
  }

} // namespace gui
