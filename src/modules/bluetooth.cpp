#include "bluetooth.hpp"
#include <fstream>
#include <filesystem>

module_bluetooth::module_bluetooth(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_bluetooth");
	label_info.hide();

	if (win->config_main["bluetooth"]["show-icon"] != "true")
		image_icon.hide();

	if (!test()) {
		// TODO: Consider adding error codes or error messages, This is too vague.
		std::printf("Bluetooth: Some errors were found, disabling..\n");
		image_icon.hide();
		return;
	}

	#ifdef MODULE_CONTROLS
	setup_control();
	#endif

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
	if (adapters.empty()) {
		std::fprintf(stderr, "No default adapter found\n");
		return;
	}
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

	update_info("PowerState");
}

bool module_bluetooth::test() {
	bool bluetoothd_running = false;
	bool adapter_available = false;

	// Check if bluetooth service is running
	DIR* dir = opendir("/proc");
	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		if (entry->d_type == DT_DIR && std::all_of(entry->d_name, entry->d_name + strlen(entry->d_name), ::isdigit)) {
			std::string pid = entry->d_name;
			std::string comm_file = "/proc/" + pid + "/comm";

			std::ifstream comm_stream(comm_file);
			if (comm_stream.is_open()) {
				std::string comm;
				comm_stream >> comm;

				if (comm == "bluetoothd") {
					bluetoothd_running = true;
					break;
				}

			}
		}
	}
	closedir(dir);

	// Check if an adapter is available
	std::filesystem::path bluetooth_path("/sys/class/bluetooth/");
	if (!std::filesystem::exists(bluetooth_path)) {
		std::fprintf(stderr, "/sys/class/bluetooth does not exist\n");
		return false;
	}

	for (const auto& entry : std::filesystem::directory_iterator(bluetooth_path)) {
		if (std::filesystem::is_directory(entry.status()))
			adapter_available = true;
	}

	return bluetoothd_running && adapter_available;
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

void module_bluetooth::update_info(const std::string& property) {
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
	control_bluetooth = Gtk::make_managed<control>(win, "bluetooth-active-symbolic", true, "bluetooth");
	container->flowbox_controls.append(*control_bluetooth);
}
#endif
