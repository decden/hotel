#ifndef GUI_DATASOURCECHANGEINTEGRATOR_H
#define GUI_DATASOURCECHANGEINTEGRATOR_H

#include "persistence/datasource.h"

#include <QObject>

#include <atomic>

namespace gui
{
  /**
   * @brief The ChangeIntegrator class observes the given data source and integrates any available changes.
   *
   * The main difficulty is that the data source events are triggered on an arbitrary thread, while the changes need
   * to be integrated on the main thread.
   */
  class ChangeIntegrator : public QObject
  {
    Q_OBJECT
  public:
    ChangeIntegrator(persistence::DataSource* dataSource);

  signals:
    void resultsAvailable();
  private slots:
    void handleAvailableResults();

  private:
    void emitResultsAvailable();

    std::atomic<bool> _eventScheduled;
    std::array<boost::signals2::connection, 2> _connections;
    persistence::DataSource* _ds;
  };

} // namespace gui

#endif // GUI_STATUSBAR_H
