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

struct NotificationData {
	guint32 id;
	guint32 replaces_id;
	Glib::ustring app_name;
	Glib::ustring app_icon;
	Glib::ustring summary;
	Glib::ustring body;
	std::vector<Glib::ustring> actions;
	Glib::RefPtr<Gdk::Pixbuf> image_data;
	int32_t expire_timeout;
	bool is_transient;
	std::tm timestamp;
	Glib::ustring sender;
};

class NotificationWidget : public Gtk::Revealer {
public:
	NotificationWidget(const NotificationData& data, bool is_alert);
	
	guint32 get_id() const { return data.id; }
	void start_timeout(std::function<void()> callback);
	void stop_timeout();
	void toggle_expand();
	
	sigc::signal<void(guint32)> signal_close;
	sigc::signal<void(Glib::ustring)> signal_action;

private:
	NotificationData data;
	bool is_alert;
	bool is_expanded;
	Gtk::Box box_main;
	Gtk::Label* lbl_body;
	Gtk::Button* btn_expand;
	sigc::connection timeout_conn;
	
	void build_ui();
	void build_body(Gtk::Box* parent);
	void build_actions(Gtk::Box* parent);
	void update_body_display();
	Glib::ustring sanitize_markup(const Glib::ustring& text);
};

class module_notifications : public module {
public:
	module_notifications(sysbar*, const bool&);
	
	void close_notification(guint32 id, guint32 reason);
	void send_action_signal(guint32 id, const Glib::ustring& action, const Glib::ustring& sender);

private:
	Gtk::Box box_header;
	Gtk::Label label_count;
	Gtk::Button button_clear;
	Gtk::FlowBox flowbox_list;
	Gtk::ScrolledWindow scroll_list;
	Gtk::Window window_alert;
	Gtk::FlowBox flowbox_alert;
	Gtk::ScrolledWindow scroll_alert;

	Glib::RefPtr<Gio::DBus::Connection> connection;
	const Gio::DBus::InterfaceVTable vtable{
		sigc::mem_fun(*this, &module_notifications::on_method_call)
	};
	guint object_id;

	guint32 next_id;
	std::map<guint32, NotificationData> notifications;
	std::map<guint32, NotificationWidget*> list_widgets;
	std::map<guint32, NotificationWidget*> alert_widgets;
	std::string command;
	
	void setup_ui();
	void setup_dbus();
	void update_ui();
	void clear_all();
	void clear_alerts();
	
	guint32 handle_notify(const Glib::ustring& sender, const Glib::VariantContainerBase& params);
	void show_notification(const NotificationData& data);
	void remove_notification(guint32 id, guint32 reason);
	void send_closed_signal(guint32 id, guint32 reason);
	
	NotificationData parse_notification(guint32 id, const Glib::ustring& sender, 
		const Glib::VariantContainerBase& params);
	void parse_hints(NotificationData& data, const std::map<Glib::ustring, Glib::VariantBase>& hints);
	
	void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& conn, const Glib::ustring& name);
	void on_method_call(const Glib::RefPtr<Gio::DBus::Connection>& conn,
		const Glib::ustring& sender, const Glib::ustring& path,
		const Glib::ustring& iface, const Glib::ustring& method,
		const Glib::VariantContainerBase& params,
		const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation);
};

#endif