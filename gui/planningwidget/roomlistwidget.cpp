#include "gui/planningwidget/roomlistwidget.h"

#include <QtGui/QPainter>

namespace gui
{
  namespace planningwidget
  {

    RoomListRoomItem::RoomListRoomItem(const Context* context, const hotel::HotelRoom* room, QGraphicsItem* parent)
        : QGraphicsRectItem(parent), _context(context), _room(room)
    {
      updateLayout();
    }

    void RoomListRoomItem::updateLayout()
    {
      auto rowGeometry = _context->layout().getRowGeometryForRoom(_room->id());
      if (rowGeometry)
        setRect(0, rowGeometry->top(), _context->appearance().roomListWidth, rowGeometry->height());
      else
        setRect(0, 0, 0, 0);
    }

    void RoomListRoomItem::paint(QPainter* painter, [[maybe_unused]] const QStyleOptionGraphicsItem* option,
                                 [[maybe_unused]] QWidget* widget)
    {
      auto& appearance = _context->appearance();
      auto itemRect = rect().adjusted(0, 0, 0, 0);

      // Draw the color highlight
      auto highlightRect = itemRect.adjusted(0, 0, 0, 0);
      highlightRect.setWidth(6);
      painter->fillRect(highlightRect, QColor(0x217683 * _room->category()->id())); // Random color for now!

      // Add a margin to the right to separate it from the planning board
      auto separatorRect = QRect(itemRect.right() - 1, itemRect.top(), 1, itemRect.height());
      painter->fillRect(separatorRect, appearance.boardWeekdayColumnColor);

      // Draw the text
      painter->save();
      painter->setClipRect(itemRect);
      painter->setPen(appearance.atomDarkTextColor);
      painter->setFont(appearance.boldHeaderFont);
      painter->drawText(itemRect.adjusted(8, 0, 0, 0), Qt::AlignVCenter, QString::fromStdString(_room->name()));
      painter->setFont(appearance.roomListCategoryFont);
      painter->drawText(itemRect.adjusted(8, 0, -5, 0), Qt::AlignVCenter | Qt::AlignRight,
                        QString::fromStdString(_room->category()->shortCode()));
      painter->restore();
    }

    RoomListWidget::RoomListWidget(Context* context, QWidget* parent) : QGraphicsView(parent), _context(context)
    {
      setAlignment(Qt::AlignLeft | Qt::AlignTop);
      setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      setFrameStyle(QFrame::Plain);

      // Set scene size
      _scene = new QGraphicsScene;
      _scene->setSceneRect(QRectF(0, 0, _context->appearance().roomListWidth, _context->layout().getHeight()));
      setScene(_scene);
      _context->setRoomListScene(_scene);

      // No scrollbars
      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    QSize RoomListWidget::sizeHint() const
    {
      auto width = _context->appearance().roomListWidth;
      return QSize(width, 0);
    }

    void RoomListWidget::drawBackground(QPainter* painter, const QRectF& rect)
    {
      auto& layout = _context->layout();
      auto& appearance = _context->appearance();

      painter->fillRect(rect, appearance.widgetBackground);

      for (auto row : layout.rowGeometries())
        appearance.drawRowBackground(painter, row, QRect(rect.left(), row.top(), rect.width(), row.height()));
    }

    void RoomListWidget::addRoomItem(hotel::HotelRoom* room)
    {
      auto item = new RoomListRoomItem(_context, room);
      _scene->addItem(item);
    }

    void RoomListWidget::clear() { _scene->clear(); }

    void RoomListWidget::updateLayout()
    {
      auto& layout = _context->layout();
      auto& appearance = _context->appearance();

      for (auto item : _scene->items())
      {
        // TODO: We should maintain a list of reservation which are currently diplayed, instead of resorting do
        // dynamic_casting things
        auto roomItem = dynamic_cast<RoomListRoomItem*>(item);
        if (roomItem != nullptr)
          roomItem->updateLayout();
      }

      _scene->setSceneRect(QRectF(0, 0, appearance.roomListWidth, layout.getHeight()));
      invalidateBackground();
      invalidateForeground();
    }

    void RoomListWidget::invalidateBackground()
    {
      _scene->invalidate(sceneRect(), QGraphicsScene::BackgroundLayer);
      update();
    }

    void RoomListWidget::invalidateForeground()
    {
      _scene->invalidate(sceneRect(), QGraphicsScene::ForegroundLayer);
      update();
    }

  } // namespace planningwidget
} // namespace gui
