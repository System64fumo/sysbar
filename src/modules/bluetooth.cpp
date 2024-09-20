#include "bluetooth.hpp"
#include "controls.hpp"

module_bluetooth::module_bluetooth(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_bluetooth");
	label_info.hide();

	auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM);
	auto proxy = Gio::DBus::Proxy::create_sync(
		connection,
		"org.bluez",
		"/org/bluez/hci0",
		"org.bluez.Adapter1");

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
	prop_proxy = Gio::DBus::Proxy::create_sync(
		connection,
		"org.bluez",
		"/org/bluez/hci0",
		"org.freedesktop.DBus.Properties");

	prop_proxy->signal_signal().connect(sigc::mem_fun(*this, &module_bluetooth::on_properties_changed));

	if (powered)
		image_icon.set_from_icon_name("bluetooth-active-symbolic");
	else
		image_icon.set_from_icon_name("bluetooth-inactive-symbolic");

	auto s_proxy = Gio::DBus::Proxy::create_sync(connection, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager");

	std::vector<Glib::VariantBase> args_vector;
	auto args = Glib::VariantContainerBase::create_tuple(args_vector);

	auto result = s_proxy->call_sync("GetManagedObjects", args);
	Glib::VariantContainerBase result_cb = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(result);
	Glib::VariantBase base = result_cb.get_child(0);

	extract_data(base);
	setup_control();
}

void module_bluetooth::on_properties_changed(
	const Glib::ustring& sender_name,
	const Glib::ustring& signal_name,
	const Glib::VariantContainerBase& parameters) {
	std::printf("Bluetooth properties updated\n");
}

void module_bluetooth::extract_data(const Glib::VariantBase& variant_base) {
	auto variant = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::DBusObjectPathString, std::map<Glib::ustring, std::map<Glib::ustring, Glib::VariantBase>>>>>(variant_base);
	auto data_map = variant.get();

	for (const auto& [object_path, interface_map] : data_map) {
		std::printf("Object Path: %s\n", object_path.c_str());

		for (const auto& [interface_name, property_map] : interface_map) {
			std::printf("  Interface: %s\n", interface_name.c_str());

			if (interface_name.find("org.bluez.Adapter") == 0) {
				adapter adp;
				adp.path = object_path;

				for (const auto& [property_name, value] : property_map) {
					if (property_name == "Alias")
						adp.alias = value.print();
					else if (property_name == "Discoverable")
						adp.discoverable = (value.print() == "true");
					else if (property_name == "Discovering")
						adp.discovering = (value.print() == "true");
					else if (property_name == "Name")
						adp.name = value.print();
					else if (property_name == "Pairable")
						adp.pairable = (value.print() == "true");
					else if (property_name == "Powered")
						adp.powered = (value.print() == "true");
				}
				adapters.push_back(adp);
			}
			else if (interface_name.find("org.bluez.Device") == 0) {
				device dev;
				dev.path = object_path;

				for (const auto& [property_name, value] : property_map) {
					if (property_name == "Blocked")
						dev.blocked = (value.print() == "true");
					else if (property_name == "Connected")
						dev.connected = (value.print() == "true");
					else if (property_name == "Icon")
						dev.icon = value.print();
					else if (property_name == "Name")
						dev.name = value.print();
					else if (property_name == "Paired")
						dev.paired = (value.print() == "true");
					else if (property_name == "Trusted")
						dev.trusted = (value.print() == "true");
				}
				devices.push_back(dev);
			}

			for (const auto& [property_name, value] : property_map) {
				std::printf("    Property: %s = %s\n", property_name.c_str(), value.print().c_str());
			}
		}
	}
}

void module_bluetooth::setup_control() {
	auto container = static_cast<module_controls*>(win->box_controls);
	control* control_bluetooth = Gtk::make_managed<control>("bluetooth-active-symbolic", false);
	container->flowbox_controls.append(*control_bluetooth);
}
