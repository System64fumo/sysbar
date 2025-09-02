#include "notifications.hpp"

#include <glibmm/dispatcher.h>
#include <gtk4-layer-shell.h>
#include <giomm/dbusownname.h>
#include <filesystem>
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

module_notifications::module_notifications(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start), notif_count(0) {
	get_style_context()->add_class("module_notifications");
	image_icon.set_from_icon_name("notification-symbolic");
	set_tooltip_text("No new notifications");
	label_info.hide();

	std::string cfg_command = win->config_main["notification"]["command"];
	if (!cfg_command.empty())
		command = cfg_command;

	setup_widget();
	setup_daemon();
}

void module_notifications::setup_widget() {
	box_notifications.set_orientation(Gtk::Orientation::VERTICAL);

	scrolledwindow_notifications.set_child(box_notifications);
	scrolledwindow_notifications.set_vexpand();
	scrolledwindow_notifications.set_propagate_natural_height();

	if (win->position / 2) {
		win->sidepanel_end->box_widgets.prepend(box_header);
		win->sidepanel_end->box_widgets.prepend(scrolledwindow_notifications);
	}
	else {
		win->sidepanel_end->box_widgets.append(box_header);
		win->sidepanel_end->box_widgets.append(scrolledwindow_notifications);
	}

	box_header.get_style_context()->add_class("notifications_header");
	box_header.set_visible(false);
	box_header.append(label_notif_count);
	label_notif_count.set_halign(Gtk::Align::START);
	label_notif_count.set_hexpand(true);
	scrolledwindow_notifications.set_visible(false);

	box_header.append(button_clear);
	button_clear.set_halign(Gtk::Align::END);
	button_clear.set_image_from_icon_name("application-exit-symbolic");
	button_clear.signal_clicked().connect([&]() {
		for (auto n_child : box_notifications.get_children()) {
			auto n = static_cast<notification*>(n_child);
				box_notifications.remove(*n);
		}
		box_header.set_visible(false);
		label_notif_count.set_text("");
		image_icon.set_from_icon_name("notification-symbolic");
		scrolledwindow_notifications.set_visible(false);
	});

	// TODO: Support other orientations
	window_alert.set_name("sysbar_notifications");
	window_alert.set_child(scrolledwindow_alert);
	window_alert.set_decorated(false);
	window_alert.set_default_size(350, -1);
	window_alert.set_hide_on_close(true);

	gtk_layer_init_for_window(window_alert.gobj());
	gtk_layer_set_namespace(window_alert.gobj(), "sysbar-notifications");
	gtk_layer_set_keyboard_mode(window_alert.gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
	gtk_layer_set_layer(window_alert.gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);

	gtk_layer_set_anchor(window_alert.gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);

	scrolledwindow_alert.set_child(flowbox_alert);
	scrolledwindow_alert.set_propagate_natural_height(true);
	scrolledwindow_alert.set_max_content_height(400);
	flowbox_alert.set_max_children_per_line(1);
	flowbox_alert.set_selection_mode(Gtk::SelectionMode::NONE);
	flowbox_alert.set_valign(Gtk::Align::START);

	win->overlay_window.signal_show().connect(sigc::mem_fun(*this, &module_notifications::on_overlay_change));
}


void module_notifications::on_overlay_change() {
	window_alert.hide();
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
	const Glib::RefPtr<Gio::DBus::Connection>& connection,
	const Glib::ustring &sender, const Glib::ustring& object_path,
	const Glib::ustring &interface_name, const Glib::ustring& method_name,
	const Glib::VariantContainerBase& parameters,
	const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) {

	if (method_name == "GetServerInformation") {
		static const auto info =
			Glib::Variant<std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>>::create(
				{"sysbar", "funky.sys64", "0.9.0", "1.0"});
			invocation->return_value(info);
	}
	else if (method_name == "GetCapabilities") {
		static const auto value = Glib::Variant<std::tuple<std::vector<Glib::ustring>>>::create(
			{{"action-icons", "actions", "body", "body-hyperlinks", "body-markup", "body-images", "persistance"}});
		invocation->return_value(value);
	}
	else if (method_name == "Notify") {
		box_header.set_visible(true);
		image_icon.set_from_icon_name("notification-new-symbolic");
		label_notif_count.set_text(std::to_string(box_notifications.get_children().size() + 1) + " Unread Notifications");
		scrolledwindow_notifications.set_visible(true);

		// TODO: This is worse
		notification *notif = Gtk::make_managed<notification>(box_notifications, sender, parameters, command);

		if (notif->replaces_id != 0) {
			for (auto n_child : box_notifications.get_children()) {
				auto n = static_cast<notification*>(n_child);
				if (n->notif_id == notif->replaces_id) {
					// TODO: Alert notifications don't get replaced yet
					box_notifications.remove(*n);
					notif->notif_id = notif->replaces_id;
				}
			}
		}

		auto id_var = Glib::VariantContainerBase::create_tuple(
			Glib::Variant<guint32>::create(notif->notif_id));

		notif->button_main.signal_clicked().connect([&, notif]() {
			// TODO: Make this switch focus to the program that sent the notification
			box_notifications.remove(*notif);
			label_notif_count.set_text(std::to_string(box_notifications.get_children().size()) + " Unread Notifications");
			if (box_notifications.get_children().size() == 0) {
				box_header.set_visible(false);
				image_icon.set_from_icon_name("notification-symbolic");
				set_tooltip_text("No new notifications");
				scrolledwindow_notifications.set_visible(false);
			}
		});

		if (!win->overlay_window.is_visible()) {
			notification *notif_alert = Gtk::make_managed<notification>(box_notifications, sender, parameters, "");
			notif_alert->button_main.signal_clicked().connect([&, notif_alert]() {
				// TODO: Make this switch focus to the program that sent the notification
				for (auto n_child : box_notifications.get_children()) {
					auto n = static_cast<notification*>(n_child);
					if (n->notif_id == notif_alert->notif_id)
						box_notifications.remove(*n);
				}

				notif_alert->timeout_connection.disconnect();
				flowbox_alert.remove(*notif_alert);
				label_notif_count.set_text(std::to_string(box_notifications.get_children().size()) + " Unread Notifications");

				if (box_notifications.get_children().size() == 0) {
					box_header.set_visible(false);
					image_icon.set_from_icon_name("notification-symbolic");
					timeout_connection.disconnect();
					window_alert.hide();
					set_tooltip_text("No new notifications"); // TODO: This does not seem to work all the time
					scrolledwindow_notifications.set_visible(false);
				}
			});
			flowbox_alert.prepend(*notif_alert);

			notif_alert->timeout_connection = Glib::signal_timeout().connect([&, notif_alert]() {
				flowbox_alert.remove(*notif_alert);
				return false;
			}, 5000);

			window_alert.show();
			notif_alert->set_reveal_child(true);
		}

		box_notifications.prepend(*notif);
		notif->set_reveal_child(true);
		set_tooltip_text(std::to_string(flowbox_alert.get_children().size()) + " unread notifications");

		invocation->return_value(id_var);

		timeout_connection.disconnect();
		timeout_connection = Glib::signal_timeout().connect([&]() {
			window_alert.hide();
			return false;
		}, 5000);
	}
}

notification::notification(const Gtk::Box& box_notifications, const Glib::ustring& sender, const Glib::VariantContainerBase& parameters, const std::string& command) {
	get_style_context()->add_class("notification");
	if (!parameters.is_of_type(Glib::VariantType("(susssasa{sv}i)")))
		return;

	Glib::VariantIter iter(parameters);
	Glib::VariantBase child;

	iter.next_value(child);
	app_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	replaces_id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();
	notif_id = box_notifications.get_children().size() + 1;

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
	if (app_icon != "" && std::filesystem::exists(std::filesystem::path(app_icon))) 
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
	label_headerbar.set_hexpand(true);
	box_notification.append(label_headerbar);

	label_body.get_style_context()->add_class("body");
	label_body.set_text(body);
	label_body.set_max_width_chars(0);
	label_body.set_halign(Gtk::Align::START);
	label_body.set_wrap(true);
	label_body.set_wrap_mode(Pango::WrapMode::WORD_CHAR);
	label_body.set_lines(3);
	label_body.set_margin_start(5);
	label_body.set_hexpand(true);

	// TODO: Add button to expand (Aka change these 2 values)
	label_body.set_single_line_mode(true);
	label_body.set_ellipsize(Pango::EllipsizeMode::END);

	box_notification.append(label_body);

	box_main.append(box_notification);
	button_main.set_child(box_main);
	set_child(button_main);
	set_transition_duration(500);
	set_transition_type(Gtk::RevealerTransitionType::SLIDE_DOWN);
	set_focusable(false);

	box_notification.set_orientation(Gtk::Orientation::VERTICAL);

	if (!command.empty()) {
		std::thread([command]() {
			(void)system(command.c_str());
		}).detach();
	}
}

void notification::handle_hint(const Glib::ustring& key, const Glib::VariantBase& value) {
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