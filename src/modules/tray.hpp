#include "../module.hpp"

#include <giomm/dbusconnection.h>
#include <giomm/dbusproxy.h>
#include <giomm/dbuswatchname.h>
#include <giomm/dbusownname.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

using DBusConnection = Glib::RefPtr<Gio::DBus::Connection>;
using DBusProxy = Glib::RefPtr<Gio::DBus::Proxy>;
using DBusVariant = Glib::VariantBase;
using DBusSignalHandler = sigc::slot<void(const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&)>;

class tray_item : public Gtk::Button {
	public:
		tray_item(const Glib::ustring &service);

	private:
		Gtk::Image icon;

		Glib::ustring dbus_name;
		Glib::ustring dbus_path;
		DBusProxy item_proxy;

		template<typename T> T get_item_property(const Glib::ustring &name, const T &default_value = {}) const {
			Glib::VariantBase variant;
			item_proxy->get_cached_property(variant, name);
			return variant && variant.is_of_type(Glib::Variant<T>::variant_type()) ? Glib::VariantBase::cast_dynamic<Glib::Variant<T>>(variant).get() : default_value;
		}

		void update_properties();
};

class tray_watcher {
	public:
		tray_watcher();

	private:
		std::map<Glib::ustring, tray_item> items;
		int hosts_counter = 0;

		guint watcher_id;
		DBusProxy watcher_proxy;

		void on_bus_acquired(const DBusConnection &conn, const Glib::ustring &name);
		void on_name_appeared(const DBusConnection &conn, const Glib::ustring &name, const Glib::ustring &owner);
		void on_name_vanished(const DBusConnection &conn, const Glib::ustring &name);
		void handle_signal(const Glib::ustring &sender, const Glib::ustring &signal, const Glib::VariantContainerBase &params);
};

class module_tray : public module {
	public:
		module_tray(const bool &icon_on_start = false, const bool &clickable = false);

	private:
		tray_watcher watcher;
		bool update_info();
};