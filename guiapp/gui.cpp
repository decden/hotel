#include "guiapp/testdata.h"

#include "persistence/datasource.h"
#include "persistence/net/netclientbackend.h"

#include "gui/dialogs/editreservation.h"
#include "gui/datasourcechangeintegrator.h"
#include "gui/planningwidget.h"
#include "gui/planningwidget/newreservationtool.h"

#include <QApplication>
#include <QGridLayout>
#include <QWindow>
#include <QPushButton>
#include <QScrollBar>

#include <memory>

int main(int argc, char** argv)
{
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication app(argc, argv);

  persistence::DataSource dataSource(std::make_unique<persistence::net::NetClientBackend>("localhost", 46835));
  //persistence::DataSource dataSource("test.db");

  if (app.arguments().contains("--createTestData"))
    guiapp::createTestData(dataSource);

  gui::PlanningWidget widget(dataSource);
  widget.registerTool("new-reservation", std::make_unique<gui::planningwidget::NewReservationTool>());
  widget.activateTool("new-reservation");

  QObject::connect(&widget, &gui::PlanningWidget::reservationDoubleClicked, [&](const hotel::Reservation &res)
  {
    if (res.status() != hotel::Reservation::Temporary)
    {
      auto dialog = new gui::dialogs::EditReservationDialog(dataSource, res.id());
      dialog->show();
    }
  });

  widget.showMaximized();

  gui::ChangeIntegrator integrator(&dataSource);

  return app.exec();
}
