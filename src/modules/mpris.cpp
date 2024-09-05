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

module_mpris::module_mpris(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_mpris");
	dispatcher_callback.connect(sigc::mem_fun(*this, &module_mpris::update_info));

	// Setup
	PlayerctlPlayer *player = nullptr;
	GList *players = playerctl_list_players(nullptr);

	// Itterate over all players
	for (auto plr = players; plr != nullptr; plr = plr->next) {
		auto plr_name = static_cast<PlayerctlPlayerName*>(plr->data);
		player = playerctl_player_new_from_name(plr_name, nullptr);
		break;
	}

	g_object_get(player, "playback-status", &status, nullptr);
	g_signal_connect(player, "playback-status", G_CALLBACK(playback_status), this);
	g_signal_connect(player, "metadata", G_CALLBACK(metadata), this);
	metadata(player, nullptr, this);
	update_info();
	setup_widget();
}

void module_mpris::update_info() {
	std::string status_icon = status ? "player_play" : "player_pause";
	image_icon.set_from_icon_name(status_icon);
	label_info.set_text(title);

	label_title.set_text(title);
	label_artist.set_text(artist);

	// No worky yet
	/*if (album_art_url.find("file://") == 0) {
		album_art_url.erase(0, 7);
		image_album_art.set(album_art_url);
	}*/

	// TODO: Playback status can be tracked using a timer by getting..
	// the "length" property every second if the player is playing
	//progressbar_playback.set_fraction(0.5);
}

void module_mpris::setup_widget() {
	auto container = static_cast<Gtk::Box*>(win->box_widgets_end);
	container->append(box_player);

	// For some reason this makes things omega unstable..
	// Disabling for now..
	image_album_art.set_from_icon_name("music-app-symbolic");
	image_album_art.set_size_request(96 ,96);
	box_player.append(image_album_art);

	box_player.append(box_right);
	box_right.set_orientation(Gtk::Orientation::VERTICAL);

	// I REALLY hate how popovers work..
	label_title.set_halign(Gtk::Align::START);
	label_title.set_wrap(true);
	label_title.set_ellipsize(Pango::EllipsizeMode::END);
	label_title.set_max_width_chars(0);
	box_right.append(label_title);

	label_artist.set_halign(Gtk::Align::START);
	label_artist.set_wrap(true);
	label_artist.set_ellipsize(Pango::EllipsizeMode::END);
	label_artist.set_max_width_chars(0);
	box_right.append(label_artist);
	//box_right.append(progressbar_playback);
}
