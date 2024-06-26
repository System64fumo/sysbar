#include "../config_parser.hpp"
#include "../config.hpp"
#include "clock.hpp"

#include <ctime>

module_clock::module_clock(const bool &icon_on_start, const bool &clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_clock");
	image_icon.set_from_icon_name("preferences-system-time-symbolic");

	config_parser config(std::string(getenv("HOME")) + "/.config/sys64/bar.conf");

	std::string cfg_format = config.get_value("clock", "format");
	if (cfg_format != "empty")
		format = cfg_format;

	update_info();
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_clock::update_info), 1000);
}

bool module_clock::update_info() {
	std::time_t now = std::time(nullptr);
	std::tm* localTime = std::localtime(&now);
	char buffer[100];
	std::strftime(buffer, sizeof(buffer), format.c_str(), localTime);
	label_info.set_text(buffer);
	return true;
}
