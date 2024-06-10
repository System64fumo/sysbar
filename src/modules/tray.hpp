#include "../module.hpp"

class module_tray : public module {
	public:
		module_tray(const bool &icon_on_start = false, const bool &clickable = false);

	private:
		bool update_info();
};
