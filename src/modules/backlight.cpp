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

	// Setup
	get_backlight_path(backlight_path);
	setup_widget();
	brightness = get_brightness();
	update_info();
	scale_backlight.set_value(brightness_literal); // Temporary

	// Listen for changes
	dispatcher_callback.connect(sigc::mem_fun(*this, &module_backlight::update_info));
	scale_backlight.signal_value_changed().connect(sigc::mem_fun(*this, &module_backlight::on_scale_brightness_change));

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
	// TODO: Prevent this from changing if currently being dragged
	//scale_backlight.set_value(brightness);
}

void module_backlight::on_scale_brightness_change() {
	int scale_val = (int)scale_backlight.get_value();
	if (scale_val != brightness) {
		std::ofstream backlight_file(backlight_path + "/brightness", std::ios::trunc);
		backlight_file << scale_val;
	}
}

void module_backlight::setup_widget() {
	auto container = static_cast<Gtk::Box*>(win->popover_end->get_child());
	container->append(scale_backlight);
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
	double max_brightness;
	brightness_file >> brightness_literal;
	max_brightness_file >> max_brightness;

	scale_backlight.set_range(0, max_brightness);
	return (brightness_literal / max_brightness) * 100;
}
