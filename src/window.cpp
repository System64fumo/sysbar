#include "window.hpp"

#include "modules/clock.hpp"
#include "modules/weather.hpp"
#include "modules/tray.hpp"
#include "modules/hyprland.hpp"
#include "modules/volume.hpp"
#include "modules/network.hpp"
#include "modules/battery.hpp"
#include "modules/notifications.hpp"
#include "modules/performance.hpp"
#include "modules/taskbar.hpp"
#include "modules/backlight.hpp"
#include "modules/mpris.hpp"

#include <gtk4-layer-shell.h>
#include <filesystem>

sysbar::sysbar(const config_bar &cfg) {
	config_main = cfg;

	// Get main monitor
	GdkDisplay *display = gdk_display_get_default();
	GListModel *monitors = gdk_display_get_monitors(display);

	int monitorCount = g_list_model_get_n_items(monitors);

	if (config_main.main_monitor < 0)
		config_main.main_monitor = 0;
	else if (config_main.main_monitor >= monitorCount)
		config_main.main_monitor = monitorCount - 1;

	monitor = GDK_MONITOR(g_list_model_get_item(monitors, config_main.main_monitor));

	// Initialize layer shell
	gtk_layer_init_for_window(gobj());
	gtk_layer_set_namespace(gobj(), "sysbar");
	gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_TOP);
	gtk_layer_set_exclusive_zone(gobj(), config_main.size);
	gtk_layer_set_monitor(gobj(), monitor);

	gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
	gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, true);
	gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
	gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);

	Gtk::RevealerTransitionType transition_type = Gtk::RevealerTransitionType::SLIDE_DOWN;

	switch (config_main.position) {
		case 0:
			transition_type = Gtk::RevealerTransitionType::SLIDE_DOWN;
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, false);
			width = -1;
			height = config_main.size;
			break;
		case 1:
			transition_type = Gtk::RevealerTransitionType::SLIDE_LEFT;
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_LEFT, false);
			width = config_main.size;
			height = -1;
			break;
		case 2:
			transition_type = Gtk::RevealerTransitionType::SLIDE_UP;
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
			width = -1;
			height = config_main.size;
			break;
		case 3:
			transition_type = Gtk::RevealerTransitionType::SLIDE_RIGHT;
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, false);
			width = config_main.size;
			height = -1;
			break;
	}

	// Initialize
	set_name("sysbar");
	set_hide_on_close(true);
	set_default_size(width, height);
	set_child(revealer_box);
	revealer_box.set_child(centerbox_main);
	revealer_box.set_transition_type(transition_type);
	revealer_box.set_transition_duration(1000);
	centerbox_main.get_style_context()->add_class("centerbox_main");
	centerbox_main.set_start_widget(box_start);
	centerbox_main.set_center_widget(box_center);
	centerbox_main.set_end_widget(box_end);
	show();
	revealer_box.set_reveal_child(true);

	// Set orientation
	if (config_main.position % 2) {
		Gtk::Orientation orientation = Gtk::Orientation::VERTICAL;
		centerbox_main.set_orientation(orientation);
		box_start.set_orientation(orientation);
		box_center.set_orientation(orientation);
		box_end.set_orientation(orientation);
	}

	// Load custom css
	std::string style_path;
	if (std::filesystem::exists(std::string(getenv("HOME")) + "/.config/sys64/bar/style.css"))
		style_path = std::string(getenv("HOME")) + "/.config/sys64/bar/style.css";
	else if (std::filesystem::exists("/usr/share/sys64/bar/style.css"))
		style_path = "/usr/share/sys64/bar/style.css";
	else
		style_path = "/usr/local/share/sys64/bar/style.css";

	css_loader css(style_path, this);

	// Overlay
	setup_overlay();
	setup_popovers();

	// Load modules
	load_modules(config_main.m_start, box_start);
	load_modules(config_main.m_center, box_center);
	load_modules(config_main.m_end, box_end);
}

void sysbar::load_modules(const std::string &modules, Gtk::Box &box) {
	std::istringstream iss(modules);
	std::string module_name;

	while (std::getline(iss, module_name, ',')) {
		module *my_module;

		if (config_main.verbose)
			std::printf("Loading module: %s\n", module_name.c_str());

		if (false)
			std::printf("You're not supposed to see this\n");

		#ifdef MODULE_CLOCK
		else if (module_name == "clock")
			my_module = Gtk::make_managed<module_clock>(this, true);
		#endif

		#ifdef MODULE_WEATHER
		else if (module_name == "weather")
			my_module = Gtk::make_managed<module_weather>(this, true);
		#endif

		#ifdef MODULE_TRAY
		else if (module_name == "tray")
			my_module = Gtk::make_managed<module_tray>(this, true);
		#endif

		#ifdef MODULE_HYPRLAND
		else if (module_name == "hyprland")
			my_module = Gtk::make_managed<module_hyprland>(this, false);
		#endif

		#ifdef MODULE_VOLUME
		else if (module_name == "volume")
			my_module = Gtk::make_managed<module_volume>(this, false);
		#endif

		#ifdef MODULE_NETWORK
		else if (module_name == "network")
			my_module = Gtk::make_managed<module_network>(this, false);
		#endif

		#ifdef MODULE_BATTERY
		else if (module_name == "battery")
			my_module = Gtk::make_managed<module_battery>(this, false);
		#endif

		#ifdef MODULE_NOTIFICATION
		else if (module_name == "notification")
			my_module = Gtk::make_managed<module_notifications>(this, false);
		#endif

		#ifdef MODULE_PERFORMANCE
		else if (module_name == "performance")
			my_module = Gtk::make_managed<module_performance>(this, false);
		#endif

		#ifdef MODULE_TASKBAR
		else if (module_name == "taskbar")
			my_module = Gtk::make_managed<module_taskbar>(this, false);
		#endif

		#ifdef MODULE_BACKLIGHT
		else if (module_name == "backlight")
			my_module = Gtk::make_managed<module_backlight>(this, false);
		#endif

		#ifdef MODULE_BACKLIGHT
		else if (module_name == "mpris")
			my_module = Gtk::make_managed<module_mpris>(this, false);
		#endif

		else {
			std::fprintf(stderr, "Unknown module: %s\n", module_name.c_str());
			return;
		}

		box.append(*my_module);
	}
}

void sysbar::handle_signal(const int &signum) {
	Glib::signal_idle().connect([this, signum]() {
		switch (signum) {
			case 10: // Show
				revealer_box.set_reveal_child(true);
				gtk_layer_set_exclusive_zone(gobj(), config_main.size);
				set_default_size(width, height);
				break;

			case 12: // Hide
				revealer_box.set_reveal_child(false);
				gtk_layer_set_exclusive_zone(gobj(), 0);
				set_default_size(1, 1);
				break;

			case 34: // Toggle
				if (revealer_box.get_reveal_child())
					handle_signal(12);
				else
					handle_signal(10);
				break;
		}
		return false;
	});
}

void sysbar::setup_popovers() {
	// TODO: Figure out which box should be shown
	for (int i = 0; i < 2; i++) {
		Gtk::Box *box_container;
		Gtk::Box *box_widget;

		switch (i) {
			case 0:
				box_container = &box_start;
				box_widgets_start = Gtk::make_managed<Gtk::Box>();
				box_widget = box_widgets_start;
				box_widgets_start->set_halign(Gtk::Align::START);
				box_widgets_start->get_style_context()->add_class("box_widgets_start");
				break;
			case 1:
				box_container = &box_end;
				box_widgets_end = Gtk::make_managed<Gtk::Box>();
				box_widget = box_widgets_end;
				box_widgets_end->set_halign(Gtk::Align::END);
				box_widgets_end->get_style_context()->add_class("box_widgets_end");
				break;
		}

		box_widget->set_valign(Gtk::Align::START);
		box_widget->set_orientation(Gtk::Orientation::VERTICAL);
		box_widget->set_size_request(300, -1);
		box_widget->set_hexpand(true);
		box_widget->hide();

		Glib::RefPtr<Gtk::GestureClick> click_gesture = Gtk::GestureClick::create();
		click_gesture->set_button(GDK_BUTTON_PRIMARY);
		click_gesture->signal_pressed().connect([&, box_widget](const int &n_press, const double &x, const double &y) {
			overlay_window.show();

			if (box_widget->get_visible())
				box_widget->hide();
			else
				box_widget->show();

			if (!box_widgets_start->get_visible() && !box_widgets_end->get_visible())
				overlay_window.hide();

		});
		box_container->add_controller(click_gesture);
		box_overlay.append(*box_widget);
	}
}

void sysbar::setup_overlay() {
	overlay_window.set_name("sysbar_overlay");
	gtk_layer_init_for_window(overlay_window.gobj());
	gtk_layer_set_namespace(overlay_window.gobj(), "sysbar-overlay");
	gtk_layer_set_layer(overlay_window.gobj(), GTK_LAYER_SHELL_LAYER_TOP);
	gtk_layer_set_exclusive_zone(overlay_window.gobj(), config_main.size);
	gtk_layer_set_monitor(overlay_window.gobj(), monitor);

	gtk_layer_set_anchor(overlay_window.gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
	gtk_layer_set_anchor(overlay_window.gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, true);
	gtk_layer_set_anchor(overlay_window.gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
	gtk_layer_set_anchor(overlay_window.gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);

	overlay_window.set_child(box_overlay);
}

extern "C" {
	sysbar *sysbar_create(const config_bar &cfg) {
		return new sysbar(cfg);
	}
	void sysbar_signal(sysbar *window, int signal) {
		window->handle_signal(signal);
	}
}
