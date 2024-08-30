#pragma once
#include "../module.hpp"
#ifdef MODULE_MPRIS

#include <playerctl.h>

class module_mpris : public module {
	public:
		module_mpris(sysbar *window, const bool &icon_on_start = true);
};

#endif
