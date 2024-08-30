#include "clock.hpp"

#include <gtkmm/calendar.h>
#include <ctime>

module_clock::module_clock(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_clock");
	image_icon.set_from_icon_name("preferences-system-time-symbolic");

	#ifdef CONFIG_FILE
	if (config->available) {
		std::string cfg_label_format = config->get_value("clock", "label-format");
		if (cfg_label_format != "empty")
			tooltip_format = cfg_label_format;

		std::string cfg_tooltip_format = config->get_value("clock", "tooltip-format");
		if (cfg_tooltip_format != "empty")
			tooltip_format = cfg_tooltip_format;

		std::string cfg_interval = config->get_value("clock", "interval");
		if (cfg_interval != "empty")
			interval = std::stoi(cfg_interval);
	}
	#endif

	update_info();
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_clock::update_info), interval);
	setup_widget();
}

bool module_clock::update_info() {
	std::time_t now = std::time(nullptr);
	std::tm* local_time = std::localtime(&now);

	char label_buffer[32];
	std::strftime(label_buffer, sizeof(label_buffer), label_format.c_str(), local_time);
	label_info.set_text(label_buffer);

	char tooltip_buffer[32];
	std::strftime(tooltip_buffer, sizeof(tooltip_buffer), tooltip_format.c_str() , local_time);
	set_tooltip_text(tooltip_buffer);

	return true;
}

void module_clock::setup_widget() {
	Gtk::Calendar calendar;
	auto container = static_cast<Gtk::Box*>(win->popover_start->get_child());
	container->append(calendar);
}
