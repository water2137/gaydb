#include <iostream>
#include <vector>
#include <string>

#include <sqlite3.h>

#include "sqliteBackend.hpp"
#include "gays.hpp"

namespace sqlite3Backend {
	static sqlite3* db = nullptr;
	
	void closeDB() {
		if (db) {
			sqlite3_close(db);
			db = nullptr;
			std::cout << "database closed\n";
		}
	}

	bool initDB(const std::string& dbPath) {
		static bool registeredExit = false;

		if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
			std::cerr << "can't open DB: " << sqlite3_errmsg(db) << "\n";
			return false;
		}

		if (!registeredExit) {
			atexit(closeDB);
			registeredExit = true;
		}

		const char* sql = R"(
        		CREATE TABLE IF NOT EXISTS users (
            			id INTEGER PRIMARY KEY AUTOINCREMENT,
            			name TEXT
        		);
    		)";

		char* errMsg = nullptr;
		if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
			std::cerr << "schema creation failed: " << errMsg << "\n";
			sqlite3_free(errMsg);
			return false;
		}
		std::vector<User> users = loadUsers();
		for (User x : users)
			gays.registerGayNoSQL(x);

		return true;
	}

	std::vector<User> loadUsers(void) {
		sqlite3_stmt* stmt;
		std::vector<User> users;

		const char* sql = "SELECT id, name FROM users;";

		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			std::cerr << "failed to prepare: " << sqlite3_errmsg(db) << "\n";
			return users;
		}

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			User u(sqlite3_column_int(stmt, 0), reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
			users.push_back(u);
		}

		sqlite3_finalize(stmt);

		return users;
	}

	int saveUser(const std::string& name) {
		sqlite3_stmt* stmt;
		const char* sql = "INSERT INTO users (name) VALUES (?);";

		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			std::cerr << "failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
			return -1;
		}

		sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::cerr << "insert failed: " << sqlite3_errmsg(db) << "\n";
			sqlite3_finalize(stmt);
			return -1;
		}

		sqlite3_finalize(stmt);

		int lastId = (int)sqlite3_last_insert_rowid(db);
		std::cout << "new user inserted with id = " << lastId << "\n";

		return lastId;
	}
}
