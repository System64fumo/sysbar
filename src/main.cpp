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
	#ifdef CONFIG_FILE
	std::string config_path;
	std::map<std::string, std::map<std::string, std::string>> config;
	std::map<std::string, std::map<std::string, std::string>> config_usr;

	bool cfg_sys = std::filesystem::exists("/usr/share/sys64/bar/config.conf");
	bool cfg_sys_local = std::filesystem::exists("/usr/local/share/sys64/bar/config.conf");
	bool cfg_usr = std::filesystem::exists(std::string(getenv("HOME")) + "/.config/sys64/bar/config.conf");

	// Load default config
	if (cfg_sys)
		config_path = "/usr/share/sys64/bar/config.conf";
	else if (cfg_sys_local)
		config_path = "/usr/local/share/sys64/bar/config.conf";
	else
		std::fprintf(stderr, "No default config found, Things will get funky!\n");

	config = config_parser(config_path).data;

	// Load user config
	if (cfg_usr)
		config_path = std::string(getenv("HOME")) + "/.config/sys64/bar/config.conf";
	else
		std::fprintf(stderr, "No user config found\n");

	config_usr = config_parser(config_path).data;

	// Merge configs
	for (const auto& [key, nested_map] : config_usr)
		config[key] = nested_map;

	// Sanity check
	if (!(cfg_sys || cfg_sys_local || cfg_usr)) {
		std::fprintf(stderr, "No config available, Something ain't right here.");
		return 1;
	}
	#endif

	 // TODO: Consider using -- arguments
	 // TODO: Add support for literally every config via runtime
	 // TODO: Add more help options

	 // Read launch arguments
	#ifdef CONFIG_RUNTIME
	while (true) {
		switch(getopt(argc, argv, "psceSVmdvh")) {
			case 'p':
				config["main"]["position"] = optarg;
				continue;

			case 's':
				config["main"]["m_start"] = optarg;
				continue;

			case 'c':
				config["main"]["m_center"] = optarg;
				continue;

			case 'e':
				config["main"]["m_end"] = optarg;
				continue;

			case 'S':
				config["main"]["size"] = optarg;
				continue;

			case 'V':
				config["main"]["verbose"] = "true";
				continue;

			case 'm':
				config["main"]["main_monitor"] = optarg;
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
	win = sysbar_create_ptr(config);

	// Catch signals
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGRTMIN, handle_signal);

	return app->run();
}
