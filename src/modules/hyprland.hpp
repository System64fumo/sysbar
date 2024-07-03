#pragma once
#ifdef MODULE_HYPRLAND

#include "../module.hpp"

class module_hyprland : public module {
	public:
		module_hyprland(const bool &icon_on_start = false, const bool &clickable = false);

	private:
		std::string data;
		int character_limit = 128;

		void update_info();
		void socket_listener();
};

#endif
