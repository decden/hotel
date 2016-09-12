#include "gui/roomlistwidget.h"

#include <QPainter>

namespace gui
{

  RoomListRoomItem::RoomListRoomItem(PlanningBoardLayout* layout, hotel::HotelRoom* room, QGraphicsItem* parent)
      : QGraphicsRectItem(parent), _layout(layout), _room(room)
  {
    updateAppearance();
  }

  void RoomListRoomItem::updateAppearance()
  {
    auto rowGeometry = _layout->getRowGeometryForRoom(_room->id());
    if (rowGeometry)
    {
      setRect(0, rowGeometry->top(), _layout->appearance().roomListWidth, rowGeometry->height());
    }
  }

  void RoomListRoomItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
  {
    auto& appearance = _layout->appearance();
    auto itemRect = rect().adjusted(0, 0, 0, 0);

    // Draw the color highlight
    auto highlightRect = itemRect.adjusted(0, 0, 0, 0);
    highlightRect.setWidth(6);
    painter->fillRect(highlightRect, QColor(0x217683 * _room->category()->id())); // Random color for now!

    painter->save();
    painter->setClipRect(itemRect);

    painter->setPen(appearance.atomDarkTextColor);
    painter->drawText(itemRect.adjusted(8, 0, 0, 0), Qt::AlignVCenter, QString::fromStdString(_room->name()));
    painter->setFont(QFont("Arial", 8));
    painter->drawText(itemRect.adjusted(8, 0, -5, 0), Qt::AlignVCenter | Qt::AlignRight, QString::fromStdString(_room->category()->shortCode()));
    painter->restore();
  }

  RoomListWidget::RoomListWidget(std::vector<std::unique_ptr<hotel::Hotel>>* hotels, QWidget* parent)
      : QGraphicsView(parent)
  {
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setFrameStyle(QFrame::Plain);

    _layout.updateRoomGeometry(*hotels);

    // Prepare the scene
    setScene(&_scene);
    for (auto& hotel : *hotels)
      for (auto& room : hotel->rooms())
        addRoomItem(room.get());

    // Set scene size
    _scene.setSceneRect(QRectF(0, 0, _layout.appearance().roomListWidth, _layout.getHeight()));

    viewport()->setMinimumWidth(100);

    // No scrollbars
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  }

  QSize RoomListWidget::sizeHint() const
  {
    auto width = _layout.appearance().roomListWidth;
    return QSize(width, 0);
  }

  void RoomListWidget::drawBackground(QPainter* painter, const QRectF& rect)
  {
    auto& appearance = _layout.appearance();

    painter->fillRect(rect, appearance.widgetBackground);

    for (auto row : _layout.rowGeometries())
      appearance.drawRowBackground(painter, row, QRect(rect.left(), row.top(), rect.width(), row.height()));
  }

  void RoomListWidget::addRoomItem(hotel::HotelRoom* room)
  {
    auto item = new RoomListRoomItem(&_layout, room);
    _scene.addItem(item);
  }

} // namespace gui
