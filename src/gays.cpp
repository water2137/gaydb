#include <algorithm>

#include "gays.hpp"
#include "sqliteBackend.hpp"

gaysClass::gaysClass() {}
std::vector<sqlite3Backend::User> gaysClass::List = {};
void gaysClass::registerGayNoSQL(sqlite3Backend::User user) {
	List.push_back(user);
}
void gaysClass::registerGay(const std::string& name) {
	int id = sqlite3Backend::saveUser(name);
	if (id < 0) return;
	sqlite3Backend::User user(id, name);
	registerGayNoSQL(user);
}

void gaysClass::unregisterGayNoSQL(sqlite3Backend::User user) {
	List.erase(std::remove_if(List.begin(), List.end(),
		[user](const sqlite3Backend::User& s){ return s.id == user.id; }),
		List.end());
}

void gaysClass::unregisterGay(const std::string& name) {
	int id = sqlite3Backend::deleteUser(name);
	if (id < 0) return;
	sqlite3Backend::User user(id, name);
	unregisterGayNoSQL(user);
}

std::vector<sqlite3Backend::User> gaysClass::getGayList(void) {
	return List;
}
gaysClass gays;
