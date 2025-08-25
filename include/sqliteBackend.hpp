#include <iostream>
#include <vector>
#ifndef SQLITEBACKEND_HPP
#define SQLITEBACKEND_HPP
#include <string>

#include <sqlite3.h>

namespace sqlite3Backend {
	struct User {
		int id;
		std::string name;
		User(int i, const std::string n) : id(i), name(n) {};
	};

	bool initDB(const std::string& dbPath);

	std::vector<User> loadUsers(void);
	
	int saveUser(const std::string& name);
	int deleteUser(const std::string& name);
}
#endif

