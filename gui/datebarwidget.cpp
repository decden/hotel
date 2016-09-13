#include "gui/datebarwidget.h"

#include <QPainter>

namespace gui
{

  DateBarDayItem::DateBarDayItem(PlanningBoardLayout* layout, int day, int weekday, bool isHighlighted)
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

    auto backgroundColor =
        _isHighlighted ? appearance.boardTodayColor : ((_weekday == 0 || _weekday == 6) ? appearance.boardOddRowColor
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

  DateBarMonthItem::DateBarMonthItem(PlanningBoardLayout* layout, int month, int year)
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

  DateBarWidget::DateBarWidget(QWidget* parent) : QGraphicsView(parent)
  {
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFrameStyle(QFrame::Plain);

    // Prepare the scene
    setScene(&_scene);

    // Set scene size
    auto height = _layout.appearance().monthBarHeight + _layout.appearance().daysBarHeight;
    _scene.setSceneRect(QRectF(-400, 0, 15984, height));
    rebuildScene();

    // No scrollbars
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  }

  QSize DateBarWidget::sizeHint() const
  {
    auto height = _layout.appearance().monthBarHeight + _layout.appearance().daysBarHeight;
    return QSize(0, height);
  }

  void DateBarWidget::rebuildScene()
  {
    _scene.clear();

    // Rebuild the scene
    int left = -400;
    int width = 15984;
    int colWidth = _layout.dateColumnWidth();

    auto today = boost::gregorian::day_clock::local_day();
    int posX;
    boost::gregorian::date date;
    std::tie(date, posX) = _layout.getNearestDatePosition(left - colWidth);
    posX -= _layout.dateColumnWidth() / 2 - 1; // Dates are centered above the dateline
    for (int i = posX; i < (left + width); i += colWidth)
    {
      auto item = new DateBarDayItem(&_layout, date.day(), date.day_of_week(), date == today);
      int w = _layout.dateColumnWidth();
      item->setRect(QRect(i - 1, 20, w + 1, 40));
      _scene.addItem(item);
      date += boost::gregorian::days(1);
    }

    std::tie(date, posX) = _layout.getNearestDatePosition(left - colWidth);
    posX -= colWidth / 2 - 1; // Dates are centered above the dateline
    posX -= (date.day() - 1) * colWidth;
    date = boost::gregorian::date(date.year(), date.month(), 1);
    for (int i = posX; i < (left + width);)
    {
      auto nextMonth = date + boost::gregorian::months(1);
      auto daysInMonth = (nextMonth - date).days();
      auto item = new DateBarMonthItem(&_layout, date.month(), date.year());
      item->setRect(QRect(i, 0, daysInMonth * colWidth, 20));
      _scene.addItem(item);
      i += daysInMonth * colWidth;
      date = nextMonth;
    }
  }

} // namespace gui
