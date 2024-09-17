#include "../module.hpp"

#include <giomm/dbusconnection.h>
#include <giomm/dbusproxy.h>

using DBusPropMap = const Gio::DBus::Proxy::MapChangedProperties&;
using DBusPropList = const std::vector<Glib::ustring>&;

struct device {
	std::string path;
	bool blocked;
	bool connected;
	std::string icon;
	std::string name;
	bool paired;
	bool trusted;
};

struct adapter {
	std::string path;
	std::string alias;
	bool discoverable;
	bool discovering;
	std::string name;
	bool pairable;
	bool powered;
};

class module_bluetooth : public module {
	public:
		module_bluetooth(sysbar *window, const bool &icon_on_start = true);

	private:
		Glib::RefPtr<Gio::DBus::Proxy> prop_proxy;
		std::vector<adapter> adapters;
		std::vector<device> devices;
		void on_properties_changed(
			const Glib::ustring& sender_name,
			const Glib::ustring& signal_name,
			const Glib::VariantContainerBase& parameters);
		void extract_data(const Glib::VariantBase& variant_base);
};
