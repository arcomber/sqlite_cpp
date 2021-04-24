#ifndef SQLITE_HPP_
#define SQLITE_HPP_

#include "sqlite3.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace itel {

	class sqlite {
	public:
		sqlite();
		~sqlite();
		int open(const std::string& filename);
		int close();
		int insert_into(const std::string table_name, std::vector<std::pair<std::string, std::string>> fields);

	private:
		sqlite3* db_;
	};

} // itel

#endif // SQLITE_HPP_
