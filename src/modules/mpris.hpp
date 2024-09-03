#pragma once
#include "../module.hpp"
#ifdef MODULE_MPRIS

#include <playerctl.h>
#include <glibmm/dispatcher.h>
#include <gtkmm/progressbar.h>

class module_mpris : public module {
	public:
		module_mpris(sysbar *window, const bool &icon_on_start = true);

		int status = 0;
		std::string artist = "";
		std::string album = "";
		std::string title = "";
		std::string length = "";

		Gtk::Box box_player;
		Gtk::Box box_right;
		Gtk::Label label_title;
		Gtk::Label label_album;
		Gtk::Label label_artist;
		Gtk::ProgressBar progressbar_playback;
		Glib::Dispatcher dispatcher_callback;

	private:
		void update_info();
		void setup_widget();
};

#endif
