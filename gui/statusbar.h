#ifndef GUI_STATUSBAR_H
#define GUI_STATUSBAR_H

#include <QWidget>

namespace gui
{
  /**
   * @brief The StatusBar class is a widget displaying a status message
   *
   * This status bar implementation also supports different colors, depending on the severity of the message.
   */
  class StatusBar : public QWidget
  {
    Q_OBJECT
  public:
    explicit StatusBar(QWidget* parent = 0);

    enum MessageSeverity
    {
      Info,
      Success,
      Warning,
      Error
    };
    Q_ENUM(MessageSeverity);

  public slots:
    /**
     * @brief showMessage displays a message in the status bar
     * @param message the message to display
     * @param severity the severity of the message; mainly influences the status bar's color
     */
    void showMessage(const QString& message, gui::StatusBar::MessageSeverity severity = Info, int timeout = 0);

  protected:
    // QWidget interface
    virtual void paintEvent(QPaintEvent* event) override;

  private:
    struct Message
    {
      MessageSeverity severity = Info;
      QString message;
      int timeout = 0;
    };

    Message _currentMessage;
  };

} // namespace gui

#endif // GUI_STATUSBAR_H
