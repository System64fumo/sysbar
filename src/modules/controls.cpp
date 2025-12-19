#include "controls.hpp"

module_controls::module_controls(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start) {
	add_css_class("widget");
	add_css_class("widget_controls");
	image_icon.hide();
	label_info.hide();

	int cols = std::stoi(win->config_main["controls"]["columns"]);

	append(flowbox_controls);
	flowbox_controls.set_homogeneous(true);
	flowbox_controls.set_hexpand(true);
	flowbox_controls.set_max_children_per_line(cols);
	flowbox_controls.set_selection_mode(Gtk::SelectionMode::NONE);

	win->sidepanel_end->grid_main.attach(*this, 0, 0, 4, 1);
}

control_page::control_page(sysbar* window, const std::string& name) : Gtk::Box(Gtk::Orientation::VERTICAL), box_body(Gtk::Orientation::VERTICAL) {
	add_css_class("control_page_" + name);
	append(box_body);

	// TODO: Detect position
	window->sidepanel_end->stack_pages.add(*this, name);
}

control::control(sysbar* window, const std::string& icon, const bool& extra, const std::string& name, const bool& position_start) {
	add_css_class("control");
	append(button_action);
	button_action.set_icon_name(icon);
	button_action.add_css_class("button_action");
	button_action.set_focusable(false);
	button_action.set_has_frame(false);
	button_action.set_hexpand(true);

	// This is for sub menus
	if (extra) {
		append(button_expand);
		button_action.add_css_class("button_expand");
		button_expand.set_focusable(false);
		button_expand.set_has_frame(false);
		button_expand.set_icon_name("arrow-right");
		page = Gtk::make_managed<control_page>(window, name);

		sidepanel* panel = position_start ? window->sidepanel_start : window->sidepanel_end;
		button_expand.signal_clicked().connect([&, panel, name]() {
			panel->set_page(name);
		});
	}
}