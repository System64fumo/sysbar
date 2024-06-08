#include "main.hpp"
#include "window.hpp"
#include "config.hpp"
#include "git_info.hpp"

#include <glibmm.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <signal.h>

void quit(int signum) {
	// Remove window
	app->release();
	app->remove_window(*win);
	delete win;
	app->quit();
}

int main(int argc, char* argv[]) {
	// Read launch arguments
	while (true) {
		switch(getopt(argc, argv, "p:dS:dVvh")) {
			case 'p':
				position = std::stoi(optarg);
				continue;

			case 'S':
				size = std::stoi(optarg);
				continue;

			case 'V':
				verbose = true;
				continue;

			case 'v':
				std::cout << "Commit: " << GIT_COMMIT_MESSAGE << std::endl;
				std::cout << "Date: " << GIT_COMMIT_DATE << std::endl;
				return 0;

			case 'h':
			default :
				std::cout << "usage:" << std::endl;
				std::cout << "  sysbar [argument...]:\n" << std::endl;
				std::cout << "arguments:" << std::endl;
				std::cout << "  -p	Set position" << std::endl;
				std::cout << "  -S	Set bar size" << std::endl;
				std::cout << "  -V	Be more verbose" << std::endl;
				std::cout << "  -v	Prints version info" << std::endl;
				std::cout << "  -h	Show this help message" << std::endl;
				return 0;

			case -1:
				break;
			}

			break;
	}

	signal(SIGINT, quit);

	app = Gtk::Application::create("funky.sys64.sysbar");
	app->hold();
	win = new sysbar();

	return app->run();
}
