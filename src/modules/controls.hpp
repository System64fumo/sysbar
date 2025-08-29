#pragma once
#include "../module.hpp"
#ifdef MODULE_CONTROLS

#include <gtkmm/flowbox.h>
#include <gtkmm/button.h>

class control_page : public Gtk::Box {
	public:
		control_page(sysbar* window, const std::string& name);

		Gtk::Box box_header;
		Gtk::Box box_body;
};

class control : public Gtk::Box {
	public:
		control(sysbar* window, const std::string& icon, const bool& extra, const std::string& name);
		Gtk::Button button_action;
		Gtk::Button button_expand;
		control_page* page;
};

class module_controls : public module {
	public:
		module_controls(sysbar*, const bool&);
		Gtk::FlowBox flowbox_controls;
};

#endif
