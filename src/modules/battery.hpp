#pragma once
#include "../module.hpp"
#ifdef MODULE_BATTERY
#include "controls.hpp"

#include <giomm/dbusproxy.h>
#include <vector>
#include <string>

struct PowerSource {
	Glib::DBusObjectPathString path;
	std::string type; // "battery", "ac", etc.
	std::string icon_name;
	double percentage;
	double voltage;
	double temperature;
	double energy;
	double energy_full_design;
	double energy_rate;
	guint32 state;
	gint64 time_to_empty;
	gint64 time_to_full;
	bool is_present;
};

class module_battery : public module {
	public:
		module_battery(sysbar*, const bool&);

	private:
		Glib::RefPtr<Gio::DBus::Proxy> battery_proxy;
		Glib::RefPtr<Gio::DBus::Proxy> ac_proxy;
		std::vector<PowerSource> power_sources;
		PowerSource* battery_source = nullptr;
		PowerSource* ac_source = nullptr;

		#ifdef MODULE_CONTROLS
		control* control_battery;
		void setup_control();
		#endif

		void find_power_sources();
		void initialize_battery_proxy(const Glib::DBusObjectPathString& battery_path);
		void initialize_ac_proxy(const Glib::DBusObjectPathString& ac_path);
		std::string get_battery_icon_name(double percentage, guint32 state);
		void update_battery_icon();
		void on_battery_properties_changed(
			const Gio::DBus::Proxy::MapChangedProperties&,
			const std::vector<Glib::ustring>&);
		void on_ac_properties_changed(
			const Gio::DBus::Proxy::MapChangedProperties&,
			const std::vector<Glib::ustring>&);
		void update_info(const std::string& property, Gio::DBus::Proxy* proxy, PowerSource* source);
		void print_all_upower_devices();
};

#endif