#include "sqlite.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

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

#ifdef PRINT_BLOB_AS_HEX
	std::ostream& operator<<(std::ostream & os, const std::vector<uint8_t>&v)
	{
	    auto previous_flags = os.flags();
		std::for_each(v.begin(), v.end(), [&os](const uint8_t& byte) {
			os << std::hex << std::setfill('0') << std::setw(2) << (byte & 0xFF) << ' ';
			});
		os.setf(previous_flags);
#else
	std::ostream& operator<<(std::ostream & os, const std::vector<uint8_t>& /* v */)
	{
	    os << "<blob>";
#endif
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const sqlite_data_type& v)
	{
		std::visit([&](const auto& element) {
				os << element;
			}, v);

		return os;
	}

	std::ostream& operator<< (std::ostream& os, const std::map<std::string, sqlite_data_type>& v) {

		for (const auto& element : v) {
			os << element.first << ": " << element.second << '|';
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

		int rc = sqlite3_close(db_);
		db_ = nullptr;
		return rc;
	}

	int sqlite::last_insert_rowid() {
		return static_cast<int>(sqlite3_last_insert_rowid(db_));
	}

	int sqlite::delete_from(const std::string& table_name) {
		std::vector<where_binding> empty;
		return delete_from(table_name, "", empty.begin(), empty.end());
	}

	int sqlite::select_star(const std::string& table_name,
		std::vector<std::map<std::string, sqlite_data_type>>& results) {
		std::vector<where_binding> empty;
		return select_star(table_name, "", empty.begin(), empty.end(), results);
	}

	const std::string sqlite::get_last_error_description() {
		if (db_ == nullptr) { return ""; }

		const char* error = sqlite3_errmsg(db_);
		std::string s(error ? error : "");
		return s;
	}

	int sqlite::step_and_finalise(sqlite3_stmt* stmt) {
		if (stmt == nullptr) { return SQLITE_ERROR; }

		// whether error or not we must call finalize
		int rc = sqlite3_step(stmt);
		// SQLITE_ROW = another row ready - possible to configure to return a value - but we just ignore anything returned
		// SQLITE_DONE = finished executing

		// caller is more interested in the result of the step
		int finalise_rc = sqlite3_finalize(stmt);  // de-allocates stmt
		return rc == SQLITE_DONE ? finalise_rc : rc;
	}

	std::string sqlite::space_if_required(const std::string& s) {
		return !s.empty() && s[0] != ' ' ? " " : "";
	}

	std::string sqlite::delete_from_helper(
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

} // sql
