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
#include "modules/bluetooth.hpp"
#include "modules/controls.hpp"
#include "modules/cellular.hpp"

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

	gdk_monitor_get_geometry(monitor, &monitor_geometry);


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
	setup_gestures();

	// Setup the controls widget
	#ifdef MODULE_CONTROLS
	setup_controls();
	#endif

	// Load modules
	load_modules(config_main.m_start, box_start);
	load_modules(config_main.m_center, box_center);
	load_modules(config_main.m_end, box_end);
}

#ifdef MODULE_CONTROLS
void sysbar::setup_controls() {
	if (config_main.m_start.find("controls") != std::string::npos) {
		box_controls = Gtk::make_managed<module_controls>(this, false);
		box_start.append(*box_controls);
	}
	else if (config_main.m_end.find("controls") != std::string::npos) {
		box_controls = Gtk::make_managed<module_controls>(this, false);
		box_end.append(*box_controls);
	}
}
#endif

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

		#ifdef MODULE_MPRIS
		else if (module_name == "mpris")
			my_module = Gtk::make_managed<module_mpris>(this, false);
		#endif

		#ifdef MODULE_BLUETOOTH
		else if (module_name == "bluetooth")
			my_module = Gtk::make_managed<module_bluetooth>(this, false);
		#endif

		#ifdef MODULE_CELLULAR
		else if (module_name == "cellular")
			my_module = Gtk::make_managed<module_cellular>(this, false);
		#endif

		#ifdef MODULE_CONTROLS
		else if (module_name == "controls")
			continue;
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
	// TODO: Clean this mess up
	// All of this is temporary because i'm lazy to write good code
	// Thank me later
	box_widgets_start = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
	box_widgets_end = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);

	scrolled_Window_start = Gtk::make_managed<Gtk::ScrolledWindow>();
	scrolled_Window_end = Gtk::make_managed<Gtk::ScrolledWindow>();

	scrolled_Window_start->set_child(*box_widgets_start);
	scrolled_Window_end->set_child(*box_widgets_end);

	if (config_main.position == 0) {
		box_widgets_start->set_size_request(300, -1);
		box_widgets_end->set_size_request(300, -1);
		box_overlay.set_orientation(Gtk::Orientation::HORIZONTAL);
		scrolled_Window_start->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_valign(Gtk::Align::START);
		box_widgets_start->set_valign(Gtk::Align::START);
		box_widgets_end->set_valign(Gtk::Align::START);
		scrolled_Window_start->set_halign(Gtk::Align::START);
		scrolled_Window_end->set_halign(Gtk::Align::END);
		scrolled_Window_start->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::EXTERNAL);
		scrolled_Window_end->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::EXTERNAL);
	}
	else if (config_main.position == 1) {
		box_widgets_start->set_size_request(-1, 300);
		box_widgets_end->set_size_request(-1, 300);
		box_overlay.set_orientation(Gtk::Orientation::VERTICAL);
		scrolled_Window_start->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_valign(Gtk::Align::END);
		scrolled_Window_start->set_halign(Gtk::Align::END);
		scrolled_Window_end->set_halign(Gtk::Align::END);
		box_widgets_start->set_halign(Gtk::Align::END);
		box_widgets_end->set_halign(Gtk::Align::END);
		scrolled_Window_start->set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::NEVER);
		scrolled_Window_end->set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::NEVER);
	}
	else if (config_main.position == 2) {
		box_widgets_start->set_size_request(300, -1);
		box_widgets_end->set_size_request(300, -1);
		box_overlay.set_orientation(Gtk::Orientation::HORIZONTAL);
		scrolled_Window_start->set_valign(Gtk::Align::END);
		scrolled_Window_end->set_valign(Gtk::Align::END);
		box_widgets_start->set_valign(Gtk::Align::END);
		box_widgets_end->set_valign(Gtk::Align::END);
		scrolled_Window_start->set_halign(Gtk::Align::START);
		scrolled_Window_end->set_halign(Gtk::Align::END);
		scrolled_Window_start->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::EXTERNAL);
		scrolled_Window_end->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::EXTERNAL);
	}
	else if (config_main.position == 3) {
		box_widgets_start->set_size_request(-1, 300);
		box_widgets_end->set_size_request(-1, 300);
		box_overlay.set_orientation(Gtk::Orientation::VERTICAL);
		scrolled_Window_start->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_valign(Gtk::Align::END);
		box_widgets_start->set_valign(Gtk::Align::START);
		box_widgets_end->set_valign(Gtk::Align::START);
		box_widgets_start->set_halign(Gtk::Align::START);
		box_widgets_end->set_halign(Gtk::Align::START);
		scrolled_Window_start->set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::NEVER);
		scrolled_Window_end->set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::NEVER);
	}

	scrolled_Window_start->set_visible(false);
	scrolled_Window_end->set_visible(false);

	box_overlay.append(*scrolled_Window_start);
	box_overlay.append(*scrolled_Window_end);
}

void sysbar::setup_overlay() {
	overlay_window.set_name("sysbar_overlay");
	gtk_layer_init_for_window(overlay_window.gobj());
	gtk_layer_set_namespace(overlay_window.gobj(), "sysbar-overlay");
	gtk_layer_set_layer(overlay_window.gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_set_keyboard_mode(overlay_window.gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
	gtk_layer_set_monitor(overlay_window.gobj(), monitor);

	gtk_layer_set_anchor(overlay_window.gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
	gtk_layer_set_anchor(overlay_window.gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, true);
	gtk_layer_set_anchor(overlay_window.gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
	gtk_layer_set_anchor(overlay_window.gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);

	overlay_window.set_child(box_overlay);
}

void sysbar::setup_gestures() {
	gesture_drag = Gtk::GestureDrag::create();
	gesture_drag->signal_drag_begin().connect(sigc::mem_fun(*this, &sysbar::on_drag_start));
	gesture_drag->signal_drag_update().connect(sigc::mem_fun(*this, &sysbar::on_drag_update));
	gesture_drag->signal_drag_end().connect(sigc::mem_fun(*this, &sysbar::on_drag_stop));
	add_controller(gesture_drag);
}

void sysbar::on_drag_start(const double &x, const double &y) {
	overlay_window.show();
	if (config_main.position % 2)
		sliding_start_widget = y < monitor_geometry.height / 2;
	else
		sliding_start_widget = x < monitor_geometry.width / 2;

	if (sliding_start_widget)
		scrolled_Window_start->show();
	else
		scrolled_Window_end->show();
}

void sysbar::on_drag_update(const double &x, const double &y) {
	double width = -1;
	double height = -1;

	if (config_main.position == 0)
		height = std::max(0.0, y);
	else if (config_main.position == 1)
		width = std::max(0.0, -x);
	else if (config_main.position == 2)
		height = std::max(0.0, -y);
	else if (config_main.position == 3)
		width = std::max(0.0, x);

	if (sliding_start_widget)
		scrolled_Window_start->set_size_request(width, height);
	else
		scrolled_Window_end->set_size_request(width, height);
}

void sysbar::on_drag_stop(const double &x, const double &y) {
	// TODO: Clean all of this up
	// Confident this can be done better
	double size;
	Gtk::Align align;

	if (config_main.position == 0) {
		size = y;
		align = Gtk::Align::START;
	}
	else if (config_main.position == 1) {
		size = -x;
		align = Gtk::Align::END;
	}
	else if (config_main.position == 2) {
		size = -y;
		align = Gtk::Align::END;
	}
	else if (config_main.position == 3) {
		size = x;
		align = Gtk::Align::START;
	}

	bool click_show;
	Gtk::ScrolledWindow *scrolled_Window = sliding_start_widget ? scrolled_Window_start : scrolled_Window_end;
	if (config_main.position % 2) // Vertical
		click_show = (x == 0 && (scrolled_Window->get_halign() != Gtk::Align::FILL));
	else
		click_show = (y == 0 && (scrolled_Window->get_valign() != Gtk::Align::FILL));

	if (size <= 0)
		size = 0;

	bool passed_threashold = size > monitor_geometry.height / 5;

	if (passed_threashold || click_show) {
		if (config_main.position % 2)
			scrolled_Window->set_halign(Gtk::Align::FILL);
		else
			scrolled_Window->set_valign(Gtk::Align::FILL);
	}
	else {
		if (config_main.position % 2)
			scrolled_Window->set_halign(align);
		else
			scrolled_Window->set_valign(align);
		scrolled_Window->hide();
	}

	scrolled_Window->set_size_request(-1, -1);

	if (!scrolled_Window_start->get_visible() && !scrolled_Window_end->get_visible())
		overlay_window.hide();
}

extern "C" {
	sysbar *sysbar_create(const config_bar &cfg) {
		return new sysbar(cfg);
	}
	void sysbar_signal(sysbar *window, int signal) {
		window->handle_signal(signal);
	}
}
