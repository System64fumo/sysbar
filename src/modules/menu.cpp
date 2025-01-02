#include "menu.hpp"
#include "../config_parser.hpp"

module_menu::module_menu(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_menu");

	// TODO: Add option to set other icons maybe even images?
	image_icon.set_from_icon_name("start-here");

	// TODO: Add option to set text
	label_info.hide();

	// Custom on_clicked handle
	gesture_click = Gtk::GestureClick::create();
	gesture_click->set_button(GDK_BUTTON_PRIMARY);
	gesture_click->signal_pressed().connect(sigc::mem_fun(*this, &module_menu::on_clicked));
	add_controller(gesture_click);
}

void module_menu::on_clicked(const int& n_press, const double& x, const double& y) {
	// TODO: Add native sysmenu integration if no command is specified
	// TODO: Add custom command support

	system("pkill -34 sysmenu || pkill -34 sysshell");

	// Prevent gestures bellow from triggering
	gesture_click->reset();
}
