#ifndef GUI_ROOMLISTWIDGET_H
#define GUI_ROOMLISTWIDGET_H

#include "gui/planningwidget/context.h"

#include <QGraphicsRectItem>
#include <QGraphicsView>

namespace gui
{
  namespace planningwidget
  {

    class RoomListRoomItem : public QGraphicsRectItem
    {
    public:
      RoomListRoomItem(const Context* context, const hotel::HotelRoom* room,
                       QGraphicsItem* parent = nullptr);

      // QGraphicsRectItem interface
      virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

      void updateLayout();

    private:
      const Context* _context;
      const hotel::HotelRoom* _room;
    };

    /**
     * @brief The RoomListWidget class shows a list of rooms
     */
    class RoomListWidget : public QGraphicsView
    {
    public:
      RoomListWidget(const Context* context, QWidget* parent = nullptr);
      virtual QSize sizeHint() const override;

      void addRoomItem(hotel::HotelRoom* room);

      //! When the layout changes, call this methods to update the scene.
      void updateLayout();

    protected:
      virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

    private:
      const Context* _context;
      QGraphicsScene* _scene;

      void invalidateBackground();
      void invalidateForeground();
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_ROOMLISTWIDGET_H
