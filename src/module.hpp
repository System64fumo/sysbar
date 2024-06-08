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
		Glib::RefPtr<Gtk::GestureClick> m_click_gesture;

		void on_clicked(int n_press, double x, double y);
		void on_dispatcher();
};
