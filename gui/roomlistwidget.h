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

    // QGraphicsRectItem interface
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void updateLayout();

  private:
    const PlanningBoardLayout* _layout;
    const hotel::HotelRoom* _room;
  };

  /**
   * @brief The RoomListWidget class shows a list of rooms
   */
  class RoomListWidget : public QGraphicsView
  {
  public:
    RoomListWidget(const PlanningBoardLayout* layout, QWidget* parent = nullptr);
    virtual QSize sizeHint() const override;

    void addRoomItem(hotel::HotelRoom* room);

    //! When the layout changes, call this methods to update the scene.
    void updateLayout();

  protected:
    virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    QGraphicsScene* _scene;
    const PlanningBoardLayout* _layout;

    void invalidateBackground();
    void invalidateForeground();
  };

} // namespace gui

#endif // GUI_ROOMLISTWIDGET_H
