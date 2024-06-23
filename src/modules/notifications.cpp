#include "../config.hpp"
#include "notifications.hpp"

module_notifications::module_notifications(const bool &icon_on_start, const bool &clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_notifications");
	image_icon.set_from_icon_name("notification-symbolic");
	label_info.hide();
}

bool module_notifications::update_info() {
	return true;
}
