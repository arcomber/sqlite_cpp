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

int main()
{
	system("pwd");

	itel::sqlite db;
	int rc = db.open("mydb.db");
	std::cout << "db.open returned: " << rc << std::endl;

	// picture from https://en.wikipedia.org/wiki/Mickey_Mouse
	std::ifstream f("Mickey_Mouse.png", std::ios::binary);

	if (!f.good()) {
		std::cout << "failed to open Mickey Mouse bitmap file\n";
		return 1;
	}

	std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});

	std::vector<itel::column_values> params {
		{"name", "Mickey Mouse"}, 
		{"age", 12},
		{"photo", buffer}
	};

	for (const auto& param : params) {
	    std::cout << "inserting param: " << param << std::endl;
	}

	rc = db.insert_into("test", params);
	std::cout << "db.insert_into(...) returned: " << rc << std::endl;

	if (rc == SQLITE_OK) {
		std::cout << "inserted into rowid: " << db.last_insert_rowid() << std::endl;
	}

	// test to insert into an invalid column
	std::vector<itel::column_values> bad_params {
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
