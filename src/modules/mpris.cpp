#include "mpris.hpp"

static void playback_status(PlayerctlPlayer *player, PlayerctlPlaybackStatus status, gpointer user_data) {
	module_mpris *self = static_cast<module_mpris*>(user_data);
	self->status = status;
	self->dispatcher_callback.emit();
}

module_mpris::module_mpris(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_mpris");
	image_icon.set_from_icon_name("player_play");
	label_info.hide();
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

	// TODO: Get status on startup
	g_signal_connect(player, "playback-status", G_CALLBACK(playback_status), this);
}

void module_mpris::update_info() {
	std::string status_icon = status ? "player_play" : "player_pause";
	image_icon.set_from_icon_name(status_icon);
}
