#pragma once
#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <gtkmm/box.h>
#include <gtkmm/popover.h>
#include <gtkmm/gestureclick.h>
#include <glibmm/dispatcher.h>

class module : public Gtk::Box {
	public:
		module(const bool &icon_on_start = true, const bool &clickable = false);
		Gtk::Label label_info;
		Gtk::Image image_icon;
		Gtk::Box box_popout;

	private:
		Gtk::Popover popover_popout;

		void on_clicked(const int &n_press, const double &x, const double &y);
		void on_dispatcher();
};
