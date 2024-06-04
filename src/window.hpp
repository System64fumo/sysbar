#pragma once
#include <gtkmm.h>

class sysbar : public Gtk::Window {

	public:
		sysbar();

	private:
		Gtk::CenterBox centerbox_main;
		Gtk::Box box_start;
		Gtk::Box box_center;
		Gtk::Box box_end;

		void load_modules();
};
