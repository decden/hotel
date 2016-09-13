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
    DateBarDayItem(const PlanningBoardLayout* layout, int day, int weekday, bool isHighlighted);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    const PlanningBoardLayout* _layout;
    int _day;
    int _weekday;
    bool _isHighlighted;
  };

  class DateBarMonthItem : public QGraphicsRectItem
  {
  public:
    DateBarMonthItem(const PlanningBoardLayout* layout, int month, int year);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    const PlanningBoardLayout* _layout;
    int _month;
    int _year;
  };

  class DateBarWidget : public QGraphicsView
  {
  public:
    DateBarWidget(const PlanningBoardLayout* layout, QWidget* parent = nullptr);

    virtual QSize sizeHint() const override;

  private:
    void rebuildScene();

    QGraphicsScene* _scene;
    const PlanningBoardLayout* _layout;
  };

} // namespace gui

#endif // GUI_DATEBARWIDGET_H
