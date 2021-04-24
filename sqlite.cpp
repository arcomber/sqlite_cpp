/*

this is how is done in this lib:

sqlite3pp::command cmd(
  db, "INSERT INTO contacts (name, phone) VALUES (:user, :phone)");
cmd.bind(":user", "Mike", sqlite3pp::nocopy);
cmd.bind(":phone", "555-1234", sqlite3pp::nocopy);
cmd.execute();

...
https://github.com/iwongu/sqlite3pp


*/

#include "sqlite.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>


namespace {
	std::string insert_into_helper(
		const std::string table_name,
		const std::vector<std::pair<std::string, std::string>>& fields) {

		std::string sqlfront{ "INSERT INTO " + table_name + " (" };
		std::string sqlend{ ") VALUES (" };

		std::string separator{ "" };
		for (const auto& field : fields) {
			sqlfront += separator + field.first;
			sqlend += separator + ':' + field.first;
			separator = ",";
		}

		sqlend += ");";
		return sqlfront + sqlend;
	}
}

namespace itel {

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

	int sqlite::insert_into(const std::string table_name, std::vector<std::pair<std::string, std::string>> fields) {
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
		for (const auto& params : fields) {
			std::cout << "inserting params: " << params.first << "->" << params.second << std::endl;
			std::string next_param{ ':' + params.first };
			int idx = sqlite3_bind_parameter_index(stmt, next_param.c_str());
			rc = sqlite3_bind_text(stmt, idx, params.second.c_str(), -1, SQLITE_STATIC);
		}

		rc = sqlite3_step(stmt);
		// SQLITE_ROW = another row ready - possible to configure to return a value - but we just ignore anything returned for insert
		// SQLITE_DONE = finished executing
		if (rc == SQLITE_DONE || rc == SQLITE_ROW) {
			rc = sqlite3_finalize(stmt);
		}

		return rc;
	}

	//int sqlite::perform_delete_rows(const std::string& sql, std::string& result) {

	//	if (db_ == nullptr) {
	//		result = "null db ptr";
	//		return SQLITE_ERROR;
	//	}

	//	char* err_msg = 0;
	//	int rc = sqlite3_exec(db_, sql.c_str(), 0, 0, &err_msg);

	//	if (rc != SQLITE_OK) {
	//		if (err_msg) {
	//			result = std::string(err_msg);
	//		}
	//		return 0;
	//	}

	//	return sqlite3_changes(db_);
	//}

} // itel
