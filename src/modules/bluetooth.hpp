#include "../module.hpp"
#include "controls.hpp"

#include <giomm/dbusconnection.h>
#include <giomm/dbusproxy.h>

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
	std::string interface;
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
		module_bluetooth(sysbar*, const bool&);
		bool test();

	private:
		Glib::RefPtr<Gio::DBus::Proxy> prop_proxy;

		std::vector<adapter> adapters;
		std::vector<device> devices;
		adapter default_adapter;

		#ifdef MODULE_CONTROLS
		control* control_bluetooth;
		void setup_control();
		#endif

		void on_properties_changed(
			const Glib::ustring&,
			const Glib::ustring&,
			const Glib::VariantContainerBase&);
		void extract_data(const Glib::VariantBase&);
		void update_info(const std::string&);
};
