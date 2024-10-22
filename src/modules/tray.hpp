#pragma once
#include "../module.hpp"
#ifdef MODULE_TRAY

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
		Glib::RefPtr<Gtk::GestureClick> gesture_right_click;

		template<typename T> T get_item_property(const Glib::ustring& name, const T &default_value = {}) const {
			Glib::VariantBase variant;
			item_proxy->get_cached_property(variant, name);
			return variant && variant.is_of_type(Glib::Variant<T>::variant_type()) ? Glib::VariantBase::cast_dynamic<Glib::Variant<T>>(variant).get() : default_value;
		}

		void on_right_clicked(const int&, const double&, const double&);
		void on_menu_item_click(Gtk::FlowBoxChild*);
		void update_properties();
		void build_menu(const Glib::VariantBase&);
};

class tray_watcher {
	public:
		tray_watcher(Gtk::Box*);

	private:
		Gtk::Box* box_container;
		std::map<Glib::ustring, tray_item> items;
		int hosts_counter;

		guint watcher_id;
		DBusConnection watcher_connection;
		DBusProxy watcher_proxy;

		const Gio::DBus::InterfaceVTable interface_table =
			Gio::DBus::InterfaceVTable(
			sigc::mem_fun(*this, &tray_watcher::on_interface_method_call),
			sigc::mem_fun(*this, &tray_watcher::on_interface_get_property));

		void on_interface_method_call(const Glib::RefPtr<Gio::DBus::Connection>&,
			const Glib::ustring&,
			const Glib::ustring&, const Glib::ustring&,
			const Glib::ustring&, const Glib::VariantContainerBase&,
			const Glib::RefPtr<Gio::DBus::MethodInvocation>&);

		void on_interface_get_property(Glib::VariantBase&,
			const Glib::RefPtr<Gio::DBus::Connection>&,
			const Glib::ustring&, const Glib::ustring&,
			const Glib::ustring&, const Glib::ustring&);

		void on_bus_acquired_host(const DBusConnection&, const Glib::ustring&);
		void on_bus_acquired_watcher(const DBusConnection&, const Glib::ustring&);
		void on_name_appeared(const DBusConnection&, const Glib::ustring &name, const Glib::ustring&);
		void on_name_vanished(const DBusConnection&, const Glib::ustring&);
		void handle_signal(const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&);
};

class module_tray : public module {
	public:
		module_tray(sysbar *window, const bool &icon_on_start = true);

	private:
		Gtk::Revealer revealer_box;
		Gtk::Box box_container;

		tray_watcher watcher = tray_watcher(&box_container);
		Glib::RefPtr<Gtk::GestureClick> gesture_click;
		void on_clicked(const int &n_press, const double &x, const double &y);
};

#endif
