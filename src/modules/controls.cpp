#include "controls.hpp"

module_controls::module_controls(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("widget_controls");
	image_icon.hide();
	label_info.hide();
	append(flowbox_controls);
	flowbox_controls.set_orientation(Gtk::Orientation::VERTICAL);
	flowbox_controls.set_homogeneous(true);
	flowbox_controls.set_hexpand(true);
	flowbox_controls.set_max_children_per_line(1);
	flowbox_controls.set_selection_mode(Gtk::SelectionMode::NONE);

	// This causes a critical error.. Why?
	// TODO: Fix said error
	auto container = static_cast<Gtk::Box*>(win->box_widgets_end);
	container->append(*this);
}

control::control(const bool& extra) {
	get_style_context()->add_class("control");
	append(button_action);
	button_action.get_style_context()->add_class("button_action");
	button_action.set_has_frame(false);
	button_action.set_hexpand(true);

	// This is for sub menus
	if (extra) {
		append(button_expand);
		button_action.get_style_context()->add_class("button_expand");
		button_expand.set_has_frame(false);
		button_expand.set_icon_name("arrow-right");
	}
}
