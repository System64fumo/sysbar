#include "main.hpp"
#include "config_parser.hpp"
#include "git_info.hpp"

#include <gtkmm/application.h>
#include <filesystem>
#include <signal.h>
#include <dlfcn.h>

void load_libsysbar() {
	void* handle = dlopen("libsysbar.so", RTLD_LAZY);
	if (!handle) {
		std::fprintf(stderr, "Cannot open library: %s\n", dlerror());
		exit(1);
	}

	sysbar_create_ptr = (sysbar_create_func)dlsym(handle, "sysbar_create");
	sysbar_handle_signal_ptr = (sysbar_handle_signal_func)dlsym(handle, "sysbar_signal");

	if (!sysbar_create_ptr) {
		std::fprintf(stderr, "Cannot load symbols: %s\n", dlerror());
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
	std::string config_path;
	if (std::filesystem::exists(std::string(getenv("HOME")) + "/.config/sys64/bar/config.conf"))
		config_path = std::string(getenv("HOME")) + "/.config/sys64/bar/config.conf";
	else if (std::filesystem::exists("/usr/share/sys64/bar/config.conf"))
		config_path = "/usr/share/sys64/bar/config.conf";
	else
		config_path = "/usr/local/share/sys64/bar/config.conf";

	config_parser config(config_path);

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
				std::printf("Commit: %s\n", GIT_COMMIT_MESSAGE);
				std::printf("Date: %s\n", GIT_COMMIT_DATE);
				return 0;

			case 'h':
			default :
				std::printf("usage:\n");
				std::printf("  sysbar [argument...]:\n\n");
				std::printf("arguments:\n");
				std::printf("  -p	Set position\n");
				std::printf("  -s	Set start modules\n");
				std::printf("  -c	Set center modules\n");
				std::printf("  -e	Set end modules\n");
				std::printf("  -S	Set bar size\n");
				std::printf("  -V	Be more verbose\n");
				std::printf("  -m	Set primary monitor\n");
				std::printf("  -v	Prints version info\n");
				std::printf("  -h	Show this help message\n");
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
