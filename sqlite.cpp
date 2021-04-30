#include "sqlite.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#define EXIT_ON_ERROR(resultcode)    \
    if (resultcode != SQLITE_OK)     \
	    return resultcode;

namespace {
	std::string insert_into_helper(
		const std::string& table_name,
		const std::vector<sql::column_values>& fields) {

		std::string sqlfront{ "INSERT INTO " + table_name + " (" };
		std::string sqlend{ ") VALUES (" };

		std::string separator{ "" };
		for (const auto& field : fields) {
			sqlfront += separator + field.column_name;
			sqlend += separator + ':' + field.column_name;
			separator = ",";
		}

		sqlend += ");";
		return sqlfront + sqlend;
	}

	// UPDATE contacts SET col1 = value1, col2 = value2, ... WHERE rowid = therowid
	std::string update_helper(
		const std::string& table_name, 
		const std::vector<sql::column_values>& fields, 
		const std::vector<sql::column_values>& where) {

		std::string sql{ "UPDATE " + table_name + " SET " };

		std::string separator{ "" };
		for (const auto& field : fields) {
			sql += separator + field.column_name + "=:" + field.column_name;
			separator = ",";
		}

		if (!where.empty()) {
			sql += " WHERE ";
		}

		separator = "";
		for (const auto& where_condition : where) {
			sql += separator + where_condition.column_name + "=:" + where_condition.column_name;
			separator = " AND ";
		}

		sql += ";";

		return sql;
	}

	/* DELETE FROM table_name WHERE condition; */
	std::string delete_from_helper(
		const std::string& table_name,
		const std::vector<sql::column_values>& where) {
		std::string sqlfront{ "DELETE FROM " + table_name };

		if (!where.empty()) {
			sqlfront += " WHERE ";
		}

		std::string separator = "";
		for (const auto& where_condition : where) {
			sqlfront += separator + where_condition.column_name + "=:" + where_condition.column_name;
			separator = " AND ";
		}

		sqlfront += ";";

		return sqlfront;
	}

	int bind_fields(sqlite3_stmt* stmt, const std::vector<sql::column_values>& fields) {

		int rc = SQLITE_OK;

		for (const auto& param : fields) {
			std::string next_param{ ':' + param.column_name };
			int idx = sqlite3_bind_parameter_index(stmt, next_param.c_str());

			switch (param.column_value.index()) {
			case 0: rc = sqlite3_bind_int(stmt, idx, std::get<0>(param.column_value)); break;
			case 1: rc = sqlite3_bind_double(stmt, idx, std::get<1>(param.column_value)); break;
			case 2: rc = sqlite3_bind_text(stmt, idx, std::get<2>(param.column_value).c_str(), -1, SQLITE_STATIC); break;
			case 3:
				rc = sqlite3_bind_blob(stmt, idx, std::get<3>(param.column_value).data(), std::get<3>(param.column_value).size(), SQLITE_STATIC);
				break;
			}
		}
		return rc;
	}

	int bind_where(sqlite3_stmt* stmt, const std::vector<sql::column_values>& where) {

		int rc = SQLITE_OK;

		for (const auto& param : where) {
			std::string next_param{ ':' + param.column_name };
			int idx = sqlite3_bind_parameter_index(stmt, next_param.c_str());

			switch (param.column_value.index()) {
			case 0: rc = sqlite3_bind_int(stmt, idx, std::get<0>(param.column_value)); break;
			case 1: rc = sqlite3_bind_double(stmt, idx, std::get<1>(param.column_value)); break;
			case 2: rc = sqlite3_bind_text(stmt, idx, std::get<2>(param.column_value).c_str(), -1, SQLITE_STATIC); break;
			case 3:
				rc = sqlite3_bind_blob(stmt, idx, std::get<3>(param.column_value).data(), std::get<3>(param.column_value).size(), SQLITE_STATIC);
				break;
			}
		}
		return rc;
	}

	int step_and_finalise(sqlite3_stmt* stmt) {
		if (stmt == nullptr) { return SQLITE_ERROR; }

		int rc = sqlite3_step(stmt);
		// SQLITE_ROW = another row ready - possible to configure to return a value - but we just ignore anything returned for insert
		// SQLITE_DONE = finished executing
		if (rc == SQLITE_DONE || rc == SQLITE_ROW) {
			rc = sqlite3_finalize(stmt);
		}

		return rc;
	}
}

namespace sql {

	std::ostream& operator<< (std::ostream& os, const column_values& v) {

		os << "name: " << v.column_name << ", value: ";

		switch (v.column_value.index()) {
		case 0: os << std::get<0>(v.column_value) << " of type int"; break;
		case 1: os << std::get<1>(v.column_value) << " of type double"; break;
		case 2: os << std::get<2>(v.column_value) << " of type string"; break;
		case 3:
		  {
			// printing binary files can result in a LOT of output.  Should this be some sort of option?
			std::for_each(std::get<3>(v.column_value).begin(), std::get<3>(v.column_value).end(), [&os](const uint8_t& byte) {
				os << std::hex << std::setfill('0') << std::setw(2) << (byte & 0xFF) << ' ';
			});
			os << " of type vector<uint8_t>"; break;
		  }
		}

		return os;
	}

	sqlite::sqlite() : db_(nullptr) {}

	sqlite::~sqlite() {
		close();
	}

	int sqlite::open(const std::string& filename) {
		return sqlite3_open(filename.c_str(), &db_);
	}

	int sqlite::close() {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		return sqlite3_close(db_);
	}

	int sqlite::insert_into(const std::string& table_name, const std::vector<column_values>& fields) {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		const std::string sql = insert_into_helper(table_name, fields);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		// loop thru each parameter, calling bind for each one
		EXIT_ON_ERROR(bind_fields(stmt, fields));

		return step_and_finalise(stmt);
	}

	int sqlite::last_insert_rowid() {
		return static_cast<int>(sqlite3_last_insert_rowid(db_));
	}

	int sqlite::update(const std::string& table_name, const std::vector<column_values>& fields, const std::vector<column_values>& where) {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		const std::string sql = update_helper(table_name, fields, where);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		// loop thru each parameter, calling bind for each one
		EXIT_ON_ERROR(bind_fields(stmt, fields));

		// loop thru each where parameter, calling bind for each one
		EXIT_ON_ERROR(bind_where(stmt, where));

		return step_and_finalise(stmt);
	}

	int sqlite::update(const std::string& table_name, const std::vector<column_values>& fields) {
		return update(table_name, fields, {});
	}

	int sqlite::delete_from(const std::string& table_name, const std::vector<column_values>& where) {
		if (db_ == nullptr) {  return SQLITE_ERROR; }

		const std::string sql = delete_from_helper(table_name, where);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		EXIT_ON_ERROR(bind_where(stmt, where));

		return step_and_finalise(stmt);
	}

	int sqlite::delete_from(const std::string& table_name) {
		return delete_from(table_name, {});
	}

	const std::string sqlite::get_last_error_description() {
		if (db_ == nullptr) { return ""; }

		const char* error = sqlite3_errmsg(db_);
		std::string s(error ? error : "");
		return s;
	}
} // itel
