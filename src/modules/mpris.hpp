#pragma once
#include "../module.hpp"
#ifdef MODULE_MPRIS

#include <playerctl.h>
#include <glibmm/dispatcher.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/button.h>

class module_mpris : public module {
	public:
		module_mpris(sysbar *window, const bool &icon_on_start = true);

		Glib::Dispatcher dispatcher_callback;
		PlayerctlPlayer *player = nullptr;

		int status = 0;
		std::string artist = "";
		std::string album = "";
		std::string title = "";
		std::string length = "";
		std::string album_art_url = "";

		void update_info();

	private:
		std::string last_album_art_url;

		Gtk::Box box_player;
		Gtk::Box box_right;
		Gtk::Image image_album_art;
		Gtk::Label label_title;
		Gtk::Label label_album;
		Gtk::Label label_artist;
		Gtk::ProgressBar progressbar_playback;

		Gtk::Box box_controls;
		Gtk::Button button_previous;
		Gtk::Button button_play_pause;
		Gtk::Button button_next;

		void setup_widget();
};

#endif
