#pragma once
#include "config.hpp"

#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <gtkmm/box.h>
#include <gtkmm/gestureclick.h>
#include <glibmm/dispatcher.h>

class module : public Gtk::Box {
	public:
		module(const config_bar &cfg, const bool &icon_on_start = true, const bool &clickable = false);
		config_bar config_main;
		Gtk::Label label_info;
		Gtk::Image image_icon;

	private:
		void on_dispatcher();
};
