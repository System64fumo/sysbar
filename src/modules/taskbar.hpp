#pragma once
#include "../module.hpp"
#ifdef MODULE_TASKBAR

#include "../wlr-foreign-toplevel-management-unstable-v1.h"

#include <gtkmm/flowbox.h>
#include <gtkmm/scrolledwindow.h>

class module_taskbar : public module {
	public:
		module_taskbar(sysbar*, const bool&);

		struct config_tb {
			int text_length = 14;
			int icon_size = 34;
			bool show_icon = true;
			bool show_label = true;
		} cfg;

		Gtk::FlowBox flowbox_main;

	private:
		Gtk::Box box_container;
		Gtk::ScrolledWindow scrolledwindow;

		void setup_proto();
};

class taskbar_item : public Gtk::Box {
	public:
		taskbar_item(module_taskbar*, const module_taskbar::config_tb&);

		Gtk::Label toplevel_label;
		Gtk::Image image_icon;
		zwlr_foreign_toplevel_handle_v1* handle;

		uint text_length;
};

#endif
