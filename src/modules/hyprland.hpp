#pragma once
#include "../module.hpp"
#ifdef MODULE_HYPRLAND

#include <glibmm/dispatcher.h>
#include <queue>
#include <mutex>

class module_hyprland : public module {
	public:
		module_hyprland(sysbar*, const bool&);

	private:
		struct workspace {
			int id;
			bool active;
			bool fullscreen;
		};
		struct monitor {
			std::string name;
			bool active;
			std::vector<workspace> workspaces;
		};

		int new_workspace;

		Glib::Dispatcher dispatcher;

		int character_limit;
		std::queue<std::string> data_queue;
		std::mutex mutex;
		std::vector<monitor> monitors;

		void update_info();
		void socket_listener();
};

#endif
