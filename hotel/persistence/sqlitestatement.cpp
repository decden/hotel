#include "sqlitestatement.h"

namespace hotel {
namespace persistence {

SqliteStatement::SqliteStatement(sqlite3 *db, const std::string &query)
{
  sqlite3_prepare_v2(db, query.c_str(), -1, &_statement, nullptr);
  if (_statement == nullptr)
    std::cerr << "Cannot create prepared statement for query: " << query << std::endl;
}

SqliteStatement::SqliteStatement(SqliteStatement &&that)
  : _statement(that._statement)
{
  that._statement = nullptr;
}

SqliteStatement &SqliteStatement::operator=(SqliteStatement &&that)
{
  if (_statement)
    sqlite3_finalize(_statement);
  _statement = that._statement;
  that._statement = nullptr;
}

SqliteStatement::~SqliteStatement()
{
  if (_statement)
    sqlite3_finalize(_statement);
}

bool SqliteStatement::prepareForQuery()
{
  if (_statement == nullptr)
  {
    std::cerr << "Cannot query nullptr statement" << std::endl;
    return false;
  }

  if (_lastResult == SQLITE_DONE)
    _lastResult = sqlite3_reset(_statement);

  if (_lastResult != SQLITE_OK)
  {
    std::cerr << "Should not query statement, which is in " << _lastResult << " state" << std::endl;
    return false;
  }

  return true;
}

void SqliteStatement::bindArgument(int pos, const char *text)
{
  sqlite3_bind_text(_statement, pos, text, -1, SQLITE_TRANSIENT);
}

void SqliteStatement::bindArgument(int pos, const std::string &text)
{
  sqlite3_bind_text(_statement, pos, text.c_str(), text.size(), SQLITE_TRANSIENT);
}

void SqliteStatement::bindArgument(int pos, int64_t value)
{
  sqlite3_bind_int64(_statement, pos, value);
}

void SqliteStatement::bindArgument(int pos, boost::gregorian::date date)
{
  bindArgument(pos, boost::gregorian::to_iso_string(date));
}

} // namespace persistence
} // namespace hotel
