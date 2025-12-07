#pragma once
#include "../module.hpp"
#include "controls.hpp"

#include <giomm/dbusconnection.h>
#include <giomm/dbusproxy.h>
#include <vector>

#ifdef MODULE_CONTROLS
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/revealer.h>
#endif

struct device {
	std::string path;
	std::string name;
	std::string icon;
	bool blocked;
	bool connected;
	bool paired;
	bool trusted;
};

struct adapter {
	std::string path;
	std::string alias;
	bool powered;
	bool discoverable;
	bool discovering;
};

class module_bluetooth : public module {
public:
	module_bluetooth(sysbar*, bool);

private:
	Glib::RefPtr<Gio::DBus::Proxy> adapter_proxy;
	Glib::RefPtr<Gio::DBus::Proxy> manager_proxy;
	Glib::RefPtr<Gio::DBus::Proxy> agent_manager;

	std::vector<adapter> adapters;
	std::vector<device> devices;
	adapter default_adapter;
	bool scanning;
	guint agent_registration_id;

	void register_agent();
	void handle_agent_method(
		const Glib::ustring&,
		const Glib::VariantContainerBase&,
		const Glib::RefPtr<Gio::DBus::MethodInvocation>&);

	void fetch_all_objects();
	void update_icon();

	void on_interfaces_added(
		const Glib::DBusObjectPathString&,
		const std::map<Glib::ustring, std::map<Glib::ustring, Glib::VariantBase>>&);
	void on_interfaces_removed(
		const Glib::DBusObjectPathString&,
		const std::vector<Glib::ustring>&);

	#ifdef MODULE_CONTROLS
	control* ctrl{nullptr};
	Gtk::FlowBox* flowbox{nullptr};
	Gtk::Button* btn_scan{nullptr};
	Gtk::Box* container{nullptr};

	void setup_control();
	void refresh_device_list();
	void create_device_widget(const device&);
	void update_device_widget(Gtk::FlowBoxChild*, const device&);

	void on_scan_clicked();
	void start_discovery();
	void stop_discovery();
	void toggle_device(const std::string&);
	void remove_device(const std::string&);
	#endif
};