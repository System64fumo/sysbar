#pragma once
#ifdef MODULE_CLOCK

#include "../module.hpp"
#include <gtkmm/button.h>

class module_clock : public module {
	public:
		module_clock(const bool &icon_on_start = false, const bool &clickable = false);
		Gtk::Button button_btn;
		Gtk::Label label_date;

	private:
		bool update_info();
};

#endif
