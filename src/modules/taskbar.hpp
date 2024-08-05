#pragma once
#ifdef MODULE_TASKBAR

#include "../module.hpp"
#include "../wlr-foreign-toplevel-management-unstable-v1.h"

class module_taskbar : public module {
	public:
		module_taskbar(const config_bar &cfg, const bool &icon_on_start = false, const bool &clickable = false);

	private:
		void setup_proto();
};

#endif
