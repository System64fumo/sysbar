#pragma once
#include "../module.hpp"
#ifdef MODULE_HYPRLAND

#include <unordered_map>
#include <glibmm/dispatcher.h>
#include <queue>
#include <mutex>

class module_hyprland : public module {
	public:
		module_hyprland(sysbar*, const bool&);

	private:
		struct window {
			std::string id;
			bool fullscreen;
		};
		struct monitor {
			std::string connector;
		};


		int new_workspace;

		Glib::Dispatcher dispatcher;

		int character_limit;
		std::queue<std::string> data_queue;
		std::mutex mutex;
		std::unordered_map<std::string, monitor> monitors;
		std::unordered_map<std::string, window> windows;
		std::string fullscreen_cause_id;
		window* window_active;
		monitor* monitor_active;

		void update_info();
		void socket_listener();
		void update_fullscreen_status(const bool& override = false);
};

#endif
