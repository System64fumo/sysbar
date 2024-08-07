#include "main.hpp"
#include "config_parser.hpp"
#include "git_info.hpp"

#include <gtkmm/application.h>
#include <iostream>
#include <signal.h>
#include <dlfcn.h>

void load_libsysbar() {
	void* handle = dlopen("libsysbar.so", RTLD_LAZY);
	if (!handle) {
		std::cerr << "Cannot open library: " << dlerror() << '\n';
		exit(1);
	}

	sysbar_create_ptr = (sysbar_create_func)dlsym(handle, "sysbar_create");
	sysbar_handle_signal_ptr = (sysbar_handle_signal_func)dlsym(handle, "sysbar_signal");

	if (!sysbar_create_ptr) {
		std::cerr << "Cannot load symbols: " << dlerror() << '\n';
		dlclose(handle);
		exit(1);
	}
}

void handle_signal(int signum) {
	sysbar_handle_signal_ptr(win, signum);
}

int main(int argc, char* argv[]) {
	// Load the config
	#ifdef CONFIG_FILE
	config_parser config(std::string(getenv("HOME")) + "/.config/sys64/bar/config.conf");

	if (config.available) {
		std::string cfg_position = config.get_value("main", "position");
		if (cfg_position != "empty")
			config_main.position = std::stoi(cfg_position);

		std::string cfg_size = config.get_value("main", "size");
		if (cfg_size != "empty")
			config_main.size = std::stoi(cfg_size);

		std::string cfg_verbose = config.get_value("main", "verbose");
		if (cfg_verbose != "empty")
			config_main.verbose = (cfg_verbose == "true");

		std::string cfg_main_monitor = config.get_value("main", "main-monitor");
		if (cfg_main_monitor != "empty")
			config_main.main_monitor = std::stoi(cfg_main_monitor);

		std::string cfg_m_start = config.get_value("main", "m_start");
		if (cfg_m_start != "empty")
			config_main.m_start = cfg_m_start;

		std::string cfg_m_center = config.get_value("main", "m_center");
		if (cfg_m_center != "empty")
			config_main.m_center = cfg_m_center;

		std::string cfg_m_end = config.get_value("main", "m_end");
		if (cfg_m_end != "empty")
			config_main.m_end = cfg_m_end;
	}
	#endif

	 // Read launch arguments
	#ifdef CONFIG_RUNTIME
	while (true) {
		switch(getopt(argc, argv, "p:ds:c:e:S:dVm:dvh")) {
			case 'p':
				config_main.position = std::stoi(optarg);
				continue;

			case 's':
				config_main.m_start = optarg;
				continue;

			case 'c':
				config_main.m_center = optarg;
				continue;

			case 'e':
				config_main.m_end = optarg;
				continue;

			case 'S':
				config_main.size = std::stoi(optarg);
				continue;

			case 'V':
				config_main.verbose = true;
				continue;

			case 'm':
				config_main.main_monitor = std::stoi(optarg);
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
				std::cout << "  -s	Set start modules" << std::endl;
				std::cout << "  -c	Set center modules" << std::endl;
				std::cout << "  -e	Set end modules" << std::endl;
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
	#endif

	// Load the application
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("funky.sys64.sysbar");
	app->hold();

	load_libsysbar();
	win = sysbar_create_ptr(config_main);

	// Catch signals
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGRTMIN, handle_signal);

	return app->run();
}
