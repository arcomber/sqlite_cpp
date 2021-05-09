#include "sqlite.hpp"

#include "sqlite3.h" // required for db_initial_setup

#include <string>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <cstdio>
#include <filesystem>
#include <algorithm>

#include "gtest/gtest.h"

using namespace sql;

namespace {

	void db_initial_setup() {

		if (remove("contacts.db") != 0) {
			perror("Error deleting contacts.db");
		}

		// we create using c library so not using any of the code to exercise
		sqlite3* db;
		char* err_msg = 0;

		int rc = sqlite3_open("contacts.db", &db);

		if (rc != SQLITE_OK) {

			std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
			sqlite3_close(db);

			FAIL() << "Cannot open database for testing\n";
			return;
		}

		const char* sql[] = {
			"DROP TABLE IF EXISTS contacts;"
			"CREATE TABLE contacts (name TEXT, company TEXT, mobile TEXT, ddi TEXT, switchboard TEXT, address1 TEXT, address2 TEXT, address3 TEXT, address4 TEXT, postcode TEXT, email TEXT, url TEXT, category TEXT, notes TEXT);"
			"CREATE INDEX idx_mobile ON contacts (mobile);"
			"CREATE INDEX idx_switchboard ON contacts (switchboard);"
			"CREATE INDEX idx_ddi ON contacts (ddi);",
			"CREATE TABLE calls(timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, callerid TEXT, contactid INTEGER);",
			"INSERT INTO contacts (name, mobile, switchboard, address1, address2, address3, postcode, email, url, category) VALUES(\"Test Person\", \"07788111222\", \"02088884444\", \"House of Commons\", \"Westminster\", \"London\", \"SW1A 0AA\", \"test@house.co.uk\", \"www.house.com\", \"Supplier\");",
			"INSERT INTO calls (callerid, contactid) VALUES(\"07788111222\", 1);"
		};

		size_t num_commands = sizeof(sql) / sizeof(char*);

		for (size_t i = 0; i < num_commands; ++i) {
			rc = sqlite3_exec(db, sql[i], 0, 0, &err_msg);

			if (rc != SQLITE_OK) {

				std::cerr << "SQL error: " << err_msg << std::endl;

				sqlite3_free(err_msg);
				sqlite3_close(db);
			}
		}
		sqlite3_close(db);
	}

	const std::string filename("contacts.db");
	std::vector<std::string> tables{ "contacts", "calls" };
}

class sqlite_cpp_tester : public ::testing::Test {
public:
	void SetUp() {
		db_initial_setup();
	}
};


TEST_F(sqlite_cpp_tester, given_a_valid_db_file_open_close_return_success) {

	sql::sqlite db;
	EXPECT_EQ(db.open("contacts.db"), SQLITE_OK);
	EXPECT_EQ(db.close(), SQLITE_OK);
}

TEST_F(sqlite_cpp_tester, given_a_valid_insert_select_returns_same_as_inserted) {

	sql::sqlite db;
	EXPECT_EQ(db.open("contacts.db"), SQLITE_OK);

	const std::vector<sql::column_values> fields {
	{"name", "Mickey Mouse"},
	{"company", "Disney"},
	{"mobile", "07755123456"},
	{"ddi", "01222333333"},
	{"switchboard", "01222444444"},
	{"address1", "1 The Avenue"},
	{"address2", "Greystoke"},
	{"address3", "Lower Wirmwood"},
	{"address4", "Baffleshire"},
	{"postcode", "PO21 4RR"},
	{"email", "mickey@disney.com"},
	{"url", "disney.com"},
	{"category", "cartoonist"},
	{"notes", "delightful mouse"}
	};

	EXPECT_EQ(db.insert_into("contacts", fields.begin(), fields.end()), SQLITE_OK);

	int lastrowid = db.last_insert_rowid();

	const std::vector<sql::where_binding>& bindings {
	  {"rowid", lastrowid}
	};
	std::vector<std::map<std::string, sql::sqlite_data_type>> results;
	
	EXPECT_EQ(db.select_star("contacts", "WHERE rowid=:rowid", bindings.begin(), bindings.end(), results), SQLITE_OK);

	EXPECT_EQ(results.size(), 1u);

	EXPECT_EQ(results[0]["name"], fields[0].column_value);
	EXPECT_EQ(results[0]["company"], fields[1].column_value);
	EXPECT_EQ(results[0]["mobile"], fields[2].column_value);
	EXPECT_EQ(results[0]["ddi"], fields[3].column_value);
	EXPECT_EQ(results[0]["switchboard"], fields[4].column_value);
	EXPECT_EQ(results[0]["address1"], fields[5].column_value);
	EXPECT_EQ(results[0]["address2"], fields[6].column_value);
	EXPECT_EQ(results[0]["address3"], fields[7].column_value);
	EXPECT_EQ(results[0]["address4"], fields[8].column_value);
	EXPECT_EQ(results[0]["postcode"], fields[9].column_value);
	EXPECT_EQ(results[0]["email"], fields[10].column_value);
	EXPECT_EQ(results[0]["url"], fields[11].column_value);
	EXPECT_EQ(results[0]["category"], fields[12].column_value);
	EXPECT_EQ(results[0]["notes"], fields[13].column_value);
}


TEST_F(sqlite_cpp_tester, given_a_valid_insert_then_update_select_returns_same_as_updated) {
	sql::sqlite db;
	EXPECT_EQ(db.open("contacts.db"), SQLITE_OK);

	const std::vector<sql::column_values> fields{
	{"name", "Mickey Mouse"},
	{"company", "Disney"},
	{"mobile", "07755123456"},
	{"ddi", "01222333333"},
	{"switchboard", "01222444444"},
	{"address1", "1 The Avenue"},
	{"address2", "Greystoke"},
	{"address3", "Lower Wirmwood"},
	{"address4", "Baffleshire"},
	{"postcode", "PO21 4RR"},
	{"email", "mickey@disney.com"},
	{"url", "disney.com"},
	{"category", "cartoonist"},
	{"notes", "delightful mouse"}
	};

	EXPECT_EQ(db.insert_into("contacts", fields.begin(), fields.end()), SQLITE_OK);

	int lastrowid = db.last_insert_rowid();

	// UPDATE
	const std::vector<sql::column_values> updated_fields{
	{"name", "Donald Duck"},
	{"company", "Disney"},
	{"mobile", "07755654321"},
	{"ddi", "01222444444"},
	{"switchboard", "01222555555"},
	{"address1", "2 The Avenue"},
	{"address2", "Greystoke"},
	{"address3", "Lower Wirmwood"},
	{"address4", "Baffleshire"},
	{"postcode", "PO21 4RR"},
	{"email", "donald@disney.com"},
	{"url", "disney.com"},
	{"category", "cartoonist"},
	{"notes", "quackers"}
	};

	const std::vector<where_binding>& update_bindings{
	  {"rowid", lastrowid}
	};

	const std::string where_clause{ "WHERE rowid=:rowid" };

	EXPECT_EQ(db.update("contacts", updated_fields.begin(), updated_fields.end(), 
		where_clause, update_bindings.begin(), update_bindings.end()), SQLITE_OK);

	std::vector<std::map<std::string, sql::sqlite_data_type>> results;

	std::vector<std::string> cols { "rowid", "*" };

	EXPECT_EQ(db.select_columns("contacts", cols.begin(), cols.end(), "WHERE rowid=:rowid", update_bindings.begin(), update_bindings.end(), results), SQLITE_OK);

	EXPECT_EQ(results.size(), 1u);

	EXPECT_EQ(results[0]["name"], updated_fields[0].column_value);
	EXPECT_EQ(results[0]["company"], updated_fields[1].column_value);
	EXPECT_EQ(results[0]["mobile"], updated_fields[2].column_value);
	EXPECT_EQ(results[0]["ddi"], updated_fields[3].column_value);
	EXPECT_EQ(results[0]["switchboard"], updated_fields[4].column_value);
	EXPECT_EQ(results[0]["address1"], updated_fields[5].column_value);
	EXPECT_EQ(results[0]["address2"], updated_fields[6].column_value);
	EXPECT_EQ(results[0]["address3"], updated_fields[7].column_value);
	EXPECT_EQ(results[0]["address4"], updated_fields[8].column_value);
	EXPECT_EQ(results[0]["postcode"], updated_fields[9].column_value);
	EXPECT_EQ(results[0]["email"], updated_fields[10].column_value);
	EXPECT_EQ(results[0]["url"], updated_fields[11].column_value);
	EXPECT_EQ(results[0]["category"], updated_fields[12].column_value);
	EXPECT_EQ(results[0]["notes"], updated_fields[13].column_value);
}

TEST_F(sqlite_cpp_tester, given_a_single_quote_in_notes_field_select_returns_same_value_inserted) {
	sql::sqlite db;
	EXPECT_EQ(db.open("contacts.db"), SQLITE_OK);

	const std::vector<sql::column_values> fields{
	{"name", "Sean O'Hennessey"},
	{"company", "Disney"},
	{"mobile", "07755123456"},
	{"ddi", "01222333333"},
	{"switchboard", "01222444444"},
	{"address1", "1 The Avenue"},
	{"address2", "Greystoke"},
	{"address3", "Lower Wirmwood"},
	{"address4", "Baffleshire"},
	{"postcode", "PO21 4RR"},
	{"email", "mickey@disney.com"},
	{"url", "disney.com"},
	{"category", "cartoonist"},
	{"notes", "single quote symbol is '"}
	};

	EXPECT_EQ(db.insert_into("contacts", fields.begin(), fields.end()), SQLITE_OK);

	int lastrowid = db.last_insert_rowid();
	const std::vector<where_binding>& update_bindings{
       {"rowid", lastrowid}
	};

	std::vector<std::string> cols{ "rowid", "*" };
	const std::string where_clause{ "WHERE rowid=:rowid" };

	std::vector<std::map<std::string, sql::sqlite_data_type>> results;

	EXPECT_EQ(db.select_columns("contacts", cols.begin(), cols.end(), 
		"WHERE rowid=:rowid", update_bindings.begin(), update_bindings.end(), results), SQLITE_OK);

	EXPECT_EQ(results.size(), 1u);

	EXPECT_EQ(results[0]["name"], fields[0].column_value);
	EXPECT_EQ(results[0]["company"], fields[1].column_value);
	EXPECT_EQ(results[0]["mobile"], fields[2].column_value);
	EXPECT_EQ(results[0]["ddi"], fields[3].column_value);
	EXPECT_EQ(results[0]["switchboard"], fields[4].column_value);
	EXPECT_EQ(results[0]["address1"], fields[5].column_value);
	EXPECT_EQ(results[0]["address2"], fields[6].column_value);
	EXPECT_EQ(results[0]["address3"], fields[7].column_value);
	EXPECT_EQ(results[0]["address4"], fields[8].column_value);
	EXPECT_EQ(results[0]["postcode"], fields[9].column_value);
	EXPECT_EQ(results[0]["email"], fields[10].column_value);
	EXPECT_EQ(results[0]["url"], fields[11].column_value);
	EXPECT_EQ(results[0]["category"], fields[12].column_value);
	EXPECT_EQ(results[0]["notes"], fields[13].column_value);
}


TEST_F(sqlite_cpp_tester, given_non_alphanumeric_characters_inserted_select_returns_same_value_inserted) {
	sql::sqlite db;
	EXPECT_EQ(db.open("contacts.db"), SQLITE_OK);

	const std::vector<sql::column_values> fields{
	{"name", "<---------------------->'"},
	{"company", "D\nisne	y"},
	{"mobile", "!!!\"0775512345'''6"},
	{"ddi", "{}===================="},
	{"switchboard", "++++++++++++++++++++++++"},
	{"address1", "&&&&&&&&&&&&&&&&&&&&&&&&&"},
	{"address2", "``````````¬|"},
	{"address3", ";'#:@~"},
	{"address4", "'''''''''''''''''''"},
	{"postcode", "!\"£$%^&*()_+"},
	{"email", "***************************"},
	{"url", "disney.com"},
	{"category", "cartoonist"},
	{"notes", "1\n2\n3\n4\n5\n"}
	};

	EXPECT_EQ(db.insert_into("contacts", fields.begin(), fields.end()), SQLITE_OK);

	int lastrowid = db.last_insert_rowid();
	const std::vector<where_binding>& update_bindings{
	   {"rowid", lastrowid}
	};

	std::vector<std::string> cols{ "rowid", "*" };
	const std::string where_clause{ "WHERE rowid=:rowid" };

	std::vector<std::map<std::string, sql::sqlite_data_type>> results;

	EXPECT_EQ(db.select_columns("contacts", cols.begin(), cols.end(), 
		"WHERE rowid=:rowid", update_bindings.begin(), update_bindings.end(), results), SQLITE_OK);

	EXPECT_EQ(results.size(), 1u);

	EXPECT_EQ(results[0]["name"], fields[0].column_value);
	EXPECT_EQ(results[0]["company"], fields[1].column_value);
	EXPECT_EQ(results[0]["mobile"], fields[2].column_value);
	EXPECT_EQ(results[0]["ddi"], fields[3].column_value);
	EXPECT_EQ(results[0]["switchboard"], fields[4].column_value);
	EXPECT_EQ(results[0]["address1"], fields[5].column_value);
	EXPECT_EQ(results[0]["address2"], fields[6].column_value);
	EXPECT_EQ(results[0]["address3"], fields[7].column_value);
	EXPECT_EQ(results[0]["address4"], fields[8].column_value);
	EXPECT_EQ(results[0]["postcode"], fields[9].column_value);
	EXPECT_EQ(results[0]["email"], fields[10].column_value);
	EXPECT_EQ(results[0]["url"], fields[11].column_value);
	EXPECT_EQ(results[0]["category"], fields[12].column_value);
	EXPECT_EQ(results[0]["notes"], fields[13].column_value);
}

TEST_F(sqlite_cpp_tester, add_integer_value_select_returns_same_value_inserted) {
	sql::sqlite db;
	EXPECT_EQ(db.open("contacts.db"), SQLITE_OK);

	const std::vector<sql::column_values> fields{
	{"callerid", "0775512345"},
	{"contactid", 2}
	};

	EXPECT_EQ(db.insert_into("calls", fields.begin(), fields.end()), SQLITE_OK);

	const std::vector<where_binding> bindings{
	   {"contactid", 2}
	};

	const char* result_cols[] { "timestamp", "callerid", "contactid" };
	size_t cols_len = sizeof(result_cols) / sizeof(result_cols[0]);
	const std::string where_clause{ "WHERE rowid=:rowid" };

	std::vector<std::map<std::string, sql::sqlite_data_type>> results;

	EXPECT_EQ(db.select_columns("calls", result_cols, result_cols+cols_len, 
		"WHERE contactid=:contactid", bindings.begin(), bindings.end(), results), SQLITE_OK);

	EXPECT_EQ(results.size(), 1u);

	EXPECT_EQ(results[0]["callerid"], fields[0].column_value);
	EXPECT_EQ(results[0]["contactid"], fields[1].column_value);
	// get 3 columns back
	EXPECT_EQ(results[0].size(), 3u);
}

// SELECT (using LIKE)
TEST_F(sqlite_cpp_tester, add_integer_value_select_like_returns_same_value_inserted) {
	sql::sqlite db;
	EXPECT_EQ(db.open("contacts.db"), SQLITE_OK);

	const sql::column_values fields[] {
	{"callerid", "0775512345"},
	{"contactid", 2}
	};

	size_t len = sizeof(fields) / sizeof(fields[0]);

	EXPECT_EQ(db.insert_into("calls", fields, fields + len), SQLITE_OK);

	const std::vector<where_binding> bindings{
	   {"callerid", "077%"}
	};

	std::vector<std::string> cols { "timestamp", "callerid", "contactid" };
	const std::string where_clause{ "WHERE callerid LIKE :callerid" };

	std::vector<std::map<std::string, sql::sqlite_data_type>> results;

	EXPECT_EQ(db.select_columns("calls", cols.begin(), cols.end(), 
		where_clause, bindings.begin(), bindings.end(), results), SQLITE_OK);

	EXPECT_EQ(results.size(), 2u);

	EXPECT_EQ(std::get<std::string>(results[0]["callerid"]), "07788111222");
	EXPECT_EQ(std::get<int>(results[0]["contactid"]), 1);

	EXPECT_EQ(results[1]["callerid"], fields[0].column_value);
	EXPECT_EQ(results[1]["contactid"], fields[1].column_value);
	// get 3 columns back
	EXPECT_EQ(results[0].size(), 3u);
}

// GETCALLS
TEST_F(sqlite_cpp_tester, join_returning_data_from_two_tables_returns_correct_data) {
	sql::sqlite db;
	EXPECT_EQ(db.open("contacts.db"), SQLITE_OK);

	const std::vector<std::string> fields { "calls.timestamp", "contacts.name", "calls.callerid", "contacts.url" };

	const std::string where_clause{ "LEFT JOIN contacts ON calls.contactid = contacts.rowid" };

	std::vector<std::map<std::string, sql::sqlite_data_type>> results;

	const std::vector<where_binding> bindings {};  // none required

	EXPECT_EQ(db.select_columns("calls", fields.begin(), fields.end(), 
		where_clause, bindings.begin(), bindings.end(), results), SQLITE_OK);

	EXPECT_EQ(results.size(), 1u);
	EXPECT_EQ(std::get<2>(results[0]["callerid"]), "07788111222");
	EXPECT_EQ(std::get<2>(results[0]["name"]), "Test Person");
	EXPECT_EQ(std::get<2>(results[0]["url"]), "www.house.com");
	EXPECT_NE(std::get<2>(results[0]["timestamp"]), "");
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
