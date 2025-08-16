#pragma once
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/revealer.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/stack.h>
#include <gtkmm/grid.h>
#include <gtkmm/box.h>

class sysbar;

class sidepanel : public Gtk::ScrolledWindow {
	public:
		sidepanel(sysbar* window, const bool& position_start);
		Gtk::Revealer* revealer_header;
		Gtk::Box box_widgets;
		Gtk::Stack stack_pages;
		Gtk::Grid grid_main;

	private:
		sysbar* window;

		Gtk::Revealer* create_header(const bool& position_start);
};
