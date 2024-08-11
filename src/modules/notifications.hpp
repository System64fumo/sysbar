#pragma once
#include "../module.hpp"
#ifdef MODULE_NOTIFICATION

#include <giomm/dbusconnection.h>

class notification : public Gtk::Box {
	public:
		notification(Gtk::Box *box_notifications, const Glib::ustring &sender, const Glib::VariantContainerBase &parameters);
		guint32 id;

	private:
		Glib::ustring app_name;
		Glib::ustring app_icon;
		Glib::ustring summary;
		Glib::ustring body;
};

class module_notifications : public module {
	public:
		module_notifications(sysbar *window, const bool &icon_on_start = true);

	private:
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
