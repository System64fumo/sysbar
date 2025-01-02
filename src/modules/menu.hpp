#pragma once
#include "../module.hpp"
#ifdef MODULE_MENU

#include <gtkmm/gestureclick.h>

class module_menu : public module {
	public:
		module_menu(sysbar*, const bool&);

	private:
		Glib::RefPtr<Gtk::GestureClick> gesture_click;
		void on_clicked(const int&, const double&, const double&);
};

#endif
