#pragma once
#include "../module.hpp"
#ifdef MODULE_VOLUME

#include "../wireplumber.hpp"
#include <gtkmm/scale.h>

class module_volume : public module {
	public:
		module_volume(sysbar *window, const bool &icon_on_start = true);

	private:
		syshud_wireplumber *sys_wp;
		Glib::Dispatcher dispatcher_callback;
		Gtk::Scale scale_volume;
		std::map<int, std::string> volume_icons;

		void update_info();
		void setup_widget();
};

#endif
