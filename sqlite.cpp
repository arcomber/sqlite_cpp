#include "sqlite.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>


namespace {
	std::string insert_into_helper(
		const std::string& table_name,
		const std::vector<itel::column_values>& fields) {

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
}

namespace itel {

	std::ostream& operator<< (std::ostream& os, const column_values& v) {

		os << "name: " << v.column_name << ", value: ";

		switch (v.column_type.index()) {
		case 0: os << std::get<0>(v.column_type) << " of type int"; break;
		case 1: os << std::get<1>(v.column_type) << " of type double"; break;
		case 2: os << std::get<2>(v.column_type) << " of type string"; break;
		case 3:
		  {
			// printing binary files can result in a LOT of output.  Should this be some sort of option?
			std::for_each(std::get<3>(v.column_type).begin(), std::get<3>(v.column_type).end(), [&os](const uint8_t& byte) {
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
		if (db_ == nullptr) {
			return 1;
		}
		return sqlite3_close(db_);
	}

	int sqlite::insert_into(const std::string& table_name, std::vector<column_values> fields) {
		if (db_ == nullptr) {
			return SQLITE_ERROR;
		}

		const std::string sql = insert_into_helper(table_name, fields);

		sqlite3_stmt* stmt = NULL;
		int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL);

		if (rc != SQLITE_OK) {
			return rc;
		}

		// loop thru each parameter, calling bind for each one
		for (const auto& param : fields) {
			std::string next_param{ ':' + param.column_name };
			int idx = sqlite3_bind_parameter_index(stmt, next_param.c_str());

			switch (param.column_type.index()) {
			case 0: rc = sqlite3_bind_int(stmt, idx, std::get<0>(param.column_type)); break;
			case 1: rc = sqlite3_bind_double(stmt, idx, std::get<1>(param.column_type)); break;
			case 2: rc = sqlite3_bind_text(stmt, idx, std::get<2>(param.column_type).c_str(), -1, SQLITE_STATIC); break;
			case 3: 
				rc = sqlite3_bind_blob(stmt, idx, std::get<3>(param.column_type).data(), std::get<3>(param.column_type).size(), SQLITE_STATIC); 
				break;
			}
		}

		rc = sqlite3_step(stmt);
		// SQLITE_ROW = another row ready - possible to configure to return a value - but we just ignore anything returned for insert
		// SQLITE_DONE = finished executing
		if (rc == SQLITE_DONE || rc == SQLITE_ROW) {
			rc = sqlite3_finalize(stmt);
		}

		return rc;
	}

	int sqlite::last_insert_rowid() {
		return static_cast<int>(sqlite3_last_insert_rowid(db_));
	}

	const std::string sqlite::get_last_error_description() {
		if (db_ == nullptr) {
			return "";
		}
		const char* error = sqlite3_errmsg(db_);
		std::string s(error ? error : "");
		return s;
	}

} // itel
