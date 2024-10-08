#pragma once
#include "config.hpp"
#include "window.hpp"

#ifdef CONFIG_FILE
#include "config_parser.hpp"
#endif

#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <gtkmm/box.h>
#include <gtkmm/gestureclick.h>
#include <glibmm/dispatcher.h>

class module : public Gtk::Box {
	public:
		module(sysbar *window, const bool &icon_on_start = true);
		config_bar config_main;
		Gtk::Label label_info;
		Gtk::Image image_icon;
		sysbar* win;

		#ifdef CONFIG_FILE
		config_parser *config;
		#endif

	private:
		void on_dispatcher();
};
