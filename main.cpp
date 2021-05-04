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

	rc = db.insert_into("test", params);
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

	rc = db.update("test", updated_params, "WHERE rowid=:rowid", bindings);
	std::cout << "db.update(...) returned: " << rc << std::endl;

	// try SELECT
	std::vector<std::vector<sql::column_values>> results;

	// simplest way
	//rc = db.select_star("test", results);

	// using select_column to specifically display sqlite table rowid
	//rc = db.select_columns("test", { "rowid", "name", "age", "photo" }, {}, results);

	// Or pass in rowid and * to display rowid and all other columns
	//rc = db.select_columns("test", { "rowid", "*" }, {}, results);

	const std::vector<where_binding>& select_bindings{
	   {"name", "Don%"}
	};

	rc = db.select_columns("test", { "rowid", "*" }, "WHERE name LIKE :name", select_bindings, results);

	std::cout << "db.select_columns(...) returned: " << rc << std::endl;

	// print header of table - column names
	std::cout << "No. rows returned: " << results.size() << std::endl;
	if (!results.empty()) {
		std::string separator;
		for (size_t col = 0; col < results[0].size(); col++) {
			std::cout << separator << std::setw(14) << results[0][col].column_name;
			separator = ", ";
		}
		std::cout << "\n";

		// print values
		for (size_t row = 0; row < results.size(); row++) {
			separator = "";
			for (size_t col = 0; col < results[0].size(); col++) {
				std::cout << separator << std::setw(14) << results[row][col].column_value;
				separator = ", ";
			}
			std::cout << "\n";
		}
	}

	// finally delete row added
    const std::vector<where_binding>& delete_bindings {
        {"rowid", lastrowid}
    };

	rc = db.delete_from("test", "WHERE rowid=:rowid", delete_bindings);
	std::cout << "db.delete_from(...) returned: " << rc << std::endl;

	// code below inserts into data into a table that does not exist

	// test to insert into an invalid column
	std::vector<sql::column_values> bad_params{
		{"nave", "Tanner"},
		{"address8", "3 The Avenue"},
		{"postcoode", "GU17 0TR"}
	};

	rc = db.insert_into("contacts", bad_params);
	std::cout << "db.insert_into(...) returned: " << rc << std::endl;

	if (rc != SQLITE_OK) {
		std::cout << db.get_last_error_description() << std::endl;
	}
}
