#ifndef GUI_DATASOURCECHANGEINTEGRATOR_H
#define GUI_DATASOURCECHANGEINTEGRATOR_H

#include "persistence/datasource.h"

#include <QObject>

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
    ~ChangeIntegrator();

  signals:
    void resultsAvailable();
  private slots:
    void handleAvailableResults();

  private:
    void emitResultsAvailable();
    persistence::DataSource* _ds;
  };

} // namespace gui

#endif // GUI_STATUSBAR_H
