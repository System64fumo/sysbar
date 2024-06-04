#include "../module.hpp"

class module_network : public module {
	public:
		module_network(bool icon_on_start = false, bool clickable = false);
		~module_network();

	private:
		bool update_info();
};
