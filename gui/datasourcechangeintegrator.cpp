#include "gui/datasourcechangeintegrator.h"

namespace gui
{

  ChangeIntegrator::ChangeIntegrator(persistence::DataSource* dataSource) : _ds(dataSource)
  {
    connect(this, SIGNAL(resultsAvailable()), this, SLOT(handleAvailableResults()));

    _connections[0] = _ds->changeQueue().connectToTaskCompletedSignal([this](int) { this->emitResultsAvailable(); });
    _connections[1] = _ds->changeQueue().connectToStreamChangesAvailableSignal([this]() { this->emitResultsAvailable(); });

    // Initial update...
    handleAvailableResults();
  }

  void ChangeIntegrator::handleAvailableResults() {
    _eventScheduled = false;
    _ds->changeQueue().applyStreamChanges();
  }

  void ChangeIntegrator::emitResultsAvailable() {
    bool eventAlreadyScheduled = _eventScheduled.exchange(true);
    if (!eventAlreadyScheduled)
      emit resultsAvailable();
  }

} // namespace gui
