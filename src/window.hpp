#pragma once
#include "config.hpp"

#include <gtkmm/window.h>
#include <gtkmm/revealer.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/box.h>

class sysbar : public Gtk::Window {

	public:
		sysbar(const config_bar &cfg);
		void handle_signal(const int &signum);

	private:
		config_bar config_main;
		Gtk::Revealer revealer_box;
		Gtk::CenterBox centerbox_main;
		Gtk::Box box_start;
		Gtk::Box box_center;
		Gtk::Box box_end;
		int width, height;

		void load_modules(const std::string &modules, Gtk::Box &box);
};

extern "C" {
	sysbar *sysbar_create(const config_bar &cfg);
	void sysbar_signal(sysbar *window, int signal);
}
