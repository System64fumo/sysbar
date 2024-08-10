#pragma once
#include "../module.hpp"
#ifdef MODULE_HYPRLAND

#include <queue>
#include <mutex>

class module_hyprland : public module {
	public:
		module_hyprland(sysbar *window, const bool &icon_on_start = true);

	private:
		int character_limit = 128;
		Glib::Dispatcher dispatcher;

		std::queue<std::string> data_queue;
		std::mutex mutex;

		void update_info();
		void socket_listener();
};

#endif
