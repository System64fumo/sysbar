#include "clock.hpp"

#include <iomanip>

module_clock::module_clock(const bool &icon_on_start, const bool &clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_clock");
	image_icon.set_from_icon_name("preferences-system-time-symbolic");

	update_info();
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_clock::update_info), 1000);
}

bool module_clock::update_info() {
	std::time_t now = std::time(nullptr);
	std::tm* localTime = std::localtime(&now);
	std::stringstream ss;
	ss << std::put_time(localTime, "%T");
	label_info.set_text(ss.str());
	return true;
}
