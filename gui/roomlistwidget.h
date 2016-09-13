#ifndef GUI_ROOMLISTWIDGET_H
#define GUI_ROOMLISTWIDGET_H

#include "gui/planningboardlayout.h"

#include <QGraphicsRectItem>
#include <QGraphicsView>

namespace gui
{
  class RoomListRoomItem : public QGraphicsRectItem
  {
  public:
    RoomListRoomItem(const PlanningBoardLayout* layout, const hotel::HotelRoom* room, QGraphicsItem* parent = nullptr);

    void updateAppearance();

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    const PlanningBoardLayout* _layout;
    const hotel::HotelRoom* _room;
  };

  class RoomListWidget : public QGraphicsView
  {
  public:
    RoomListWidget(const PlanningBoardLayout* layout, QWidget* parent = nullptr);
    virtual QSize sizeHint() const override;

    void addRoomItem(hotel::HotelRoom* room);

  protected:
    virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    QGraphicsScene* _scene;
    const PlanningBoardLayout* _layout;
  };

} // namespace gui

#endif // GUI_ROOMLISTWIDGET_H
