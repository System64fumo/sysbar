#include "../config_parser.hpp"
#include "../config.hpp"
#include "battery.hpp"

#include <giomm/dbusconnection.h>
#include <iostream>

module_battery::module_battery(const config_bar &cfg, const bool &icon_on_start, const bool &clickable) : module(cfg, icon_on_start, clickable) {
	get_style_context()->add_class("module_battery");
	image_icon.set_from_icon_name("battery-missing-symbolic"); // Fallback
	label_info.set_text("0"); // Fallback
	label_info.hide(); // TODO: Add option for this
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
	update_info();
}

void module_battery::on_properties_changed(
	const Gio::DBus::Proxy::MapChangedProperties &properties,
	const std::vector<Glib::ustring> &invalidated) {

	// TODO: Check for more stuff
	/*for (const auto& prop : properties) {
		if (config_main.verbose)
			std::cout << "Value: " << prop.first << std::endl;

		if (prop.first ==  "IconName") {
			Glib::Variant<Glib::ustring> icon_name;
			proxy->get_cached_property(icon_name, "IconName");
			image_icon.set_from_icon_name(icon_name.get());
		}

		else if (prop.first ==  "UpdateTime") {
			Glib::Variant<double> charge;
			proxy->get_cached_property(charge, "Percentage");
			label_info.set_text(std::to_string(charge.get()));
		}
	}*/
	update_info();
}

void module_battery::update_info() {
	// TODO: only get the appropiate property on change

	Glib::Variant<double> charge;
	proxy->get_cached_property(charge, "Percentage");
	label_info.set_text(std::to_string(charge.get()));

	Glib::Variant<Glib::ustring> icon_name;
	proxy->get_cached_property(icon_name, "IconName");
	image_icon.set_from_icon_name(icon_name.get());
}
