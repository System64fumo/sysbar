#pragma once
#ifdef MODULE_CLOCK

#include "../module.hpp"

class module_clock : public module {
	public:
		module_clock(const bool &icon_on_start = false, const bool &clickable = false);

	private:
		bool update_info();
};

#endif
