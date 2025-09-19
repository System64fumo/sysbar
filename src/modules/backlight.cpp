#include "backlight.hpp"

#include <fstream>
#include <filesystem>
#include <sys/inotify.h>
#include <thread>

module_backlight::module_backlight(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_backlight");
	image_icon.set_from_icon_name("brightness-display-symbolic");
	brightness_icons[0] = "display-brightness-low-symbolic";
	brightness_icons[1] = "display-brightness-medium-symbolic";
	brightness_icons[2] = "display-brightness-high-symbolic";

	std::string cfg_bl_path = win->config_main["backlight"]["path"];
	if (!cfg_bl_path.empty())
		backlight_path = cfg_bl_path;

	if (win->config_main["backlight"]["show-icon"] != "true") {
		image_icon.hide();
		label_info.set_margin_end(win->size / 3);
	}

	if (win->config_main["backlight"]["show-label"] != "true")
		label_info.hide();

	std::string cfg_layout = win->config_main["backlight"]["widget-layout"];
	if (!cfg_layout.empty()) {
		widget_layout.clear();
		for (char c : cfg_layout) {
			widget_layout.push_back(c - '0');
		}
	}

	min_brightness = std::stoi(win->config_main["backlight"]["min-level"]);
	high_brightness = std::stoi(win->config_main["backlight"]["high-level"]);

	// Setup
	get_backlight_path(backlight_path);
	if (backlight_path.empty()) // TODO: Maybe replace this with a test function?
		return;

	brightness = get_brightness();
	update_info();
	setup_widget();

	// TODO: Fix this, It's broken
	// Listen for changes
	// dispatcher_callback.connect(sigc::mem_fun(*this, &module_backlight::update_info));

	// std::thread([&]() {
	// 	int inotify_fd = inotify_init();
	// 	inotify_add_watch(inotify_fd, backlight_path.c_str(), IN_MODIFY);

	// 	int last_brightness = get_brightness();
	// 	char buffer[1024];

	// 	while (true) {
	// 		ssize_t ret = read(inotify_fd, buffer, 1024);
	// 		(void)ret; // Return value does not matter

	// 		brightness = get_brightness();
	// 		if (brightness == last_brightness)
	// 			break;

	// 		last_brightness = brightness;
	// 		dispatcher_callback.emit();
	// 	}
	// }).detach();
}

void module_backlight::update_info() {
	label_info.set_text(std::to_string(brightness));
	image_widget_icon.set_from_icon_name(brightness_icons[brightness / 35.0f]);
	// TODO: Prevent this from changing if currently being dragged
	//scale_backlight.set_value(brightness);
}

void module_backlight::on_scale_brightness_change() {
	brightness = std::lround(scale_backlight.get_value());

	// Probably not ideal to open and close the file every time..
	FILE* backlight_file = fopen((backlight_path + "/brightness").c_str(), "w");
	fprintf(backlight_file, "%d\n", std::lround(brightness * (max_brightness / 100.0f)));
	fclose(backlight_file);

	image_widget_icon.set_from_icon_name(brightness_icons[brightness / 35]);
}

void module_backlight::setup_widget() {
	Gtk::Box *box_widget = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);

	box_widget->get_style_context()->add_class("widget");
	box_widget->get_style_context()->add_class("widget_backlight");
	image_widget_icon.set_from_icon_name("brightness-display-symbolic");

	scale_backlight.set_hexpand(true);
	scale_backlight.set_vexpand(true);
	scale_backlight.set_value(brightness);
	scale_backlight.set_range(min_brightness, 100);
	scale_backlight.set_digits(0);

	if (high_brightness != 0)
		scale_backlight.add_mark(high_brightness, Gtk::PositionType::BOTTOM, "");

	scale_backlight.signal_value_changed().connect(sigc::mem_fun(*this, &module_backlight::on_scale_brightness_change));

	if (widget_layout[2] < widget_layout[3]) { // Vertical layout
		box_widget->set_orientation(Gtk::Orientation::VERTICAL);
		scale_backlight.set_orientation(Gtk::Orientation::VERTICAL);
		box_widget->append(scale_backlight);
		box_widget->append(image_widget_icon);
		scale_backlight.set_inverted(true);
	}
	else { // Horizontal layout
		box_widget->append(image_widget_icon);
		box_widget->append(scale_backlight);
	}

	win->sidepanel_end->grid_main.attach(*box_widget, widget_layout[0], widget_layout[1], widget_layout[2], widget_layout[3]);
}

void module_backlight::get_backlight_path(const std::string& custom_backlight_path) {
	if (custom_backlight_path != "") {
		backlight_path = custom_backlight_path;
		return;
	}
	std::string path = "/sys/class/backlight/";
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		if (std::filesystem::is_directory(entry.path())) {
			backlight_path = entry.path();
			return;
		}
	}
}

int module_backlight::get_brightness() {
	std::lock_guard<std::mutex> lock(brightness_mutex);
	std::ifstream brightness_file(backlight_path + "/brightness");
	std::ifstream max_brightness_file(backlight_path + "/max_brightness");
	brightness_file >> brightness_literal;
	max_brightness_file >> max_brightness;

	return (brightness_literal / max_brightness) * 100;
}
