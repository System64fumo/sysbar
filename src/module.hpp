#pragma once
#include "config.hpp"
#include "window.hpp"

#include <gtkmm/label.h>
#include <gtkmm/image.h>

class module : public Gtk::Box {
	public:
		module(sysbar*, const bool& = true);
		Gtk::Label label_info;
		Gtk::Image image_icon;
		sysbar* win;

	private:
		void on_dispatcher();
};
