#pragma once
#include "../module.hpp"
#ifdef MODULE_BATTERY

#include <giomm/dbusproxy.h>

class module_battery : public module {
	public:
		module_battery(sysbar*, const bool&);

	private:
		Glib::RefPtr<Gio::DBus::Proxy> proxy;

		void setup();
		void on_properties_changed(
			const Gio::DBus::Proxy::MapChangedProperties&,
			const std::vector<Glib::ustring>&);
		void update_info(const std::string&);
};

#endif
