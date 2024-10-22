#pragma once
#include "../module.hpp"
#ifdef MODULE_BACKLIGHT

#include <mutex>
#include <glibmm/dispatcher.h>
#include <gtkmm/scale.h>
#include <fstream>

class module_backlight : public module {
	public:
		module_backlight(sysbar*, const bool&);

	private:
		std::ofstream brightness_file;
		std::ifstream max_brightness_file;
		int brightness;
		int max_brightness;
		double brightness_literal;
		int inotify_fd;
		std::string backlight_path;
		std::mutex brightness_mutex;
		std::map<int, std::string> volume_brightness;

		Gtk::Scale scale_backlight;
		Gtk::Image image_widget_icon;
		Glib::Dispatcher dispatcher_callback;

		void update_info();
		void on_scale_brightness_change();
		void get_backlight_path(const std::string&);
		int get_brightness();
		void setup_widget();
};

#endif
