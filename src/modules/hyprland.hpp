#pragma once
#ifdef MODULE_HYPRLAND

#include "../module.hpp"

#include <queue>
#include <mutex>

class module_hyprland : public module {
	public:
		module_hyprland(const config &cfg, const bool &icon_on_start = false, const bool &clickable = false);

	private:
		int character_limit = 128;
		Glib::Dispatcher dispatcher;

		std::queue<std::string> data_queue;
		std::mutex mutex;

		void update_info();
		void socket_listener();
};

#endif
