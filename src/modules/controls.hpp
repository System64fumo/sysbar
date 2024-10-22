#pragma once
#include "../module.hpp"
#ifdef MODULE_CONTROLS

#include <gtkmm/flowbox.h>
#include <gtkmm/button.h>

class control : public Gtk::Box {
	public:
		control(const std::string& icon, const bool& extra);
		Gtk::Button button_action;
		Gtk::Button button_expand;
};

class module_controls : public module {
	public:
		module_controls(sysbar*, const bool&);
		Gtk::FlowBox flowbox_controls;
};

#endif
