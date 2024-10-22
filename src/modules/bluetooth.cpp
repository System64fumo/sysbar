#include "bluetooth.hpp"

module_bluetooth::module_bluetooth(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_bluetooth");
	label_info.hide();

	auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM);

	// Get initial data
	auto om_proxy = Gio::DBus::Proxy::create_sync(
		connection,
		"org.bluez",
		"/",
		"org.freedesktop.DBus.ObjectManager");

	std::vector<Glib::VariantBase> args_vector;
	auto args = Glib::VariantContainerBase::create_tuple(args_vector);
	auto result = om_proxy->call_sync("GetManagedObjects", args);
	auto result_cb = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(result);
	auto result_base = result_cb.get_child(0);

	extract_data(result_base);
	default_adapter = adapters.front();

	// TODO: This should probably use the first adapter's path-
	// rather than using a hardcoded path..

	// Setup change detection
	prop_proxy = Gio::DBus::Proxy::create_sync(
		connection,
		"org.bluez",
		"/org/bluez/hci0",
		"org.freedesktop.DBus.Properties");

	prop_proxy->signal_signal().connect(
		sigc::mem_fun(*this, &module_bluetooth::on_properties_changed));

	#ifdef MODULE_CONTROLS
	setup_control();
	#endif
	update_info("PowerState");
}

void module_bluetooth::on_properties_changed(
	const Glib::ustring& sender_name,
	const Glib::ustring& signal_name,
	const Glib::VariantContainerBase& parameters) {

	std::printf("Bluetooth properties updated\n");
	Glib::VariantIter iter(parameters);
	Glib::VariantBase child;
	iter.next_value(child);
	Glib::ustring adp_interface = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();
	iter.next_value(child);

	// Other adapters do not matter
	if (std::string(adp_interface.c_str()) != default_adapter.interface)
		return;

	auto variant_dict = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>>(child);
	std::map<Glib::ustring, Glib::VariantBase> map_hints = variant_dict.get();
	for (const auto& [key, value] : map_hints) {
		std::printf("Property changed: %s\n", key.c_str());
		if (key == "PowerState") {
			std::string powered = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string>>(value).get();
			default_adapter.powered = (powered == "on");
		}
		update_info(key);
	}
}

void module_bluetooth::extract_data(const Glib::VariantBase& variant_base) {
	auto variant = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::DBusObjectPathString, std::map<Glib::ustring, std::map<Glib::ustring, Glib::VariantBase>>>>>(variant_base);
	auto data_map = variant.get();

	for (const auto& [object_path, interface_map] : data_map) {
		if (win->verbose)
			std::printf("Object Path: %s\n", object_path.c_str());

		for (const auto& [interface_name, property_map] : interface_map) {
			if (win->verbose)
				std::printf("  Interface: %s\n", interface_name.c_str());

			if (interface_name.find("org.bluez.Adapter") == 0) {
				adapter adp;
				adp.interface = interface_name;
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

			if (win->verbose)
				for (const auto& [property_name, value] : property_map) {
					std::printf("    Property: %s = %s\n", property_name.c_str(), value.print().c_str());
				}
		}
	}
}

void module_bluetooth::update_info(std::string property) {
	if (property == "PowerState") {
		std::string icon;
		if (default_adapter.powered)
			icon = "bluetooth-active-symbolic";
		else
			icon = "bluetooth-disabled-symbolic";

		image_icon.set_from_icon_name(icon);

		if (!win->box_controls)
			return;

		control_bluetooth->button_action.set_icon_name(icon);
	}
}

#ifdef MODULE_CONTROLS
void module_bluetooth::setup_control() {
	if (!win->box_controls)
		return;

	auto container = static_cast<module_controls*>(win->box_controls);
	control_bluetooth = Gtk::make_managed<control>("bluetooth-active-symbolic", false);
	container->flowbox_controls.append(*control_bluetooth);
}
#endif
