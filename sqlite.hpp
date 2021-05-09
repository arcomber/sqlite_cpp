/*
sqlite - a thin c++ wrapper of sqlite c library
version 0.0.1
https://github.com/arcomber/sqlite_cpp
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2021 Angus Comber
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
#include <map>

#define EXIT_ON_ERROR(resultcode) \
if (resultcode != SQLITE_OK) \
{ \
    sqlite3_finalize(stmt); \
    return resultcode; \
}

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

	struct where_binding {
		std::string column_name;
		sqlite_data_type column_value;
	};

	std::ostream& operator<< (std::ostream& os, const column_values& v);
	std::ostream& operator<< (std::ostream& os, const sqlite_data_type& v);
	std::ostream& operator<< (std::ostream& os, const std::map<std::string, sqlite_data_type>& v);


	class sqlite {
	public:
		sqlite();
		~sqlite();

		/* database must be opened before calling an sql operation */
		int open(const std::string& filename);

		/* close database connection */
		int close();

		/* INSERT INTO (col1, col2) VALUES (:col1, :col2); table_name is table to insert into
		begin and end are iterators to a collection of column name -> value key value pairs */
		template <typename columns_iterator>
		int insert_into(const std::string& table_name, columns_iterator begin, columns_iterator end);

		/* returns rowid of last successfully inserted row. If no rows
		inserted since this database connectioned opened, returns zero. */
		int last_insert_rowid();

		/* UPDATE contacts SET col1 = value1, col2 = value2, ... WHERE rowid = therowid;
		table_name is table to update,
		columns_begin and columns_end are iterators to a collection of column name -> value key value pairs
		where_clause is WHERE clause predicate expressed as : parameterised query
		where_bindings_begin and end are iterators to a collection of where bindings used in the where clause */
		template <typename columns_iterator, typename where_bindings_iterator>
		int update(
			const std::string& table_name,
			columns_iterator columns_begin, 
			columns_iterator columns_end,
			const std::string& where_clause,
			where_bindings_iterator where_bindings_begin,
			where_bindings_iterator where_bindings_end);

		/* UPDATE contacts SET col1 = value1, col2 = value2, ...;
		same as update(table_name, begin, end, where) except no WHERE clause so potential to change EVERY row. USE WITH CAUTION. */
		template <typename columns_iterator>
		int update(const std::string& table_name, 
			columns_iterator begin,
			columns_iterator end);

		/* DELETE FROM table_name WHERE condition; */
		template <typename where_bindings_iterator>
		int delete_from(const std::string& table_name, 
			const std::string& where_clause, 
			where_bindings_iterator where_bindings_begin,
			where_bindings_iterator where_bindings_end);

		/* DELETE FROM table_name;
		same as delete_from(table_name, where) except no WHERE clause so potential to delete EVERY row. USE WITH CAUTION. */
		int delete_from(const std::string& table_name);

		/* SELECT * FROM table_name WHERE col1 = x;
		table_name is table to select,
		where_clause is WHERE clause predicate expressed as : parameterised query
		where_bindings_begin and end are iterators to a collection of where bindings used in the where clause
		results is a table of values */
		template <typename where_bindings_iterator>
		int select_star(const std::string& table_name,
			const std::string& where_clause,
			where_bindings_iterator where_bindings_begin,
			where_bindings_iterator where_bindings_end,
		    std::vector<std::map<std::string, sqlite_data_type>>& results);
		/* SELECT * FROM table_name;
		table_name is table to select,
		results is a table of values */
		int select_star(const std::string& table_name,
			std::vector<std::map<std::string, sqlite_data_type>>& results);

		/* SELECT col1, col2 FROM table_name WHERE col1 = x;
		table_name is table to select,
		name_begin and end are iterators to a collection of column name strings in table to select
		where_clause is the sql WHERE clause
		where_bindings_begin and end are iterators to a collection of where bindings used in the where clause
		results is a table of values */
		template <typename column_names_iterator, typename where_bindings_iterator>
		int select_columns(const std::string& table_name,
			column_names_iterator name_begin,
			column_names_iterator name_end,
			const std::string& where_clause,
			where_bindings_iterator where_bindings_begin,
			where_bindings_iterator where_bindings_end,
			std::vector<std::map<std::string, sqlite_data_type>>& results);

		/* get error text relating to last sqlite error. Call this function
		whenever an operation returns a sqlite error code */
		const std::string get_last_error_description();

	private:
		sqlite3* db_;

		template <typename columns_iterator>
		int bind_fields(sqlite3_stmt* stmt, columns_iterator begin, columns_iterator end);

		template <typename columns_iterator>
		std::string  insert_into_helper(const std::string& table_name, columns_iterator begin, columns_iterator end);

		template <typename columns_iterator>
		std::string update_helper(
			const std::string& table_name,
			columns_iterator begin,
			columns_iterator end,
			const std::string& where_clause);

		template <typename where_binding_iterator>
		int bind_where(sqlite3_stmt* stmt, where_binding_iterator begin, where_binding_iterator end);

		int step_and_finalise(sqlite3_stmt* stmt);

		std::string space_if_required(const std::string& s);

		std::string delete_from_helper(
			const std::string& table_name,
			const std::string& where_clause);

		template <typename column_names_iterator>
		const std::string select_helper(
			const std::string& table_name,
			column_names_iterator name_begin,
			column_names_iterator name_end,
			const std::string& where_clause);
	};

	template <typename columns_iterator>
	int sqlite::bind_fields(sqlite3_stmt* stmt, columns_iterator begin, columns_iterator end) {

		int rc = SQLITE_OK;

		for (auto it = begin; it != end; ++it) {
			std::string next_param{ ':' + it->column_name };
			int idx = sqlite3_bind_parameter_index(stmt, next_param.c_str());

			switch (it->column_value.index()) {
			case 0: rc = sqlite3_bind_int(stmt, idx, std::get<0>(it->column_value)); break;
			case 1: rc = sqlite3_bind_double(stmt, idx, std::get<1>(it->column_value)); break;
			case 2: rc = sqlite3_bind_text(stmt, idx, std::get<2>(it->column_value).c_str(), -1, SQLITE_STATIC); break;
			case 3:
				rc = sqlite3_bind_blob(stmt, idx, std::get<3>(it->column_value).data(), 
					static_cast<int>(std::get<3>(it->column_value).size()), SQLITE_STATIC);
				break;
			}
		}
		return rc;
	}

	template <typename columns_iterator>
	int sqlite::insert_into(const std::string& table_name, columns_iterator begin, columns_iterator end) {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		const std::string sql = insert_into_helper(table_name, begin, end);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		EXIT_ON_ERROR(bind_fields(stmt, begin, end));

		return step_and_finalise(stmt);
	}

		template <typename columns_iterator, typename where_bindings_iterator>
		int sqlite::update(
			const std::string & table_name,
			columns_iterator columns_begin,
			columns_iterator columns_end,
			const std::string & where_clause,
			where_bindings_iterator where_bindings_begin,
			where_bindings_iterator where_bindings_end) {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		const std::string sql = update_helper(table_name, columns_begin, columns_end, where_clause);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		EXIT_ON_ERROR(bind_fields(stmt, columns_begin, columns_end));

		EXIT_ON_ERROR(bind_where(stmt, where_bindings_begin, where_bindings_end));

		return step_and_finalise(stmt);
	}

	template <typename columns_iterator>
	int sqlite::update(const std::string& table_name,
		columns_iterator begin,
		columns_iterator end) {
		return update(table_name, begin, end, "", {});
	}

	template <typename columns_iterator>
	std::string  sqlite::insert_into_helper(const std::string& table_name, 
		columns_iterator begin, 
		columns_iterator end) {

		std::string sqlfront{ "INSERT INTO " + table_name + " (" };
		std::string sqlend{ ") VALUES (" };

		std::string separator{ "" };
		for (auto field = begin; field != end; ++field) {
			sqlfront += separator + field->column_name;
			sqlend += separator + ':' + field->column_name;
			separator = ",";
		}

		sqlend += ");";
		return sqlfront + sqlend;
	}

	template <typename columns_iterator>
	std::string sqlite::update_helper(
		const std::string& table_name,
		columns_iterator begin, 
		columns_iterator end,
		const std::string& where_clause) {

		std::string sql{ "UPDATE " + table_name + " SET " };

		std::string separator{ "" };
		for (auto field = begin; field != end; ++field) {
			sql += separator + field->column_name + "=:" + field->column_name;
			separator = ",";
		}

		if (!where_clause.empty()) {
			sql += space_if_required(where_clause);
			sql += where_clause;
		}

		sql += ";";

		return sql;
	}

	template <typename where_binding_iterator>
	int sqlite::bind_where(sqlite3_stmt* stmt, where_binding_iterator begin, where_binding_iterator end) { //const std::vector<sql::where_binding>& binding) {

		int rc = SQLITE_OK;

		for (auto param = begin; param != end; ++param) {

			std::string next_param{ ':' + param->column_name };

			int idx = sqlite3_bind_parameter_index(stmt, next_param.c_str());

			switch (param->column_value.index()) {
			case 0: rc = sqlite3_bind_int(stmt, idx, std::get<0>(param->column_value)); break;
			case 1: rc = sqlite3_bind_double(stmt, idx, std::get<1>(param->column_value)); break;
			case 2:  rc = sqlite3_bind_text(stmt, idx, std::get<2>(param->column_value).c_str(), -1, SQLITE_STATIC); break;
			case 3:
				rc = sqlite3_bind_blob(stmt, idx, std::get<3>(param->column_value).data(),
					static_cast<int>(std::get<3>(param->column_value).size()), SQLITE_STATIC);
				break;
			}
		}
		return rc;
	}

	template <typename column_names_iterator, typename where_bindings_iterator>
	int sqlite::select_columns(const std::string& table_name,
		column_names_iterator name_begin,
		column_names_iterator name_end,
		const std::string& where_clause,
		where_bindings_iterator where_bindings_begin,
		where_bindings_iterator where_bindings_end,
		std::vector<std::map<std::string, sqlite_data_type>>& results) {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		const std::string sql = select_helper(table_name, name_begin, name_end, where_clause);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		EXIT_ON_ERROR(bind_where(stmt, where_bindings_begin, where_bindings_end));

		int num_cols = sqlite3_column_count(stmt);

		std::vector<std::string> column_names;
		for (int i = 0; i < num_cols; i++) {
			const char* colname = sqlite3_column_name(stmt, i);
			column_names.push_back(colname ? colname : "");
		}

		int rc = 0;
		while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
			std::map<std::string, sqlite_data_type> row;
			for (int i = 0; i < num_cols; i++)
			{
				switch (sqlite3_column_type(stmt, i))
				{
				case SQLITE3_TEXT:
				{
					const unsigned char* value = sqlite3_column_text(stmt, i);
					int len = sqlite3_column_bytes(stmt, i);
					row[column_names[i]] = std::string(value, value + len);
				}
				break;
				case SQLITE_INTEGER:
				{
					row[column_names[i]] = sqlite3_column_int(stmt, i);
				}
				break;
				case SQLITE_FLOAT:
				{
					row[column_names[i]] = sqlite3_column_double(stmt, i);
				}
				break;
				case SQLITE_BLOB:
				{
					const uint8_t* value = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(stmt, i));
					int len = sqlite3_column_bytes(stmt, i);
					row[column_names[i]] = std::vector<uint8_t>(value, value + len);
				}
				break;
				case SQLITE_NULL:
				{
					row[column_names[i]] = "null";
				}
				break;
				default:
					break;
				}
			}
			results.push_back(row);
		}

		return sqlite3_finalize(stmt);
	}

	template <typename column_names_iterator>
	const std::string sqlite::select_helper(
		const std::string& table_name,
		column_names_iterator name_begin,
		column_names_iterator name_end,
		const std::string& where_clause) {

		std::string sql{ "SELECT " };

		std::string separator{ "" };

		for (auto field = name_begin; field != name_end; ++field) {
			sql += separator + *field;
			separator = ",";
		}

		if (name_begin == name_end) {
			sql += "*";
		}

		sql += " FROM " + table_name;

		if (!where_clause.empty()) {
			sql += space_if_required(where_clause);
			sql += where_clause;
		}

		sql += ";";

		return sql;
	}


	template <typename where_bindings_iterator>
	int sqlite::delete_from(const std::string& table_name,
		const std::string& where_clause,
		where_bindings_iterator where_bindings_begin,
		where_bindings_iterator where_bindings_end) {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		const std::string sql = delete_from_helper(table_name, where_clause);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		EXIT_ON_ERROR(bind_where(stmt, where_bindings_begin, where_bindings_end));

		return step_and_finalise(stmt);
	}

	template <typename where_bindings_iterator>
	int sqlite::select_star(const std::string& table_name,
		const std::string& where_clause,
		where_bindings_iterator where_bindings_begin,
		where_bindings_iterator where_bindings_end,
		std::vector<std::map<std::string, sqlite_data_type>>& results) {
		std::string empty;
		return select_columns(table_name, empty.begin(), empty.end(), where_clause, where_bindings_begin, where_bindings_end, results);
	}
} // itel

#endif // SQLITE_HPP_

