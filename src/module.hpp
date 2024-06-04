#pragma once
#include <gtkmm.h>

class module : public Gtk::Box {
	public:
		module(bool icon_on_start = true, bool clickable = false);
		Gtk::Label label_info;
		Gtk::Image image_icon;
		Gtk::Box box_popout;

	private:
		Gtk::Popover popover_popout;
		Glib::RefPtr<Gtk::GestureClick> m_click_gesture;

		void on_clicked(int n_press, double x, double y);
		void on_dispatcher();
};
