/*
This example assumes that a database named contacts.db has already been created with the following structure:
table: contacts
created by:
	"DROP TABLE IF EXISTS contacts;"
	"CREATE TABLE contacts (name TEXT, company TEXT, mobile TEXT, ddi TEXT, switchboard TEXT, address1 TEXT, address2 TEXT, address3 TEXT, address4 TEXT, postcode TEXT, email TEXT, url TEXT, category TEXT, notes TEXT);"
	"CREATE INDEX idx_mobile ON contacts (mobile);"
	"CREATE INDEX idx_switchboard ON contacts (switchboard);"
	"CREATE INDEX idx_ddi ON contacts (ddi);",
*/

#include "sqlite.hpp"

#include <iostream>
#include <string>
#include <vector>

int main()
{
	system("pwd");

	itel::sqlite db;
	int rc = db.open("contacts.db");
	std::cout << "db.open returned: " << rc << std::endl;

	std::vector<std::pair<std::string, std::string>> params { 
		{"name", "Banner"}, 
		{"address1", "3 The Avenue"},
		{"postcode", "GU17 0TR"}
	};

	rc = db.insert_into("contacts", params);
	std::cout << "db.insert_into(...) returned: " << rc << std::endl;

	// test to insert into an invalid colum
	std::vector<std::pair<std::string, std::string>> bad_params {
	    {"nave", "Tanner"},
	    {"address8", "3 The Avenue"},
	    {"postcoode", "GU17 0TR"}
	};

	rc = db.insert_into("contacts", bad_params);
	std::cout << "db.insert_into(...) returned: " << rc << std::endl;

	if (rc != SQLITE_OK) {
		// how do we provide more detailed information on error
	}
}
