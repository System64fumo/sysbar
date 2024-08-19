#pragma once
#include "../module.hpp"
#ifdef MODULE_TASKBAR

#include "../wlr-foreign-toplevel-management-unstable-v1.h"

#include <gtkmm/flowbox.h>
#include <gtkmm/scrolledwindow.h>

class module_taskbar : public module {
	public:
		module_taskbar(sysbar *window, const bool &icon_on_start = true);

		Gtk::FlowBox flowbox_main;

	private:
		Gtk::Box box_container;
		Gtk::ScrolledWindow scrolledwindow;
		void setup_proto();
};

class taskbar_item : public Gtk::Box {
	public:
		taskbar_item(const Gtk::FlowBox&);
		Gtk::Label toplevel_label;
};

#endif
