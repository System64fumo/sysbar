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
		Gtk::Box box_sidepanel;
		Gtk::Box box_widgets;
		Gtk::Stack stack_pages;
		Gtk::Grid grid_main;

		void set_page(const std::string& destination);

	private:
		sysbar* window;
		Gtk::Revealer revealer_header;
		Gtk::Box box_header;
		Gtk::Label label_header;
		Gtk::Button button_return;
		Gtk::Revealer revealer_return;

		bool position_start;

		void setup_header();
};
