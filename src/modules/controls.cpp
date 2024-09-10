#include "controls.hpp"

module_controls::module_controls(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("widget_controls");
	image_icon.hide();
	label_info.hide();
	append(flowbox_controls);

	// This causes a critical error.. Why?
	// TODO: Fix said error
	auto container = static_cast<Gtk::Box*>(win->box_widgets_end);
	container->append(*this);
}
