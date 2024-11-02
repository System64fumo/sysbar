#pragma once
#include "../module.hpp"
#ifdef MODULE_CLOCK

class module_clock : public module {
	public:
		module_clock(sysbar*, const bool&);

	private:
		int interval = 1000;
		std::string label_format = "%H:%M";
		std::string tooltip_format = "%Y/%m/%d";
		std::vector<int> widget_layout = {0, 0, 4, 4};

		bool update_info();
		void setup_widget();
};

#endif
