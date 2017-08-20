#include "gui/datasourcechangeintegrator.h"

#include "persistence/changequeue.h"

namespace gui
{

  ChangeIntegrator::ChangeIntegrator(persistence::Backend* backend) : _backend(backend)
  {
    connect(this, SIGNAL(resultsAvailable()), this, SLOT(handleAvailableResults()));

    _connections[0] = _backend->changeQueue().connectToTaskCompletedSignal([this]() { this->emitResultsAvailable(); });
    _connections[1] = _backend->changeQueue().connectToStreamChangesAvailableSignal([this]() { this->emitResultsAvailable(); });

    // Initial update...
    handleAvailableResults();
  }

  void ChangeIntegrator::handleAvailableResults() {
    _eventScheduled = false;
    _backend->changeQueue().applyStreamChanges();
    _backend->changeQueue().notifyCompletedTasks();
  }

  void ChangeIntegrator::emitResultsAvailable() {
    bool eventAlreadyScheduled = _eventScheduled.exchange(true);
    if (!eventAlreadyScheduled)
      emit resultsAvailable();
  }

} // namespace gui
