#pragma once
#ifdef MODULE_TASKBAR

#include "../module.hpp"
#include "../wlr-foreign-toplevel-management-unstable-v1.h"

#include <gtkmm/flowbox.h>
#include <gtkmm/scrolledwindow.h>

class module_taskbar : public module {
	public:
		module_taskbar(const config_bar &cfg, const bool &icon_on_start = false, const bool &clickable = false);

		Gtk::FlowBox flowbox_main;

	private:
		Gtk::Box box_container;
		Gtk::ScrolledWindow scrolledwindow;
		void setup_proto();
};

#endif
