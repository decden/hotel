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
    RoomListRoomItem(PlanningBoardLayout* layout, hotel::HotelRoom* room, QGraphicsItem* parent = nullptr);

    void updateAppearance();

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    PlanningBoardLayout* _layout;
    hotel::HotelRoom* _room;
  };

  class RoomListWidget : public QGraphicsView
  {
  public:
    RoomListWidget(std::vector<std::unique_ptr<hotel::Hotel>>* hotels, QWidget* parent = nullptr);
    virtual QSize sizeHint() const override;

  protected:
    virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    void addRoomItem(hotel::HotelRoom* room);

    QGraphicsScene _scene;
    PlanningBoardLayout _layout;
  };

} // namespace gui

#endif // GUI_ROOMLISTWIDGET_H
