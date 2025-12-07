#pragma once
#include "../module.hpp"
#ifdef MODULE_NOTIFICATION

#include <giomm/dbusconnection.h>
#include <gtkmm/revealer.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/flowbox.h>
#include <map>
#include <ctime>

class module_notifications;

class notification : public Gtk::Revealer {
public:
	notification(
		guint32 id, 
		const Glib::ustring& sender, 
		const Glib::VariantContainerBase& parameters, 
		const std::string& command,
		const Glib::RefPtr<Gio::DBus::Connection>& connection,
		module_notifications* module
	);

	Gtk::Button button_main;
	sigc::connection timeout_connection;
	sigc::signal<void(guint32)> signal_close;

	guint32 notif_id;
	guint32 replaces_id;
	int32_t expire_timeout;
	bool is_transient;

	void cleanup();

private:
	Gtk::Box box_main;
	Glib::ustring app_name;
	Glib::ustring app_icon;
	Glib::ustring summary;
	Glib::ustring body;
	Glib::RefPtr<Gdk::Pixbuf> image_data;
	std::vector<Glib::ustring> actions;
	std::tm timestamp;
	
	Glib::ustring dbus_sender;
	Glib::RefPtr<Gio::DBus::Connection> dbus_connection;
	module_notifications* parent_module;

	bool parse_parameters(const Glib::VariantContainerBase& parameters);
	void build_ui();
	void parse_and_build_body(Gtk::Box* parent);
	void build_actions(Gtk::Box* parent);
	std::string parse_markup(const Glib::ustring& text);
	void invoke_action(const Glib::ustring& action_key);
	void handle_hint(const Glib::ustring& key, const Glib::VariantBase& value);
};

class module_notifications : public module {
public:
	module_notifications(sysbar*, const bool&);

	Gtk::Box box_notifications;
	Gtk::ScrolledWindow scrolledwindow_notifications;

	void close_notification(guint32 id, guint32 reason);

private:
	Gtk::Box box_header;
	Gtk::Label label_notif_count;
	Gtk::Button button_clear;
	Gtk::Window window_alert;
	Gtk::FlowBox flowbox_alert;
	Gtk::ScrolledWindow scrolledwindow_alert;
	sigc::connection alert_timeout_connection;
	Glib::RefPtr<Gio::DBus::Connection> daemon_connection;
	const Gio::DBus::InterfaceVTable interface_vtable{
		sigc::mem_fun(*this, &module_notifications::on_interface_method_call)
	};

	std::string command;
	guint object_id;
	guint32 next_notif_id;
	std::map<guint32, notification*> notifications_map;

	void setup_widget();
	void on_overlay_change();
	void setup_daemon();
	void update_ui_state();
	void clear_all_notifications();
	void remove_notification(guint32 id, guint32 reason);
	void send_closed_signal(guint32 id, guint32 reason);
	void show_alert_notification(guint32 notif_id, const Glib::ustring& sender, const Glib::VariantContainerBase& parameters);
	guint32 handle_notify(const Glib::ustring& sender, const Glib::VariantContainerBase& parameters);
	
	void on_bus_acquired(
		const Glib::RefPtr<Gio::DBus::Connection>& connection, 
		const Glib::ustring& name
	);
	void on_interface_method_call(
		const Glib::RefPtr<Gio::DBus::Connection>& connection,
		const Glib::ustring& sender, 
		const Glib::ustring& object_path,
		const Glib::ustring& interface_name, 
		const Glib::ustring& method_name,
		const Glib::VariantContainerBase& parameters,
		const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation
	);
};

#endif