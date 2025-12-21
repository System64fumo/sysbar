#pragma once
#include "../module.hpp"
#ifdef MODULE_TRAY

#include <giomm/dbusproxy.h>
#include <giomm/dbuswatchname.h>
#include <giomm/dbusownname.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/popover.h>
#include <gtkmm/gestureclick.h>

class tray_item : public Gtk::Image {
	public:
		tray_item(const Glib::ustring& service);
		~tray_item();

	private:
		Gtk::Popover popover_context;
		Gtk::FlowBox flowbox_context;
		Glib::ustring dbus_name;
		Glib::ustring dbus_path;
		Glib::DBusObjectPathString menu_path;
		Glib::RefPtr<Gio::DBus::Proxy> item_proxy;
		Glib::RefPtr<Gtk::GestureClick> gesture_right_click;
		bool menu_needs_rebuild = true;

		void on_right_clicked(const int&, const double&, const double&);
		void on_menu_item_click(Gtk::FlowBoxChild*);
		void update_properties();
		void build_menu(const Glib::VariantBase&);
		void rebuild_menu();
		static Glib::RefPtr<Gdk::Pixbuf> extract_pixbuf(const std::vector<std::tuple<gint32, gint32, std::vector<guint8>>>&);
};

class tray_watcher {
	public:
		tray_watcher(Gtk::Box*);
		~tray_watcher();

	private:
		Gtk::Box* box_container;
		std::map<Glib::ustring, std::unique_ptr<tray_item>> items;
		std::map<Glib::ustring, guint> item_watches;

		guint watcher_id = 0;
		Glib::RefPtr<Gio::DBus::Connection> watcher_connection;
		Glib::RefPtr<Gio::DBus::Proxy> watcher_proxy;

		const Gio::DBus::InterfaceVTable interface_table = 
			Gio::DBus::InterfaceVTable(
				sigc::mem_fun(*this, &tray_watcher::on_interface_method_call),
				sigc::mem_fun(*this, &tray_watcher::on_interface_get_property));

		void on_interface_method_call(const Glib::RefPtr<Gio::DBus::Connection>&,
			const Glib::ustring&, const Glib::ustring&, const Glib::ustring&,
			const Glib::ustring&, const Glib::VariantContainerBase&,
			const Glib::RefPtr<Gio::DBus::MethodInvocation>&);

		void on_interface_get_property(Glib::VariantBase&,
			const Glib::RefPtr<Gio::DBus::Connection>&,
			const Glib::ustring&, const Glib::ustring&,
			const Glib::ustring&, const Glib::ustring&);

		void on_bus_acquired_host(const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring&);
		void on_bus_acquired_watcher(const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring&);
		void on_name_appeared(const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring&, const Glib::ustring&);
		void on_name_vanished(const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring&);
		void handle_signal(const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&);
		void add_item(const Glib::ustring&, const Glib::ustring&);
		void remove_item(const Glib::ustring&);
};

class module_tray : public module {
	public:
		module_tray(sysbar*, const bool& = true);

	private:
		bool m_icon_on_start;
		Gtk::Revealer revealer_box;
		Gtk::Box box_container;
		tray_watcher watcher;
		Glib::RefPtr<Gtk::GestureClick> gesture_click;

		void on_clicked(const int&, const double&, const double&);
};

#endif