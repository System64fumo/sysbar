#pragma once
#include "config.hpp"
#include "css.hpp"

#include <gtkmm/revealer.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/box.h>

class sysbar : public Gtk::Window {

	public:
		sysbar(const config_bar &cfg);
		void handle_signal(const int &signum);

		config_bar config_main;
		Gtk::Window overlay_window;
		Gtk::Box *box_widgets_start;
		Gtk::Box *box_widgets_end;
		Gtk::Box *box_controls = nullptr;

	private:
		Gtk::Revealer revealer_box;
		Gtk::CenterBox centerbox_main;
		Gtk::Box box_overlay;
		Gtk::Box box_start;
		Gtk::Box box_center;
		Gtk::Box box_end;

		Gtk::ScrolledWindow *scrolled_Window_start;
		Gtk::ScrolledWindow *scrolled_Window_end;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag_overlay;
		GdkMonitor *monitor;
		GdkRectangle monitor_geometry;
		double initial_size_start, initial_size_end;
		int width, height;
		bool sliding_start_widget;

		void load_modules(const std::string &modules, Gtk::Box &box);
		void setup_controls();
		void setup_overlay();
		void setup_overlay_widgets();
		void setup_gestures();

		void on_drag_start(const double &x, const double &y);
		void on_drag_update(const double &x, const double &y);
		void on_drag_stop(const double &x, const double &y);
};

extern "C" {
	sysbar *sysbar_create(const config_bar &cfg);
	void sysbar_signal(sysbar *window, int signal);
}
