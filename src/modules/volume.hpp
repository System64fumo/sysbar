#pragma once
#include "../module.hpp"
#ifdef MODULE_VOLUME

#include "../wireplumber.hpp"

class module_volume : public module {
	public:
		module_volume(const config_bar &cfg, const bool &icon_on_start = false, const bool &clickable = false);

	private:
		sysvol_wireplumber *sys_wp;
		Glib::Dispatcher dispatcher_callback;
		std::map<int, std::string> volume_icons;

		void update_info();
};

#endif
