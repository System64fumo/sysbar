#include "../config_parser.hpp"
#include "mpris.hpp"

static void playback_status(PlayerctlPlayer *player, PlayerctlPlaybackStatus status, gpointer user_data) {
	std::string status_msg = status ? "paused" : "playing";
	std::printf("Player status: %s\n", status_msg.c_str());
}

module_mpris::module_mpris(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_mpris");
	image_icon.set_from_icon_name("player_play");
	label_info.hide();

	// Setup
	PlayerctlPlayer *player = nullptr;
	PlayerctlPlayerManager *player_manager = playerctl_player_manager_new(nullptr);
	GList *players = playerctl_list_players(nullptr);

	// Itterate over all players
	for (auto plr = players; plr != nullptr; plr = plr->next) {
		auto plr_name = static_cast<PlayerctlPlayerName*>(plr->data);
		player = playerctl_player_new_from_name(plr_name, nullptr);
		break;
	}

	g_signal_connect(player, "playback-status", G_CALLBACK(playback_status), nullptr);
}
