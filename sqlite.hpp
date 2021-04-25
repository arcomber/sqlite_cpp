#ifndef SQLITE_HPP_
#define SQLITE_HPP_

#include "sqlite3.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <cstdint>
#include <iostream>

namespace itel {

/*
sqlite types can be: NULL, INTEGER, REAL, TEXT, BLOB
NULL: we don't support this type
INTEGER: int
REAL: double
TEXT: std::string
BLOB: std::vector<uint8_t>
*/
	using sqlite_data_type = std::variant<int, double, std::string, std::vector<uint8_t> >;

	struct column_values {
		std::string column_name;
		sqlite_data_type column_type;
	};

	std::ostream& operator<< (std::ostream& os, const column_values& v);

	class sqlite {
	public:
		/* sqlite constructor */
		sqlite();
		/* cleanup */
		~sqlite();
		/* database must be opened before calling an sql operation */
		int open(const std::string& filename);
		/* close database connection */
		int close();
		/* INSERT INTO (col1, col2) VALUES (:col1, :col2); table_name is table to insert into
		fields are a list of column name to column value key value pairs */
		int insert_into(const std::string& table_name, std::vector<column_values> fields);
		/* returns rowid of last successfully inserted row. If no rows 
		inserted since this database connectioned opened, returns zero. */
		int last_insert_rowid();
		/* get error text relating to last sqlite error. Call this function
		whenever an operation returns a sqlite error code */
		const std::string get_last_error_description();

	private:
		sqlite3* db_;
	};

} // itel

#endif // SQLITE_HPP_
