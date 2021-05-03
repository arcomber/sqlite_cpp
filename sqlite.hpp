/*
sqlite - a thin c++ wrapper of sqlite c library
version 0.0.1
https://github.com/arcomber/sqlite_cpp
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2013-2019 Niels Lohmann <http://nlohmann.me>.
Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef SQLITE_HPP_
#define SQLITE_HPP_

// uncomment below to print blob data as hex in ostream overload of column_values
// #define PRINT_BLOB_AS_HEX

#include "sqlite3.h"

#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <iostream>

namespace sql {

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
		sqlite_data_type column_value;
	};

	std::ostream& operator<< (std::ostream& os, const column_values& v);
	std::ostream& operator<< (std::ostream& os, const sqlite_data_type& v);

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
		int insert_into(const std::string& table_name, const std::vector<column_values>& fields);

		/* returns rowid of last successfully inserted row. If no rows 
		inserted since this database connectioned opened, returns zero. */
		int last_insert_rowid();

		/* UPDATE contacts SET col1 = value1, col2 = value2, ... WHERE rowid = therowid;
        table_name is table to update, fields are a list of column name to column value key value pairs to update and
        where is a list of where criteria/filters */
		int update(const std::string& table_name, const std::vector<column_values>& fields, const std::vector<column_values>& where);

		/* UPDATE contacts SET col1 = value1, col2 = value2, ...;  
		same as update(table_name, fields, where) except no WHERE criteria so potential to change EVERY row. USE WITH CAUTION. */
		int update(const std::string& table_name, const std::vector<column_values>& fields);

		/* DELETE FROM table_name WHERE condition; */
		int delete_from(const std::string& table_name, const std::vector<column_values>& where);

		/* DELETE FROM table_name;
		same as delete_from(table_name, where) except no WHERE criteria so potential to delete EVERY row. USE WITH CAUTION. */
		int delete_from(const std::string& table_name);

		/* SELECT * FROM table_name WHERE col1 = x; 
		table_name is table to select, 
		where is list of key value pairs as criterion for select
		results is a table of values */
		int select_star(const std::string& table_name, 
			            const std::vector<column_values>& where, 
			                  std::vector<std::vector<sql::column_values>>& results);

		/* SELECT * FROM table_name; 
		table_name is table to select, 
		results is a table of values */
		int select_star(const std::string& table_name,
			std::vector<std::vector<sql::column_values>>& results);

		/* SELECT col1, col2 FROM table_name WHERE col1 = x;
		table_name is table to select, 
		fields are list of fields in table to select
		where is list of key value pairs as criterion for select
		results is a table of values */
		int select_columns(const std::string& table_name,
			               const std::vector<std::string>& fields,
			               const std::vector<column_values>& where,
			                     std::vector<std::vector<sql::column_values>>& results);

		/* get error text relating to last sqlite error. Call this function
		whenever an operation returns a sqlite error code */
		const std::string get_last_error_description();

	private:
		sqlite3* db_;
	};

} // itel

#endif // SQLITE_HPP_
