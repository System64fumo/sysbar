#pragma once
#include "../module.hpp"
#ifdef MODULE_NOTIFICATION

#include <giomm/dbusconnection.h>
#include <gtkmm/button.h>

class notification : public Gtk::Button {
	public:
		notification(std::vector<notification*> notifications, Gtk::Box *box_notifications, const Glib::ustring &sender, const Glib::VariantContainerBase &parameters, const std::string);
		guint32 notif_id;

	private:
		Gtk::Box box_main;

		Glib::ustring app_name;
		guint32 replaces_id;
		Glib::ustring app_icon;
		Glib::ustring summary;
		Glib::ustring body;
		Glib::RefPtr<Gdk::Pixbuf> image_data;

		void handle_hint(Glib::ustring, const Glib::VariantBase&);
};

class module_notifications : public module {
	public:
		module_notifications(sysbar *window, const bool &icon_on_start = true);
		int notif_count = 0;
		std::vector<notification*> notifications;

	private:
		std::string command;
		Gtk::Box *box_notifications;
		guint object_id;
		Glib::RefPtr<Gio::DBus::Connection> daemon_connection;
		const Gio::DBus::InterfaceVTable interface_vtable{sigc::mem_fun(*this, &module_notifications::on_interface_method_call)};

		bool update_info();
		void setup_widget();
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
