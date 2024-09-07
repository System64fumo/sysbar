#include "bluetooth.hpp"

module_bluetooth::module_bluetooth(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_bluetooth");
	label_info.hide();

	auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM);
	auto proxy = Gio::DBus::Proxy::create_sync(connection, "org.bluez", "/org/bluez/hci0", "org.bluez.Adapter1");

	if (!proxy)
		return;

	Glib::Variant<bool> powered;
	proxy->get_cached_property(powered, "Powered");

	Glib::Variant<Glib::ustring> address;
	proxy->get_cached_property(address, "Address");

	Glib::Variant<Glib::ustring> name;
	proxy->get_cached_property(name, "Name");

	std::printf("Addr: %s\n", address.get().c_str());
	std::printf("Name: %s\n", name.get().c_str());

	// TODO: Figure out why this doesn't work
	proxy->signal_properties_changed().connect(sigc::mem_fun(*this, &module_bluetooth::update_info));

	if (powered)
		image_icon.set_from_icon_name("bluetooth-active-symbolic");
	else
		image_icon.set_from_icon_name("bluetooth-inactive-symbolic");
}

void module_bluetooth::update_info(DBusPropMap changed, DBusPropList invalid) {
	std::printf("Bluetooth properties updated\n");
}
