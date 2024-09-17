#pragma once
#include "../module.hpp"
#ifdef MODULE_CELLULAR

class module_cellular : public module {
	public:
		module_cellular(sysbar *window, const bool &icon_on_start = false);

	private:
		void setup();
		void on_properties_changed(
			const Glib::ustring& sender_name,
			const Glib::ustring& signal_name,
			const Glib::VariantContainerBase& parameters);
};

#endif
