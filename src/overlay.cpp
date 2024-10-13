#include "window.hpp"

#include <gtk4-layer-shell.h>

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
	int default_size = 300;
	box_widgets_start = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
	box_widgets_end = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);

	box_widgets_start->get_style_context()->add_class("box_widgets_start");
	box_widgets_end->get_style_context()->add_class("box_widgets_end");

	scrolled_Window_start = Gtk::make_managed<Gtk::ScrolledWindow>();
	scrolled_Window_end = Gtk::make_managed<Gtk::ScrolledWindow>();

	scrolled_Window_start->set_child(*box_widgets_start);
	scrolled_Window_end->set_child(*box_widgets_end);
	scrolled_Window_start->set_kinetic_scrolling(false);
	scrolled_Window_end->set_kinetic_scrolling(false);

	if (config_main.position == 0) {
		scrolled_Window_start->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_valign(Gtk::Align::START);
		box_widgets_start->set_valign(Gtk::Align::START);
		box_widgets_end->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_halign(Gtk::Align::END);
	}
	else if (config_main.position == 1) {
		scrolled_Window_start->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_valign(Gtk::Align::END);
		scrolled_Window_start->set_halign(Gtk::Align::END);
		scrolled_Window_end->set_halign(Gtk::Align::END);
		box_widgets_start->set_halign(Gtk::Align::END);
		box_widgets_end->set_halign(Gtk::Align::END);
	}
	else if (config_main.position == 2) {
		scrolled_Window_start->set_valign(Gtk::Align::END);
		scrolled_Window_end->set_valign(Gtk::Align::END);
		box_widgets_start->set_valign(Gtk::Align::END);
		box_widgets_end->set_valign(Gtk::Align::END);
		scrolled_Window_end->set_halign(Gtk::Align::END);
	}
	else if (config_main.position == 3) {
		scrolled_Window_start->set_valign(Gtk::Align::START);
		scrolled_Window_end->set_valign(Gtk::Align::END);
		box_widgets_start->set_valign(Gtk::Align::START);
		box_widgets_end->set_valign(Gtk::Align::START);
		box_widgets_start->set_halign(Gtk::Align::START);
		box_widgets_end->set_halign(Gtk::Align::START);
	}

	// Common
	if (config_main.position % 2) { // Vertical
		box_widgets_start->set_size_request(-1, default_size);
		box_widgets_end->set_size_request(-1, default_size);
		scrolled_Window_start->set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::NEVER);
		scrolled_Window_end->set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::NEVER);
		box_overlay.set_orientation(Gtk::Orientation::VERTICAL);
	}
	else { // Horizontal
		box_widgets_start->set_size_request(default_size, -1);
		box_widgets_end->set_size_request(default_size, -1);
		scrolled_Window_start->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::EXTERNAL);
		scrolled_Window_end->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::EXTERNAL);
		box_overlay.set_orientation(Gtk::Orientation::HORIZONTAL);
		scrolled_Window_start->set_halign(Gtk::Align::START);
	}

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
		on_drag_start(x, y);

		if (!gesture_drag->get_current_event()->get_pointer_emulated())
			gesture_drag->reset();
	});
	gesture_drag->signal_drag_update().connect(sigc::mem_fun(*this, &sysbar::on_drag_update));
	gesture_drag->signal_drag_end().connect(sigc::mem_fun(*this, &sysbar::on_drag_stop));
	add_controller(gesture_drag);

	gesture_drag_overlay = Gtk::GestureDrag::create();
	gesture_drag_overlay->signal_drag_begin().connect([&](const double &x, const double &y) {
		if (!gesture_drag_overlay->get_current_event()->get_pointer_emulated()) {
			gesture_drag_overlay->reset();
			return;
		}
		on_drag_start(x, y);
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
			initial_size_start = box_widgets_start->get_allocated_width();
			initial_size_end = box_widgets_end->get_allocated_width();
		}
		else {
			initial_size_start = box_widgets_start->get_allocated_height();
			initial_size_end = box_widgets_end->get_allocated_height();
		}
	}

	if (sliding_start_widget)
		scrolled_Window_start->show();
	else
		scrolled_Window_end->show();
}

void sysbar::on_drag_update(const double &x, const double &y) {
	double width = -1;
	double height = -1;
	double initial_size = sliding_start_widget ? initial_size_start : initial_size_end;

	if (config_main.position == 0)
		height = std::max(0.0, y + initial_size);
	else if (config_main.position == 1)
		width = std::max(0.0, -x + initial_size);
	else if (config_main.position == 2)
		height = std::max(0.0, -y + initial_size);
	else if (config_main.position == 3)
		width = std::max(0.0, x + initial_size);

	if (sliding_start_widget)
		scrolled_Window_start->set_size_request(width, height);
	else
		scrolled_Window_end->set_size_request(width, height);
}

// Like can you actually even read and understand what any of this does?
// No.
// TODO: Clean this section up in particular
void sysbar::on_drag_stop(const double &x, const double &y) {
	double size = 0;
	double initial_size = sliding_start_widget ? initial_size_start : initial_size_end;
	double size_threshold = sliding_start_widget ? box_widgets_start->get_allocated_height() : box_widgets_end->get_allocated_height();;
	Gtk::Align align = Gtk::Align::START;

	if (config_main.position == 0) {
		size = std::max(0.0, y + initial_size);
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
	}

	Gtk::ScrolledWindow* scrolled_Window = sliding_start_widget ? scrolled_Window_start : scrolled_Window_end;

	// Ensure size is not negative
	size = std::max(0.0, size);
	bool passed_threshold = size > (size_threshold * 0.75);

	if ((passed_threshold && size != size_threshold) || size == 0) {
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
