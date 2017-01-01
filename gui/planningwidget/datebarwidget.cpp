#include "gui/planningwidget/datebarwidget.h"

#include <QPainter>

namespace gui
{
  namespace planningwidget
  {

    DateBarDayItem::DateBarDayItem(DateBarWidget* parent, const PlanningBoardAppearance &appearance, boost::gregorian::date date, bool isPivot, bool isToday)
        : QGraphicsRectItem(), _parent(parent), _appearance(appearance), _date(date), _isPivot(isPivot), _isToday(isToday)
    {
      if (isPivot)
        setZValue(1);
    }

    void DateBarDayItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
      auto itemRect = rect().adjusted(0, 0, 0, 0);

      auto day = _date.day();
      auto dayOfWeek = _date.day_of_week();

      painter->save();
      painter->setPen(_appearance.boardWeekdayColumnColor);

      QColor backgroundColor;
      if (_isPivot && _isToday)
        backgroundColor = _appearance.boardPivotTodayColor;
      else if (_isPivot && !_isToday)
        backgroundColor = _appearance.boardPivotColor;
      else
        backgroundColor = (dayOfWeek == 0 || dayOfWeek == 6) ? _appearance.boardOddRowColor : _appearance.boardEvenRowColor;

      auto borderColor = _isPivot ? _appearance.boardPivotTodayColor.darker() : _appearance.boardWeekdayColumnColor;
      auto textColor = _isPivot ? _appearance.atomLightTextColor : _appearance.atomDarkTextColor;

      painter->setPen(borderColor);
      painter->fillRect(itemRect.adjusted(-0.5, 0.5, -0.5, -0.5), backgroundColor);
      painter->drawRect(itemRect.adjusted(-0.5, 0.5, -0.5, -0.5));

      painter->setClipRect(itemRect);
      painter->setPen(textColor);
      painter->setFont(_isToday ? _appearance.boldHeaderFont : _appearance.headerFont);
      painter->drawText(itemRect.adjusted(0, 5, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                        QString("%1").arg(_appearance.getShortWeekdayName(dayOfWeek)));
      painter->drawText(itemRect.adjusted(0, 0, 0, -5), Qt::AlignHCenter | Qt::AlignBottom, QString("%1").arg(day));
      painter->restore();
    }

    void DateBarDayItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
      _parent->dateItemClicked(_date);
    }

    DateBarMonthItem::DateBarMonthItem(const PlanningBoardAppearance &appearance, int month, int year)
        : QGraphicsRectItem(), _appearance(appearance), _month(month), _year(year)
    {
    }

    void DateBarMonthItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
      auto color = (_month % 2 == 0) ? _appearance.boardEvenRowColor : _appearance.boardOddRowColor;

      painter->save();
      painter->setClipRect(rect());
      painter->fillRect(rect(), color);
      painter->setPen(_appearance.atomDarkTextColor);
      painter->setFont(_appearance.boldHeaderFont);
      painter->drawText(rect().adjusted(5, 0, 0, 0), Qt::AlignVCenter,
                        QString("%1 - %2").arg(_appearance.getMonthName(_month)).arg(_year));
      painter->restore();
    }

    DateBarWidget::DateBarWidget(const Context* context, QWidget* parent)
        : QGraphicsView(parent), _context(context)
    {
      setAlignment(Qt::AlignLeft | Qt::AlignTop);
      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      auto height = _context->appearance().monthBarHeight + _context->appearance().daysBarHeight;
      setMaximumHeight(height);
      setMinimumHeight(height);
      setFrameStyle(QFrame::Plain);

      // Prepare the scene
      _scene = new QGraphicsScene;
      setScene(_scene);

      // Set scene size
      auto sceneRect = _context->layout().sceneRect();
      sceneRect.setHeight(height);
      _scene->setSceneRect(sceneRect);
      rebuildScene();

      // No scrollbars
      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    QSize DateBarWidget::sizeHint() const
    {
      auto height = _context->appearance().monthBarHeight + _context->appearance().daysBarHeight;
      return QSize(0, height);
    }

    void DateBarWidget::updateLayout()
    {
      auto height = _context->appearance().monthBarHeight + _context->appearance().daysBarHeight;
      auto sceneRect = _context->layout().sceneRect();
      sceneRect.setHeight(height);
      _scene->setSceneRect(sceneRect);
      rebuildScene();
    }

    void DateBarWidget::rebuildScene()
    {
      _scene->clear();
      auto& appearance = _context->appearance();

      // Rebuild the scene
      auto sceneRect = _context->layout().sceneRect();
      int left = sceneRect.left();
      int right = sceneRect.right();
      int colWidth = _context->layout().dateColumnWidth();

      auto today = boost::gregorian::day_clock::local_day();
      auto pivotDate = _context->layout().pivotDate();

      // Add the day items
      int positionX;
      boost::gregorian::date date;
      std::tie(date, positionX) = _context->layout().getNearestDatePosition(left - colWidth);
      positionX -= colWidth / 2 - 1; // Dates are centered above the dateline
      for (int x = positionX; x < right; x += colWidth)
      {
        auto item = new DateBarDayItem(this, _context->appearance(), date, date == pivotDate, date == today);
        item->setRect(QRect(x - 1, appearance.monthBarHeight, colWidth + 1, appearance.daysBarHeight));
        _scene->addItem(item);
        date += boost::gregorian::days(1);
      }

      // Add the month items
      std::tie(date, positionX) = _context->layout().getNearestDatePosition(left - colWidth);
      positionX -= colWidth / 2 - 1;            // Dates are centered above the dateline
      positionX -= (date.day() - 1) * colWidth; // Roll back to the beginning of the month
      date = boost::gregorian::date(date.year(), date.month(), 1);
      for (int i = positionX; i < right;)
      {
        auto nextMonth = date + boost::gregorian::months(1);
        auto monthWidth = (nextMonth - date).days() * colWidth;
        auto item = new DateBarMonthItem(_context->appearance(), date.month(), date.year());
        item->setRect(QRect(i, 0, monthWidth, appearance.monthBarHeight));
        _scene->addItem(item);
        i += monthWidth;
        date = nextMonth;
      }
    }

    void DateBarWidget::dateItemClicked(boost::gregorian::date date)
    {
      emit dateClicked(date);
    }
  } // namespace planningwidget
} // namespace gui
