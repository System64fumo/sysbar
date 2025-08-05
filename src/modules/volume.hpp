#pragma once
#include "../module.hpp"
#ifdef MODULE_VOLUME

#include "../wireplumber.hpp"
#include <gtkmm/scale.h>

class module_volume : public module {
	public:
		module_volume(sysbar*, const bool&);

	private:
		Glib::Dispatcher dispatcher_callback;
		Glib::Dispatcher dispatcher_load;
		Gtk::Scale scale_volume;
		Gtk::Image image_widget_icon;
		Gtk::Box box_widget;
		syshud_wireplumber sys_wp;

		std::map<int, std::string> volume_icons;
		std::vector<int> widget_layout = {0, 4, 4, 1};

		void update_info();
		void setup_widget();
		void load_wireplumber();
};

#endif
