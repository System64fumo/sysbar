#pragma once
#include "config.hpp"
#include "sidepanel.hpp"

#include <gtkmm/window.h>
#include <gtkmm/revealer.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/box.h>
#include <gtkmm/stack.h>
#include <gtkmm/grid.h>

class sysbar : public Gtk::Window {
	public:
		sysbar(const std::map<std::string, std::map<std::string, std::string>>&);
		void handle_signal(const int&);

		std::map<std::string, std::map<std::string, std::string>> config_main;
		Gtk::Window overlay_window;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag_s; // TODO: Temporary, Rework this
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag_e; // TODO: Temporary, Rework this
		Gtk::Box* box_controls = nullptr;

		sidepanel* sidepanel_start = nullptr;
		sidepanel* sidepanel_end = nullptr;
		int default_size_start = 350;
		int default_size_end = 350;
		std::string network_icon = "network-error-symbolic"; // TODO: Terrible implementation..

		// Main config
		int position = 0;
		int size = 40;
		int icon_size = 16;
		int layer = 2;
		bool verbose = false;
		bool visible = true;

	private:
		Gtk::Revealer revealer_box;
		Gtk::CenterBox centerbox_main;
		Gtk::Box box_overlay;
		Gtk::Box box_start;
		Gtk::Box box_center;
		Gtk::Box box_end;

		Glib::RefPtr<Gtk::GestureDrag> gesture_drag_start;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag_end;

		GdkMonitor* monitor = nullptr;
		GdkRectangle monitor_geometry;
		double initial_size_start = 0;
		double initial_size_end = 0;
		int bar_width = 40;
		int bar_height = 40;
		bool sliding_start_widget = false;
		bool gesture_touch = false;

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
