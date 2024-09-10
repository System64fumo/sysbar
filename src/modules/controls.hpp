#pragma once
#include "../module.hpp"
#ifdef MODULE_CONTROLS
#include <gtkmm/flowbox.h>

class module_controls : public module {
	public:
		module_controls(sysbar *window, const bool &icon_on_start = true);
		Gtk::FlowBox flowbox_controls;
};

#endif
