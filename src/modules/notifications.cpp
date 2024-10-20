#include "notifications.hpp"

#include <giomm/dbusownname.h>
#include <thread>

const auto introspection_data = Gio::DBus::NodeInfo::create_for_xml(
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<node name=\"/org/freedesktop/Notifications\">"
	"	<interface name=\"org.freedesktop.Notifications\">"
	"		<method name=\"GetCapabilities\">"
	"			<arg direction=\"out\" name=\"capabilities\"	type=\"as\"/>"
	"		</method>"
	"		<method name=\"Notify\">"
	"			<arg direction=\"in\"	name=\"app_name\"		type=\"s\"/>"
	"			<arg direction=\"in\"	name=\"replaces_id\"	 type=\"u\"/>"
	"			<arg direction=\"in\"	name=\"app_icon\"		type=\"s\"/>"
	"			<arg direction=\"in\"	name=\"summary\"		 type=\"s\"/>"
	"			<arg direction=\"in\"	name=\"body\"			type=\"s\"/>"
	"			<arg direction=\"in\"	name=\"actions\"		 type=\"as\"/>"
	"			<arg direction=\"in\"	name=\"hints\"			type=\"a{sv}\"/>"
	"			<arg direction=\"in\"	name=\"expire_timeout\"	type=\"i\"/>"
	"			<arg direction=\"out\" name=\"id\"				type=\"u\"/>"
	"		</method>"
	"		<method name=\"CloseNotification\">"
	"			<arg direction=\"in\"	name=\"id\"				type=\"u\"/>"
	"		</method>"
	"		<method name=\"GetServerInformation\">"
	"			<arg direction=\"out\" name=\"name\"			type=\"s\"/>"
	"			<arg direction=\"out\" name=\"vendor\"			type=\"s\"/>"
	"			<arg direction=\"out\" name=\"version\"		 type=\"s\"/>"
	"			<arg direction=\"out\" name=\"spec_version\"	type=\"s\"/>"
	"		</method>"
	"		<signal name=\"NotificationClosed\">"
	"			<arg name=\"id\"		 type=\"u\"/>"
	"			<arg name=\"reason\"	 type=\"u\"/>"
	"		</signal>"
	"		<signal name=\"ActionInvoked\">"
	"			<arg name=\"id\"		 type=\"u\"/>"
	"			<arg name=\"action_key\" type=\"s\"/>"
	"		</signal>"
	"	</interface>"
	"</node>")->lookup_interface();

module_notifications::module_notifications(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_notifications");
	image_icon.set_from_icon_name("notification-symbolic");
	label_info.hide();

	#ifdef CONFIG_FILE
	if (config->available) {
		std::string cfg_command = config->get_value("notification", "command");
		if (cfg_command != "empty")
			command = cfg_command;
	}
	#endif

	setup_widget();
	setup_daemon();
}

bool module_notifications::update_info() {
	return true;
}

void module_notifications::setup_widget() {
	auto container = static_cast<Gtk::Box*>(win->box_widgets_end);
	box_notifications = Gtk::make_managed<Gtk::Box>();
	box_notifications->set_orientation(Gtk::Orientation::VERTICAL);

	container->append(*box_notifications);

	// TODO: Support other orientations
	popover_alert.set_parent(*win);
	popover_alert.set_child(box_alert);
	popover_alert.set_autohide(false);
	box_alert.set_orientation(Gtk::Orientation::VERTICAL);
	box_alert.set_size_request(200, 0);
}

void module_notifications::setup_daemon() {
	Gio::DBus::own_name(Gio::DBus::BusType::SESSION,
		"org.freedesktop.Notifications",
		sigc::mem_fun(*this, &module_notifications::on_bus_acquired),
		{},
		{},
		Gio::DBus::BusNameOwnerFlags::REPLACE);
}

void module_notifications::on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connection, const Glib::ustring &name) {
	object_id = connection->register_object(
		"/org/freedesktop/Notifications",
		introspection_data,
		interface_vtable);

	daemon_connection = connection;
}

void module_notifications::on_interface_method_call(
	const Glib::RefPtr<Gio::DBus::Connection> &connection,
	const Glib::ustring &sender, const Glib::ustring &object_path,
	const Glib::ustring &interface_name, const Glib::ustring &method_name,
	const Glib::VariantContainerBase &parameters,
	const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {

	if (method_name == "GetServerInformation") {
		static const auto info =
			Glib::Variant<std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>>::create(
				{"sysbar", "funky.sys64", "0.9.0", "1.0"});
			invocation->return_value(info);
	}
	else if (method_name == "Notify") {
		// TODO: This is terrible.
		notification *notif = Gtk::make_managed<notification>(notifications, box_notifications, sender, parameters, command);
		auto id_var = Glib::VariantContainerBase::create_tuple(
			Glib::Variant<guint32>::create(notif->notif_id));

		notif->signal_clicked().connect([&, notif]() {
			box_notifications->remove(*notif);
			delete &notif;
		});

		invocation->return_value(id_var);
		notifications.push_back(notif);
		//popover_alert.popup();
	}
}

notification::notification(std::vector<notification*> notifications, Gtk::Box *box_notifications, const Glib::ustring &sender, const Glib::VariantContainerBase &parameters, const std::string command) {
	get_style_context()->add_class("notification");
	if (!parameters.is_of_type(Glib::VariantType("(susssasa{sv}i)")))
		return;

	Glib::VariantIter iter(parameters);
	Glib::VariantBase child;

	iter.next_value(child);
	app_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	replaces_id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();
	notif_id = notifications.size() + 1;

	// Pretty sure a memory leak happens here
	// TODO: Fix said memory leak
	if (replaces_id != 0) {
		for (auto notif : notifications) {
			if (notif->notif_id == replaces_id) {
				// GTK My dear friend.. Why do you complain?
				box_notifications->remove(*notif);
				notif_id = replaces_id;
			}
		}
	}

	iter.next_value(child);
	app_icon = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	summary = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	body = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	// TODO: Fix this? doesn't seem to work?
	iter.next_value(child);
	auto variant_array = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::ustring>>>(child);
	std::vector<Glib::ustring> actions = variant_array.get();

	iter.next_value(child);
	auto variant_dict = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>>(child);
	std::map<Glib::ustring, Glib::VariantBase> map_hints = variant_dict.get();

	// Expiration is not currently supported
	iter.next_value(child);
	int32_t expires = Glib::VariantBase::cast_dynamic<Glib::Variant<int32_t>>(child).get();
	(void)expires; // Make the compiler shut up

	// Actions are currently unsupported
	/*for (const auto& action : actions) {
	}*/
	for (const auto& [key, value] : map_hints) {
		handle_hint(key, value);
	}

	Gtk::Box box_notification;
	Gtk::Label label_headerbar, label_body;
	Gtk::Image image_icon;

	// Load image data from path
	if (app_icon != "")
		image_data = Gdk::Pixbuf::create_from_file(app_icon);

	// Load image from pixbuf if applicable
	if (image_data != nullptr) {
		image_icon.get_style_context()->add_class("image");
		image_icon.set(image_data);
		image_data = image_data->scale_simple(64, 64, Gdk::InterpType::BILINEAR);
		image_icon.set_size_request(64, 64);
		box_main.append(image_icon);
	}

	label_headerbar.get_style_context()->add_class("summary");
	label_headerbar.set_text(summary);
	label_headerbar.set_halign(Gtk::Align::START);
	label_headerbar.set_max_width_chars(0);
	label_headerbar.set_wrap(true);
	label_headerbar.set_ellipsize(Pango::EllipsizeMode::END);
	label_headerbar.set_wrap_mode(Pango::WrapMode::WORD_CHAR);
	label_headerbar.set_margin_start(5);
	box_notification.append(label_headerbar);

	label_body.get_style_context()->add_class("body");
	label_body.set_text(body);
	label_body.set_max_width_chars(0);
	label_body.set_halign(Gtk::Align::START);
	label_body.set_wrap(true);
	label_body.set_ellipsize(Pango::EllipsizeMode::END);
	label_body.set_wrap_mode(Pango::WrapMode::WORD_CHAR);
	label_body.set_lines(3);
	label_body.set_margin_start(5);
	box_notification.append(label_body);

	box_main.append(box_notification);
	set_child(box_main);
	set_focusable(false);

	box_notification.set_orientation(Gtk::Orientation::VERTICAL);
	box_notifications->prepend(*this);

	if (!command.empty()) {
		std::thread([command]() {
			system(command.c_str());
		}).detach();
	}
}

void notification::handle_hint(Glib::ustring key, const Glib::VariantBase &value) {
	if (key == "icon_data") {
		auto container = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(value);
		int width = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(0)).get();
		int height = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(1)).get();
		int rowstride = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(2)).get();
		bool has_alpha = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(container.get_child(3)).get();
		int bits_per_sample = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(4)).get();
		//int channels = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(5)).get(); // Unused
		auto pixel_data = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<uint8_t>>>(container.get_child(6)).get();

		image_data = Gdk::Pixbuf::create_from_data(
			pixel_data.data(),
			Gdk::Colorspace::RGB,
			has_alpha,
			bits_per_sample,
			width,
			height,
			rowstride
		)->copy();
	}
}
