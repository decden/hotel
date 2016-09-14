#include "gui/planningwidget/datebarwidget.h"

#include <QPainter>

namespace gui
{
  namespace planningwidget
  {

    DateBarDayItem::DateBarDayItem(const PlanningBoardLayout* layout, int day, int weekday, bool isHighlighted)
        : QGraphicsRectItem(), _layout(layout), _day(day), _weekday(weekday), _isHighlighted(isHighlighted)
    {
      if (isHighlighted)
        setZValue(1);
    }

    void DateBarDayItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
      auto& appearance = _layout->appearance();
      auto itemRect = rect().adjusted(0, 0, 0, 0);

      painter->save();

      painter->setPen(appearance.boardWeekdayColumnColor);

      auto borderRect = itemRect.adjusted(0.5, 0.5, -0.5, -0.5);

      auto backgroundColor = _isHighlighted ? appearance.boardTodayColor
                                            : ((_weekday == 0 || _weekday == 6) ? appearance.boardOddRowColor
                                                                                : appearance.boardEvenRowColor);
      auto borderColor = _isHighlighted ? appearance.boardTodayColor.darker() : appearance.boardWeekdayColumnColor;
      auto textColor = _isHighlighted ? appearance.atomLightTextColor : appearance.atomDarkTextColor;

      painter->setPen(borderColor);
      painter->fillRect(itemRect.adjusted(-0.5, 0.5, -0.5, -0.5), backgroundColor);
      painter->drawRect(itemRect.adjusted(-0.5, 0.5, -0.5, -0.5));

      painter->setClipRect(itemRect);
      painter->setPen(textColor);
      painter->setFont(appearance.boldHeaderFont);
      painter->drawText(itemRect.adjusted(0, 5, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                        QString("%1").arg(appearance.getShortWeekdayName(_weekday)));
      painter->drawText(itemRect.adjusted(0, 0, 0, -5), Qt::AlignHCenter | Qt::AlignBottom, QString("%1").arg(_day));
      painter->restore();
    }

    DateBarMonthItem::DateBarMonthItem(const PlanningBoardLayout* layout, int month, int year)
        : QGraphicsRectItem(), _layout(layout), _month(month), _year(year)
    {
    }

    void DateBarMonthItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
      auto& appearance = _layout->appearance();

      auto color = (_month % 2 == 0) ? appearance.boardEvenRowColor : appearance.boardOddRowColor;

      painter->save();
      painter->setClipRect(rect());
      painter->fillRect(rect(), color);
      painter->setPen(appearance.atomDarkTextColor);
      painter->setFont(appearance.boldHeaderFont);
      painter->drawText(rect().adjusted(5, 0, 0, 0), Qt::AlignVCenter,
                        QString("%1 - %2").arg(appearance.getMonthName(_month)).arg(_year));
      painter->restore();
    }

    DateBarWidget::DateBarWidget(const PlanningBoardLayout* layout, QWidget* parent)
        : QGraphicsView(parent), _layout(layout)
    {
      setAlignment(Qt::AlignLeft | Qt::AlignTop);
      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      setFrameStyle(QFrame::Plain);

      // Prepare the scene
      _scene = new QGraphicsScene;
      setScene(_scene);

      // Set scene size
      auto height = _layout->appearance().monthBarHeight + _layout->appearance().daysBarHeight;
      auto sceneRect = _layout->sceneRect();
      sceneRect.setHeight(height);
      _scene->setSceneRect(sceneRect);
      rebuildScene();

      // No scrollbars
      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    QSize DateBarWidget::sizeHint() const
    {
      auto height = _layout->appearance().monthBarHeight + _layout->appearance().daysBarHeight;
      return QSize(0, height);
    }

    void DateBarWidget::updateLayout()
    {
      auto height = _layout->appearance().monthBarHeight + _layout->appearance().daysBarHeight;
      auto sceneRect = _layout->sceneRect();
      sceneRect.setHeight(height);
      _scene->setSceneRect(sceneRect);
      rebuildScene();
    }

    void DateBarWidget::rebuildScene()
    {
      _scene->clear();
      auto& appearance = _layout->appearance();

      // Rebuild the scene
      auto sceneRect = _layout->sceneRect();
      int left = sceneRect.left();
      int right = sceneRect.right();
      int colWidth = _layout->dateColumnWidth();

      auto today = boost::gregorian::day_clock::local_day();

      // Add the day items
      int positionX;
      boost::gregorian::date date;
      std::tie(date, positionX) = _layout->getNearestDatePosition(left - colWidth);
      positionX -= colWidth / 2 - 1; // Dates are centered above the dateline
      for (int x = positionX; x < right; x += colWidth)
      {
        auto item = new DateBarDayItem(_layout, date.day(), date.day_of_week(), date == today);
        item->setRect(QRect(x - 1, appearance.monthBarHeight, colWidth + 1, appearance.daysBarHeight));
        _scene->addItem(item);
        date += boost::gregorian::days(1);
      }

      // Add the month items
      std::tie(date, positionX) = _layout->getNearestDatePosition(left - colWidth);
      positionX -= colWidth / 2 - 1;            // Dates are centered above the dateline
      positionX -= (date.day() - 1) * colWidth; // Roll back to the beginning of the month
      date = boost::gregorian::date(date.year(), date.month(), 1);
      for (int i = positionX; i < right;)
      {
        auto nextMonth = date + boost::gregorian::months(1);
        auto monthWidth = (nextMonth - date).days() * colWidth;
        auto item = new DateBarMonthItem(_layout, date.month(), date.year());
        item->setRect(QRect(i, 0, monthWidth, appearance.monthBarHeight));
        _scene->addItem(item);
        i += monthWidth;
        date = nextMonth;
      }
    }
  } // namespace planningwidget
} // namespace gui
