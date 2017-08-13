#include "gui/dialogs/editreservation.h"

#include <QDate>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QMessageBox>

namespace gui
{
  namespace dialogs
  {
    EditReservationDialog::EditReservationDialog(persistence::DataSource& ds, int objectId)
        : QDialog(nullptr), _dataSource(ds)
    {
      setAttribute(Qt::WA_DeleteOnClose);
      setMinimumWidth(500);
      setWindowTitle(tr("Edit Reservation"));

      // Connect events
      _reservationStreamHandle.initializedSignal.connect(boost::bind(&EditReservationDialog::reservationsInitialized, this));
      _reservationStreamHandle.itemsAddedSignal.connect(boost::bind(&EditReservationDialog::reservationsAdded, this, boost::placeholders::_1));
      _reservationStreamHandle.itemsUpdatedSignal.connect(boost::bind(&EditReservationDialog::reservationsUpdated, this, boost::placeholders::_1));
      _reservationStreamHandle.itemsRemovedSignal.connect(boost::bind(&EditReservationDialog::reservationsRemoved, this, boost::placeholders::_1));
      _reservationStreamHandle.allItemsRemovedSignal.connect(boost::bind(&EditReservationDialog::allReservationsRemoved, this));

      // Connect to data stream
      nlohmann::json options;
      options["id"] = objectId;
      _reservationStreamHandle.connect(ds, "reservation.by_id", options);

      auto layout = new QGridLayout();
      auto buttonsLayout = new QHBoxLayout();
      auto outerLayout = new QVBoxLayout();
      outerLayout->setMargin(0);
      layout->setMargin(10);
      buttonsLayout->setMargin(10);
      _statusBar = new StatusBar();
      _cbxStatus = new QComboBox();
      _cbxStatus->addItems({"New", "Confirmed", "CheckedIn", "CheckedOut", "Archived"});
      _txtDescription = new QLineEdit();
      _spbNumberOfAdults = new QSpinBox();
      _spbNumberOfAdults->setRange(0, 10);
      _spbNumberOfChildren = new QSpinBox();
      _spbNumberOfChildren->setRange(0, 10);
      _btnSave = new QPushButton(tr("Save"));

      outerLayout->addWidget(_statusBar);
      outerLayout->addLayout(layout);
      outerLayout->addStretch();
      outerLayout->addLayout(buttonsLayout);

      layout->addWidget(new QLabel(tr("Status:")), 0, 0);
      layout->addWidget(new QLabel(tr("Description:")), 1, 0);
      layout->addWidget(new QLabel(tr("Number of adults:")), 2, 0);
      layout->addWidget(new QLabel(tr("Number of children:")), 3, 0);
      layout->addWidget(_cbxStatus, 0, 1);
      layout->addWidget(_txtDescription, 1, 1);
      layout->addWidget(_spbNumberOfAdults, 2, 1);
      layout->addWidget(_spbNumberOfChildren, 3, 1);

      buttonsLayout->addStretch();
      buttonsLayout->addWidget(_btnSave);

      setLayout(outerLayout);

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
      assert(_status == Status::NotInitialized);
      assert(reservations.size() == 1);
      assert(_reservation == boost::none);

      _reservation = std::move(reservations[0]);
      _cbxStatus->setCurrentIndex(_reservation->status() - 2);
      _txtDescription->setText(QString::fromStdString(_reservation->description()));
      _spbNumberOfAdults->setValue(_reservation->numberOfAdults());
      _spbNumberOfChildren->setValue(_reservation->numberOfChildren());
    }

    void EditReservationDialog::reservationsUpdated(const std::vector<hotel::Reservation> &reservations)
    {
      // While saving, we are expecting updates to come in. We can safely ignore those.
      if (_status == Status::Saving)
        return;

      assert (_status == Status::Ready);
      assert(reservations.size() == 1);

      QMessageBox msgBox(this);
      msgBox.addButton(tr("Keep my changes"), QMessageBox::ApplyRole);
      auto *discardChangesButton = msgBox.addButton(tr("Discard my changes"), QMessageBox::ResetRole);
      msgBox.setWindowTitle(tr("Edit conflict"));
      msgBox.setText(tr("While you were editing this item, somebody else did change this item.\nWhat do you want to do with your changes?"));
      msgBox.exec();
      if (msgBox.clickedButton() == discardChangesButton)
      {
        _reservation = std::move(reservations[0]);
        _cbxStatus->setCurrentIndex(_reservation->status() - 2);
        _txtDescription->setText(QString::fromStdString(_reservation->description()));
        _spbNumberOfAdults->setValue(_reservation->numberOfAdults());
        _spbNumberOfChildren->setValue(_reservation->numberOfChildren());
      }

      updateUI();
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
      {
        auto description = QString::fromStdString(_reservation->description());
        auto fromDate = _reservation->dateRange().begin();
        auto toDate = _reservation->dateRange().end();
        auto fromDateText = QDate(fromDate.year(), fromDate.month(), fromDate.day()).toString("dd-MM-yyyy");
        auto toDateText = QDate(toDate.year(), toDate.month(), toDate.day()).toString("dd-MM-yyyy");
        _statusBar->showMessage(tr("%1 - From %2 to %3").arg(description, fromDateText, toDateText), gui::StatusBar::Success);
      }
      else if (_status == Status::Ready && _reservation == boost::none)
        _statusBar->showMessage(tr("Could not load data"), gui::StatusBar::Error);
      else if (_status == Status::NotInitialized)
        _statusBar->showMessage(tr("Loading..."), gui::StatusBar::Info);
      else if (_status == Status::Removed)
        _statusBar->showMessage(tr("The reservation was deleted while you were editing it"), gui::StatusBar::Error);
      else if (_status == Status::Saving)
        _statusBar->showMessage(tr("Saving..."), gui::StatusBar::Info);
    }
  } // namespace dialogs
} // namespace gui
