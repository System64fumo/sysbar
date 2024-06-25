#include "../config.hpp"
#include "tray.hpp"

#include <gtkmm/button.h>
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
	auto watcher_name = "org.kde.StatusNotifierWatcher";
	Gio::DBus::own_name(Gio::DBus::BusType::SESSION, watcher_name, sigc::mem_fun(*this, &tray_watcher::on_bus_acquired_watcher));
	Gio::DBus::own_name(Gio::DBus::BusType::SESSION, host_id, sigc::mem_fun(*this, &tray_watcher::on_bus_acquired_host));
}

void tray_watcher::on_bus_acquired_host(const DBusConnection& conn, const Glib::ustring& name) {
	if (verbose)
		std::cout << "on_bus_acquired: " << name << std::endl;

	watcher_id = Gio::DBus::watch_name(conn,"org.kde.StatusNotifierWatcher",
		sigc::mem_fun(*this, &tray_watcher::on_name_appeared),
		sigc::mem_fun(*this, &tray_watcher::on_name_vanished));
}

void tray_watcher::on_bus_acquired_watcher(const DBusConnection& conn, const Glib::ustring& name) {
	if (verbose)
		std::cout << "on_bus_acquired_watcher: " << name << std::endl;

	const auto introspection_data = Gio::DBus::NodeInfo::create_for_xml(
	R"(
	<?xml version="1.0" encoding="UTF-8"?>
	<node name="/StatusNotifierWatcher">
		<interface name="org.kde.StatusNotifierWatcher">
			<method name="RegisterStatusNotifierItem">
				<arg direction="in" name="service" type="s"/>
			</method>
			<method name="RegisterStatusNotifierHost">
				<arg direction="in" name="service" type="s"/>
			</method>
	
			<property name="RegisteredStatusNotifierItems" type="as" access="read"/>
			<property name="IsStatusNotifierHostRegistered" type="b" access="read"/>
			<property name="ProtocolVersion" type="i" access="read"/>
	
			<signal name="StatusNotifierItemRegistered">
				<arg name="service" type="s"/>
			</signal>
			<signal name="StatusNotifierItemUnregistered">
				<arg name="service" type="s"/>
			</signal>
			<signal name="StatusNotifierHostRegistered"/>
		</interface>
	</node>
	)")->lookup_interface();

	conn->register_object("/StatusNotifierWatcher", introspection_data, interface_table);
	watcher_connection = conn;
}

void tray_watcher::on_interface_method_call(const Glib::RefPtr<Gio::DBus::Connection> & connection,
	const Glib::ustring & sender, const Glib::ustring & object_path,
	const Glib::ustring & interface_name, const Glib::ustring & method_name,
	const Glib::VariantContainerBase & parameters,
	const Glib::RefPtr<Gio::DBus::MethodInvocation> & invocation) {

	if (verbose)
		std::cerr << "on_interface_method_call: " << method_name << std::endl;

	handle_signal(sender, method_name, parameters);

	invocation->return_value(Glib::VariantContainerBase());
}

void tray_watcher::on_interface_get_property(Glib::VariantBase & property,
	const Glib::RefPtr<Gio::DBus::Connection> & connection,
	const Glib::ustring & sender, const Glib::ustring & object_path,
	const Glib::ustring & interface_name, const Glib::ustring & property_name) {

	// TODO: This sucks, Use real data!!
	// Write actual code to get stuff
	if (property_name == "RegisteredStatusNotifierItems") {
		std::vector<Glib::ustring> sn_items_names;
		sn_items_names.reserve(1);
		property = Glib::Variant<std::vector<Glib::ustring>>::create(sn_items_names);
	}
	else if (property_name == "IsStatusNotifierHostRegistered") {
		property = Glib::Variant<bool>::create(0);
	}
	else if (property_name == "ProtocolVersion") {
		property = Glib::Variant<int>::create(0);
	}
	else {
		std::cerr << "Unknown property: " << property_name << std::endl;
	}
}

void tray_watcher::on_name_appeared(const DBusConnection &conn, const Glib::ustring &name, const Glib::ustring &owner) {
	if (verbose)
		std::cout << "on_name_appeared: " << name << ", Owner: " << owner << std::endl;

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
	if (verbose)
		std::cout << "on_name_vanished: " << name << std::endl;

	Gio::DBus::unwatch_name(watcher_id);
}

void tray_watcher::handle_signal(const Glib::ustring& sender, const Glib::ustring& signal, const Glib::VariantContainerBase& params) {
	if (verbose)
		std::cout << "handle_signal: " << sender << ", Signal: " << signal << std::endl;

	if (!params.is_of_type(Glib::VariantType("(s)")))
		return;

	Glib::Variant<Glib::ustring> item_path;
	params.get_child(item_path);
	Glib::ustring service = item_path.get();

	if (signal == "RegisterStatusNotifierItem") {
		if (verbose)
			std::cout << "Adding: " << sender << std::endl;

		auto dbus_name = ((service[0] == '/') ? sender : service);
		auto dbus_path = ((service[0] == '/') ? service : "/StatusNotifierItem");

		items.emplace(sender, dbus_name + dbus_path);
		auto it = items.find(sender);
		tray_item& item = it->second;
		box_container->prepend(item);

		// TODO: Add cleanup code
		Gio::DBus::watch_name(
			Gio::DBus::BusType::SESSION,
			sender, {}, [this, sender] (const DBusConnection &conn, const Glib::ustring &name) {
			watcher_connection->emit_signal(
				"/StatusNotifierWatcher",
				"org.kde.StatusNotifierWatcher",
				"StatusNotifierItemUnregistered",
				{},
				Glib::Variant<std::tuple<Glib::ustring>>::create(std::tuple(sender)));
		});
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
tray_item::tray_item(const Glib::ustring &service) {
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

	// Right click menu
	gesture_right_click = Gtk::GestureClick::create();
	gesture_right_click->set_button(GDK_BUTTON_SECONDARY);
	gesture_right_click->signal_pressed().connect(sigc::mem_fun(*this, &tray_item::on_right_clicked));
	add_controller(gesture_right_click);

	flowbox_context.set_selection_mode(Gtk::SelectionMode::NONE);
	popover_context.set_child(flowbox_context);
	popover_context.set_has_arrow(false);
	popover_context.set_parent(*this);
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
// Menu constructed, Now how the hell do i bind actions?
void tray_item::build_menu(const Glib::VariantBase &layout) {
	auto layout_tuple = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(layout);
	auto id_variant = layout_tuple.get_child(0);
	auto properties_variant = layout_tuple.get_child(1);
	auto children_variant = layout_tuple.get_child(2);

	int id = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(id_variant).get();
	auto properties_dict = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>>(properties_variant);
	auto children = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::VariantBase>>>(children_variant);

	if (verbose) {
		std::cout << "ID variant type: " << id_variant.get_type_string() << std::endl;
		std::cout << "Properties variant type: " << properties_variant.get_type_string() << std::endl;
		std::cout << "Children variant type: " << children_variant.get_type_string() << std::endl;
		std::cout << "ID: " << id << std::endl;
		std::cout << "Properties:" << std::endl;
	}

	for (const auto& key_value : properties_dict.get()) {
		if (verbose)
			std::cout << "  " << key_value.first << " = " << key_value.second.print() << std::endl;

		if (key_value.first == "label") {
			std::string label = key_value.second.print();

			// Cleanup
			label = label.substr(1, label.length() - 2);
			if (label[0] == '_') {
				label.erase(0, 1);
			}

			Gtk::Button item(label);
			flowbox_context.append(item);
		}
	}

	for (const auto& child : children.get()) {
		build_menu(child);
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

	build_menu(layout_variant);
}

void tray_item::on_right_clicked(int n_press, double x, double y) {
	if (verbose)
		std::cout << "Right clicked" << std::endl;
	popover_context.popup();
}
