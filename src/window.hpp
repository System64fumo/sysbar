#pragma once
#include "config.hpp"

#include <gtkmm/window.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/box.h>

class sysbar : public Gtk::Window {

	public:
		sysbar(const config &cfg);

	private:
		config config_main;
		Gtk::CenterBox centerbox_main;
		Gtk::Box box_start;
		Gtk::Box box_center;
		Gtk::Box box_end;

		void load_modules(const std::string &modules, Gtk::Box &box);
};

extern "C" {
	sysbar *syspower_create(const config &cfg);
}
