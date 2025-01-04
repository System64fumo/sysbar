#pragma once
#include "config.hpp"

#include <gtkmm/window.h>
#include <gtkmm/revealer.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/box.h>
#include <gtkmm/grid.h>

class sysbar : public Gtk::Window {
	public:
		sysbar(const std::map<std::string, std::map<std::string, std::string>>&);
		void handle_signal(const int&);

		std::map<std::string, std::map<std::string, std::string>> config_main;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;
		Gtk::Box* box_controls = nullptr;
		Gtk::Grid grid_widgets_start;
		Gtk::Grid grid_widgets_end;

		// Main config
		int position;
		int size;
		int layer;
		bool verbose;

	private:
		Gtk::Revealer revealer_box;
		Gtk::CenterBox centerbox_main;
		Gtk::Box box_overlay;
		Gtk::Box box_start;
		Gtk::Box box_center;
		Gtk::Box box_end;

		Gtk::Window overlay_window;
		Gtk::ScrolledWindow scrolled_Window_start;
		Gtk::ScrolledWindow scrolled_Window_end;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag_start;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag_end;

		GdkMonitor* monitor;
		GdkRectangle monitor_geometry;
		double initial_size_start, initial_size_end;
		int bar_width, bar_height;
		bool sliding_start_widget;
		bool gesture_touch;

		void load_modules(const std::string&, Gtk::Box&);
		void setup_overlay();
		void setup_overlay_widgets();
		void setup_gestures();

		void on_drag_start(const double&, const double&);
		void on_drag_update(const double&, const double&);
		void on_drag_stop(const double&, const double&);
};

extern "C" {
	sysbar* sysbar_create(const std::map<std::string, std::map<std::string, std::string>>&);
	void sysbar_signal(sysbar*, int);
}
