#pragma once
#include "../module.hpp"
#ifdef MODULE_BACKLIGHT

#include <mutex>
#include <glibmm/dispatcher.h>

class module_backlight : public module {
	public:
		module_backlight(sysbar *window, const bool &icon_on_start = true);

	private:
		int brightness;
		int inotify_fd;
		std::string backlight_path;
		std::mutex brightness_mutex;
		Glib::Dispatcher dispatcher_callback;

		void update_info();
		void get_backlight_path(std::string custom_backlight_path);
		int get_brightness();
};

#endif
