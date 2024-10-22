#include "battery.hpp"

#include <giomm/dbusconnection.h>

module_battery::module_battery(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_battery");
	image_icon.set_from_icon_name("battery-missing-symbolic"); // Fallback
	label_info.set_text("0"); // Fallback

	if (win->config_main["battery"]["show-label"] != "true")
		label_info.hide();

	setup();
}

void module_battery::setup() {
	proxy = Gio::DBus::Proxy::create_sync(
		Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM),
		"org.freedesktop.UPower",
		"/org/freedesktop/UPower/devices/DisplayDevice",
		"org.freedesktop.UPower.Device");

	proxy->signal_properties_changed().connect(
		sigc::mem_fun(*this, &module_battery::on_properties_changed));

	// Trigger a manual update
	update_info("IconName");
	update_info("UpdateTime");
}

void module_battery::on_properties_changed(
	const Gio::DBus::Proxy::MapChangedProperties &properties,
	const std::vector<Glib::ustring> &invalidated) {

	for (const auto& prop : properties) {
		update_info(prop.first);
	}
}

void module_battery::update_info(const std::string& property) {
	// TODO: Check for more stuff

	if (property ==  "IconName") {
		Glib::Variant<Glib::ustring> icon_name;
		proxy->get_cached_property(icon_name, "IconName");
		image_icon.set_from_icon_name(icon_name.get());
	}

	else if (property ==  "UpdateTime") {
		Glib::Variant<double> charge;
		proxy->get_cached_property(charge, "Percentage");
		label_info.set_text(std::to_string((int)charge.get()));
	}
}
