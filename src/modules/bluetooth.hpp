#include "../module.hpp"

#include <giomm/dbusconnection.h>
#include <giomm/dbusproxy.h>

using DBusPropMap = const Gio::DBus::Proxy::MapChangedProperties&;
using DBusPropList = const std::vector<Glib::ustring>&;

class module_bluetooth : public module {
	public:
		module_bluetooth(sysbar *window, const bool &icon_on_start = true);

	private:
		void update_info(DBusPropMap changed, DBusPropList invalid);
};
