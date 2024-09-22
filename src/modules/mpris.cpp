#include "mpris.hpp"

static void playback_status(PlayerctlPlayer *player, PlayerctlPlaybackStatus status, gpointer user_data) {
	module_mpris *self = static_cast<module_mpris*>(user_data);
	self->status = status;
	self->dispatcher_callback.emit();
}

static void metadata(PlayerctlPlayer *player, GVariant *metadata, gpointer user_data) {
	module_mpris *self = static_cast<module_mpris*>(user_data);

	// This could be better..
	self->artist = playerctl_player_get_artist(player, nullptr);
	self->album = playerctl_player_get_album(player, nullptr);
	self->title = playerctl_player_get_title(player, nullptr);

	auto length = playerctl_player_print_metadata_prop(player, "mpris:length", nullptr);
	if (length)
		self->length = length;
	g_free(length);

	auto album_art_url = playerctl_player_print_metadata_prop(player, "mpris:artUrl", nullptr);
	if (album_art_url)
		self->album_art_url = album_art_url;
	g_free(album_art_url);

	self->dispatcher_callback.emit();
}

static void player_appeared(PlayerctlPlayerManager* manager, PlayerctlPlayerName* player_name, gpointer user_data) {
	module_mpris *self = static_cast<module_mpris*>(user_data);
	self->player = playerctl_player_new_from_name(player_name, nullptr);

	g_object_get(self->player, "playback-status", &self->status, nullptr);
	g_signal_connect(self->player, "playback-status", G_CALLBACK(playback_status), self);
	g_signal_connect(self->player, "metadata", G_CALLBACK(metadata), self);
	metadata(self->player, nullptr, self);

	self->update_info();
}

static void player_vanished(PlayerctlPlayerManager* manager, PlayerctlPlayerName* player_name, gpointer user_data) {
	module_mpris *self = static_cast<module_mpris*>(user_data);
	self->player = nullptr;

	// Itterate over all players
	GList *players = playerctl_list_players(nullptr);
	for (auto plr = players; plr != nullptr; plr = plr->next) {
		auto plr_name = static_cast<PlayerctlPlayerName*>(plr->data);
		player_appeared(nullptr, plr_name, self);
		return;
	}
}

module_mpris::module_mpris(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_mpris");

	#ifdef CONFIG_FILE
	if (config->available) {
		std::string cfg_icon = config->get_value("mpris", "show-icon");
		if (cfg_icon != "true")
			image_icon.hide();

		std::string cfg_label = config->get_value("mpris", "show-label");
		if (cfg_label != "true")
			label_info.hide();
	}
	#endif

	dispatcher_callback.connect(sigc::mem_fun(*this, &module_mpris::update_info));

	// Setup
	auto manager = playerctl_player_manager_new(nullptr);
	g_object_connect(manager, "signal::name-appeared", G_CALLBACK(player_appeared), this, nullptr);
	g_object_connect(manager, "signal::name-vanished", G_CALLBACK(player_vanished), this, nullptr);

	setup_widget();

	// Reuse code to get available players
	player_vanished(nullptr, nullptr, this);
}

void module_mpris::update_info() {
	std::string status_icon = status ? "player_play" : "player_pause";
	image_icon.set_from_icon_name(status_icon);
	button_play_pause.set_icon_name(status_icon);
	label_info.set_text(title);

	label_title.set_text(title);
	label_artist.set_text(artist);

	if (album_art_url.find("file://") == 0) {
		album_art_url.erase(0, 7);
		Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file(album_art_url);
		int width = pixbuf->get_width();
		int height = pixbuf->get_height();

		int square_size = std::min(width, height);
		int offset_x = (width - square_size) / 2;
		int offset_y = (height - square_size) / 2;
		pixbuf = Gdk::Pixbuf::create_subpixbuf(pixbuf, offset_x, offset_y, square_size, square_size);
		image_album_art.set(pixbuf);
	}

	// TODO: Playback status can be tracked using a timer by getting..
	// the "length" property every second if the player is playing
	//progressbar_playback.set_fraction(0.5);
}

void module_mpris::setup_widget() {
	auto container = static_cast<Gtk::Box*>(win->box_widgets_end);
	container->append(box_player);

	box_player.get_style_context()->add_class("widget_mpris");
	image_album_art.get_style_context()->add_class("image_album_art");
	image_album_art.set_from_icon_name("music-app-symbolic");
	image_album_art.set_size_request(96 ,96);
	box_player.append(image_album_art);

	box_player.append(box_right);
	box_right.set_orientation(Gtk::Orientation::VERTICAL);

	label_title.get_style_context()->add_class("label_title");
	label_title.set_ellipsize(Pango::EllipsizeMode::END);
	label_title.set_max_width_chars(0);
	box_right.append(label_title);

	label_artist.get_style_context()->add_class("label_artist");
	label_artist.set_ellipsize(Pango::EllipsizeMode::END);
	label_artist.set_max_width_chars(0);
	box_right.append(label_artist);

	button_previous.set_icon_name("media-skip-backward");
	button_play_pause.set_icon_name("media-playback-pause");
	button_next.set_icon_name("media-skip-forward");

	button_previous.set_focusable(false);
	button_play_pause.set_focusable(false);
	button_next.set_focusable(false);

	button_previous.set_has_frame(false);
	button_play_pause.set_has_frame(false);
	button_next.set_has_frame(false);

	button_previous.signal_clicked().connect([&]() {
		playerctl_player_previous(player, nullptr);
	});

	button_play_pause.signal_clicked().connect([&]() {
		playerctl_player_play_pause(player, nullptr);
	});

	button_next.signal_clicked().connect([&]() {
		playerctl_player_next(player, nullptr);
	});

	box_right.append(box_controls);
	box_controls.get_style_context()->add_class("box_controls");
	box_controls.set_vexpand(true);
	box_controls.set_hexpand(true);
	box_controls.set_halign(Gtk::Align::CENTER);
	box_controls.set_valign(Gtk::Align::END);

	box_controls.append(button_previous);
	box_controls.append(button_play_pause);
	box_controls.append(button_next);
	
	//box_right.append(progressbar_playback);
}
