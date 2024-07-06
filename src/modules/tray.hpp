#pragma once
#ifdef MODULE_TRAY

#include "../module.hpp"

#include <giomm/dbusconnection.h>
#include <giomm/dbusproxy.h>
#include <giomm/dbuswatchname.h>
#include <giomm/dbusownname.h>
#include <gtkmm/box.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/image.h>
#include <gtkmm/revealer.h>
#include <gtkmm/popover.h>

using DBusConnection = Glib::RefPtr<Gio::DBus::Connection>;
using DBusProxy = Glib::RefPtr<Gio::DBus::Proxy>;
using DBusVariant = Glib::VariantBase;
using DBusSignalHandler = sigc::slot<void(const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&)>;

class tray_item : public Gtk::Image {
	public:
		tray_item(const Glib::ustring &service);
		~tray_item();

	private:
		Gtk::Popover popover_context;
		Gtk::FlowBox flowbox_context;
		Glib::ustring dbus_name;
		Glib::ustring dbus_path;
		Glib::DBusObjectPathString menu_path;
		DBusProxy item_proxy;

		template<typename T> T get_item_property(const Glib::ustring &name, const T &default_value = {}) const {
			Glib::VariantBase variant;
			item_proxy->get_cached_property(variant, name);
			return variant && variant.is_of_type(Glib::Variant<T>::variant_type()) ? Glib::VariantBase::cast_dynamic<Glib::Variant<T>>(variant).get() : default_value;
		}

		Glib::RefPtr<Gtk::GestureClick> gesture_right_click;
		void on_right_clicked(const int &n_press, const double &x, const double &y);
		void on_menu_item_click(Gtk::FlowBoxChild *child);
		void update_properties();
		void build_menu(const Glib::VariantBase &layout);
};

class tray_watcher {
	public:
		tray_watcher(Gtk::Box *box);

	private:
		Gtk::Box *box_container;
		std::map<Glib::ustring, tray_item> items;
		int hosts_counter = 0;

		guint watcher_id;
		DBusConnection watcher_connection;
		DBusProxy watcher_proxy;

	const Gio::DBus::InterfaceVTable interface_table =
		Gio::DBus::InterfaceVTable(
		sigc::mem_fun(*this, &tray_watcher::on_interface_method_call),
		sigc::mem_fun(*this, &tray_watcher::on_interface_get_property));

	void on_interface_method_call(const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &sender,
		const Glib::ustring &object_path, const Glib::ustring &interface_name,
		const Glib::ustring &method_name, const Glib::VariantContainerBase &parameters,
		const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);

	void on_interface_get_property(Glib::VariantBase &property,
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &sender, const Glib::ustring &object_path,
		const Glib::ustring &interface_name, const Glib::ustring &property_name);

		void on_bus_acquired_host(const DBusConnection &conn, const Glib::ustring &name);
		void on_bus_acquired_watcher(const DBusConnection &conn, const Glib::ustring &name);
		void on_name_appeared(const DBusConnection &conn, const Glib::ustring &name, const Glib::ustring &owner);
		void on_name_vanished(const DBusConnection &conn, const Glib::ustring &name);
		void handle_signal(const Glib::ustring &sender, const Glib::ustring &signal, const Glib::VariantContainerBase &params);
};

class module_tray : public module {
	public:
		module_tray(const bool &icon_on_start = false, const bool &clickable = false);

	private:
		Gtk::Revealer revealer_box;
		Gtk::Box box_container;

		tray_watcher watcher = tray_watcher(&box_container);
		Glib::RefPtr<Gtk::GestureClick> gesture_click;
		void on_clicked(const int &n_press, const double &x, const double &y);
};

#endif
