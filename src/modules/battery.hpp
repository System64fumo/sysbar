#pragma once
#include "../module.hpp"
#ifdef MODULE_BATTERY
#include "controls.hpp"

#include <giomm/dbusproxy.h>

class module_battery : public module {
	public:
		module_battery(sysbar*, const bool&);

	private:
		Glib::RefPtr<Gio::DBus::Proxy> proxy;

		#ifdef MODULE_CONTROLS
		control* control_battery;
		void setup_control();
		#endif

		void setup();
		void on_properties_changed(
			const Gio::DBus::Proxy::MapChangedProperties&,
			const std::vector<Glib::ustring>&);
		void update_info(const std::string&);
};

#endif
