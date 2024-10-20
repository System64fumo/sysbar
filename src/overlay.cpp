#include "window.hpp"

#include <gtk4-layer-shell.h>

/*
	Just as a heads up to whomever decides to work on this.
	You are about to see some horrible & nonsensical code.
	YOU HAVE BEEN WARNED!!
*/

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

void sysbar::setup_overlay_widgets() {
	int default_size_start = 350;
	int default_size_end = 350;

	#ifdef CONFIG_FILE
	if (config->available) {
		std::string cfg_sidepanel_start_size = config->get_value("main", "sidepanel-start-size");
		if (cfg_sidepanel_start_size != "empty")
			default_size_start = std::stoi(cfg_sidepanel_start_size);

		std::string cfg_sidepanel_end_size = config->get_value("main", "sidepanel-end-size");
		if (cfg_sidepanel_end_size != "empty")
			default_size_end = std::stoi(cfg_sidepanel_start_size);
	}
	#endif

	int size = (config_main.position % 2) ? monitor_geometry.height : monitor_geometry.width;

	grid_widgets_start.get_style_context()->add_class("grid_widgets_start");
	grid_widgets_end.get_style_context()->add_class("grid_widgets_end");

	scrolled_Window_start = Gtk::make_managed<Gtk::ScrolledWindow>();
	scrolled_Window_end = Gtk::make_managed<Gtk::ScrolledWindow>();
	scrolled_Window_start->set_child(grid_widgets_start);
	scrolled_Window_end->set_child(grid_widgets_end);
	scrolled_Window_start->set_kinetic_scrolling(false);
	scrolled_Window_end->set_kinetic_scrolling(false);

	if (config_main.position == 0) {
		scrolled_Window_start->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_valign(Gtk::Align::START);
		grid_widgets_start.set_valign(Gtk::Align::START);
		grid_widgets_end.set_valign(Gtk::Align::START);
	}
	else if (config_main.position == 1) {
		scrolled_Window_start->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_valign(Gtk::Align::END);
		scrolled_Window_start->set_halign(Gtk::Align::END);
		scrolled_Window_end->set_halign(Gtk::Align::END);
		grid_widgets_start.set_halign(Gtk::Align::END);
		grid_widgets_end.set_halign(Gtk::Align::END);
	}
	else if (config_main.position == 2) {
		scrolled_Window_start->set_valign(Gtk::Align::END);
		scrolled_Window_end->set_valign(Gtk::Align::END);
		grid_widgets_start.set_valign(Gtk::Align::END);
		grid_widgets_end.set_valign(Gtk::Align::END);
		scrolled_Window_end->set_halign(Gtk::Align::END);
	}
	else if (config_main.position == 3) {
		scrolled_Window_start->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_valign(Gtk::Align::END);
		grid_widgets_start.set_valign(Gtk::Align::START);
		grid_widgets_end.set_valign(Gtk::Align::START);
		grid_widgets_start.set_halign(Gtk::Align::START);
		grid_widgets_end.set_halign(Gtk::Align::START);
	}

	// Common
	bool vertical_layout = config_main.position % 2;
	if (vertical_layout) { // Vertical
		if (size / 2 > default_size_start) {
			grid_widgets_start.set_size_request(-1, default_size_start);
			scrolled_Window_start->set_halign(Gtk::Align::START);
		}
		else
			scrolled_Window_start->set_hexpand(true);

		if (size / 2 > default_size_end) {
			grid_widgets_end.set_size_request(-1, default_size_end);
			scrolled_Window_end->set_halign(Gtk::Align::END);
		}
		else
			scrolled_Window_end->set_hexpand(true);

		box_overlay.set_orientation(Gtk::Orientation::VERTICAL);
	}
	else { // Horizontal
		if (size / 2 > default_size_start) {
			grid_widgets_start.set_size_request(default_size_start, -1);
			scrolled_Window_start->set_halign(Gtk::Align::START);
		}
		else
			scrolled_Window_start->set_hexpand(true);

		if (size / 2 > default_size_end) {
			grid_widgets_end.set_size_request(default_size_end, -1);
			scrolled_Window_end->set_halign(Gtk::Align::END);
		}
		else
			scrolled_Window_end->set_hexpand(true);

		box_overlay.set_orientation(Gtk::Orientation::HORIZONTAL);
	}

	scrolled_Window_start->set_policy(
		vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER,
		!vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER);
	scrolled_Window_end->set_policy(
		vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER,
		!vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER);

	scrolled_Window_start->set_visible(false);
	scrolled_Window_end->set_visible(false);

	box_overlay.append(*scrolled_Window_start);
	box_overlay.append(*scrolled_Window_end);
}

// TODO: The whole gesture system needs a rework
// This is a horrible mess..
void sysbar::setup_gestures() {
	gesture_drag = Gtk::GestureDrag::create();
	gesture_drag->signal_drag_begin().connect([&](const double &x, const double &y) {
		gesture_touch = gesture_drag->get_current_event()->get_pointer_emulated();
		on_drag_start(x, y);

		if (!gesture_touch)
			gesture_drag->reset();
	});
	gesture_drag->signal_drag_update().connect(sigc::mem_fun(*this, &sysbar::on_drag_update));
	gesture_drag->signal_drag_end().connect(sigc::mem_fun(*this, &sysbar::on_drag_stop));
	add_controller(gesture_drag);

	gesture_drag_overlay = Gtk::GestureDrag::create();
	gesture_drag_overlay->signal_drag_begin().connect([&](const double &x, const double &y) {
		gesture_touch = gesture_drag_overlay->get_current_event()->get_pointer_emulated();
		if (!gesture_touch) {
			gesture_drag_overlay->reset();
			return;
		}

		on_drag_start(x, y);
		on_drag_update(0, 0);
	});
	gesture_drag_overlay->signal_drag_update().connect(sigc::mem_fun(*this, &sysbar::on_drag_update));
	gesture_drag_overlay->signal_drag_end().connect(sigc::mem_fun(*this, &sysbar::on_drag_stop));
	overlay_window.add_controller(gesture_drag_overlay);
}

void sysbar::on_drag_start(const double &x, const double &y) {
	overlay_window.show();

	scrolled_Window_start->set_valign(Gtk::Align::START);
	scrolled_Window_end->set_valign(Gtk::Align::START);

	if (config_main.position % 2) {
		sliding_start_widget = y < monitor_geometry.height / 2;
		initial_size_start = scrolled_Window_start->get_width();
		initial_size_end = scrolled_Window_end->get_width();
	}
	else {
		sliding_start_widget = x < monitor_geometry.width / 2;
		initial_size_start = scrolled_Window_start->get_height();
		initial_size_end = scrolled_Window_end->get_height();
	}

	if (initial_size_start != 0 || initial_size_end != 0) {
		if (config_main.position % 2) {
			initial_size_start = grid_widgets_start.get_allocated_width();
			initial_size_end = grid_widgets_end.get_allocated_width();
		}
		else {
			initial_size_start = grid_widgets_start.get_allocated_height();
			initial_size_end = grid_widgets_end.get_allocated_height();
		}
	}

	scrolled_Window_start->set_visible(sliding_start_widget);
	scrolled_Window_end->set_visible(!sliding_start_widget);
}

void sysbar::on_drag_update(const double &x, const double &y) {
	double width = -1;
	double height = -1;
	double initial_size = sliding_start_widget ? initial_size_start : initial_size_end;
	Gtk::ScrolledWindow* scrolled_Window = sliding_start_widget ? scrolled_Window_start : scrolled_Window_end;

	if (config_main.position == 0)
		height = std::max(0.0, y + initial_size);
	else if (config_main.position == 1)
		width = std::max(0.0, -x + initial_size);
	else if (config_main.position == 2)
		height = std::max(0.0, -y + initial_size);
	else if (config_main.position == 3)
		width = std::max(0.0, x + initial_size);

	scrolled_Window->set_size_request(width, height);
}

// Like can you actually even read and understand what any of this does?
// No.
// TODO: Clean this section up in particular
void sysbar::on_drag_stop(const double &x, const double &y) {
	double size = 0;
	double initial_size = sliding_start_widget ? initial_size_start : initial_size_end;
	double size_threshold = sliding_start_widget ? grid_widgets_start.get_allocated_height() : grid_widgets_end.get_allocated_height();;
	Gtk::ScrolledWindow* scrolled_Window = sliding_start_widget ? scrolled_Window_start : scrolled_Window_end;
	Gtk::Align align = Gtk::Align::START;

	if (config_main.position == 0) {
		size = std::max(0.0, y + initial_size);
		align = Gtk::Align::FILL;
	}
	else if (config_main.position == 1) {
		size = std::max(0.0, -x + initial_size);
		align = Gtk::Align::END;
	}
	else if (config_main.position == 2) {
		size = std::max(0.0, -y + initial_size);
		align = Gtk::Align::END;
	}
	else if (config_main.position == 3) {
		size = std::max(0.0, x + initial_size);
		align = Gtk::Align::FILL;
	}

	// Ensure size is not negative
	size = std::max(0.0, size);
	bool passed_threshold = size > (size_threshold * 0.75);

	if (!((passed_threshold && gesture_touch) || (size == 0 && !gesture_touch)))
		scrolled_Window->hide();

	if (config_main.position % 2)
		scrolled_Window->set_halign(align);
	else
		scrolled_Window->set_valign(align);

	scrolled_Window->set_size_request(-1, -1);

	if (!scrolled_Window_start->get_visible() && !scrolled_Window_end->get_visible())
		overlay_window.hide();
}
