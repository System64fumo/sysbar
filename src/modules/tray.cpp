#include "tray.hpp"

#include <gtkmm/separator.h>
#include <filesystem>

module_tray::module_tray(sysbar* window, const bool& icon_on_start) : 
	module(window, icon_on_start), 
	m_icon_on_start(icon_on_start),
	watcher(&box_container) {
	
	add_css_class("module_tray");
	label_info.hide();

	box_container.add_css_class("tray_container");

	// TODO: Add an option to disable the revealer
	// TODO: Add an option to set the revealer's transition duration
	// TODO: Set custom transition types based on bar position or icon_on_start

	Gtk::RevealerTransitionType transition_type;

	// Set orientation
	if (win->position % 2) {
		image_icon.set_from_icon_name(icon_on_start ? "arrow-down" : "arrow-up");
		transition_type = icon_on_start ? Gtk::RevealerTransitionType::SLIDE_DOWN : Gtk::RevealerTransitionType::SLIDE_UP;
		box_container.set_orientation(Gtk::Orientation::VERTICAL);
	}
	else {
		image_icon.set_from_icon_name(icon_on_start ? "arrow-right" : "arrow-left");
		transition_type = icon_on_start ? Gtk::RevealerTransitionType::SLIDE_RIGHT : Gtk::RevealerTransitionType::SLIDE_LEFT;
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

void module_tray::on_clicked(const int& n_press, const double& x, const double& y) {
	bool revealed = revealer_box.get_reveal_child();

	revealer_box.set_reveal_child(!revealed);
	if (win->position % 2)
		image_icon.set_from_icon_name(m_icon_on_start == revealed ? "arrow-down" : "arrow-up");
	else
		image_icon.set_from_icon_name(m_icon_on_start == revealed ? "arrow-right" : "arrow-left");

	// Prevent gestures bellow from triggering
	gesture_click->reset();
}

tray_watcher::tray_watcher(Gtk::Box* box) : box_container(box) {
	auto pid = std::to_string(getpid());
	auto host_id = "org.kde.StatusNotifierHost-" + pid + "-1";
	
	Gio::DBus::own_name(Gio::DBus::BusType::SESSION, "org.kde.StatusNotifierWatcher", 
		sigc::mem_fun(*this, &tray_watcher::on_bus_acquired_watcher));
	Gio::DBus::own_name(Gio::DBus::BusType::SESSION, host_id, 
		sigc::mem_fun(*this, &tray_watcher::on_bus_acquired_host));
}

tray_watcher::~tray_watcher() {
	if (watcher_id)
		Gio::DBus::unwatch_name(watcher_id);
	
	for (auto& [key, watch_id] : item_watches)
		Gio::DBus::unwatch_name(watch_id);
}

void tray_watcher::on_bus_acquired_host(const Glib::RefPtr<Gio::DBus::Connection>& conn, const Glib::ustring& name) {
	watcher_id = Gio::DBus::watch_name(conn, "org.kde.StatusNotifierWatcher",
		sigc::mem_fun(*this, &tray_watcher::on_name_appeared),
		sigc::mem_fun(*this, &tray_watcher::on_name_vanished));
}

void tray_watcher::on_bus_acquired_watcher(const Glib::RefPtr<Gio::DBus::Connection>& conn, const Glib::ustring& name) {
	static const char* introspection = R"(
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
	)";

	auto introspection_data = Gio::DBus::NodeInfo::create_for_xml(introspection)->lookup_interface();
	conn->register_object("/StatusNotifierWatcher", introspection_data, interface_table);
	watcher_connection = conn;
}

void tray_watcher::on_interface_method_call(
	const Glib::RefPtr<Gio::DBus::Connection>& connection,
	const Glib::ustring& sender,
	const Glib::ustring& object_path,
	const Glib::ustring& interface_name,
	const Glib::ustring& method_name,
	const Glib::VariantContainerBase& parameters,
	const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) {

	handle_signal(sender, method_name, parameters);
	invocation->return_value(Glib::VariantContainerBase());
}

void tray_watcher::on_interface_get_property(Glib::VariantBase& property,
	const Glib::RefPtr<Gio::DBus::Connection>& connection,
	const Glib::ustring& sender, const Glib::ustring& object_path,
	const Glib::ustring& interface_name, const Glib::ustring& property_name) {

	if (property_name == "RegisteredStatusNotifierItems") {
		std::vector<Glib::ustring> item_names;
		item_names.reserve(items.size());
		for (const auto& [service, _] : items)
			item_names.push_back(service);
		property = Glib::Variant<std::vector<Glib::ustring>>::create(item_names);
	}
	else if (property_name == "IsStatusNotifierHostRegistered") {
		property = Glib::Variant<bool>::create(true);
	}
	else if (property_name == "ProtocolVersion") {
		property = Glib::Variant<int>::create(0);
	}
}

void tray_watcher::on_name_appeared(const Glib::RefPtr<Gio::DBus::Connection>& conn, const Glib::ustring& name, const Glib::ustring& owner) {
	Gio::DBus::Proxy::create(conn, "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher",
		[this, host_name = name](const Glib::RefPtr<Gio::AsyncResult>& result) {
			watcher_proxy = Gio::DBus::Proxy::create_finish(result);
			watcher_proxy->call("RegisterStatusNotifierHost", Glib::Variant<std::tuple<Glib::ustring>>::create({host_name}));
			watcher_proxy->signal_signal().connect(sigc::mem_fun(*this, &tray_watcher::handle_signal));

			Glib::Variant<std::vector<Glib::ustring>> registered_items;
			watcher_proxy->get_cached_property(registered_items, "RegisteredStatusNotifierItems");

			for (const auto& service : registered_items.get())
				add_item(service, service);
		});
}

void tray_watcher::on_name_vanished(const Glib::RefPtr<Gio::DBus::Connection>& conn, const Glib::ustring& name) {
	if (watcher_id) {
		Gio::DBus::unwatch_name(watcher_id);
		watcher_id = 0;
	}
}

void tray_watcher::handle_signal(const Glib::ustring& sender, const Glib::ustring& signal, const Glib::VariantContainerBase& params) {
	if (!params.is_of_type(Glib::VariantType("(s)")))
		return;

	Glib::Variant<Glib::ustring> item_path;
	params.get_child(item_path);
	Glib::ustring service = item_path.get();

	if (signal == "RegisterStatusNotifierItem") {
		auto dbus_name = (service[0] == '/') ? sender : service;
		auto dbus_path = (service[0] == '/') ? service : "/StatusNotifierItem";

		add_item(sender, dbus_name + dbus_path);

		item_watches[sender] = Gio::DBus::watch_name(
			Gio::DBus::BusType::SESSION,
			sender, {}, [this, sender](const Glib::RefPtr<Gio::DBus::Connection>& conn, const Glib::ustring& name) {
				remove_item(sender);
			});
	}
	else if (signal == "StatusNotifierItemUnregistered") {
		remove_item(service);
	}
}

void tray_watcher::add_item(const Glib::ustring& key, const Glib::ustring& service) {
	if (items.find(key) != items.end())
		return;

	auto item = std::make_unique<tray_item>(service);
	box_container->prepend(*item);
	items[key] = std::move(item);
}

void tray_watcher::remove_item(const Glib::ustring& key) {
	auto it = items.find(key);
	if (it == items.end())
		return;

	box_container->remove(*it->second);
	items.erase(it);

	auto watch_it = item_watches.find(key);
	if (watch_it != item_watches.end()) {
		Gio::DBus::unwatch_name(watch_it->second);
		item_watches.erase(watch_it);
	}

	if (watcher_connection) {
		watcher_connection->emit_signal(
			"/StatusNotifierWatcher",
			"org.kde.StatusNotifierWatcher",
			"StatusNotifierItemUnregistered",
			{},
			Glib::Variant<std::tuple<Glib::ustring>>::create(std::tuple(key)));
	}
}

tray_item::tray_item(const Glib::ustring& service) {
	add_css_class("tray_item");
	const auto slash_ind = service.find('/');
	dbus_name = service.substr(0, slash_ind);
	dbus_path = (slash_ind != Glib::ustring::npos) ? service.substr(slash_ind) : "/StatusNotifierItem";

	Gio::DBus::Proxy::create_for_bus(
		Gio::DBus::BusType::SESSION,
		dbus_name,
		dbus_path,
		"org.kde.StatusNotifierItem",
		[this](const Glib::RefPtr<Gio::AsyncResult>& result) {
			item_proxy = Gio::DBus::Proxy::create_for_bus_finish(result);

			item_proxy->signal_signal().connect([this](const Glib::ustring&,
				const Glib::ustring& signal,
				const Glib::VariantContainerBase&) {
					if (signal.size() >= 3 && signal.substr(0, 3) == "New") {
						update_properties();
					}
				});

			update_properties();
		}
	);

	set_size_request(22, 22);

	gesture_right_click = Gtk::GestureClick::create();
	gesture_right_click->set_button(GDK_BUTTON_SECONDARY);
	gesture_right_click->signal_pressed().connect(sigc::mem_fun(*this, &tray_item::on_right_clicked));
	add_controller(gesture_right_click);

	flowbox_context.signal_child_activated().connect(sigc::mem_fun(*this, &tray_item::on_menu_item_click));
	flowbox_context.set_selection_mode(Gtk::SelectionMode::NONE);
	flowbox_context.set_max_children_per_line(1);

	popover_context.add_css_class("context_menu");
	popover_context.set_child(flowbox_context);
	popover_context.set_offset(0, 5);
	popover_context.set_has_arrow(false);
	popover_context.set_parent(*this);
}

tray_item::~tray_item() {
	popover_context.unparent();
}

Glib::RefPtr<Gdk::Pixbuf> tray_item::extract_pixbuf(const std::vector<std::tuple<gint32, gint32, std::vector<guint8>>>& pixbuf_data) {
	if (pixbuf_data.empty())
		return {};

	auto chosen_image = std::max_element(pixbuf_data.begin(), pixbuf_data.end());
	auto [width, height, data] = *chosen_image;

	for (size_t i = 0; i + 3 < data.size(); i += 4) {
		const auto alpha = data[i];
		data[i] = data[i + 1];
		data[i + 1] = data[i + 2];
		data[i + 2] = data[i + 3];
		data[i + 3] = alpha;
	}

	auto* data_ptr = new std::vector<guint8>(std::move(data));

	return Gdk::Pixbuf::create_from_data(
		data_ptr->data(),
		Gdk::Colorspace::RGB,
		true,
		8,
		width,
		height,
		4 * width,
		[data_ptr](auto*) { delete data_ptr; }
	);
}

void tray_item::build_menu(const Glib::VariantBase& layout) {
	auto layout_tuple = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(layout);

	int id = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(layout_tuple.get_child(0)).get();
	auto properties_dict = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>>(layout_tuple.get_child(1));
	auto children = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::VariantBase>>>(layout_tuple.get_child(2));

	for (const auto& [key, value] : properties_dict.get()) {
		if (key == "label") {
			std::string label_str = value.print();
			label_str = label_str.substr(1, label_str.length() - 2);

			Gtk::Label* label = Gtk::make_managed<Gtk::Label>(label_str);
			label->set_name(std::to_string(id));
			label->set_use_underline(true);
			flowbox_context.append(*label);
		}
		else if (key == "type") {
			std::string type = value.print();
			type = type.substr(1, type.size() - 2);

			if (type == "separator") {
				Gtk::FlowBoxChild* child = Gtk::make_managed<Gtk::FlowBoxChild>();
				child->set_child(*Gtk::make_managed<Gtk::Separator>());
				child->set_sensitive(false);
				flowbox_context.append(*child);
			}
		}
	}

	for (const auto& child : children.get())
		build_menu(child);
}

void tray_item::update_properties() {
	Glib::VariantBase variant;
	
	item_proxy->get_cached_property(variant, "Title");
	auto label = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(variant).get();

	item_proxy->get_cached_property(variant, "ToolTip");
	auto [tooltip_icon_name, tooltip_icon_data, tooltip_title, tooltip_text] = variant ?
		Glib::VariantBase::cast_dynamic<Glib::Variant<std::tuple<Glib::ustring, std::vector<std::tuple<gint32, gint32, std::vector<guint8>>>, Glib::ustring, Glib::ustring>>>(variant).get() :
		std::make_tuple(Glib::ustring{}, std::vector<std::tuple<gint32, gint32, std::vector<guint8>>>{}, Glib::ustring{}, Glib::ustring{});

	set_tooltip_text(!tooltip_title.empty() ? tooltip_title : label);

	item_proxy->get_cached_property(variant, "Status");
	auto status = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(variant).get();
	Glib::ustring icon_type_name = (status == "NeedsAttention") ? "AttentionIcon" : "Icon";

	item_proxy->get_cached_property(variant, icon_type_name + "Name");
	auto icon_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(variant).get();

	item_proxy->get_cached_property(variant, "IconThemePath");
	auto icon_theme_path = variant ? Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(variant).get() : "";

	if (!icon_name.empty() && !icon_theme_path.empty()) {
		const std::string icon_path = icon_theme_path + "/" + icon_name + ".png";
		if (std::filesystem::exists(icon_path)) {
			set(icon_path);
		}
	}
	else {
		item_proxy->get_cached_property(variant, icon_type_name + "Pixmap");
		if (variant) {
			auto pixmap = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<std::tuple<gint32, gint32, std::vector<guint8>>>>>(variant).get();
			auto pixbuf = extract_pixbuf(pixmap);
			if (pixbuf)
				set(pixbuf);
		}
	}

	item_proxy->get_cached_property(variant, "Menu");
	if (variant)
		menu_path = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::DBusObjectPathString>>(variant).get();
}

void tray_item::rebuild_menu() {
	while (auto child = flowbox_context.get_first_child())
		flowbox_context.remove(*child);

	try {
		auto proxy = Gio::DBus::Proxy::create_sync(
			Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SESSION),
			dbus_name,
			menu_path,
			"com.canonical.dbusmenu"
		);

		std::vector<Glib::VariantBase> args_vector;
		args_vector.push_back(Glib::Variant<int>::create(0));
		args_vector.push_back(Glib::Variant<int>::create(-1));
		args_vector.push_back(Glib::Variant<std::vector<Glib::ustring>>::create({}));

		auto args = Glib::VariantContainerBase::create_tuple(args_vector);
		auto result = proxy->call_sync("GetLayout", args);
		auto result_tuple = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(result);

		build_menu(result_tuple.get_child(1));
	}
	catch (const Glib::Error& e) {
		std::fprintf(stderr, "Failed to rebuild menu: %s\n", e.what());
	}
}

void tray_item::on_right_clicked(const int& n_press, const double& x, const double& y) {
	if (!menu_path.empty()) {
		rebuild_menu();
		popover_context.popup();
	}
}

void tray_item::on_menu_item_click(Gtk::FlowBoxChild* child) {
	Gtk::Label* label = dynamic_cast<Gtk::Label*>(child->get_child());
	if (!label)
		return;

	try {
		auto now = std::chrono::system_clock::now();
		auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

		auto parameters_variant = Glib::Variant<std::tuple<int32_t, Glib::ustring, Glib::VariantBase, uint32_t>>::create(
			std::make_tuple(
				std::stoi(label->get_name()),
				"clicked",
				Glib::Variant<int32_t>::create(0),
				static_cast<uint32_t>(timestamp)
			)
		);

		auto proxy = Gio::DBus::Proxy::create_sync(
			Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SESSION),
			dbus_name,
			menu_path,
			"com.canonical.dbusmenu"
		);

		proxy->call_sync("Event", parameters_variant);
		popover_context.popdown();
	}
	catch (const Glib::Error& e) {
		std::fprintf(stderr, "Failed to send menu event: %s\n", e.what());
	}
}