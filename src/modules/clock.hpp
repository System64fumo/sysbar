#pragma once
#include "../module.hpp"
#ifdef MODULE_CLOCK

class module_clock : public module {
	public:
		module_clock(const config_bar &cfg, const bool &icon_on_start = false, const bool &clickable = false);

	private:
		int interval = 1000;
		std::string label_format = "%H:%M";
		std::string tooltip_format = "%Y/%m/%d";
		bool update_info();
};

#endif
