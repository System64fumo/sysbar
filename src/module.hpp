#pragma once
#include "config.hpp"
#include "window.hpp"

#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <gtkmm/box.h>
#include <gtkmm/gestureclick.h>
#include <glibmm/dispatcher.h>

class module : public Gtk::Box {
	public:
		module(sysbar *window, const bool &icon_on_start = true);
		Gtk::Label label_info;
		Gtk::Image image_icon;
		sysbar* win;

	private:
		int position;
		int size;

		void on_dispatcher();
};
