#ifndef GUI_DATASOURCECHANGEINTEGRATOR_H
#define GUI_DATASOURCECHANGEINTEGRATOR_H

#include "persistence/backend.h"

#include <QtCore/QObject>

#include <boost/signals2.hpp>

#include <atomic>
#include <array>

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
    ChangeIntegrator(persistence::Backend* backend);

  signals:
    void resultsAvailable();
  private slots:
    void handleAvailableResults();

  private:
    void emitResultsAvailable();

    std::atomic<bool> _eventScheduled;
    boost::signals2::connection _connection;
    persistence::Backend* _backend;
  };

} // namespace gui

#endif // GUI_STATUSBAR_H
