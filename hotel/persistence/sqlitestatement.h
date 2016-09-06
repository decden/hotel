#ifndef HOTEL_PERSISTENCE_SQLITESTATEMENT_H
#define HOTEL_PERSISTENCE_SQLITESTATEMENT_H

#include <boost/date_time.hpp>
#include <sqlite3.h>

#include <string>

namespace hotel
{
  namespace persistence
  {

    /**
     * @brief The SqliteStatement class holds a prepared SQL statement
     *
     * The class furthermore provides facilities for the execution of the query and the binding of values.
     */
    class SqliteStatement
    {
    public:
      SqliteStatement(sqlite3* db, const std::string& query);
      SqliteStatement(SqliteStatement&& that);
      SqliteStatement& operator=(SqliteStatement&& that);
      SqliteStatement(const SqliteStatement& that) = delete;
      SqliteStatement& operator=(const SqliteStatement& that) = delete;
      ~SqliteStatement();

      /**
       * @brief Executes the SQL statements with the given parameters
       * The parameters are sequentially bound to the prepared statement.
       */
      template <typename... Args> bool execute(Args... args)
      {
        if (!prepareForQuery())
          return false;
        bindArguments(args...);
        _lastResult = sqlite3_step(_statement);
        return true;
      }
      bool execute()
      {
        if (!prepareForQuery())
          return false;
        _lastResult = sqlite3_step(_statement);
        return true;
      }

      /**
       * @brief After calling execute, this functions returns whether there is a result row to read
       * Call hasResultRow and readRow in a loop in order to get all query results.
       */
      bool hasResultRow() { return _lastResult == SQLITE_ROW; }

      template <typename... Args> void readRow(Args&... args)
      {
        readRowInternal(args...);
        _lastResult = sqlite3_step(_statement);
      }

    private:
      // Prepares the statement to be queried again and checks some simple preconditions
      bool prepareForQuery();

      void bindArgument(int pos, const char* text);
      void bindArgument(int pos, const std::string& text);
      void bindArgument(int pos, int64_t value);
      void bindArgument(int pos, boost::gregorian::date date);

      void readArg(int pos, std::string& val);
      void readArg(int pos, int& val);
      void readArg(int pos, boost::gregorian::date& date);

      template <int Pos> void readRowInternal() {}
      template <int Pos = 0, typename T, typename... Args> void readRowInternal(T& val, Args&... others)
      {
        readArg(Pos, val);
        readRowInternal<Pos + 1, Args...>(others...);
      }

      template <int Pos> void bindArguments() {}
      template <int Pos = 1, typename T, typename... Args> void bindArguments(T val, Args... others)
      {
        bindArgument(Pos, val);
        bindArguments<Pos + 1, Args...>(others...);
      }

      int _lastResult = SQLITE_OK;
      sqlite3_stmt* _statement;
    };

  } // namespace persistence
} // namespace hotel

#endif // HOTEL_PERSISTENCE_SQLITESTATEMENT_H
