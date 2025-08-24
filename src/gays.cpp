#include "gays.hpp"
#include "sqliteBackend.hpp"

gaysClass::gaysClass() {}
std::vector<sqlite3Backend::User> gaysClass::List = {};
void gaysClass::registerGayNoSQL(sqlite3Backend::User user) {
	List.push_back(user);
}
void gaysClass::registerGay(const std::string& name) {
	int id = sqlite3Backend::saveUser(name);
	sqlite3Backend::User user(id, name);
	List.push_back(user);
}

std::vector<sqlite3Backend::User> gaysClass::getGayList(void) {
	return List;
}
gaysClass gays;
