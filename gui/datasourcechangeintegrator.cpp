#include "gui/datasourcechangeintegrator.h"

namespace gui
{

  ChangeIntegrator::ChangeIntegrator(persistence::DataSource* dataSource) : _ds(dataSource)
  {
    connect(this, SIGNAL(resultsAvailable()), this, SLOT(handleAvailableResults()));
    _ds->resultsAvailableSignal().connect([this]() { this->emitResultsAvailable(); });
  }

  ChangeIntegrator::~ChangeIntegrator() { _ds->resultsAvailableSignal().disconnect_all_slots(); }

  void ChangeIntegrator::handleAvailableResults() { _ds->processIntegrationQueue(); }

  void ChangeIntegrator::emitResultsAvailable() { emit resultsAvailable(); }

} // namespace gui
