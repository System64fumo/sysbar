#pragma once
#include "config.hpp"

#include <gtkmm/window.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/box.h>

class sysbar : public Gtk::Window {

	public:
		sysbar(const config_bar &cfg);

	private:
		config_bar config_main;
		Gtk::CenterBox centerbox_main;
		Gtk::Box box_start;
		Gtk::Box box_center;
		Gtk::Box box_end;

		void load_modules(const std::string &modules, Gtk::Box &box);
};

extern "C" {
	sysbar *sysbar_create(const config_bar &cfg);
}
