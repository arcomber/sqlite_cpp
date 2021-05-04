#include "sqlite.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#define EXIT_ON_ERROR(resultcode) \
if (resultcode != SQLITE_OK) \
{ \
    sqlite3_finalize(stmt); \
    return resultcode; \
}

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

	std::string space_if_required(const std::string& s) {
		return !s.empty() && s[0] != ' ' ? " " : "";
	}

	std::string update_helper(
		const std::string& table_name,
		const std::vector<sql::column_values>& fields,
		const std::string& where_clause) {

		std::string sql{ "UPDATE " + table_name + " SET " };

		std::string separator{ "" };
		for (const auto& field : fields) {
			sql += separator + field.column_name + "=:" + field.column_name;
			separator = ",";
		}

		if (!where_clause.empty()) {
			sql += space_if_required(where_clause);
			sql += where_clause;
		}

		sql += ";";

		return sql;
	}

	std::string delete_from_helper(
		const std::string& table_name,
		const std::string& where_clause) {
		std::string sql{ "DELETE FROM " + table_name };

		if (!where_clause.empty()) {
			sql += space_if_required(where_clause);
			sql += where_clause;
		}

		sql += ";";

		return sql;
	}

	const std::string select_helper(
		const std::string& table_name,
		const std::vector<std::string>& fields,
		const std::string& where_clause) {

		std::string sql{ "SELECT " };

		std::string separator{ "" };
		for (const auto& field : fields) {
			sql += separator + field;
			separator = ",";
		}

		if (fields.empty()) {
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

	int bind_where(sqlite3_stmt* stmt, const std::vector<sql::where_binding>& binding) {

		int rc = SQLITE_OK;

		for (const auto& param : binding) {
			std::string next_param{ ':' + param.column_name };

			int idx = sqlite3_bind_parameter_index(stmt, next_param.c_str());

			switch (param.column_value.index()) {
			case 0: rc = sqlite3_bind_int(stmt, idx, std::get<0>(param.column_value)); break;
			case 1: rc = sqlite3_bind_double(stmt, idx, std::get<1>(param.column_value)); break;
			case 2:  rc = sqlite3_bind_text(stmt, idx, std::get<2>(param.column_value).c_str(), -1, SQLITE_STATIC); break;
			case 3:
				rc = sqlite3_bind_blob(stmt, idx, std::get<3>(param.column_value).data(), std::get<3>(param.column_value).size(), SQLITE_STATIC);
				break;
			}
		}
		return rc;
	}

	int step_and_finalise(sqlite3_stmt* stmt) {
		if (stmt == nullptr) { return SQLITE_ERROR; }

		// whether error or not we must call finalize
		int rc = sqlite3_step(stmt);
		// SQLITE_ROW = another row ready - possible to configure to return a value - but we just ignore anything returned
		// SQLITE_DONE = finished executing

		// caller is more interested in the result of the step
		int finalise_rc = sqlite3_finalize(stmt);  // de-allocates stmt
		return rc == SQLITE_DONE ? finalise_rc : rc;
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
#ifdef PRINT_BLOB_AS_HEX
			auto previous_flags = os.flags();
			std::for_each(std::get<3>(v.column_value).begin(), std::get<3>(v.column_value).end(), [&os](const uint8_t& byte) {
				os << std::hex << std::setfill('0') << std::setw(2) << (byte & 0xFF) << ' ';
				});
			os << " of type vector<uint8_t>";
			os.setf(previous_flags);
#else
			os << "<blob> of type vector<uint8_t>";
#endif
			break;
		}
		}

		return os;
	}

	std::ostream& operator<< (std::ostream& os, const sqlite_data_type& v) {
		switch (v.index()) {
		case 0: os << std::get<0>(v); break;
		case 1: os << std::get<1>(v); break;
		case 2: os << std::get<2>(v); break;
		case 3:
		{
#ifdef PRINT_BLOB_AS_HEX
			auto previous_flags = os.flags();
			std::for_each(std::get<3>(v.column_value).begin(), std::get<3>(v.column_value).end(), [&os](const uint8_t& byte) {
				os << std::hex << std::setfill('0') << std::setw(2) << (byte & 0xFF) << ' ';
				});
			os.setf(previous_flags);
#else
			os << "<blob>";
#endif
			break;
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

		EXIT_ON_ERROR(bind_fields(stmt, fields));

		return step_and_finalise(stmt);
	}

	int sqlite::last_insert_rowid() {
		return static_cast<int>(sqlite3_last_insert_rowid(db_));
	}

	int sqlite::update(const std::string& table_name, const std::vector<column_values>& fields, 
		const std::string& where_clause, const std::vector<where_binding>& bindings) {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		const std::string sql = update_helper(table_name, fields, where_clause);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		EXIT_ON_ERROR(bind_fields(stmt, fields));

		EXIT_ON_ERROR(bind_where(stmt, bindings));

		return step_and_finalise(stmt);
	}

	int sqlite::update(const std::string& table_name, const std::vector<column_values>& fields) {
		return update(table_name, fields, "", {});
	}

	int sqlite::delete_from(const std::string& table_name, const std::string& where_clause, const std::vector<where_binding>& bindings) {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		const std::string sql = delete_from_helper(table_name, where_clause);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		EXIT_ON_ERROR(bind_where(stmt, bindings));

		return step_and_finalise(stmt);
	}

	int sqlite::delete_from(const std::string& table_name) {
		return delete_from(table_name, "", {});
	}

	int sqlite::select_columns(const std::string& table_name,
		const std::vector<std::string>& fields,
		const std::string& where_clause,
		const std::vector<where_binding>& bindings,
		std::vector<std::vector<sql::column_values>>& results) {
		if (db_ == nullptr) { return SQLITE_ERROR; }

		const std::string sql = select_helper(table_name, fields, where_clause);

		sqlite3_stmt* stmt = NULL;
		EXIT_ON_ERROR(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL));

		EXIT_ON_ERROR(bind_where(stmt, bindings));

		int num_cols = sqlite3_column_count(stmt);

		std::vector<std::string> column_names;
		for (int i = 0; i < num_cols; i++) {
			const char* colname = sqlite3_column_name(stmt, i);
			column_names.push_back(colname ? colname : "");
		}

		int rc = 0;
		while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
		std::vector<sql::column_values> row_values;
		for (int i = 0; i < num_cols; i++)
		{
			switch (sqlite3_column_type(stmt, i))
			{
			case SQLITE3_TEXT:
			{
				const unsigned char* value = sqlite3_column_text(stmt, i);
				int len = sqlite3_column_bytes(stmt, i);
				// value must be copied because after call to finalize, memory will be invalidated
				std::string s(value, value + len);
				column_values cv;
				cv.column_name = column_names[i];
				cv.column_value = s;
				row_values.push_back(cv);
			}
			break;
			case SQLITE_INTEGER:
			{
				int value = sqlite3_column_int(stmt, i);
				column_values cv;
				cv.column_name = column_names[i];
				cv.column_value = value;
				row_values.push_back(cv);
			}
			break;
			case SQLITE_FLOAT:
			{
				double value = sqlite3_column_double(stmt, i);
				column_values cv;
				cv.column_name = column_names[i];
				cv.column_value = value;
				row_values.push_back(cv);
			}
			break;
			case SQLITE_BLOB:
			{
				const uint8_t* value = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(stmt, i));
				int len = sqlite3_column_bytes(stmt, i);
				column_values cv;
				cv.column_name = column_names[i];
				// value must be copied because after call to finalize, memory will be invalidated
				cv.column_value = std::vector<uint8_t>(value, value + len);
				row_values.push_back(cv);
			}
			break;
			case SQLITE_NULL:
			{
				column_values cv;
				cv.column_name = column_names[i];
				cv.column_value = "null";
				row_values.push_back(cv);
			}
			break;
			default:
				break;
			}
		}
		results.push_back(row_values);
		}

		return sqlite3_finalize(stmt);
	}

	int sqlite::select_star(const std::string& table_name,
		const std::string& where_clause,
		const std::vector<where_binding>& bindings,
		std::vector<std::vector<sql::column_values>>& results) {
		return select_columns(table_name, {}, where_clause, bindings, results);
	}

	int sqlite::select_star(const std::string& table_name,
		std::vector<std::vector<sql::column_values>>& results) {
		return select_star(table_name, "", {}, results);
	}

	const std::string sqlite::get_last_error_description() {
		if (db_ == nullptr) { return ""; }

		const char* error = sqlite3_errmsg(db_);
		std::string s(error ? error : "");
		return s;
	}
} // sql
