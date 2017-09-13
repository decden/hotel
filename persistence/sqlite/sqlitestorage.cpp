#include "persistence/sqlite/sqlitestorage.h"

#include "hotel/person.h"

#include <iostream>

namespace persistence
{
  namespace sqlite
  {

    namespace
    {
      void executeSQL(sqlite3* db, const std::string& sql)
      {
        if (!SqliteStatement(db, sql).execute())
          std::cerr << "Cannot execute query: " << sql;
      }

      std::string serializeReservationStatus(hotel::Reservation::ReservationStatus status)
      {
        using Status = hotel::Reservation::ReservationStatus;

        switch (status)
        {
        case Status::Unknown:
          return "unknown";
        case Status::New:
          return "new";
        case Status::Confirmed:
          return "confirmed";
        case Status::CheckedIn:
          return "checked-in";
        case Status::CheckedOut:
          return "checked-out";
        case Status::Archived:
          return "archived";
        default:
          std::cerr << "Unknown reservation status: " << status;
          assert(false);
          return "unknown";
        };
      }

      hotel::Reservation::ReservationStatus parseReservationStatus(const std::string& str) {
        using Status = hotel::Reservation::ReservationStatus;

        if (str == "unknown")
          return Status::Unknown;
        if (str == "new")
          return Status::New;
        if (str == "confirmed")
          return Status::Confirmed;
        if (str == "checked-in")
          return Status::CheckedIn;
        if (str == "checked-out")
          return Status::CheckedOut;
        if (str == "archived")
          return Status::Archived;

        assert(false);
        std::cerr << "Unknown reservation status: " << str;
        return Status::Unknown;
      }
    }

    SqliteStorage::SqliteStorage(const std::string& file) : _db(nullptr)
    {
      if (sqlite3_open(file.c_str(), &_db))
      {
        std::cerr << "Cannot open sqlite database: " << file << std::endl;
        sqlite3_close(_db);
        _db = nullptr;
      }

      if (_db != nullptr)
      {
        createSchema();
        prepareQueries();
      }
    }

    SqliteStorage::~SqliteStorage() { sqlite3_close(_db); }

    void SqliteStorage::deleteAll()
    {
      if (_db == nullptr)
        return;

      _statements.clear();
      executeSQL(_db, "DROP TABLE IF EXISTS h_reservation_atom;");
      executeSQL(_db, "DROP TABLE IF EXISTS h_reservation;");
      executeSQL(_db, "DROP TABLE IF EXISTS h_room;");
      executeSQL(_db, "DROP TABLE IF EXISTS h_room_category;");
      executeSQL(_db, "DROP TABLE IF EXISTS h_hotel;");

      createSchema();
      prepareQueries();
    }

    void SqliteStorage::deleteReservationById(int id)
    {
      query("reservation.delete").execute(id, id);
    }


    template<>
    std::vector<hotel::Hotel> SqliteStorage::loadAll()
    {
      std::vector<hotel::Hotel> results;

      // Read hotels
      auto& hotelsQuery = query("hotel.all");
      hotelsQuery.execute();
      while (hotelsQuery.hasResultRow())
      {
        int id;
        int revision;
        std::string name;
        hotelsQuery.readRow(id, revision, name);
        results.emplace_back(name);
        results.back().setId(id);
        results.back().setRevision(revision);
      }

      for (auto& hotel : results)
      {
        // Read categories
        auto& categoriesQuery = query("room_category.by_hotel_id");
        categoriesQuery.execute(hotel.id());
        while (categoriesQuery.hasResultRow())
        {
          int id;
          std::string short_code;
          std::string name;
          categoriesQuery.readRow(id, short_code, name);
          auto category = std::make_unique<hotel::RoomCategory>(short_code, name);
          category->setId(id);
          hotel.addRoomCategory(std::move(category));
        }

        // Read rooms
        auto& roomsQuery = query("room.by_hotel_id");
        roomsQuery.execute(hotel.id());
        while (roomsQuery.hasResultRow())
        {
          int id;
          int category_id;
          std::string name;
          roomsQuery.readRow(id, category_id, name);
          auto room = std::make_unique<hotel::HotelRoom>(name);
          room->setId(id);
          auto category = hotel.getCategoryById(category_id);
          if (category)
            hotel.addRoom(std::move(room), category->shortCode());
          else
            std::cerr << "Did not find category with id " << category_id << " in hotel " << hotel.name()
                      << " for room " << name << std::endl;
        }
      }

      return results;
    }

    template<>
    std::vector<hotel::Reservation> SqliteStorage::loadAll()
    {
      std::vector<hotel::Reservation> result;

      auto& reservationsQuery = query("reservation_and_atoms.all");
      reservationsQuery.execute();
      std::unique_ptr<hotel::Reservation> current = nullptr;
      while (reservationsQuery.hasResultRow())
      {
        int reservationId;
        int reservationRevision;
        std::string description;
        std::string reservationStatus;
        int adults;
        int children;
        int atomId;
        int roomId;
        boost::gregorian::date dateFrom;
        boost::gregorian::date dateTo;
        reservationsQuery.readRow(reservationId, reservationRevision, description, reservationStatus, adults, children,
                                  atomId, roomId, dateFrom, dateTo);

        if (current == nullptr || current->id() != reservationId)
        {
          if (current)
          {
            result.push_back(std::move(*current));
            current = nullptr;
          }
          current = std::make_unique<hotel::Reservation>(description, roomId,
                                                         boost::gregorian::date_period(dateFrom, dateTo));
          current->setId(reservationId);
          current->setRevision(reservationRevision);
          current->setStatus(parseReservationStatus(reservationStatus));
          current->setNumberOfAdults(adults);
          current->setNumberOfChildren(children);
        }
        else
        {
          current->addContinuation(roomId, dateTo);
        }
        (*current->atoms().rbegin()).setId(atomId);
      }
      if (current)
      {
        result.push_back(std::move(*current));
        current = nullptr;
      }

      return result;
    }

    template<>
    boost::optional<hotel::Hotel> SqliteStorage::loadById(int id)
    {
      boost::optional<hotel::Hotel> result;

      // Read hotels
      auto& hotelQuery = query("hotel.by_id");
      hotelQuery.execute(id);
      if (hotelQuery.hasResultRow())
      {
        int id;
        int revision;
        std::string name;
        hotelQuery.readRow(id, revision, name);
        result = hotel::Hotel(name);
        result->setId(id);
        result->setRevision(revision);
      }


      if (result)
      {
        // Read categories
        auto& categoriesQuery = query("room_category.by_hotel_id");
        categoriesQuery.execute(result->id());
        while (categoriesQuery.hasResultRow())
        {
          int id;
          std::string short_code;
          std::string name;
          categoriesQuery.readRow(id, short_code, name);
          auto category = std::make_unique<hotel::RoomCategory>(short_code, name);
          category->setId(id);
          result->addRoomCategory(std::move(category));
        }

        // Read rooms
        auto& roomsQuery = query("room.by_hotel_id");
        roomsQuery.execute(result->id());
        while (roomsQuery.hasResultRow())
        {
          int id;
          int category_id;
          std::string name;
          roomsQuery.readRow(id, category_id, name);
          auto room = std::make_unique<hotel::HotelRoom>(name);
          room->setId(id);
          auto category = result->getCategoryById(category_id);
          if (category)
            result->addRoom(std::move(room), category->shortCode());
          else
            std::cerr << "Did not find category with id " << category_id << " in hotel " << result->name()
                      << " for room " << name << std::endl;
        }
      }

      return result;
    }

    template<>
    boost::optional<hotel::Reservation> SqliteStorage::loadById(int id)
    {
      boost::optional<hotel::Reservation> result;

      auto& reservationsQuery = query("reservation_and_atoms.by_reservation_id");
      reservationsQuery.execute(id);
      while (reservationsQuery.hasResultRow())
      {
        int reservationRevision;
        std::string description;
        std::string reservationStatus;
        int adults;
        int children;
        int atomId;
        int roomId;
        boost::gregorian::date dateFrom;
        boost::gregorian::date dateTo;
        reservationsQuery.readRow(reservationRevision, description, reservationStatus, adults, children,
                                  atomId, roomId, dateFrom, dateTo);

        if (result == boost::none)
        {
          result = hotel::Reservation(description, roomId, boost::gregorian::date_period(dateFrom, dateTo));
          result->setId(id);
          result->setRevision(reservationRevision);
          result->setStatus(parseReservationStatus(reservationStatus));
          result->setNumberOfAdults(adults);
          result->setNumberOfChildren(children);
        }
        else
        {
          result->addContinuation(roomId, dateTo);
        }
        (*result->atoms().rbegin()).setId(atomId);
      }

      return result;
    }

    void SqliteStorage::storeNewHotel(hotel::Hotel& hotel)
    {
      // First, store the hotel
      query("hotel.insert").execute(hotel.name());
      hotel.setId(static_cast<int>(lastInsertId()));
      hotel.setRevision(1);

      // Store all of the categories
      for (auto& category : hotel.categories())
      {
        query("room_category.insert").execute(hotel.id(), category->shortCode(), category->name());
        category->setId(static_cast<int>(lastInsertId()));
      }

      // Store all of the rooms
      for (auto& room : hotel.rooms())
      {
        query("room.insert").execute(hotel.id(), room->category()->id(), room->name());
        room->setId(static_cast<int>(lastInsertId()));
      }
    }

    void SqliteStorage::storeNewReservationAndAtoms(hotel::Reservation& reservation)
    {
      auto reservationStatus = serializeReservationStatus(reservation.status());
      query("reservation.insert").execute(reservation.description(), reservationStatus,
                                          reservation.numberOfAdults(), reservation.numberOfChildren());
      reservation.setId(static_cast<int>(lastInsertId()));
      reservation.setRevision(1);
      for (auto& atom : reservation.atoms())
      {
        auto& q = query("reservation_atom.insert");
        q.execute(reservation.id(), atom.roomId(), atom.dateRange().begin(), atom.dateRange().end());
        atom.setId(static_cast<int>(lastInsertId()));
      }
    }

    template <>
    bool SqliteStorage::update<hotel::Hotel>(hotel::Hotel& value)
    {
      auto& q = query("hotel.update");
      q.execute(value.name(), value.id(), value.revision());

      int updatedRows = sqlite3_changes(_db);
      if (updatedRows == 1)
      {
        value.setRevision(value.revision() + 1);
        return true;
      }
      return false;
    }

    template <>
    bool SqliteStorage::update<hotel::Reservation>(hotel::Reservation& value)
    {
      auto& q = query("reservation.update");
      q.execute(value.description(), serializeReservationStatus(value.status()), value.numberOfAdults(),
                value.numberOfChildren(), value.id(), value.revision());

      // TODO, we need to update also the atoms!

      int updatedRows = sqlite3_changes(_db);
      if (updatedRows == 1)
      {
        value.setRevision(value.revision() + 1);
        return true;
      }
      return false;
    }

    template<>
    bool SqliteStorage::update<hotel::Person>(hotel::Person& value)
    {
      // TODO: Not yet implemented
      return false;
    }

    SqliteStatement& SqliteStorage::query(const std::string& key)
    {
      auto it = _statements.find(key);
      if (it == _statements.end())
      {
        std::cerr << "Query '" << key << "' does not exist!" << std::endl;
      }

      return it->second;
    }

    int64_t SqliteStorage::lastInsertId() { return sqlite3_last_insert_rowid(_db); }

    void SqliteStorage::beginTransaction() { sqlite3_exec(_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr); }
    void SqliteStorage::commitTransaction() { sqlite3_exec(_db, "COMMIT TRANSACTION", nullptr, nullptr, nullptr); }
    void SqliteStorage::rollbackTransaction() { sqlite3_exec(_db, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr); }

    void SqliteStorage::prepareQueries()
    {
      _statements.emplace("hotel.insert", SqliteStatement(_db, "INSERT INTO h_hotel (name) VALUES (?);"));
      _statements.emplace("hotel.update", SqliteStatement(_db, "UPDATE h_hotel SET name=?, revision=revision+1 WHERE id=? and revision=?;"));
      _statements.emplace("hotel.all", SqliteStatement(_db, "SELECT id, revision, name FROM h_hotel;"));
      _statements.emplace("hotel.by_id", SqliteStatement(_db, "SELECT id, revision, name FROM h_hotel WHERE id = ?;"));
      _statements.emplace(
          "room_category.insert",
          SqliteStatement(_db, "INSERT INTO h_room_category (hotel_id, short_code, name) VALUES (?, ?, ?);"));
      _statements.emplace("room_category.by_hotel_id",
                          SqliteStatement(_db, "SELECT id, short_code, name FROM h_room_category WHERE hotel_id = ?;"));
      _statements.emplace("room.insert",
                          SqliteStatement(_db, "INSERT INTO h_room (hotel_id, category_id, name) VALUES (?, ?, ?);"));
      _statements.emplace("room.by_hotel_id",
                          SqliteStatement(_db, "SELECT id, category_id, name FROM h_room WHERE hotel_id = ?;"));

      _statements.emplace(
          "reservation_and_atoms.all",
          SqliteStatement(_db, "SELECT r.id, r.revision, r.description, r.status, r.adults, r.children, a.id, a.room_id, a.date_from, a.date_to "
                               "FROM h_reservation as r, h_reservation_atom as a WHERE "
                               "a.reservation_id = r.id ORDER BY r.id, a.date_from;"));
      _statements.emplace(
          "reservation_and_atoms.by_reservation_id",
          SqliteStatement(_db, "SELECT r.revision, r.description, r.status, r.adults, r.children, a.id, a.room_id, a.date_from, a.date_to "
                               "FROM h_reservation as r, h_reservation_atom as a WHERE "
                               "a.reservation_id = r.id and r.id = ? ORDER BY r.id, a.date_from;"));
      _statements.emplace("reservation.insert",
                          SqliteStatement(_db, "INSERT INTO h_reservation (description, status, adults, children) VALUES (?, ?, ?, ?);"));
      _statements.emplace("reservation.update",
                          SqliteStatement(_db, "UPDATE h_reservation SET description=?, status=?, adults=?, children=?, revision=revision+1 WHERE id = ? AND revision = ?;"));
      _statements.emplace("reservation.delete",
                          SqliteStatement(_db, "DELETE FROM h_reservation_atom where reservation_id = ?; DELETE FROM h_reservation WHERE reservation_id = ?;"));
      _statements.emplace("reservation_atom.insert",
                          SqliteStatement(_db, "INSERT INTO h_reservation_atom (reservation_id, room_id, "
                                               "date_from, date_to) VALUES (?, ?, ?, ?);"));
    }

    void SqliteStorage::createSchema()
    {
      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_hotel ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "revision INTEGER NOT NULL DEFAULT 1, "
                      "name TEXT NOT NULL);");
      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_room_category ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "hotel_id INTEGER NOT NULL," // Foreign key
                      "short_code TEXT NOT NULL,"
                      "name TEXT NOT NULL);");
      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_room ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "hotel_id INTEGER NOT NULL,"    // Foreign key
                      "category_id INTEGER NOT NULL," // Foreign key
                      "name TEXT NOT NULL);");

      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_reservation ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "revision INTEGER NOT NULL DEFAULT 1, "
                      "description TEXT NOT NULL, "
                      "status TEXT NOT NULL,"
                      "adults INTEGER NOT NULL,"
                      "children INTEGER NOT NULL);");
      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_reservation_atom ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "reservation_id INTEGER NOT NULL," // Foreign key
                      "room_id INTEGER NOT NULL,"        // Foreign key
                      "date_from TEXT NOT NULL,"
                      "date_to TEXT NOT NULL);");
    }

  } // namespace sqlite
} // namespace persistence
