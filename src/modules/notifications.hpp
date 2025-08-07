#pragma once
#include "../module.hpp"
#ifdef MODULE_NOTIFICATION

#include <giomm/dbusconnection.h>
#include <gtkmm/revealer.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/flowbox.h>

class notification : public Gtk::Revealer {
	public:
		notification(const Gtk::Box&, const Glib::ustring&, const Glib::VariantContainerBase&, const std::string&);

		Gtk::Button button_main;
		sigc::connection timeout_connection;

		guint32 notif_id;
		guint32 replaces_id;

	private:
		Gtk::Box box_main;

		Glib::ustring app_name;
		Glib::ustring app_icon;
		Glib::ustring summary;
		Glib::ustring body;
		Glib::RefPtr<Gdk::Pixbuf> image_data;

		void handle_hint(const Glib::ustring&, const Glib::VariantBase&);
};

class module_notifications : public module {
	public:
		module_notifications(sysbar*, const bool&);

		int notif_count;
		Gtk::Box box_notifications;
		Gtk::ScrolledWindow scrolledwindow_notifications;

	private:
		Gtk::Box box_header;
		Gtk::Label label_notif_count;
		Gtk::Button button_clear;
		Gtk::Window window_alert;
		Gtk::FlowBox flowbox_alert;
		Gtk::ScrolledWindow scrolledwindow_alert;
		sigc::connection timeout_connection;
		Glib::RefPtr<Gio::DBus::Connection> daemon_connection;
		const Gio::DBus::InterfaceVTable interface_vtable{sigc::mem_fun(*this, &module_notifications::on_interface_method_call)};

		std::string command;
		guint object_id;

		void setup_widget();
		void on_overlay_change();
		void setup_daemon();
		void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connection, const Glib::ustring &name);
		void on_interface_method_call(
			const Glib::RefPtr<Gio::DBus::Connection> &connection,
			const Glib::ustring &sender, const Glib::ustring &object_path,
			const Glib::ustring &interface_name, const Glib::ustring &method_name,
			const Glib::VariantContainerBase &parameters,
			const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);
};

#endif
