#pragma once
#include "config.hpp"
#include "css.hpp"

#include <gtkmm/revealer.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/box.h>

class sysbar : public Gtk::Window {

	public:
		sysbar(const config_bar &cfg);
		void handle_signal(const int &signum);

		config_bar config_main;
		Gtk::Window overlay_window;
		Gtk::Box *box_widgets_start;
		Gtk::Box *box_widgets_end;
		Gtk::Box *box_controls;

	private:
		Gtk::Revealer revealer_box;
		Gtk::CenterBox centerbox_main;
		Gtk::Box box_overlay;
		Gtk::Box box_start;
		Gtk::Box box_center;
		Gtk::Box box_end;

		GdkMonitor *monitor;
		int width, height;

		void load_modules(const std::string &modules, Gtk::Box &box);
		void setup_controls();
		void setup_popovers();
		void setup_overlay();
};

extern "C" {
	sysbar *sysbar_create(const config_bar &cfg);
	void sysbar_signal(sysbar *window, int signal);
}
