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

module_notifications::module_notifications(const config_bar &cfg, const bool &icon_on_start, const bool &clickable) : module(cfg, icon_on_start, clickable) {
	get_style_context()->add_class("module_notifications");
	image_icon.set_from_icon_name("notification-symbolic");
	label_info.hide();
	setup_daemon();
}

bool module_notifications::update_info() {
	return true;
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
		notification notif(sender, parameters);
		auto id_var = Glib::VariantContainerBase::create_tuple(
			Glib::Variant<guint32>::create(notif.id));

		invocation->return_value(id_var);
	}
}

notification::notification(const Glib::ustring &sender, const Glib::VariantContainerBase &parameters) {
	std::cout << "New Notification!" << std::endl;

	if (!parameters.is_of_type(Glib::VariantType("(susssasa{sv}i)")))
		return;

	Glib::VariantIter iter(parameters);
	Glib::VariantBase child;

	iter.next_value(child);
	app_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();

	iter.next_value(child);
	app_icon = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	summary = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	body = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	// TODO: Actually do stuff with this
	std::cout << "app_name: " << app_name << std::endl;
	std::cout << "id: " << id << std::endl;
	std::cout << "app_icon: " << app_icon << std::endl;
	std::cout << "summary: " << summary << std::endl;
	std::cout << "body: " << body << std::endl;
}
