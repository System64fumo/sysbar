#include "notifications.hpp"

#include <giomm/dbusownname.h>
#include <iostream>

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
	setup_widget();
	setup_daemon();
}

bool module_notifications::update_info() {
	return true;
}

void module_notifications::setup_widget() {
	auto container = static_cast<Gtk::Box*>(win->popover_end->get_child());
	box_notifications = Gtk::make_managed<Gtk::Box>();
	box_notifications->set_orientation(Gtk::Orientation::VERTICAL);
	container->append(*box_notifications);
}

void module_notifications::setup_daemon() {
	if (config_main.verbose)
		std::cout << "setup" << std::endl;
	Gio::DBus::own_name(Gio::DBus::BusType::SESSION,
		"org.freedesktop.Notifications",
		sigc::mem_fun(*this, &module_notifications::on_bus_acquired),
		{},
		{},
		Gio::DBus::BusNameOwnerFlags::REPLACE);
}

void module_notifications::on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connection, const Glib::ustring &name) {
	if (config_main.verbose)
		std::cout << "on_bus_acquired" << std::endl;

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
	if (config_main.verbose)
		std::cout << "on_interface_method_call: " << method_name << std::endl;

	if (method_name == "GetServerInformation") {
		static const auto info =
			Glib::Variant<std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>>::create(
				{"sysbar", "funky.sys64", "0.9.0", "1.0"});
			invocation->return_value(info);
	}
	else if (method_name == "Notify") {
		notification notif(box_notifications, sender, parameters);
		auto id_var = Glib::VariantContainerBase::create_tuple(
			Glib::Variant<guint32>::create(notif_count));
		invocation->return_value(id_var);
		notif_count++;
	}
}

notification::notification(Gtk::Box *box_notifications, const Glib::ustring &sender, const Glib::VariantContainerBase &parameters) {
	std::cout << "New Notification!" << std::endl;

	if (!parameters.is_of_type(Glib::VariantType("(susssasa{sv}i)")))
		return;

	Glib::VariantIter iter(parameters);
	Glib::VariantBase child;

	iter.next_value(child);
	app_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	// TODO: This is used to replace existing notifications
	// Currently unsupported
	iter.next_value(child);
	id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();

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

	iter.next_value(child);
	int32_t expires = Glib::VariantBase::cast_dynamic<Glib::Variant<int32_t>>(child).get();

	// TODO: Actually do stuff with this
	std::cout << "app_name: " << app_name << std::endl;
	std::cout << "replaces id: " << id << std::endl;
	std::cout << "app_icon: " << app_icon << std::endl;
	std::cout << "summary: " << summary << std::endl;
	std::cout << "body: " << body << std::endl;
	for (const auto& action : actions) {
		std::cout << action << std::endl;
	}
	for (const auto& [key, value] : map_hints) {
		std::cout << "Key: " << key << ", Value Type: " << value.get_type_string() << std::endl;
	}
	std::cout << "expires: " << expires << std::endl;

	Gtk::Box box_notification;
	Gtk::Label label_headerbar, label_body;
	Gtk::Image image_icon;

	if (app_icon != "") {
		image_icon.set(app_icon);
		image_icon.set_size_request(64, 64);
		append(image_icon);
	}

	label_headerbar.set_text(summary);
	label_headerbar.set_halign(Gtk::Align::START);
	label_headerbar.set_max_width_chars(0);
	label_headerbar.set_wrap(true);
	box_notification.append(label_headerbar);

	label_body.set_text(body);
	label_body.set_max_width_chars(0);
	label_headerbar.set_halign(Gtk::Align::START);
	label_body.set_wrap(true);
	label_body.set_natural_wrap_mode(Gtk::NaturalWrapMode::NONE);
	box_notification.append(label_body);

	append(box_notification);

	box_notification.set_orientation(Gtk::Orientation::VERTICAL);
	box_notifications->append(*this);
}
