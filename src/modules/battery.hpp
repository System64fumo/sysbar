#pragma once
#include "../module.hpp"
#ifdef MODULE_BATTERY

#include <giomm/dbusproxy.h>

class module_battery : public module {
	public:
		module_battery(sysbar *window, const bool &icon_on_start = false);

	private:
		bool show_percentage = false;
		Glib::RefPtr<Gio::DBus::Proxy> proxy;

		void setup();
		void on_properties_changed(
			const Gio::DBus::Proxy::MapChangedProperties &properties,
			const std::vector<Glib::ustring> &invalidated);
		void update_info(const std::string &property);
};

#endif
