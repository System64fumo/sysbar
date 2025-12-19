#include "battery.hpp"

#include <giomm/dbusconnection.h>
#include <cstdio>

module_battery::module_battery(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start) {
	add_css_class("module_battery");
	
	image_icon.set_from_icon_name("battery-missing-symbolic"); // Fallback
	label_info.set_text("No battery detected"); // Fallback

	if (win->config_main["battery"]["show-label"] != "true")
		label_info.hide();

	setup_control();

	if (win->verbose)
		print_all_upower_devices();

	find_power_sources();
}

void module_battery::find_power_sources() {
	try {
		auto upower_proxy = Gio::DBus::Proxy::create_sync(
			Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM),
			"org.freedesktop.UPower",
			"/org/freedesktop/UPower",
			"org.freedesktop.UPower");

		Glib::VariantContainerBase result = upower_proxy->call_sync("EnumerateDevices");

		Glib::VariantBase devices_variant;
		result.get_child(devices_variant, 0);
		
		if (devices_variant.is_of_type(Glib::VARIANT_TYPE_OBJECT_PATH_ARRAY)) {
			Glib::Variant<std::vector<Glib::DBusObjectPathString>> devices =
				Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::DBusObjectPathString>>>(devices_variant);
			
			for (const auto& device_path : devices.get()) {
				try {
					auto device_proxy = Gio::DBus::Proxy::create_sync(
						Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM),
						"org.freedesktop.UPower",
						device_path,
						"org.freedesktop.UPower.Device");

					Glib::Variant<guint32> type;
					device_proxy->get_cached_property(type, "Type");
					
					guint32 type_val = type.get();
					if (type_val == 2) { // 2 = Battery
						if (!battery_source) {
							if (win->verbose)
								std::printf("Found battery: %s\n", device_path.c_str());
							initialize_battery_proxy(device_path);
						}
					} else if (type_val == 1) { // 1 = AC adapter
						if (!ac_source) {
							if (win->verbose)
								std::printf("Found AC adapter: %s\n", device_path.c_str());
							initialize_ac_proxy(device_path);
						}
					}
					
				} catch (const std::exception& e) {
					std::printf("Error checking device %s: %s\n", device_path.c_str(), e.what());
				}
			}
		}
		
		if (!battery_source) {
			std::printf("No battery found, using fallback\n");
		}
		if (!ac_source) {
			std::printf("No AC adapter found\n");
		}
		
	} catch (const std::exception& e) {
		std::fprintf(stderr, "Error finding power sources: %s\n", e.what());
	}
}

void module_battery::initialize_battery_proxy(const Glib::DBusObjectPathString& battery_path) {
	battery_proxy = Gio::DBus::Proxy::create_sync(
		Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM),
		"org.freedesktop.UPower",
		battery_path,
		"org.freedesktop.UPower.Device");

	battery_proxy->signal_properties_changed().connect(
		sigc::mem_fun(*this, &module_battery::on_battery_properties_changed));

	// Initialize the battery source
	battery_source = new PowerSource();
	battery_source->path = battery_path;
	battery_source->type = "battery";

	// Trigger a manual update to get initial values
	update_info("IconName", battery_proxy.get(), battery_source);
	update_info("Percentage", battery_proxy.get(), battery_source);
	update_info("State", battery_proxy.get(), battery_source);
	update_info("Voltage", battery_proxy.get(), battery_source);
	update_info("Temperature", battery_proxy.get(), battery_source);
	update_info("Energy", battery_proxy.get(), battery_source);
	update_info("EnergyFullDesign", battery_proxy.get(), battery_source);
	update_info("EnergyRate", battery_proxy.get(), battery_source);
	update_info("TimeToEmpty", battery_proxy.get(), battery_source);
	update_info("TimeToFull", battery_proxy.get(), battery_source);
	
	// Update the icon immediately with the initial values
	update_battery_icon();
}

void module_battery::initialize_ac_proxy(const Glib::DBusObjectPathString& ac_path) {
	ac_proxy = Gio::DBus::Proxy::create_sync(
		Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM),
		"org.freedesktop.UPower",
		ac_path,
		"org.freedesktop.UPower.Device");

	ac_proxy->signal_properties_changed().connect(
		sigc::mem_fun(*this, &module_battery::on_ac_properties_changed));

	// Initialize the AC source
	ac_source = new PowerSource();
	ac_source->path = ac_path;
	ac_source->type = "ac";

	// Trigger a manual update for AC properties
	update_info("Online", ac_proxy.get(), ac_source);
}

std::string module_battery::get_battery_icon_name(double percentage, guint32 state) {
	std::string icon_base;
	int level = static_cast<int>(percentage / 10) * 10;
	
	// Clamp level to valid range (0-100 in steps of 10)
	if (level < 0) level = 0;
	if (level > 100) level = 100;
	
	icon_base = "battery-level-" + std::to_string(level);

	// Add charging suffix if battery is charging
	if (state == 1 || state == 5) {
		icon_base += "-charging";
	}
	else if (state == 4) {
		if (ac_source != nullptr && ac_source->is_present)
			icon_base += "-charged";
	}

	return icon_base + "-symbolic";
}

void module_battery::update_battery_icon() {
	if (!battery_source) return;
	
	std::string icon_name = get_battery_icon_name(battery_source->percentage, battery_source->state);
	image_icon.set_from_icon_name(icon_name);
	
	#ifdef MODULE_CONTROLS
	if (control_battery) control_battery->button_action.set_image_from_icon_name(icon_name);
	#endif
}

void module_battery::on_battery_properties_changed(
	const Gio::DBus::Proxy::MapChangedProperties &properties,
	const std::vector<Glib::ustring> &invalidated) {

	for (const auto& prop : properties) {
		update_info(prop.first, battery_proxy.get(), battery_source);
	}

	update_battery_icon();
}

void module_battery::on_ac_properties_changed(
	const Gio::DBus::Proxy::MapChangedProperties &properties,
	const std::vector<Glib::ustring> &invalidated) {

	for (const auto& prop : properties) {
		update_info(prop.first, ac_proxy.get(), ac_source);
	}

	if (battery_source) {
		update_battery_icon();
	}
}

void module_battery::update_info(const std::string& property, Gio::DBus::Proxy* proxy, PowerSource* source) {
	if (property == "UpdateTime") {
		return;
	}
	else if (property == "IconName") {
		Glib::Variant<Glib::ustring> icon_name;
		proxy->get_cached_property(icon_name, "IconName");
		if (source) {
			source->icon_name = icon_name.get();
		}
	}
	else if (property == "Percentage") {
		Glib::Variant<double> charge;
		proxy->get_cached_property(charge, "Percentage");
		if (win->verbose)
			std::printf("Percentage changed to: %.1f%%\n", charge.get());
		if (source && source->type == "battery") {
			source->percentage = charge.get();
			label_info.set_text(std::to_string((int)charge.get()));
		}
	}
	else if (property == "State") {
		Glib::Variant<guint32> state;
		proxy->get_cached_property(state, "State");
		const char* state_names[] = {"Unknown", "Charging", "Discharging", "Empty", "Fully charged", "Pending charge", "Pending discharge"};
		guint32 state_val = state.get();
		if (win->verbose)
			std::printf("State changed to: %s\n", (state_val < G_N_ELEMENTS(state_names) ? state_names[state_val] : "Unknown"));
		if (source) {
			source->state = state_val;
		}
	}
	else if (property == "TimeToEmpty") {
		Glib::Variant<gint64> time_to_empty;
		proxy->get_cached_property(time_to_empty, "TimeToEmpty");
		if (win->verbose)
			std::printf("Time to empty changed to: %ld seconds\n", time_to_empty.get());
		if (source) {
			source->time_to_empty = time_to_empty.get();
		}
	}
	else if (property == "TimeToFull") {
		Glib::Variant<gint64> time_to_full;
		proxy->get_cached_property(time_to_full, "TimeToFull");
		if (win->verbose)
			std::printf("Time to full changed to: %ld seconds\n", time_to_full.get());
		if (source) {
			source->time_to_full = time_to_full.get();
		}
	}
	else if (property == "Temperature") {
		Glib::Variant<double> temperature;
		proxy->get_cached_property(temperature, "Temperature");
		if (win->verbose)
			std::printf("Temperature changed to: %.1f\n", temperature.get());
		if (source) {
			source->temperature = temperature.get();
		}
	}
	else if (property == "Voltage") {
		Glib::Variant<double> voltage;
		proxy->get_cached_property(voltage, "Voltage");
		if (win->verbose)
			std::printf("Voltage changed to: %.2fv\n", voltage.get());
		if (source) {
			source->voltage = voltage.get();
		}
	}
	else if (property == "Energy") {
		Glib::Variant<double> energy;
		proxy->get_cached_property(energy, "Energy");
		if (win->verbose)
			std::printf("Energy changed to: %.1f\n", energy.get());
		if (source) {
			source->energy = energy.get();
		}
	}
	else if (property == "EnergyRate") {
		Glib::Variant<double> energy_rate;
		proxy->get_cached_property(energy_rate, "EnergyRate");
		if (win->verbose)
			std::printf("EnergyRate changed to: %.1f\n", energy_rate.get());
		if (source) {
			source->energy_rate = energy_rate.get();
		}
	}
	else if (property == "EnergyFullDesign") {
		Glib::Variant<double> energy_full_design;
		proxy->get_cached_property(energy_full_design, "EnergyFullDesign");
		if (win->verbose)
			std::printf("EnergyFullDesign changed to: %.1f\n", energy_full_design.get());
		if (source) {
			source->energy_full_design = energy_full_design.get();
		}
	}
	else if (property == "Online") {
		Glib::Variant<bool> online;
		proxy->get_cached_property(online, "Online");
		if (win->verbose)
			std::printf("AC adapter online: %s\n", (online.get() ? "yes" : "no"));
		if (source && source->type == "ac") {
			source->is_present = online.get();
		}
	}
	else {
		if (win->verbose)
			std::printf("Unhandled property change: %s\n", property.c_str());
	}
}

void module_battery::print_all_upower_devices() {
	try {
		auto upower_proxy = Gio::DBus::Proxy::create_sync(
			Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM),
			"org.freedesktop.UPower",
			"/org/freedesktop/UPower",
			"org.freedesktop.UPower");

		Glib::VariantContainerBase result = upower_proxy->call_sync("EnumerateDevices");

		Glib::VariantBase devices_variant;
		result.get_child(devices_variant, 0);
		
		if (devices_variant.is_of_type(Glib::VARIANT_TYPE_OBJECT_PATH_ARRAY)) {
			Glib::Variant<std::vector<Glib::DBusObjectPathString>> devices =
				Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::DBusObjectPathString>>>(devices_variant);
			
			for (const auto& device_path : devices.get()) {
				std::printf("Device: %s\n", device_path.c_str());
				
				try {
					auto device_proxy = Gio::DBus::Proxy::create_sync(
						Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM),
						"org.freedesktop.UPower",
						device_path,
						"org.freedesktop.UPower.Device");
					
					Glib::Variant<guint32> type;
					device_proxy->get_cached_property(type, "Type");
					
					guint32 type_val = type.get();
					if (type_val == 2) {
						std::printf("  -> Type: Battery\n");
					} else if (type_val == 1) {
						std::printf("  -> Type: Power Adapter\n");
					} else {
						const char* type_names[] = {"Unknown", "Line Power", "Battery", "Ups", "Monitor", "Mouse", "Keyboard", "PDA", "Phone"};
						std::printf("  -> Type: %s\n", (type_val < G_N_ELEMENTS(type_names) ? type_names[type_val] : "Unknown"));
					}
					
				} catch (const std::exception& e) {
					std::printf("  -> Error getting device type: %s\n", e.what());
				}
				std::printf("\n");
			}
		} else {
			std::printf("Unexpected variant type: %s\n", devices_variant.get_type_string().c_str());
		}
		
	}
	catch (const std::exception& e) {
		std::fprintf(stderr, "Error enumerating UPower devices: %s\n", e.what());
	}
}

#ifdef MODULE_CONTROLS
void module_battery::setup_control() {
	if (!win->box_controls)
		return;

	auto container = static_cast<module_controls*>(win->box_controls);
	control_battery = Gtk::make_managed<control>(win, "battery-full-symbolic", true, "battery", false);
	container->flowbox_controls.append(*control_battery);

	// TODO: Clicking on the main button should trigger low power mode
	// Additional controls for battery management
}
#endif