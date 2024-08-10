#include "module.hpp"
#include "config.hpp"

module::module(const config_bar &cfg, const bool &icon_on_start, const bool &clickable) {
	config_main = cfg;
	// TODO: Read config to see if the icon should appear before or after
	// the label.

	// TODO: Read config to see if the label should be displayed for the module.

	// Initialization
	get_style_context()->add_class("module");
	append(label_info);
	set_cursor(Gdk::Cursor::create("pointer"));

	// Set orientation
	if (config_main.position % 2) {
		Gtk::Orientation orientation = Gtk::Orientation::VERTICAL;
		set_orientation(orientation);
	}

	if (icon_on_start) {
		prepend(image_icon);
		if (config_main.position % 2)
			label_info.set_margin_bottom(config_main.size / 3);
		else
			label_info.set_margin_end(config_main.size / 3);
	}
	else {
		append(image_icon);
		if (config_main.position % 2)
			label_info.set_margin_top(config_main.size / 3);
		else
			label_info.set_margin_start(config_main.size / 3);
	}

	// TODO: add user customizable margins
	image_icon.set_size_request(config_main.size, config_main.size);
}
