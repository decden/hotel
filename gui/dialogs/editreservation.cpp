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
      _reservationStreamHandle.initializedSignal.connect(boost::bind(&EditReservationDialog::reservationsInitialized, this));
      _reservationStreamHandle.itemsAddedSignal.connect(boost::bind(&EditReservationDialog::reservationsAdded, this, boost::placeholders::_1));
      _reservationStreamHandle.itemsRemovedSignal.connect(boost::bind(&EditReservationDialog::reservationsRemoved, this, boost::placeholders::_1));
      _reservationStreamHandle.allItemsRemovedSignal.connect(boost::bind(&EditReservationDialog::allReservationsRemoved, this));

      // Connect to data stream
      nlohmann::json options;
      options["id"] = objectId;
      _reservationStreamHandle.connect(ds, "reservation.by_id", options);

      auto layout = new QGridLayout();
      _lblMessage = new QLabel();
      _cbxStatus = new QComboBox();
      _cbxStatus->addItems({"New", "Confirmed", "CheckedIn", "CheckedOut", "Archived"});
      _txtDescription = new QLineEdit();
      _spbNumberOfAdults = new QSpinBox();
      _spbNumberOfAdults->setRange(0, 10);
      _spbNumberOfChildren = new QSpinBox();
      _spbNumberOfChildren->setRange(0, 10);
      _btnSave = new QPushButton(tr("Save"));

      layout->addWidget(_lblMessage, 0, 0, 1, 2);
      layout->addWidget(new QLabel(tr("Status:")), 1, 0);
      layout->addWidget(new QLabel(tr("Description:")), 2, 0);
      layout->addWidget(new QLabel(tr("Number of adults:")), 3, 0);
      layout->addWidget(new QLabel(tr("Number of children:")), 4, 0);
      layout->addWidget(_cbxStatus, 1, 1);
      layout->addWidget(_txtDescription, 2, 1);
      layout->addWidget(_spbNumberOfAdults, 3, 1);
      layout->addWidget(_spbNumberOfChildren, 4, 1);
      layout->addWidget(_btnSave, 5, 1);
      setLayout(layout);

      // Connect events
      connect(_btnSave, SIGNAL(clicked(bool)), this, SLOT(saveClicked()));

      updateUI();
    }

    void EditReservationDialog::saveClicked()
    {
      assert(_status == Status::Ready);
      assert(_reservation != boost::none);

      _status = Status::Saving;

      auto updatedReservation = std::make_unique<hotel::Reservation>(*_reservation);
      updatedReservation->setStatus(static_cast<hotel::Reservation::ReservationStatus>(_cbxStatus->currentIndex() + 2));
      updatedReservation->setDescription(_txtDescription->text().toStdString());
      updatedReservation->setNumberOfAdults(_spbNumberOfAdults->value());
      updatedReservation->setNumberOfChildren(_spbNumberOfChildren->value());

      _saveTask = _dataSource.queueOperation(persistence::op::UpdateReservation{std::move(updatedReservation)});
      _saveTaskUpdatedConnection = _saveTask->connectToChangedSignal(boost::bind(&EditReservationDialog::saveTaskUpdated, this));

      updateUI();
    }

    void EditReservationDialog::reservationsInitialized()
    {
      assert(_status == Status::NotInitialized);
      _status = Status::Ready;

      updateUI();
    }

    void EditReservationDialog::reservationsAdded(const std::vector<hotel::Reservation> &reservations)
    {      
      // For now, ignore changes that happen once the stream has been initialized
      if (_status != Status::NotInitialized)
        return;

      assert(reservations.size() == 1);
      assert(_reservation == boost::none);

      _reservation = std::move(reservations[0]);
      _cbxStatus->setCurrentIndex(_reservation->status() - 2);
      _txtDescription->setText(QString::fromStdString(_reservation->description()));
      _spbNumberOfAdults->setValue(_reservation->numberOfAdults());
      _spbNumberOfChildren->setValue(_reservation->numberOfChildren());
    }

    void EditReservationDialog::reservationsRemoved(const std::vector<int> &ids)
    {
      assert(ids.size() == 1);
      assert(_reservation != boost::none);
      assert(_reservation->id() == ids[0]);
      _reservation = boost::none;
      _status = Status::Removed;
      updateUI();
    }

    void EditReservationDialog::allReservationsRemoved()
    {
      _reservation = boost::none;
      _status = Status::Removed;
      updateUI();
    }

    void EditReservationDialog::saveTaskUpdated()
    {
      if (_saveTask != boost::none && _saveTask->completed())
      {
        close();
      }
    }

    void EditReservationDialog::updateUI()
    {
      setEnabled(_status == Status::Ready && _reservation != boost::none);

      if (_status == Status::Ready && _reservation != boost::none)
        _lblMessage->setText(tr("Ready"));
      else if (_status == Status::Ready && _reservation == boost::none)
        _lblMessage->setText(tr("Could not load data"));
      else if (_status == Status::NotInitialized)
        _lblMessage->setText(tr("Loading..."));
      else if (_status == Status::Removed)
        _lblMessage->setText(tr("The reservation was deleted while you were editing it"));
      else if (_status == Status::Saving)
        _lblMessage->setText(tr("Saving..."));
    }
  } // namespace dialogs
} // namespace gui
