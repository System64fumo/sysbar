#pragma once
#include "../module.hpp"
#ifdef MODULE_TASKBAR

#include "../wlr-foreign-toplevel-management-unstable-v1.h"

#include <gtkmm/flowbox.h>
#include <gtkmm/scrolledwindow.h>

class module_taskbar : public module {
	public:
		module_taskbar(sysbar *window, const bool &icon_on_start = true);

		int text_length = 14;
		int icon_size = 34;
		Gtk::FlowBox flowbox_main;

	private:
		Gtk::Box box_container;
		Gtk::ScrolledWindow scrolledwindow;

		void setup_proto();
};

class taskbar_item : public Gtk::Box {
	public:
		taskbar_item(const Gtk::FlowBox&, const int &length, const uint &size);
		uint text_length;

		Gtk::Label toplevel_label;
		Gtk::Image image_icon;
		zwlr_foreign_toplevel_handle_v1* handle;
};

#endif
