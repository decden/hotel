#ifndef GUI_DATEBARWIDGET_H
#define GUI_DATEBARWIDGET_H

#include "gui/planningboardlayout.h"

#include <QGraphicsRectItem>
#include <QGraphicsView>

#include <boost/date_time.hpp>

namespace gui
{
  class DateBarDayItem : public QGraphicsRectItem
  {
  public:
    DateBarDayItem(PlanningBoardLayout* layout, int day, int weekday, bool isHighlighted);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    PlanningBoardLayout* _layout;
    int _day;
    int _weekday;
    bool _isHighlighted;
  };

  class DateBarMonthItem : public QGraphicsRectItem
  {
  public:
    DateBarMonthItem(PlanningBoardLayout* layout, int month, int year);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    PlanningBoardLayout* _layout;
    int _month;
    int _year;
  };

  class DateBarWidget : public QGraphicsView
  {
  public:
    DateBarWidget(QWidget* parent = nullptr);

    virtual QSize sizeHint() const override;

  private:
    void rebuildScene();

    PlanningBoardLayout _layout;
    QGraphicsScene _scene;
  };

} // namespace gui

#endif // GUI_DATEBARWIDGET_H
