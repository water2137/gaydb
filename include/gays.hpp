#ifndef GAYS_HPP
#define GAYS_HPP
#include <vector>
#include <string>

#include "sqliteBackend.hpp"

class gaysClass {
private:
	static std::vector<sqlite3Backend::User> List;
public:
	gaysClass();
	void registerGay(const std::string& name);
	void registerGayNoSQL(sqlite3Backend::User user);
	std::vector<sqlite3Backend::User> getGayList(void);
};
extern gaysClass gays;
#endif
