#include "../config.hpp"
#include "tray.hpp"

#include <filesystem>
#include <iostream>

// Tray module
module_tray::module_tray(const bool &icon_on_start, const bool &clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_tray");
	label_info.hide();

	box_container.get_style_context()->add_class("tray_container");

	// TODO: Add an option to disable the revealer
	// TODO: Add an option to set the revealer's transition duration
	// TODO: Set custom transition types based on bar position or icon_on_start

	Gtk::RevealerTransitionType transition_type = Gtk::RevealerTransitionType::SLIDE_LEFT;

	// Set orientation
	if (position % 2) {
		image_icon.set_from_icon_name("arrow-down");
		transition_type = Gtk::RevealerTransitionType::SLIDE_DOWN;
		box_container.set_orientation(Gtk::Orientation::VERTICAL);
	}
	else {
		image_icon.set_from_icon_name("arrow-right");
	}

	revealer_box.set_child(box_container);
	revealer_box.set_transition_type(transition_type);
	revealer_box.set_transition_duration(250);
	revealer_box.set_reveal_child(false);

	if (icon_on_start)
		append(revealer_box);
	else
		prepend(revealer_box);

	// Custom on_clicked handle
	gesture_click = Gtk::GestureClick::create();
	gesture_click->set_button(GDK_BUTTON_PRIMARY);
	gesture_click->signal_pressed().connect(sigc::mem_fun(*this, &module_tray::on_clicked));
	add_controller(gesture_click);
}

void module_tray::on_clicked(int n_press, double x, double y) {
	// TODO: Change icon order when the icon is not at the start
	// Also use top/down arrows for vertical bars
	if (revealer_box.get_reveal_child()) {
		if (position % 2)
			image_icon.set_from_icon_name("arrow-down");
		else
		image_icon.set_from_icon_name("arrow-right");
		revealer_box.set_reveal_child(false);
	}
	else {
		if (position % 2)
			image_icon.set_from_icon_name("arrow-up");
		else
		image_icon.set_from_icon_name("arrow-left");
		revealer_box.set_reveal_child(true);
	}
}

// Tray watcher
tray_watcher::tray_watcher(Gtk::Box *box) : watcher_id(0) {
	box_container = box;
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
				if (verbose)
					std::cout << "Adding service: " << service << std::endl;
				items.emplace(service, service);
				auto it = items.find(service);
				tray_item& item = it->second;
				box_container->prepend(item);
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
		if (verbose)
			std::cout << "Adding: " << service << std::endl;
		items.emplace(service, service);
		auto it = items.find(service);
		tray_item& item = it->second;
		box_container->prepend(item);
	}
	else if (signal == "StatusNotifierItemUnregistered") {
		if (verbose)
			std::cout << "Removing: " << service << std::endl;
		auto it = items.find(service);
		tray_item& item = it->second;
		box_container->remove(item);
		items.erase(service);
	}
}

// Tray item
tray_item::tray_item(const Glib::ustring & service) {
	get_style_context()->add_class("tray_item");
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

	// TODO: add option to use custom icon sizes
	set_size_request(22,22);

	gesture_right_click = Gtk::GestureClick::create();
	gesture_right_click->set_button(GDK_BUTTON_SECONDARY);
	gesture_right_click->signal_pressed().connect(sigc::mem_fun(*this, &tray_item::on_right_clicked));
	add_controller(gesture_right_click);

	popovermenu_context.set_has_arrow(false);
	popovermenu_context.set_parent(*this);
}

static Glib::RefPtr<Gdk::Pixbuf> extract_pixbuf(std::vector<std::tuple<gint32, gint32, std::vector<guint8>>> && pixbuf_data) {
	if (pixbuf_data.empty())
		return {};

	auto chosen_image = std::max_element(pixbuf_data.begin(), pixbuf_data.end());
	auto & [width, height, data] = *chosen_image;

	// Convert ARGB to RGBA
	for (size_t i = 0; i + 3 < data.size(); i += 4) {
		const auto alpha = data[i];
		data[i]	 = data[i + 1];
		data[i + 1] = data[i + 2];
		data[i + 2] = data[i + 3];
		data[i + 3] = alpha;
	}

	auto *data_ptr = new auto(std::move(data));
	return Gdk::Pixbuf::create_from_data(
		data_ptr->data(), Gdk::Colorspace::RGB, true, 8, width, height,
		4 * width, [data_ptr] (auto*) { delete data_ptr; });
}

// TODO: Construct a menu from this mess
void print_menu_layout(const Glib::VariantBase& layout) {
	auto layout_tuple = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(layout);
	auto id_variant = layout_tuple.get_child(0);
	auto properties_variant = layout_tuple.get_child(1);
	auto children_variant = layout_tuple.get_child(2);

	std::cout << "ID variant type: " << id_variant.get_type_string() << std::endl;
	std::cout << "Properties variant type: " << properties_variant.get_type_string() << std::endl;
	std::cout << "Children variant type: " << children_variant.get_type_string() << std::endl;

	int id = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(id_variant).get();
	auto properties_dict = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>>(properties_variant);
	auto children = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::VariantBase>>>(children_variant);

	std::cout << "ID: " << id << std::endl;
	std::cout << "Properties:" << std::endl;
	for (const auto& key_value : properties_dict.get()) {
		std::cout << "  " << key_value.first << " = " << key_value.second.print() << std::endl;
	}

	for (const auto& child : children.get()) {
		print_menu_layout(child);
	}
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

	if (verbose) {
		std::cout << "Label: " << label << std::endl;
		std::cout << "ToolTip: " << tooltip_title + tooltip_text << std::endl;
		std::cout << "IconThemePath: " << icon_theme_path << std::endl;
		std::cout << "icon_type_name: " << icon_type_name << std::endl;
		std::cout << "IconName: " << icon_name << std::endl;
		std::cout << "Status: " << status << std::endl;
		std::cout << "dbus_name: " << dbus_name << std::endl;
		std::cout << "menu_path: " << menu_path << std::endl;
	}

	if (!tooltip_title.empty())
		set_tooltip_text(tooltip_title);
	else
		set_tooltip_text(label);

	std::string icon_path = icon_theme_path + "/" + icon_name + ".png";
	if (std::filesystem::exists(icon_path)) {
		if (verbose)
			std::cout << "Loading icon from: " << icon_path << std::endl;
		set(icon_path);
	}
	else {
		if (verbose)
			std::cout << "Loading icon from pixmap" << std::endl;
		const auto pixmap_data = extract_pixbuf(get_item_property<std::vector<std::tuple<gint32, gint32, std::vector<guint8>>>>(icon_type_name + "Pixmap"));
		set(pixmap_data);
	}
	std::cout << std::endl;

	// TODO: Write context menu code
	// At this point i don't even think this is even worth it,
	// This is just too painful to write.
	if (menu_path.empty())
		return;

	// Everything beyond this point is highly experimental/filler code.
	// DBus is such a pain to work with this might get deleted/abandoned later.

	auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SESSION);

	auto proxy = Gio::DBus::Proxy::create_sync(
		connection, dbus_name, menu_path,
		"com.canonical.dbusmenu"	// Does this change?
	);

	auto parent_id = Glib::Variant<int>::create(0);
	auto recursion_depth = Glib::Variant<int>::create(1);
	auto property_names = Glib::Variant<std::vector<Glib::ustring>>::create({});

	std::vector<Glib::VariantBase> args_vector;
	args_vector.push_back(parent_id);
	args_vector.push_back(recursion_depth);
	args_vector.push_back(property_names);

	auto args = Glib::VariantContainerBase::create_tuple(args_vector);

	auto result = proxy->call_sync("GetLayout", args);
	auto result_tuple = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(result);
	auto layout_variant = result_tuple.get_child(1);

	if (verbose)
		print_menu_layout(layout_variant);
}

void tray_item::on_right_clicked(int n_press, double x, double y) {
	if (verbose)
		std::cout << "Right clicked" << std::endl;
	popovermenu_context.popup();
}
