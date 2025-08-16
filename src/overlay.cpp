#include "window.hpp"

#include <gtkmm/label.h>
#include <gtkmm/button.h>
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
	default_size_start = 350;
	default_size_end = 350;
	int size = (position % 2) ? monitor_geometry.height : monitor_geometry.width;

	sidepanel_start = Gtk::make_managed<sidepanel>(this, true);
	box_overlay.append(*sidepanel_start);

	sidepanel_end = Gtk::make_managed<sidepanel>(this, false);
	box_overlay.append(*sidepanel_end);

	// TODO: This is a mess, Clean it up.
	if (position == 0) {
		overlay_window.get_style_context()->add_class("position_top");
		sidepanel_start->set_valign(Gtk::Align::START);
		sidepanel_end->set_valign(Gtk::Align::START);
		sidepanel_start->box_widgets.set_orientation(Gtk::Orientation::VERTICAL);
		sidepanel_end->box_widgets.set_orientation(Gtk::Orientation::VERTICAL);
		sidepanel_start->grid_main.set_valign(Gtk::Align::START);
		sidepanel_end->grid_main.set_valign(Gtk::Align::START);
		sidepanel_start->box_widgets.set_valign(Gtk::Align::START);
		sidepanel_end->box_widgets.set_valign(Gtk::Align::START);
	}
	else if (position == 1) {
		overlay_window.get_style_context()->add_class("position_right");
		sidepanel_start->set_valign(Gtk::Align::START);
		sidepanel_end->set_valign(Gtk::Align::END);
		sidepanel_start->set_halign(Gtk::Align::END);
		sidepanel_end->set_halign(Gtk::Align::END);
		sidepanel_start->grid_main.set_halign(Gtk::Align::END);
		sidepanel_end->grid_main.set_halign(Gtk::Align::END);
		sidepanel_start->box_widgets.set_valign(Gtk::Align::START);
		sidepanel_end->box_widgets.set_valign(Gtk::Align::START);
	}
	else if (position == 2) {
		overlay_window.get_style_context()->add_class("position_bottom");
		sidepanel_start->set_valign(Gtk::Align::END);
		sidepanel_end->set_valign(Gtk::Align::END);
		sidepanel_start->box_widgets.set_orientation(Gtk::Orientation::VERTICAL);
		sidepanel_end->box_widgets.set_orientation(Gtk::Orientation::VERTICAL);
		sidepanel_start->grid_main.set_valign(Gtk::Align::END);
		sidepanel_end->grid_main.set_valign(Gtk::Align::END);
		sidepanel_start->box_widgets.set_valign(Gtk::Align::END);
		sidepanel_end->box_widgets.set_valign(Gtk::Align::END);
	}
	else if (position == 3) {
		overlay_window.get_style_context()->add_class("position_left");
		sidepanel_start->set_valign(Gtk::Align::START);
		sidepanel_end->set_valign(Gtk::Align::END);
		sidepanel_start->grid_main.set_valign(Gtk::Align::START);
		sidepanel_end->grid_main.set_valign(Gtk::Align::START);
		sidepanel_start->grid_main.set_halign(Gtk::Align::START);
		sidepanel_end->grid_main.set_halign(Gtk::Align::START);
		sidepanel_start->box_widgets.set_valign(Gtk::Align::END);
		sidepanel_end->box_widgets.set_valign(Gtk::Align::END);
	}

	// Common
	bool vertical_layout = position % 2;
	if (vertical_layout) { // Vertical
		if (size / 2 > default_size_start) {
			sidepanel_start->grid_main.set_size_request(-1, default_size_start);
			sidepanel_start->set_halign(Gtk::Align::START);
		}
		else
			sidepanel_start->set_hexpand(true);

		if (size / 2 > default_size_end) {
			sidepanel_end->grid_main.set_size_request(-1, default_size_end);
			sidepanel_end->set_halign(Gtk::Align::END);
		}
		else
			sidepanel_end->set_hexpand(true);

		box_overlay.set_orientation(Gtk::Orientation::VERTICAL);
	}
	else { // Horizontal
		if (size / 2 > default_size_start) {
			sidepanel_start->grid_main.set_size_request(default_size_start, -1);
			sidepanel_start->set_halign(Gtk::Align::START);
		}
		else {
			sidepanel_start->set_hexpand(true);
			sidepanel_start->set_halign(Gtk::Align::FILL);
		}

		if (size / 2 > default_size_end) {
			sidepanel_end->grid_main.set_size_request(default_size_end, -1);
			sidepanel_end->set_halign(Gtk::Align::END);
		}
		else {
			sidepanel_end->set_hexpand(true);
			sidepanel_end->set_halign(Gtk::Align::FILL);
		}

		box_overlay.set_orientation(Gtk::Orientation::HORIZONTAL);
	}

	sidepanel_start->set_policy(
		vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER,
		!vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER);
	sidepanel_end->set_policy(
		vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER,
		!vertical_layout ? Gtk::PolicyType::EXTERNAL : Gtk::PolicyType::NEVER);

	sidepanel_start->set_visible(false);
	sidepanel_end->set_visible(false);
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
	sidepanel_start->add_controller(gesture_drag_start);

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
	sidepanel_end->add_controller(gesture_drag_end);
}

void sysbar::on_drag_start(const double& x, const double& y) {
	overlay_window.show();

	sidepanel_start->set_valign(Gtk::Align::START);
	sidepanel_end->set_valign(Gtk::Align::START);

	if (position == 0) {
		sidepanel_start->set_valign(Gtk::Align::START);
		sidepanel_end->set_valign(Gtk::Align::START);
	}
	else if (position == 1) {
		sidepanel_start->set_halign(Gtk::Align::END);
		sidepanel_end->set_halign(Gtk::Align::END);
	}
	else if (position == 2) {
		sidepanel_start->set_valign(Gtk::Align::END);
		sidepanel_end->set_valign(Gtk::Align::END);
	}
	else if (position == 3) {
		sidepanel_start->set_halign(Gtk::Align::START);
		sidepanel_end->set_halign(Gtk::Align::START);
	}

	if (position % 2) {
		sliding_start_widget = y < monitor_geometry.height / 2;
		initial_size_start = sidepanel_start->get_width();
		initial_size_end = sidepanel_end->get_width();
	}
	else {
		sliding_start_widget = x < monitor_geometry.width / 2;
		initial_size_start = sidepanel_start->get_height();
		initial_size_end = sidepanel_end->get_height();
	}

	if (initial_size_start != 0 || initial_size_end != 0) {
		if (position % 2) {
			initial_size_start = sidepanel_start->grid_main.get_allocated_width();
			initial_size_end = sidepanel_end->grid_main.get_allocated_width();
		}
		else {
			initial_size_start = sidepanel_start->grid_main.get_allocated_height();
			initial_size_end = sidepanel_end->grid_main.get_allocated_height();
		}
	}

	sidepanel_start->set_visible(sliding_start_widget);
	sidepanel_end->set_visible(!sliding_start_widget);
}

void sysbar::on_drag_update(const double& x, const double& y) {
	double drag_width = -1;
	double drag_height = -1;
	double initial_size = sliding_start_widget ? initial_size_start : initial_size_end;
	Gtk::ScrolledWindow* scrolled_Window = sliding_start_widget ? sidepanel_start : sidepanel_end;

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
	double size_threshold = sliding_start_widget ? sidepanel_start->grid_main.get_allocated_height() : sidepanel_end->grid_main.get_allocated_height();;
	Gtk::ScrolledWindow* scrolled_Window = sliding_start_widget ? sidepanel_start : sidepanel_end;

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

	if (!sidepanel_start->get_visible() && !sidepanel_end->get_visible())
		overlay_window.hide();
}
