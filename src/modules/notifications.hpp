#pragma once
#ifdef MODULE_NOTIFICATION

#include "../module.hpp"

class module_notifications : public module {
	public:
		module_notifications(const bool &icon_on_start = false, const bool &clickable = false);

	private:
		bool update_info();
};

#endif
