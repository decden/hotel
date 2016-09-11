#include "planningboardwidget.h"

#include <QPainter>

namespace gui
{

  PlanningBoardLayout::PlanningBoardLayout() { _originDate = boost::gregorian::day_clock::local_day(); }

  void PlanningBoardLayout::updateRoomGeometry(std::vector<std::unique_ptr<hotel::Hotel>>& hotels)
  {
    _roomIdToYPosition.clear();
    int y = 0;
    for (auto& hotel : hotels)
    {
      for (auto& room : hotel->rooms())
      {
        _roomIdToYPosition[room->id()] = y;
        y += 20;
      }
      y += 10;
    }
  }

  QRectF PlanningBoardLayout::getAtomRect(int roomId, boost::gregorian::date_period dateRange)
  {
    auto pos = getDatePositionX(dateRange.begin());
    auto width = dateRange.length().days() * 24 - 1;
    auto y = _roomIdToYPosition[roomId]; // TODO: check if the room exist first!

    return QRectF(pos, y, width, 20);
  }

  int PlanningBoardLayout::getDatePositionX(boost::gregorian::date date) const
  {
    return (date - _originDate).days() * 24;
  }

  int PlanningBoardLayout::getHeight() const
  {
    // TODO: calculate this in the right way! This does not account for gaps or uneven spacing!
    return _roomIdToYPosition.size() * 22;
  }

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
    const auto itemColor = QColor(0xfffafafa);
    const auto selectionColor = _layout->selectionColor;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(isSelected() ? selectionColor : itemColor);
    painter->setPen(itemColor.darker(200));
    painter->drawRoundedRect(itemRect.adjusted(-0.5, -0.5, -0.5, -0.5), 5, 5, Qt::AbsoluteSize);

    painter->setClipRect(itemRect);
    painter->setPen(QColor(0, 0, 0));
    // painter->setFont()
    auto description = QString::fromStdString(_atom->reservation()->description());
    painter->drawText(itemRect.adjusted(5, 2, -2, -2), Qt::AlignLeft | Qt::AlignVCenter, description);
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
          const int handleSize = 3;
          const int linkOverhang = 10;
          const QColor& handleColor = _layout->selectionColor;
          painter->fillRect(QRect(x1, y1 - handleSize, handleSize, handleSize * 2), handleColor);
          painter->fillRect(QRect(x2 - handleSize + 1, y2 - handleSize, handleSize, handleSize * 2), handleColor);

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

  void PlanningBoardWidget::addReservations(const std::vector<std::unique_ptr<hotel::Reservation> > &reservations)
  {
    for (auto& reservation : reservations)
    {
      auto item = new PlanningBoardReservationItem(&_layout, reservation.get());
      _scene.addItem(item);
    }
  }

} // namespace gui
