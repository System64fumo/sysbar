#pragma once
#include <gtkmm/window.h>
#include <gtkmm/revealer.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/box.h>

class sysbar : public Gtk::Window {

	public:
		sysbar();

	private:
		Gtk::Revealer revealer_box;
		Gtk::CenterBox centerbox_main;
		Gtk::Box box_start;
		Gtk::Box box_center;
		Gtk::Box box_end;

		void load_modules(std::string modules, Gtk::Box &box);
};
