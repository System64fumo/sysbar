#include "../config_parser.hpp"
#include "../config.hpp"
#include "clock.hpp"

#include <ctime>

module_clock::module_clock(const bool &icon_on_start, const bool &clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_clock");
	image_icon.set_from_icon_name("preferences-system-time-symbolic");

	#ifdef CONFIG_FILE
	config_parser config(std::string(getenv("HOME")) + "/.config/sys64/bar.conf");

	std::string cfg_format = config.get_value("clock", "format");
	if (cfg_format != "empty")
		format = cfg_format;

	std::string cfg_interval = config.get_value("clock", "interval");
	if (cfg_interval != "empty")
		interval = std::stoi(cfg_interval);
	#endif

	update_info();
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_clock::update_info), interval);
}

bool module_clock::update_info() {
	std::time_t now = std::time(nullptr);
	std::tm* local_time = std::localtime(&now);
	char buffer[32];
	std::strftime(buffer, sizeof(buffer), format.c_str(), local_time);
	label_info.set_text(buffer);
	return true;
}
