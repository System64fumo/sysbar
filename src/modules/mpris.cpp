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
	self->length = playerctl_player_print_metadata_prop(player, "mpris:length", nullptr);

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

	// TODO: Get metadata on startup
	g_object_get(player, "playback-status", &status, nullptr);
	g_signal_connect(player, "playback-status", G_CALLBACK(playback_status), this);
	g_signal_connect(player, "metadata", G_CALLBACK(metadata), this);
	update_info();
}

void module_mpris::update_info() {
	std::string status_icon = status ? "player_play" : "player_pause";
	image_icon.set_from_icon_name(status_icon);
	label_info.set_text(title);
}
