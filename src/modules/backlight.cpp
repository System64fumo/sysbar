#include "../config_parser.hpp"
#include "backlight.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/inotify.h>
#include <thread>

module_backlight::module_backlight(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_backlight");
	image_icon.set_from_icon_name("brightness-display-symbolic");

	#ifdef CONFIG_FILE
	config_parser config(std::string(getenv("HOME")) + "/.config/sys64/bar/config.conf");

	if (config.available) {
		std::string cfg_bl_path = config.get_value("backlight", "path");
		if (cfg_bl_path != "empty")
			backlight_path = cfg_bl_path;

		std::string cfg_icon = config.get_value("backlight", "show-icon");
		if (cfg_icon != "true") {
			image_icon.hide();
			label_info.set_margin_end(config_main.size / 3);
		}

		std::string cfg_label = config.get_value("backlight", "show-label");
		if (cfg_label != "true") {
			label_info.hide();
		}
	}
	#endif

	get_backlight_path(backlight_path);

	// Read initial value
	label_info.set_text(std::to_string(get_brightness()));

	// Begin listening for changes
	dispatcher_callback.connect(sigc::mem_fun(*this, &module_backlight::update_info));
	std::thread monitor_thread([&]() {
		int inotify_fd = inotify_init();
		inotify_add_watch(inotify_fd, backlight_path.c_str(), IN_MODIFY);

		int last_brightness = get_brightness();
		char buffer[1024];

		while (true) {
			ssize_t ret = read(inotify_fd, buffer, 1024);
			(void)ret; // Return value does not matter

			brightness = get_brightness();
			if (brightness != last_brightness) {
				last_brightness = brightness;
				dispatcher_callback.emit();
			}
		}
	});
	monitor_thread.detach();
}

void module_backlight::update_info() {
	label_info.set_text(std::to_string(brightness));
}

void module_backlight::get_backlight_path(std::string custom_backlight_path) {
	if (custom_backlight_path != "") {
		backlight_path = custom_backlight_path;
		return;
	}
	else {
		std::string path = "/sys/class/backlight/";
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			if (std::filesystem::is_directory(entry.path())) {
				backlight_path = entry.path();
				return;
			}
		}
		std::cout << "Unable to automatically detect your backlight" << std::endl;
	}
}

int module_backlight::get_brightness() {
	std::lock_guard<std::mutex> lock(brightness_mutex);
	std::ifstream brightness_file(backlight_path + "/brightness");
	std::ifstream max_brightness_file(backlight_path + "/max_brightness");
	double brightness;
	double max_brightness;
	brightness_file >> brightness;
	max_brightness_file >> max_brightness;

	return (brightness / max_brightness) * 100;
}
