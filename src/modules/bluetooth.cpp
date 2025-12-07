#include "bluetooth.hpp"
#include <thread>

constexpr const char* AGENT_PATH = "/org/bluez/sysbar";
constexpr const char* DEFAULT_PIN = "0000"; // TODO: Offer the user the ability to use a custom pin.

static Glib::ustring get_string(const Glib::VariantBase& v) {
	if (v.gobj() && v.is_of_type(Glib::Variant<Glib::ustring>::variant_type())) {
		return Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(v).get();
	}
	return {};
}

static bool get_bool(const Glib::VariantBase& v) {
	if (v.gobj() && v.is_of_type(Glib::Variant<bool>::variant_type())) {
		return Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
	}
	return false;
}

module_bluetooth::module_bluetooth(sysbar* win, bool show_icon) : module(win, show_icon) {
	get_style_context()->add_class("module_bluetooth");
	label_info.hide();

	if (win->config_main["bluetooth"]["show-icon"] != "true")
		image_icon.hide();

	auto conn = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM);

	manager_proxy = Gio::DBus::Proxy::create_sync(
		conn, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager");

	manager_proxy->signal_signal().connect([this](
		const Glib::ustring&,
		const Glib::ustring& signal,
		const Glib::VariantContainerBase& params) {
		Glib::VariantIter iter(params);
		Glib::VariantBase tmp;
		Glib::DBusObjectPathString path;
		
		iter.next_value(tmp);
		path = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::DBusObjectPathString>>(tmp).get();
		iter.next_value(tmp);

		if (signal == "InterfacesAdded") {
			auto ifaces = Glib::VariantBase::cast_dynamic<
				Glib::Variant<std::map<Glib::ustring, std::map<Glib::ustring, Glib::VariantBase>>>>(tmp).get();
			on_interfaces_added(path, ifaces);
		}
		else if (signal == "InterfacesRemoved") {
			auto ifaces = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::ustring>>>(tmp).get();
			on_interfaces_removed(path, ifaces);
		}
	});

	register_agent();
	fetch_all_objects();
	update_icon();

	#ifdef MODULE_CONTROLS
	if (win->box_controls)
		setup_control();
	#endif
}

void module_bluetooth::register_agent() {
	auto conn = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM);

	static const char* introspection_xml = R"(
		<node>
			<interface name='org.bluez.Agent1'>
				<method name='Release'/>
				<method name='RequestPinCode'>
					<arg type='o' name='device' direction='in'/>
					<arg type='s' name='pincode' direction='out'/>
				</method>
				<method name='DisplayPinCode'>
					<arg type='o' name='device' direction='in'/>
					<arg type='s' name='pincode' direction='in'/>
				</method>
				<method name='RequestPasskey'>
					<arg type='o' name='device' direction='in'/>
					<arg type='u' name='passkey' direction='out'/>
				</method>
				<method name='DisplayPasskey'>
					<arg type='o' name='device' direction='in'/>
					<arg type='u' name='passkey' direction='in'/>
					<arg type='q' name='entered' direction='in'/>
				</method>
				<method name='RequestConfirmation'>
					<arg type='o' name='device' direction='in'/>
					<arg type='u' name='passkey' direction='in'/>
				</method>
				<method name='RequestAuthorization'>
					<arg type='o' name='device' direction='in'/>
				</method>
				<method name='AuthorizeService'>
					<arg type='o' name='device' direction='in'/>
					<arg type='s' name='uuid' direction='in'/>
				</method>
				<method name='Cancel'/>
			</interface>
		</node>
	)";

	auto introspection = Gio::DBus::NodeInfo::create_for_xml(introspection_xml);

	agent_registration_id = conn->register_object(
		AGENT_PATH,
		introspection->lookup_interface("org.bluez.Agent1"),
		[this](auto&&, auto&&, auto&&, auto&&, const Glib::ustring& method_name,
		       const Glib::VariantContainerBase& params,
		       const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) {
			handle_agent_method(method_name, params, invocation);
		});

	agent_manager = Gio::DBus::Proxy::create_sync(
		conn, "org.bluez", "/org/bluez", "org.bluez.AgentManager1");

	agent_manager->call_sync("RegisterAgent",
		Glib::VariantContainerBase::create_tuple({
			Glib::Variant<Glib::DBusObjectPathString>::create(AGENT_PATH),
			Glib::Variant<Glib::ustring>::create("KeyboardDisplay")
		}));

	agent_manager->call_sync("RequestDefaultAgent",
		Glib::VariantContainerBase::create_tuple({
			Glib::Variant<Glib::DBusObjectPathString>::create(AGENT_PATH)
		}));
}

void module_bluetooth::handle_agent_method(
	const Glib::ustring& method_name,
	const Glib::VariantContainerBase&,
	const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) {
	if (method_name == "RequestPinCode") {
		invocation->return_value(Glib::VariantContainerBase::create_tuple(
			Glib::Variant<Glib::ustring>::create(DEFAULT_PIN)));
	}
	else if (method_name == "RequestPasskey") {
		invocation->return_value(Glib::VariantContainerBase::create_tuple(
			Glib::Variant<uint32_t>::create(0)));
	}
	else if (method_name == "Release" || method_name == "DisplayPinCode" ||
	         method_name == "DisplayPasskey" || method_name == "RequestConfirmation" ||
	         method_name == "RequestAuthorization" || method_name == "AuthorizeService" ||
	         method_name == "Cancel") {
		invocation->return_value(Glib::VariantContainerBase());
	}
	else {
		invocation->return_error(Glib::Error(G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
			"Method not implemented"));
	}
}

void module_bluetooth::fetch_all_objects() {
	adapters.clear();
	devices.clear();

	auto reply = manager_proxy->call_sync("GetManagedObjects");
	auto map = Glib::VariantBase::cast_dynamic<
		Glib::Variant<std::map<Glib::DBusObjectPathString,
			std::map<Glib::ustring, std::map<Glib::ustring, Glib::VariantBase>>>>>(reply.get_child(0)).get();

	for (const auto& [path, ifaces] : map) {
		for (const auto& [iface, props] : ifaces) {
			if (iface == "org.bluez.Adapter1") {
				adapter a{.path = path};
				if (props.count("Alias")) a.alias = get_string(props.at("Alias"));
				if (props.count("Powered")) a.powered = get_bool(props.at("Powered"));
				if (props.count("Discoverable")) a.discoverable = get_bool(props.at("Discoverable"));
				if (props.count("Discovering")) a.discovering = get_bool(props.at("Discovering"));
				adapters.push_back(a);
				if (default_adapter.path.empty())
					default_adapter = a;
			}
			else if (iface == "org.bluez.Device1") {
				device d{.path = path};
				if (props.count("Name")) d.name = get_string(props.at("Name"));
				else if (props.count("Alias")) d.name = get_string(props.at("Alias"));
				if (props.count("Icon")) d.icon = get_string(props.at("Icon"));
				if (props.count("Connected")) d.connected = get_bool(props.at("Connected"));
				if (props.count("Paired")) d.paired = get_bool(props.at("Paired"));
				if (props.count("Trusted")) d.trusted = get_bool(props.at("Trusted"));
				if (props.count("Blocked")) d.blocked = get_bool(props.at("Blocked"));
				devices.push_back(d);
			}
		}
	}

	if (!default_adapter.path.empty()) {
		auto conn = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM);
		adapter_proxy = Gio::DBus::Proxy::create_sync(
			conn, "org.bluez", default_adapter.path, "org.bluez.Adapter1");
	}

	update_icon();

	#ifdef MODULE_CONTROLS
	if (flowbox)
		refresh_device_list();
	#endif
}

void module_bluetooth::on_interfaces_added(
	const Glib::DBusObjectPathString& path,
	const std::map<Glib::ustring, std::map<Glib::ustring, Glib::VariantBase>>& ifaces) {
	bool changed = false;

	for (const auto& [iface, props] : ifaces) {
		if (iface == "org.bluez.Adapter1" && default_adapter.path.empty()) {
			default_adapter.path = path;
			default_adapter.powered = get_bool(props.at("Powered"));
			auto conn = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM);
			adapter_proxy = Gio::DBus::Proxy::create_sync(conn, "org.bluez", path, "org.bluez.Adapter1");
			changed = true;
		}
		else if (iface == "org.bluez.Device1") {
			device d{.path = path};
			if (props.count("Name")) d.name = get_string(props.at("Name"));
			else if (props.count("Alias")) d.name = get_string(props.at("Alias"));
			if (props.count("Icon")) d.icon = get_string(props.at("Icon"));
			if (props.count("Connected")) d.connected = get_bool(props.at("Connected"));
			if (props.count("Paired")) d.paired = get_bool(props.at("Paired"));
			if (props.count("Trusted")) d.trusted = get_bool(props.at("Trusted"));
			devices.push_back(d);
			changed = true;
		}
	}

	if (changed) {
		update_icon();
		#ifdef MODULE_CONTROLS
		if (flowbox) refresh_device_list();
		#endif
	}
}

void module_bluetooth::on_interfaces_removed(
	const Glib::DBusObjectPathString& path,
	const std::vector<Glib::ustring>& ifaces) {
	bool changed = false;

	for (const auto& iface : ifaces) {
		if (iface == "org.bluez.Device1") {
			auto it = std::find_if(devices.begin(), devices.end(),
				[&path](const device& d) { return d.path == path; });

			if (it != devices.end()) {
				devices.erase(it);
				changed = true;
			}
		}
	}

	if (changed) {
		update_icon();
		#ifdef MODULE_CONTROLS
		if (flowbox) refresh_device_list();
		#endif
	}
}

void module_bluetooth::update_icon() {
	const char* icon;
	
	if (default_adapter.path.empty() || !default_adapter.powered) {
		icon = "bluetooth-disabled-symbolic";
	} else {
		bool any_connected = std::any_of(devices.begin(), devices.end(),
			[](const device& d) { return d.connected; });
		icon = any_connected ? "bluetooth-connected-symbolic" : "bluetooth-active-symbolic";
	}

	image_icon.set_from_icon_name(icon);

	#ifdef MODULE_CONTROLS
	if (ctrl) ctrl->button_action.set_icon_name(icon);
	#endif
}

#ifdef MODULE_CONTROLS
void module_bluetooth::setup_control() {
	auto* controls = static_cast<module_controls*>(win->box_controls);
	ctrl = Gtk::make_managed<control>(win, "bluetooth-active-symbolic", true, "bluetooth", false);
	controls->flowbox_controls.append(*ctrl);

	container = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
	container->set_margin(12);

	btn_scan = Gtk::make_managed<Gtk::Button>("Scan for devices");
	btn_scan->signal_clicked().connect([this]() { on_scan_clicked(); });
	container->append(*btn_scan);

	auto* scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
	scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
	scroll->set_propagate_natural_height(true);

	flowbox = Gtk::make_managed<Gtk::FlowBox>();
	flowbox->set_max_children_per_line(1);
	flowbox->set_row_spacing(4);
	flowbox->signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
		if (!child) return;

		for (auto* c : flowbox->get_children()) {
			auto* flowbox_child = dynamic_cast<Gtk::FlowBoxChild*>(c);
			if (!flowbox_child) continue;
			
			auto* box = dynamic_cast<Gtk::Box*>(flowbox_child->get_child());
			if (!box) continue;
			
			auto* img = box->get_first_child();
			auto* box_text = img ? img->get_next_sibling() : nullptr;
			auto* revealer = box_text ? dynamic_cast<Gtk::Revealer*>(box_text->get_next_sibling()) : nullptr;
			
			if (revealer) {
				revealer->set_reveal_child(false);
			}
		}

		auto* box = dynamic_cast<Gtk::Box*>(child->get_child());
		if (!box) return;
		
		auto* img = box->get_first_child();
		auto* box_text = img ? img->get_next_sibling() : nullptr;
		auto* revealer = box_text ? dynamic_cast<Gtk::Revealer*>(box_text->get_next_sibling()) : nullptr;
		
		if (revealer) {
			revealer->set_reveal_child(!revealer->get_reveal_child());
		}
	});
	scroll->set_child(*flowbox);

	container->append(*scroll);
	ctrl->page->append(*container);

	refresh_device_list();
}

void module_bluetooth::refresh_device_list() {
	std::vector<Gtk::FlowBoxChild*> to_remove;
	for (auto* child : flowbox->get_children()) {
		auto* flowbox_child = dynamic_cast<Gtk::FlowBoxChild*>(child);
		if (!flowbox_child) continue;

		std::string* path_ptr = static_cast<std::string*>(flowbox_child->get_data("device_path"));
		if (!path_ptr) continue;

		auto it = std::find_if(devices.begin(), devices.end(),
			[&](const device& d) { return d.path == *path_ptr; });
		
		if (it == devices.end()) {
			to_remove.push_back(flowbox_child);
		}
	}
	for (auto* child : to_remove) {
		flowbox->remove(*child);
	}

	for (const auto& dev : devices) {
		bool found = false;
		for (auto* child : flowbox->get_children()) {
			auto* flowbox_child = dynamic_cast<Gtk::FlowBoxChild*>(child);
			if (!flowbox_child) continue;

			std::string* path_ptr = static_cast<std::string*>(flowbox_child->get_data("device_path"));
			if (path_ptr && *path_ptr == dev.path) {
				update_device_widget(flowbox_child, dev);
				found = true;
				break;
			}
		}
		
		if (!found) {
			create_device_widget(dev);
		}
	}
}

void module_bluetooth::create_device_widget(const device& dev) {
	auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
	box->set_margin(6);

	auto* img = Gtk::make_managed<Gtk::Image>();
	img->set_from_icon_name(dev.icon.empty() ? "bluetooth-symbolic" : dev.icon);
	img->set_pixel_size(32);
	box->append(*img);

	auto* box_text = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
	auto* lbl = Gtk::make_managed<Gtk::Label>(dev.name.empty() ? "Unknown Device" : dev.name);
	lbl->set_hexpand(true);
	lbl->set_halign(Gtk::Align::START);

	auto* status = Gtk::make_managed<Gtk::Label>();
	status->set_hexpand(true);
	status->set_halign(Gtk::Align::START);
	status->set_sensitive(false);

	box_text->append(*lbl);
	box_text->append(*status);
	box->append(*box_text);

	auto* revealer = Gtk::make_managed<Gtk::Revealer>();
	revealer->set_transition_type(Gtk::RevealerTransitionType::SLIDE_LEFT);
	revealer->set_reveal_child(false);

	auto* btn_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
	auto* toggle_btn = Gtk::make_managed<Gtk::Button>();
	toggle_btn->signal_clicked().connect([this, path = dev.path]() { toggle_device(path); });
	btn_box->append(*toggle_btn);

	auto* rm_btn = Gtk::make_managed<Gtk::Button>();
	rm_btn->set_icon_name("edit-delete-symbolic");
	rm_btn->set_tooltip_text("Remove");
	rm_btn->signal_clicked().connect([this, path = dev.path]() { remove_device(path); });
	btn_box->append(*rm_btn);

	revealer->set_child(*btn_box);
	box->append(*revealer);

	auto* child = Gtk::make_managed<Gtk::FlowBoxChild>();
	child->set_child(*box);
	
	auto* path_copy = new std::string(dev.path);
	child->set_data("device_path", path_copy, [](void* data) { delete static_cast<std::string*>(data); });

	update_device_widget(child, dev);
	flowbox->append(*child);
}

void module_bluetooth::update_device_widget(Gtk::FlowBoxChild* child, const device& dev) {
	auto* box = dynamic_cast<Gtk::Box*>(child->get_child());
	if (!box) return;

	auto* img = box->get_first_child();
	auto* box_text = img ? dynamic_cast<Gtk::Box*>(img->get_next_sibling()) : nullptr;
	auto* revealer = box_text ? dynamic_cast<Gtk::Revealer*>(box_text->get_next_sibling()) : nullptr;

	if (box_text) {
		auto* lbl = box_text->get_first_child();
		auto* status = lbl ? dynamic_cast<Gtk::Label*>(lbl->get_next_sibling()) : nullptr;
		if (status) {
			status->set_text(dev.connected ? "Connected" : (dev.paired ? "Paired" : "Unpaired"));
		}
	}

	if (revealer) {
		auto* btn_box = dynamic_cast<Gtk::Box*>(revealer->get_child());
		auto* toggle_btn = btn_box ? dynamic_cast<Gtk::Button*>(btn_box->get_first_child()) : nullptr;
		if (toggle_btn) {
			toggle_btn->set_icon_name(dev.connected ? "gtk-connect" : "gtk-disconnect");
			toggle_btn->set_tooltip_text(dev.connected ? "Disconnect" : "Connect");
		}
	}
}

void module_bluetooth::on_scan_clicked() {
	scanning ? stop_discovery() : start_discovery();
}

void module_bluetooth::start_discovery() {
	if (scanning || !default_adapter.powered || !adapter_proxy)
		return;

	std::thread([this]() {
		adapter_proxy->call_sync("StartDiscovery");
		scanning = true;
		Glib::signal_idle().connect_once([this]() {
			if (btn_scan) btn_scan->set_label("Stop scanning");
		});
	}).detach();
}

void module_bluetooth::stop_discovery() {
	if (!scanning || !adapter_proxy)
		return;

	std::thread([this]() {
		adapter_proxy->call_sync("StopDiscovery");
		scanning = false;
		Glib::signal_idle().connect_once([this]() {
			if (btn_scan) btn_scan->set_label("Scan for devices");
		});
	}).detach();
}

void module_bluetooth::toggle_device(const std::string& path) {
	std::thread([this, path]() {
		try {
			auto conn = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM);
			auto dev_proxy = Gio::DBus::Proxy::create_sync(conn, "org.bluez", path, "org.bluez.Device1");

			auto it = std::find_if(devices.begin(), devices.end(),
				[&path](const device& d) { return d.path == path; });
			if (it == devices.end())
				return;

			if (it->connected) {
				dev_proxy->call_sync("Disconnect");
			} else {
				if (!it->paired)
					dev_proxy->call_sync("Pair");
				dev_proxy->call_sync("Connect");
			}

			Glib::signal_idle().connect_once([this]() { fetch_all_objects(); });
		}
		catch (const Glib::Error& e) {
			if (e.matches(G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD) ||
			    e.matches(G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED))
				Glib::signal_idle().connect_once([this]() { fetch_all_objects(); });
		}
	}).detach();
}

void module_bluetooth::remove_device(const std::string& path) {
	if (!adapter_proxy)
		return;

	std::thread([this, path]() {
		adapter_proxy->call_sync("RemoveDevice",
			Glib::VariantContainerBase::create_tuple({
				Glib::Variant<Glib::DBusObjectPathString>::create(path)
			}));
		Glib::signal_idle().connect_once([this]() { fetch_all_objects(); });
	}).detach();
}
#endif