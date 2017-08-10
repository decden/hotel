#include "gui/dialogs/editreservation.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

namespace gui
{
  namespace dialogs
  {
    EditReservationDialog::EditReservationDialog(persistence::DataSource& ds, int objectId)
        : QDialog(nullptr), _dataSource(ds)
    {
      setWindowTitle(tr("Edit Reservation"));

      // Connect events
      _reservationStreamHandle.itemsAddedSignal.connect(boost::bind(&EditReservationDialog::reservationsAdded, this, boost::placeholders::_1));
      _reservationStreamHandle.itemsRemovedSignal.connect(boost::bind(&EditReservationDialog::reservationsRemoved, this, boost::placeholders::_1));
      _reservationStreamHandle.allItemsRemovedSignal.connect(boost::bind(&EditReservationDialog::allReservationsRemoved, this));

      // Connect to data stream
      nlohmann::json options;
      options["id"] = objectId;
      _reservationStreamHandle.connect(ds, "reservation.by_id", options);

      auto layout = new QGridLayout();
      _cbxStatus = new QComboBox();
      _cbxStatus->addItems({"New", "Confirmed", "CheckedIn", "CheckedOut", "Archived"});
      _txtDescription = new QLineEdit();
      _spbNumberOfAdults = new QSpinBox();
      _spbNumberOfAdults->setRange(0, 10);
      _spbNumberOfChildren = new QSpinBox();
      _spbNumberOfChildren->setRange(0, 10);
      _btnSave = new QPushButton(tr("Save"));

      layout->addWidget(new QLabel(tr("Status:")), 0, 0);
      layout->addWidget(new QLabel(tr("Description:")), 1, 0);
      layout->addWidget(new QLabel(tr("Number of adults:")), 2, 0);
      layout->addWidget(new QLabel(tr("Number of children:")), 3, 0);
      layout->addWidget(_cbxStatus, 0, 1);
      layout->addWidget(_txtDescription, 1, 1);
      layout->addWidget(_spbNumberOfAdults, 2, 1);
      layout->addWidget(_spbNumberOfChildren, 3, 1);
      layout->addWidget(_btnSave, 4, 1);
      setLayout(layout);
      setEnabled(false);

      // Connect events
      connect(_btnSave, SIGNAL(clicked(bool)), this, SLOT(saveClicked()));
    }

    void EditReservationDialog::saveClicked()
    {
      assert(_reservation != boost::none);
      auto updatedReservation = std::make_unique<hotel::Reservation>(*_reservation);
      updatedReservation->setStatus(static_cast<hotel::Reservation::ReservationStatus>(_cbxStatus->currentIndex() + 2));
      updatedReservation->setDescription(_txtDescription->text().toStdString());
      updatedReservation->setNumberOfAdults(_spbNumberOfAdults->value());
      updatedReservation->setNumberOfChildren(_spbNumberOfChildren->value());

      _dataSource.queueOperation(persistence::op::UpdateReservation{std::move(updatedReservation)});
      // TODO: Wait for task completion
      close();
    }

    void EditReservationDialog::reservationsAdded(const std::vector<hotel::Reservation> &reservations)
    {
      assert(reservations.size() == 1);
      assert(_reservation == boost::none);
      _reservation = std::move(reservations[0]);
      _cbxStatus->setCurrentIndex(_reservation->status() - 2);
      _txtDescription->setText(QString::fromStdString(_reservation->description()));
      _spbNumberOfAdults->setValue(_reservation->numberOfAdults());
      _spbNumberOfChildren->setValue(_reservation->numberOfChildren());
      setEnabled(true);
      setWindowTitle(tr("Edit Reservation %1").arg(_reservation->description().c_str()));
    }

    void EditReservationDialog::reservationsRemoved(const std::vector<int> &ids)
    {
      assert(ids.size() == 1);
      assert(_reservation != boost::none);
      assert(_reservation->id() == ids[0]);
      setWindowTitle(tr("Edit Reservation - Reservation was deleted"));
      setEnabled(false);
    }

    void EditReservationDialog::allReservationsRemoved()
    {
      _reservation = boost::none;
      setWindowTitle(tr("Edit Reservation - Reservation was deleted"));
      setEnabled(false);
    }
  } // namespace dialogs
} // namespace gui
