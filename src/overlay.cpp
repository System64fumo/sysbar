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


	std::string cfg_sidepanel_start_size = config_main["main"]["sidepanel-start-size"];
	if (!cfg_sidepanel_start_size.empty())
		default_size_start = std::stoi(cfg_sidepanel_start_size);

	std::string cfg_sidepanel_end_size = config_main["main"]["sidepanel-end-size"];
	if (!cfg_sidepanel_end_size.empty())
		default_size_end = std::stoi(cfg_sidepanel_start_size);


	int size = (position % 2) ? monitor_geometry.height : monitor_geometry.width;

	grid_widgets_start.get_style_context()->add_class("grid_widgets_start");
	grid_widgets_end.get_style_context()->add_class("grid_widgets_end");

	box_widgets_start.get_style_context()->add_class("sidepanel_start");
	box_widgets_end.get_style_context()->add_class("sidepanel_end");

	scrolled_Window_start.set_child(box_widgets_start);
	box_widgets_start.append(grid_widgets_start);
	grid_widgets_start.set_vexpand_set();
	scrolled_Window_end.set_child(box_widgets_end);
	box_widgets_end.append(grid_widgets_end);
	grid_widgets_end.set_vexpand_set();
	scrolled_Window_start.set_kinetic_scrolling(false);
	scrolled_Window_end.set_kinetic_scrolling(false);

	if (position == 0) {
		overlay_window.get_style_context()->add_class("position_top");
		scrolled_Window_start.set_valign(Gtk::Align::START);
		scrolled_Window_end.set_valign(Gtk::Align::START);
		box_widgets_start.set_orientation(Gtk::Orientation::VERTICAL);
		box_widgets_end.set_orientation(Gtk::Orientation::VERTICAL);
		grid_widgets_start.set_valign(Gtk::Align::START);
		grid_widgets_end.set_valign(Gtk::Align::START);
		box_widgets_start.set_valign(Gtk::Align::START);
		box_widgets_end.set_valign(Gtk::Align::START);
	}
	else if (position == 1) {
		overlay_window.get_style_context()->add_class("position_right");
		scrolled_Window_start.set_valign(Gtk::Align::START);
		scrolled_Window_end.set_valign(Gtk::Align::END);
		scrolled_Window_start.set_halign(Gtk::Align::END);
		scrolled_Window_end.set_halign(Gtk::Align::END);
		grid_widgets_start.set_halign(Gtk::Align::END);
		grid_widgets_end.set_halign(Gtk::Align::END);
		box_widgets_start.set_valign(Gtk::Align::START);
		box_widgets_end.set_valign(Gtk::Align::START);
	}
	else if (position == 2) {
		overlay_window.get_style_context()->add_class("position_bottom");
		scrolled_Window_start.set_valign(Gtk::Align::END);
		scrolled_Window_end.set_valign(Gtk::Align::END);
		box_widgets_start.set_orientation(Gtk::Orientation::VERTICAL);
		box_widgets_end.set_orientation(Gtk::Orientation::VERTICAL);
		grid_widgets_start.set_valign(Gtk::Align::END);
		grid_widgets_end.set_valign(Gtk::Align::END);
		box_widgets_start.set_valign(Gtk::Align::END);
		box_widgets_end.set_valign(Gtk::Align::END);
	}
	else if (position == 3) {
		overlay_window.get_style_context()->add_class("position_left");
		scrolled_Window_start.set_valign(Gtk::Align::START);
		scrolled_Window_end.set_valign(Gtk::Align::END);
		grid_widgets_start.set_valign(Gtk::Align::START);
		grid_widgets_end.set_valign(Gtk::Align::START);
		grid_widgets_start.set_halign(Gtk::Align::START);
		grid_widgets_end.set_halign(Gtk::Align::START);
		box_widgets_start.set_valign(Gtk::Align::END);
		box_widgets_end.set_valign(Gtk::Align::END);
	}

	// Common
	bool vertical_layout = position % 2;
	if (vertical_layout) { // Vertical
		if (size / 2 > default_size_start) {
			grid_widgets_start.set_size_request(-1, default_size_start);
			scrolled_Window_start.set_halign(Gtk::Align::START);
		}
		else
			scrolled_Window_start.set_hexpand(true);

		if (size / 2 > default_size_end) {
			grid_widgets_end.set_size_request(-1, default_size_end);
			scrolled_Window_end.set_halign(Gtk::Align::END);
		}
		else
			scrolled_Window_end.set_hexpand(true);

		box_overlay.set_orientation(Gtk::Orientation::VERTICAL);
	}
	else { // Horizontal
		if (size / 2 > default_size_start) {
			grid_widgets_start.set_size_request(default_size_start, -1);
			scrolled_Window_start.set_halign(Gtk::Align::START);
		}
		else {
			scrolled_Window_start.set_hexpand(true);
			scrolled_Window_start.set_halign(Gtk::Align::FILL);
		}

		if (size / 2 > default_size_end) {
			grid_widgets_end.set_size_request(default_size_end, -1);
			scrolled_Window_end.set_halign(Gtk::Align::END);
		}
		else {
			scrolled_Window_end.set_hexpand(true);
			scrolled_Window_end.set_halign(Gtk::Align::FILL);
		}

		box_overlay.set_orientation(Gtk::Orientation::HORIZONTAL);
	}

	scrolled_Window_start.set_policy(
		vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER,
		!vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER);
	scrolled_Window_end.set_policy(
		vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER,
		!vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER);

	scrolled_Window_start.set_visible(false);
	scrolled_Window_end.set_visible(false);

	box_overlay.append(scrolled_Window_start);
	box_overlay.append(scrolled_Window_end);
}

// TODO: The whole gesture system needs a rework
// This is a horrible mess..
void sysbar::setup_gestures() {
	gesture_drag = Gtk::GestureDrag::create();
	gesture_drag->signal_drag_begin().connect([&](const double& x, const double& y) {
		gesture_touch = gesture_drag->get_current_event()->get_pointer_emulated();
		on_drag_start(x, y);

		if (!gesture_touch)
			gesture_drag->reset();
	});
	gesture_drag->signal_drag_update().connect(sigc::mem_fun(*this, &sysbar::on_drag_update));
	gesture_drag->signal_drag_end().connect(sigc::mem_fun(*this, &sysbar::on_drag_stop));
	add_controller(gesture_drag);

	gesture_drag_start = Gtk::GestureDrag::create();
	gesture_drag_start->signal_drag_begin().connect([&](const double& x, const double& y) {
		gesture_touch = gesture_drag_start->get_current_event()->get_pointer_emulated();
		if (!gesture_touch) {
			gesture_drag_start->reset();
			return;
		}

		on_drag_start(0, 0);
		on_drag_update(0, 0);
	});
	gesture_drag_start->signal_drag_update().connect(sigc::mem_fun(*this, &sysbar::on_drag_update));
	gesture_drag_start->signal_drag_end().connect(sigc::mem_fun(*this, &sysbar::on_drag_stop));
	scrolled_Window_start.add_controller(gesture_drag_start);

	gesture_drag_end = Gtk::GestureDrag::create();
	gesture_drag_end->signal_drag_begin().connect([&](const double& x, const double& y) {
		gesture_touch = gesture_drag_end->get_current_event()->get_pointer_emulated();
		if (!gesture_touch) {
			gesture_drag_end->reset();
			return;
		}

		on_drag_start(monitor_geometry.height, monitor_geometry.width);
		on_drag_update(0, 0);
	});
	gesture_drag_end->signal_drag_update().connect(sigc::mem_fun(*this, &sysbar::on_drag_update));
	gesture_drag_end->signal_drag_end().connect(sigc::mem_fun(*this, &sysbar::on_drag_stop));
	scrolled_Window_end.add_controller(gesture_drag_end);
}

void sysbar::on_drag_start(const double& x, const double& y) {
	overlay_window.show();

	scrolled_Window_start.set_valign(Gtk::Align::START);
	scrolled_Window_end.set_valign(Gtk::Align::START);

	if (position == 0) {
		scrolled_Window_start.set_valign(Gtk::Align::START);
		scrolled_Window_end.set_valign(Gtk::Align::START);
	}
	else if (position == 1) {
		scrolled_Window_start.set_halign(Gtk::Align::END);
		scrolled_Window_end.set_halign(Gtk::Align::END);
	}
	else if (position == 2) {
		scrolled_Window_start.set_valign(Gtk::Align::END);
		scrolled_Window_end.set_valign(Gtk::Align::END);
	}
	else if (position == 3) {
		scrolled_Window_start.set_halign(Gtk::Align::START);
		scrolled_Window_end.set_halign(Gtk::Align::START);
	}

	if (position % 2) {
		sliding_start_widget = y < monitor_geometry.height / 2;
		initial_size_start = scrolled_Window_start.get_width();
		initial_size_end = scrolled_Window_end.get_width();
	}
	else {
		sliding_start_widget = x < monitor_geometry.width / 2;
		initial_size_start = scrolled_Window_start.get_height();
		initial_size_end = scrolled_Window_end.get_height();
	}

	if (initial_size_start != 0 || initial_size_end != 0) {
		if (position % 2) {
			initial_size_start = grid_widgets_start.get_allocated_width();
			initial_size_end = grid_widgets_end.get_allocated_width();
		}
		else {
			initial_size_start = grid_widgets_start.get_allocated_height();
			initial_size_end = grid_widgets_end.get_allocated_height();
		}
	}

	scrolled_Window_start.set_visible(sliding_start_widget);
	scrolled_Window_end.set_visible(!sliding_start_widget);
}

void sysbar::on_drag_update(const double& x, const double& y) {
	double drag_width = -1;
	double drag_height = -1;
	double initial_size = sliding_start_widget ? initial_size_start : initial_size_end;
	Gtk::ScrolledWindow* scrolled_Window = sliding_start_widget ? &scrolled_Window_start : &scrolled_Window_end;

	if (position == 0)
		drag_height = y + initial_size;
	else if (position == 1)
		drag_width = -x + initial_size; // This is buggy (As usual)
	else if (position == 2)
		drag_height = -y + initial_size; // So is this
	else if (position == 3)
		drag_width = x + initial_size;

	// This clamp ensures the values are not bellow 0
	drag_width = std::max(0.0, drag_width);
	drag_height = std::max(0.0, drag_height);

	// And this ensures we don't go outside of the screen
	drag_width = std::min((double)monitor_geometry.width - bar_width, drag_width);
	drag_height = std::min((double)monitor_geometry.height - bar_height, drag_height);

	scrolled_Window->set_size_request(drag_width, drag_height);
}

void sysbar::on_drag_stop(const double& x, const double& y) {
	double size = 0;
	double initial_size = sliding_start_widget ? initial_size_start : initial_size_end;
	double size_threshold = sliding_start_widget ? grid_widgets_start.get_allocated_height() : grid_widgets_end.get_allocated_height();;
	Gtk::ScrolledWindow* scrolled_Window = sliding_start_widget ? &scrolled_Window_start : &scrolled_Window_end;

	if (position == 0)
		size = y + initial_size;
	else if (position == 1)
		size = -x + initial_size;
	else if (position == 2)
		size = -y + initial_size;
	else if (position == 3)
		size = x + initial_size;

	// Ensure size is not negative
	size = std::max(0.0, size);
	bool passed_threshold = size > (size_threshold * 0.75);

	if (!((passed_threshold && gesture_touch) || (size == 0 && !gesture_touch)))
		scrolled_Window->hide();

	if (position % 2)
		scrolled_Window->set_halign(Gtk::Align::FILL);
	else
		scrolled_Window->set_valign(Gtk::Align::FILL);

	scrolled_Window->set_size_request(-1, -1);

	if (!scrolled_Window_start.get_visible() && !scrolled_Window_end.get_visible())
		overlay_window.hide();
}
