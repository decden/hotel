#include "gui/statusbar.h"

#include <QPaintEvent>
#include <QPainter>

namespace
{
  const QColor infoColor = QColor(0x268bd2);
  const QColor successColor = QColor(0x859900);
  const QColor warningColor = QColor(0xb58900);
  const QColor errorColor = QColor(0xcb4b16);
}

namespace gui
{

  StatusBar::StatusBar(QWidget* parent) : QWidget(parent)
  {
    qRegisterMetaType<gui::StatusBar::MessageSeverity>("gui::StatusBar::MessageSeverity");
    setMaximumHeight(30);
    setMinimumHeight(30);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  }

  void StatusBar::showMessage(const QString& message, StatusBar::MessageSeverity severity, int timeout)
  {
    _currentMessage.message = message;
    _currentMessage.severity = severity;
    _currentMessage.timeout = timeout;
    update();
  }

  void StatusBar::paintEvent(QPaintEvent* event)
  {
    auto rect = event->rect();
    QPainter painter(this);

    auto color = infoColor;
    if (_currentMessage.severity == Success)
      color = successColor;
    if (_currentMessage.severity == Warning)
      color = warningColor;
    if (_currentMessage.severity == Error)
      color = errorColor;

    painter.fillRect(rect, color);
    painter.setPen(QColor(0xffffff));
    painter.drawText(rect.adjusted(5, 5, -5, -5), Qt::AlignVCenter, _currentMessage.message);
  }

} // namespace gui
