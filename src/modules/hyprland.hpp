#pragma once
#include "../module.hpp"
#ifdef MODULE_HYPRLAND

#include <queue>
#include <mutex>

class module_hyprland : public module {
	public:
		module_hyprland(sysbar*, const bool&);

	private:
		Glib::Dispatcher dispatcher;

		int character_limit;
		std::queue<std::string> data_queue;
		std::mutex mutex;

		void update_info();
		void socket_listener();
};

#endif
