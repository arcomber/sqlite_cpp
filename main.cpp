/*
	This example assumes you have created a database as follows:
	sqlite3.exe mydb.db

	CREATE TABLE test (name TEXT, age INTEGER, photo BLOB);
*/
#include "sqlite.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace sql;

int main()
{
	sql::sqlite db;
	int rc = db.open("mydb.db");

	std::cout << "db.open returned: " << rc << std::endl;

	// picture from https://en.wikipedia.org/wiki/Mickey_Mouse
	std::ifstream f("Mickey_Mouse.png", std::ios::binary);

	if (!f.good()) {
		std::cout << "failed to open Mickey Mouse bitmap file\n";
		return 1;
	}

	std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});

	std::vector<sql::column_values> params {
		{"name", "Mickey Mouse"},
		{"age", 12},
		{"photo", buffer}
	};

	for (const auto& param : params) {
		std::cout << "inserting param: " << param << std::endl;
	}

	rc = db.insert_into("test", params.begin(), params.end());
	std::cout << "db.insert_into(...) returned: " << rc << std::endl;

	int lastrowid = 0;

	if (rc == SQLITE_OK) {
		lastrowid = db.last_insert_rowid();
		std::cout << "inserted into rowid: " << lastrowid << std::endl;
	}

	// let us now update this record
	std::vector<sql::column_values> updated_params{
	{"name", "Donald Duck"},
	{"age", 23}
	};

	const std::vector<where_binding>& bindings{
		{"rowid", lastrowid}
	};

	rc = db.update("test", updated_params.begin(), updated_params.end(), "WHERE rowid=:rowid", bindings.begin(), bindings.end());
	std::cout << "db.update(...) returned: " << rc << std::endl;

	// try SELECT
	std::vector<std::map<std::string, sql::sqlite_data_type>> results;

	// simplest way
	//rc = db.select_star("test", results);

	// using select_column to specifically display sqlite table rowid
	//rc = db.select_columns("test", { "rowid", "name", "age", "photo" }, {}, results);

	// Or pass in rowid and * to display rowid and all other columns
	//rc = db.select_columns("test", { "rowid", "*" }, {}, results);

	const std::vector<where_binding>& select_bindings{
	   {"name", "Don%"}
	};

	std::vector<std::string> cols{ "rowid", "*" };
	rc = db.select_columns("test", cols.begin(), cols.end(), "WHERE name LIKE :name", select_bindings.begin(), select_bindings.end(), results);

	std::cout << "db.select_columns(...) returned: " << rc << std::endl;

	// print rows
	int i = 0;
	for (const auto& row : results) {
		std::cout << "row" << ++i << ": " << row << std::endl;
	}

	// finally delete row added
    const std::vector<where_binding>& delete_bindings {
        {"rowid", lastrowid}
    };

	rc = db.delete_from("test", "WHERE rowid=:rowid", delete_bindings.begin(), delete_bindings.end());
	std::cout << "db.delete_from(...) returned: " << rc << std::endl;

	// code below inserts into data into a table that does not exist

	// test to insert into an invalid column
	std::vector<sql::column_values> bad_params{
		{"nave", "Tanner"},
		{"address8", "3 The Avenue"},
		{"postcoode", "GU17 0TR"}
	};

	rc = db.insert_into("contacts", bad_params.begin(), bad_params.end());
	std::cout << "db.insert_into(...) returned: " << rc << std::endl;

	if (rc != SQLITE_OK) {
		std::cout << db.get_last_error_description() << std::endl;
	}
}
