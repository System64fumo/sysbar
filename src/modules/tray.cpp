#include "tray.hpp"

#include <iostream>

// Tray module
module_tray::module_tray(const bool &icon_on_start, const bool &clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_tray");
	image_icon.set_from_icon_name("arrow-right");
	label_info.hide();
}

bool module_tray::update_info() {
	return true;
}

// Tray watcher
tray_watcher::tray_watcher() : watcher_id(0) {
	auto pid = std::to_string(getpid());
	auto host_id = "org.kde.StatusNotifierHost-" + pid + "-" + std::to_string(++hosts_counter);
	Gio::DBus::own_name(Gio::DBus::BusType::SESSION, host_id, sigc::mem_fun(*this, &tray_watcher::on_bus_acquired));
}

void tray_watcher::on_bus_acquired(const DBusConnection& conn, const Glib::ustring& name) {
	watcher_id = Gio::DBus::watch_name(conn,"org.kde.StatusNotifierWatcher",
		sigc::mem_fun(*this, &tray_watcher::on_name_appeared),
		sigc::mem_fun(*this, &tray_watcher::on_name_vanished));
}

void tray_watcher::on_name_appeared(const DBusConnection &conn, const Glib::ustring &name, const Glib::ustring &owner) {
	Gio::DBus::Proxy::create(conn, "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher",
		[this, host_name = name](const Glib::RefPtr<Gio::AsyncResult> &result) {
			watcher_proxy = Gio::DBus::Proxy::create_finish(result);
			watcher_proxy->call("RegisterStatusNotifierHost", Glib::Variant<std::tuple<Glib::ustring>>::create({ host_name }));
			watcher_proxy->signal_signal().connect(sigc::mem_fun(*this, &tray_watcher::handle_signal));

			Glib::Variant<std::vector<Glib::ustring>> registered_items;
			watcher_proxy->get_cached_property(registered_items, "RegisteredStatusNotifierItems");

			// Add existing items
			for (const auto& service : registered_items.get()) {
				std::cout << "Adding service: " << service << std::endl;
				items.emplace(service, service);
			}
		});
}

void tray_watcher::on_name_vanished(const DBusConnection& conn, const Glib::ustring& name) {
	Gio::DBus::unwatch_name(watcher_id);
}

void tray_watcher::handle_signal(const Glib::ustring& sender, const Glib::ustring& signal, const Glib::VariantContainerBase& params) {
	if (!params.is_of_type(Glib::VariantType("(s)")))
		return;

	Glib::Variant<Glib::ustring> item_path;
	params.get_child(item_path);
	Glib::ustring service = item_path.get();

	if (signal == "StatusNotifierItemRegistered") {
		std::cout << "Adding: " << service << std::endl;
		items.emplace(service, service);
	}
	else if (signal == "StatusNotifierItemUnregistered") {
		std::cout << "Removing: " << service << std::endl;
		items.erase(service);
	}
}

// Tray item
tray_item::tray_item(const Glib::ustring & service) {
	const auto slash_ind = service.find('/');
	dbus_name = service.substr(0, slash_ind);
	dbus_path = (slash_ind != Glib::ustring::npos) ? service.substr(slash_ind) : "/StatusNotifierItem";

	Gio::DBus::Proxy::create_for_bus(
		Gio::DBus::BusType::SESSION, dbus_name, dbus_path, "org.kde.StatusNotifierItem",
		[this](const Glib::RefPtr<Gio::AsyncResult> &result) {
			item_proxy = Gio::DBus::Proxy::create_for_bus_finish(result);

			item_proxy->signal_signal().connect([this](const Glib::ustring &sender, const Glib::ustring &signal,
													   const Glib::VariantContainerBase &params) {
				if (signal.size() >= 3) {
					std::string_view property(signal.c_str() + 3, signal.size() - 3);
					std::cout << "Property updated: " << property << std::endl;
					update_properties();
				}
			});

			update_properties();
		}
	);
}

void tray_item::update_properties() {
	// TODO: Actually do something with the info we got
	auto label = get_item_property<Glib::ustring>("Title");
	auto [tooltip_icon_name, tooltip_icon_data, tooltip_title, tooltip_text] = get_item_property<std::tuple<Glib::ustring, std::vector<std::tuple<gint32, gint32, std::vector<guint8>>>, Glib::ustring, Glib::ustring>>("ToolTip");
	auto icon_theme_path = get_item_property<Glib::ustring>("IconThemePath");
	Glib::ustring icon_type_name = get_item_property<Glib::ustring>("Status") == "NeedsAttention" ? "AttentionIcon" : "Icon";
	auto icon_name = get_item_property<Glib::ustring>(icon_type_name + "Name");
	auto status = get_item_property<Glib::ustring>("Status");
	auto menu_path = get_item_property<Glib::DBusObjectPathString>("Menu");

	std::cout << "Label: " << label << std::endl;
	std::cout << "ToolTip: " << tooltip_title + tooltip_text << std::endl;
	std::cout << "IconThemePath: " << icon_theme_path << std::endl;
	std::cout << "icon_type_name: " << icon_type_name << std::endl;
	std::cout << "IconName: " << icon_name << std::endl;
	std::cout << "Status: " << status << std::endl;
	std::cout << "menu_path: " << menu_path << std::endl;
	std::cout << std::endl;

	// TODO: Write context menu code
	if (menu_path.empty())
		return;
}
