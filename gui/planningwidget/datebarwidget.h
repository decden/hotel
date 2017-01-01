#ifndef GUI_DATEBARWIDGET_H
#define GUI_DATEBARWIDGET_H

#include "gui/planningwidget/context.h"
#include "gui/planningwidget/planningboardlayout.h"

#include <QGraphicsRectItem>
#include <QGraphicsView>

#include <boost/date_time.hpp>

namespace gui
{
  namespace planningwidget
  {
    class DateBarWidget;

    /**
     * @brief The DateBarDayItem class is a graphics item showing one single day in the day bar
     */
    class DateBarDayItem : public QGraphicsRectItem
    {
    public:
      DateBarDayItem(DateBarWidget* parent, const PlanningBoardAppearance& appearance, boost::gregorian::date date, bool isPivot, bool isToday);

      virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    private:
      DateBarWidget* _parent;
      const PlanningBoardAppearance& _appearance;
      boost::gregorian::date _date;

      bool _isPivot;
      bool _isToday;

    protected:
      void mousePressEvent(QGraphicsSceneMouseEvent *event);
    };

    class DateBarMonthItem : public QGraphicsRectItem
    {
    public:
      DateBarMonthItem(const PlanningBoardAppearance& appearance, int month, int year);

      virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    private:
      const PlanningBoardAppearance& _appearance;
      int _month;
      int _year;
    };

    /**
     * @brief The DateBarWidget class shows a horizontal date bar displaying month, day of month and day of week
     */
    class DateBarWidget : public QGraphicsView
    {
      Q_OBJECT
    public:
      DateBarWidget(const Context* context, QWidget* parent = nullptr);

      // QGraphicsView interface
      virtual QSize sizeHint() const override;

      //! When the layout changes, call this methods to update the scene.
      void updateLayout();

    signals:
      void dateClicked(boost::gregorian::date date);

    private:
      void rebuildScene();

      // Methods called by the scene's items
      friend class DateBarDayItem;
      void dateItemClicked(boost::gregorian::date date);

      const Context* _context;
      QGraphicsScene* _scene;
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_DATEBARWIDGET_H
