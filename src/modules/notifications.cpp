#include "notifications.hpp"

#include <glibmm/dispatcher.h>
#include <gtk4-layer-shell.h>
#include <giomm/dbusownname.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <regex>

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

module_notifications::module_notifications(sysbar* window, const bool& icon_on_start) 
	: module(window, icon_on_start), next_notif_id(1) {
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
	box_notifications.set_spacing(5);

	scrolledwindow_notifications.set_child(box_notifications);
	scrolledwindow_notifications.set_vexpand();
	scrolledwindow_notifications.set_propagate_natural_height();

	if (win->position / 2) {
		win->sidepanel_end->box_sidepanel.prepend(box_header);
		win->sidepanel_end->box_sidepanel.prepend(scrolledwindow_notifications);
	}
	else {
		win->sidepanel_end->box_sidepanel.append(box_header);
		win->sidepanel_end->box_sidepanel.append(scrolledwindow_notifications);
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
	button_clear.signal_clicked().connect(sigc::mem_fun(*this, &module_notifications::clear_all_notifications));

	window_alert.set_name("sysbar_notifications");
	window_alert.set_child(scrolledwindow_alert);
	window_alert.set_decorated(false);
	window_alert.set_default_size(350, -1);
	window_alert.set_hide_on_close(true);
	window_alert.remove_css_class("background");

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

void module_notifications::clear_all_notifications() {
	std::vector<Gtk::Widget*> to_remove;
	for (auto n_child : box_notifications.get_children()) {
		to_remove.push_back(n_child);
	}
	
	for (auto widget : to_remove) {
		auto n = static_cast<notification*>(widget);
		n->cleanup();
		box_notifications.remove(*n);
	}
	
	notifications_map.clear();
	update_ui_state();
}

void module_notifications::update_ui_state() {
	size_t count = notifications_map.size();
	
	if (count == 0) {
		box_header.set_visible(false);
		scrolledwindow_notifications.set_visible(false);
		label_notif_count.set_text("");
		set_tooltip_text("No new notifications");
		image_icon.set_from_icon_name("notification-symbolic");
	} else {
		box_header.set_visible(true);
		scrolledwindow_notifications.set_visible(true);
		label_notif_count.set_text(std::to_string(count) + " Unread Notification" + (count > 1 ? "s" : ""));
		set_tooltip_text(label_notif_count.get_text());
		image_icon.set_from_icon_name("notification-new-symbolic");
	}
}

void module_notifications::on_overlay_change() {
	window_alert.hide();
	alert_timeout_connection.disconnect();
}

void module_notifications::setup_daemon() {
	Gio::DBus::own_name(
		Gio::DBus::BusType::SESSION,
		"org.freedesktop.Notifications",
		sigc::mem_fun(*this, &module_notifications::on_bus_acquired),
		{}, {},
		Gio::DBus::BusNameOwnerFlags::REPLACE
	);
}

void module_notifications::on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& name) {
	object_id = connection->register_object(
		"/org/freedesktop/Notifications",
		introspection_data,
		interface_vtable
	);
	daemon_connection = connection;
}

void module_notifications::on_interface_method_call(
	const Glib::RefPtr<Gio::DBus::Connection>& connection,
	const Glib::ustring& sender, const Glib::ustring& object_path,
	const Glib::ustring& interface_name, const Glib::ustring& method_name,
	const Glib::VariantContainerBase& parameters,
	const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) {

	if (method_name == "GetServerInformation") {
		static const auto info = Glib::Variant<std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>>::create(
			{"sysbar", "funky.sys64", "0.9.0", "1.2"}
		);
		invocation->return_value(info);
	}
	else if (method_name == "GetCapabilities") {
		static const auto value = Glib::Variant<std::tuple<std::vector<Glib::ustring>>>::create(
			{{"action-icons", "actions", "body", "body-hyperlinks", "body-markup", "body-images", "persistence"}}
		);
		invocation->return_value(value);
	}
	else if (method_name == "CloseNotification") {
		Glib::VariantIter iter(parameters);
		Glib::VariantBase child;
		iter.next_value(child);
		guint32 id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();
		
		close_notification(id, 3);
		invocation->return_value({});
	}
	else if (method_name == "Notify") {
		guint32 id = handle_notify(sender, parameters);
		auto id_var = Glib::VariantContainerBase::create_tuple(
			Glib::Variant<guint32>::create(id)
		);
		invocation->return_value(id_var);
	}
}

guint32 module_notifications::handle_notify(const Glib::ustring& sender, const Glib::VariantContainerBase& parameters) {
	if (!parameters.is_of_type(Glib::VariantType("(susssasa{sv}i)")))
		return 0;

	Glib::VariantIter iter(parameters);
	Glib::VariantBase child;
	
	iter.next_value(child);
	iter.next_value(child);
	guint32 replaces_id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();
	
	guint32 notif_id;
	if (replaces_id != 0 && notifications_map.count(replaces_id) > 0) {
		notif_id = replaces_id;
		notification* old_notif = notifications_map[replaces_id];
		old_notif->cleanup();
		box_notifications.remove(*old_notif);
		notifications_map.erase(replaces_id);
	} else {
		notif_id = next_notif_id++;
	}

	notification* notif = Gtk::make_managed<notification>(
		notif_id, sender, parameters, command, daemon_connection, this
	);
	
	notifications_map[notif_id] = notif;

	notif->signal_close.connect([this, notif_id](guint32 reason) {
		remove_notification(notif_id, reason);
	});

	box_notifications.prepend(*notif);
	notif->set_reveal_child(true);
	
	update_ui_state();

	if (!win->overlay_window.is_visible() && !notif->is_transient) {
		show_alert_notification(notif_id, sender, parameters);
	}

	return notif_id;
}

void module_notifications::show_alert_notification(guint32 notif_id, const Glib::ustring& sender, const Glib::VariantContainerBase& parameters) {
	notification* notif_alert = Gtk::make_managed<notification>(
		notif_id, sender, parameters, "", daemon_connection, this
	);
	
	notif_alert->signal_close.connect([this, notif_id, notif_alert](guint32 reason) {
		notif_alert->cleanup();
		flowbox_alert.remove(*notif_alert);
		
		if (flowbox_alert.get_first_child() == nullptr) {
			alert_timeout_connection.disconnect();
			window_alert.hide();
		}
		
		remove_notification(notif_id, reason);
	});

	flowbox_alert.prepend(*notif_alert);
	notif_alert->set_reveal_child(true);
	
	int timeout = notif_alert->expire_timeout;
	if (timeout == 0) timeout = 5000;
	else if (timeout < 0) timeout = 10000;
	
	notif_alert->timeout_connection = Glib::signal_timeout().connect([this, notif_alert]() {
		notif_alert->cleanup();
		flowbox_alert.remove(*notif_alert);
		
		if (flowbox_alert.get_first_child() == nullptr) {
			alert_timeout_connection.disconnect();
			window_alert.hide();
		}
		
		return false;
	}, timeout);

	window_alert.show();
	
	alert_timeout_connection.disconnect();
	alert_timeout_connection = Glib::signal_timeout().connect([this]() {
		window_alert.hide();
		return false;
	}, timeout + 100);
}

void module_notifications::remove_notification(guint32 id, guint32 reason) {
	if (notifications_map.count(id) == 0)
		return;
	
	notification* notif = notifications_map[id];
	notif->cleanup();
	
	if (notif->get_parent() == &box_notifications) {
		box_notifications.remove(*notif);
	}
	
	notifications_map.erase(id);
	update_ui_state();
	
	send_closed_signal(id, reason);
}

void module_notifications::send_closed_signal(guint32 id, guint32 reason) {
	if (!daemon_connection)
		return;
		
	std::vector<Glib::VariantBase> params;
	params.push_back(Glib::Variant<guint32>::create(id));
	params.push_back(Glib::Variant<guint32>::create(reason));
	auto signal_params = Glib::VariantContainerBase::create_tuple(params);
	
	daemon_connection->emit_signal(
		"/org/freedesktop/Notifications",
		"org.freedesktop.Notifications",
		"NotificationClosed",
		{},
		signal_params
	);
}

void module_notifications::close_notification(guint32 id, guint32 reason) {
	remove_notification(id, reason);
}

notification::notification(
	guint32 id, 
	const Glib::ustring& sender, 
	const Glib::VariantContainerBase& parameters, 
	const std::string& cmd,
	const Glib::RefPtr<Gio::DBus::Connection>& connection,
	module_notifications* module)
	: notif_id(id)
	, replaces_id(0)
	, expire_timeout(-1)
	, is_transient(false)
	, dbus_sender(sender)
	, dbus_connection(connection)
	, parent_module(module) {
	
	get_style_context()->add_class("notification");
	set_transition_duration(500);
	set_transition_type(Gtk::RevealerTransitionType::SLIDE_DOWN);
	set_focusable(false);
	
	if (!parse_parameters(parameters))
		return;

	auto now = std::chrono::system_clock::now();
	auto time_t_now = std::chrono::system_clock::to_time_t(now);
	timestamp = *std::localtime(&time_t_now);

	build_ui();
	
	if (!cmd.empty()) {
		std::thread([cmd]() {
			(void)system(cmd.c_str());
		}).detach();
	}
}

bool notification::parse_parameters(const Glib::VariantContainerBase& parameters) {
	if (!parameters.is_of_type(Glib::VariantType("(susssasa{sv}i)")))
		return false;

	Glib::VariantIter iter(parameters);
	Glib::VariantBase child;

	iter.next_value(child);
	app_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	replaces_id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();

	iter.next_value(child);
	app_icon = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	summary = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	body = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();

	iter.next_value(child);
	auto variant_array = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::ustring>>>(child);
	actions = variant_array.get();

	iter.next_value(child);
	auto variant_dict = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>>(child);
	std::map<Glib::ustring, Glib::VariantBase> map_hints = variant_dict.get();

	iter.next_value(child);
	expire_timeout = Glib::VariantBase::cast_dynamic<Glib::Variant<int32_t>>(child).get();

	for (const auto& [key, value] : map_hints) {
		handle_hint(key, value);
	}

	return true;
}

void notification::build_ui() {
	Gtk::Box* box_notification = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
	Gtk::Box* box_top = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
	box_top->set_spacing(5);

	if (!app_icon.empty() && std::filesystem::exists(std::filesystem::path(app_icon))) {
		try {
			image_data = Gdk::Pixbuf::create_from_file(app_icon);
		} catch (...) {}
	}

	if (image_data) {
		Gtk::Image* image_icon = Gtk::make_managed<Gtk::Image>();
		image_icon->get_style_context()->add_class("image");
		image_data = image_data->scale_simple(64, 64, Gdk::InterpType::BILINEAR);
		image_icon->set(image_data);
		image_icon->set_pixel_size(64);
		box_main.append(*image_icon);
	}

	Gtk::Label* label_summary = Gtk::make_managed<Gtk::Label>();
	label_summary->get_style_context()->add_class("summary");
	label_summary->set_markup(parse_markup(summary));
	label_summary->set_halign(Gtk::Align::START);
	label_summary->set_max_width_chars(0);
	label_summary->set_wrap(true);
	label_summary->set_ellipsize(Pango::EllipsizeMode::END);
	label_summary->set_wrap_mode(Pango::WrapMode::WORD_CHAR);
	label_summary->set_hexpand(true);
	box_top->append(*label_summary);

	char time_str[6];
	strftime(time_str, sizeof(time_str), "%H:%M", &timestamp);
	Gtk::Label* label_time = Gtk::make_managed<Gtk::Label>(time_str);
	label_time->get_style_context()->add_class("timestamp");
	label_time->set_halign(Gtk::Align::END);
	box_top->append(*label_time);

	box_notification->append(*box_top);

	if (!body.empty()) {
		parse_and_build_body(box_notification);
	}

	build_actions(box_notification);

	box_main.append(*box_notification);
	button_main.set_child(box_main);
	button_main.add_css_class("flat");
	button_main.set_focusable(false);
	button_main.signal_clicked().connect([this]() {
		if (!actions.empty() && actions[0] == "default") {
			invoke_action("default");
		} else {
			signal_close.emit(2);
		}
	});
	
	set_child(button_main);
}

void notification::parse_and_build_body(Gtk::Box* parent) {
	std::string text = body;
	std::regex img_regex("<img[^>]*src=['\"]([^'\"]*)['\"][^>]*alt=['\"]([^'\"]*)['\"][^>]*/?>");
	
	auto create_label = [&](const std::string& segment) {
		if (segment.empty()) return;
		
		Gtk::Label* label = Gtk::make_managed<Gtk::Label>();
		label->get_style_context()->add_class("body");
		label->set_markup(parse_markup(segment));
		label->set_max_width_chars(0);
		label->set_halign(Gtk::Align::START);
		label->set_wrap(true);
		label->set_wrap_mode(Pango::WrapMode::WORD_CHAR);
		label->set_xalign(0.0);
		parent->append(*label);
	};
	
	std::smatch match;
	size_t search_pos = 0;
	
	while (search_pos < text.length()) {
		std::string remaining = text.substr(search_pos);
		
		if (std::regex_search(remaining, match, img_regex)) {
			if (match.position() > 0) {
				create_label(remaining.substr(0, match.position()));
			}
			
			std::string img_path = match[1].str();
			std::string alt_text = match[2].str();
			
			if (std::filesystem::exists(img_path)) {
				try {
					auto pixbuf = Gdk::Pixbuf::create_from_file(img_path);
					if (pixbuf->get_width() > 300 || pixbuf->get_height() > 200) {
						int new_width = 300;
						int new_height = (pixbuf->get_height() * 300) / pixbuf->get_width();
						if (new_height > 200) {
							new_height = 200;
							new_width = (pixbuf->get_width() * 200) / pixbuf->get_height();
						}
						pixbuf = pixbuf->scale_simple(new_width, new_height, Gdk::InterpType::BILINEAR);
					}
					
					Gtk::Image* img = Gtk::make_managed<Gtk::Image>();
					img->set(pixbuf);
					img->set_halign(Gtk::Align::START);
					parent->append(*img);
				} catch (...) {
					if (!alt_text.empty()) {
						create_label(alt_text);
					}
				}
			} else if (!alt_text.empty()) {
				create_label(alt_text);
			}
			
			search_pos += match.position() + match.length();
		} else {
			create_label(remaining);
			break;
		}
	}
}

void notification::build_actions(Gtk::Box* parent) {
	if (actions.empty() || actions.size() < 2)
		return;

	Gtk::Box* box_actions = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
	box_actions->set_spacing(5);
	box_actions->set_margin_top(5);
	
	bool has_actions = false;
	for (size_t i = 0; i < actions.size(); i += 2) {
		if (i + 1 >= actions.size()) break;
		
		Glib::ustring action_key = actions[i];
		Glib::ustring action_label = actions[i + 1];
		
		if (action_key == "default") continue;
		
		Gtk::Button* btn = Gtk::make_managed<Gtk::Button>(action_label);
		btn->set_hexpand(true);
		btn->signal_clicked().connect([this, action_key]() {
			invoke_action(action_key);
		});
		box_actions->append(*btn);
		has_actions = true;
	}
	
	if (has_actions) {
		parent->append(*box_actions);
	}
}

std::string notification::parse_markup(const Glib::ustring& text) {
	std::string result = text;
	
	std::regex link_regex("<a[^>]*href=['\"]([^'\"]*)['\"][^>]*>(.*?)</a>");
	result = std::regex_replace(result, link_regex, "<u>$2</u> ($1)");
	
	std::regex invalid_tag_regex("<(?!/?(b|i|u|s|sub|sup|small|big|tt|span)[ />])");
	result = std::regex_replace(result, invalid_tag_regex, "&lt;");
	
	return result;
}

void notification::invoke_action(const Glib::ustring& action_key) {
	if (dbus_connection) {
		std::vector<Glib::VariantBase> params;
		params.push_back(Glib::Variant<guint32>::create(notif_id));
		params.push_back(Glib::Variant<Glib::ustring>::create(action_key));
		auto signal_params = Glib::VariantContainerBase::create_tuple(params);
		
		dbus_connection->emit_signal(
			"/org/freedesktop/Notifications",
			"org.freedesktop.Notifications",
			"ActionInvoked",
			dbus_sender,
			signal_params
		);
	}
	
	signal_close.emit(2);
}

void notification::handle_hint(const Glib::ustring& key, const Glib::VariantBase& value) {
	if (key == "transient") {
		try {
			is_transient = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value).get();
		} catch (...) {}
	}
	else if (key == "image-data" || key == "image_data" || key == "icon_data") {
		try {
			auto container = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(value);
			int width = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(0)).get();
			int height = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(1)).get();
			int rowstride = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(2)).get();
			bool has_alpha = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(container.get_child(3)).get();
			int bits_per_sample = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(4)).get();
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
		} catch (...) {}
	}
}

void notification::cleanup() {
	timeout_connection.disconnect();
}