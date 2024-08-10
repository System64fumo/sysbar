#pragma once
#include "../module.hpp"
#ifdef MODULE_VOLUME

#include "../wireplumber.hpp"

class module_volume : public module {
	public:
		module_volume(sysbar *window, const bool &icon_on_start = true);

	private:
		sysvol_wireplumber *sys_wp;
		Glib::Dispatcher dispatcher_callback;
		std::map<int, std::string> volume_icons;

		void update_info();
};

#endif
