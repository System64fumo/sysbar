#include "tray.hpp"

module_tray::module_tray(const bool &icon_on_start, const bool &clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_tray");
	image_icon.set_from_icon_name("arrow-right");
	label_info.hide();
}

bool module_tray::update_info() {
	return true;
}
