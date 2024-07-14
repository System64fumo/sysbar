#pragma once
#ifdef MODULE_BATTERY

#include "../module.hpp"

#include <giomm/dbusproxy.h>

class module_battery : public module {
	public:
		module_battery(const config_bar &cfg, const bool &icon_on_start = false, const bool &clickable = false);

	private:
		Glib::RefPtr<Gio::DBus::Proxy> proxy;

		void setup();
		void on_properties_changed(
			const Gio::DBus::Proxy::MapChangedProperties &properties,
			const std::vector<Glib::ustring> &invalidated);
};

#endif
