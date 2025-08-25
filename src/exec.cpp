#include <iostream>
#include <string>
#include <sstream>
#include <numeric>

#include "sqliteBackend.hpp"
#include "gays.hpp"

namespace execOwenSQL {
	std::vector<std::string> splitBySpaces(const std::string& str) {
		std::istringstream iss(str);
		std::vector<std::string> result;
		std::string word;

		while (iss >> word) {
			result.push_back(word);
		}

		return result;
	}

	void runCMD(const std::string& line) {
		static bool hasInit = 0;
		std::vector<std::string> args = splitBySpaces(line);
		int argsSize = args.size(); if (argsSize < 1) return;
		std::string restArgs;
		if (argsSize > 1) 
			restArgs = std::accumulate(
				std::next(args.begin(), 2), args.end(), args[1],
				[](std::string a, const std::string& b) { return std::move(a) + " " + b; });
		if (args[0] == "use") {
			if (argsSize < 2) {
				std::cerr << "use needs second argument" << std::endl; 
				return;
			}
			if (!hasInit) {
				if (sqlite3Backend::initDB(restArgs))
					hasInit = !hasInit;
				else
					std::cerr << "an error occured" << std::endl;
			} else
				std::cerr << "cannot reinit, exit to use other db" << std::endl;
		} else {
			if (!hasInit) {
				std::cerr << "you have not initialized, use 'use [db path]' to do so" << std::endl;
				return;
			}
			if (args[0] == "addGay") {
				if (argsSize < 2) {
					std::cerr << "addGay needs second argument" << std::endl;
					return;
				}
				gays.registerGay(restArgs);
			} else if (args[0] == "removeGay") {
				//std::cerr << "ERROR: gays will always be gay" << std::endl;
				if (argsSize < 2) {
					std::cerr << "removeGay needs second argument" << std::endl;
					return;
				}
				gays.unregisterGay(restArgs);
			} else if (args[0] == "showGay") {
				for (const auto& gay : gays.getGayList())
					std::cout << gay.name << "\n";
				std::cout << std::flush;
			} else
				std::cerr << "invalid command\ncommands: removeGay showGay addGay use" << std::endl;
		}
	}
}
