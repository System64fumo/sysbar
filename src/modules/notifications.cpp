#include "notifications.hpp"
#include <gtk4-layer-shell.h>
#include <giomm/dbusownname.h>
#include <filesystem>
#include <thread>

const auto introspection_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<node name="/org/freedesktop/Notifications">
	<interface name="org.freedesktop.Notifications">
		<method name="GetCapabilities">
			<arg direction="out" name="capabilities" type="as"/>
		</method>
		<method name="Notify">
			<arg direction="in" name="app_name" type="s"/>
			<arg direction="in" name="replaces_id" type="u"/>
			<arg direction="in" name="app_icon" type="s"/>
			<arg direction="in" name="summary" type="s"/>
			<arg direction="in" name="body" type="s"/>
			<arg direction="in" name="actions" type="as"/>
			<arg direction="in" name="hints" type="a{sv}"/>
			<arg direction="in" name="expire_timeout" type="i"/>
			<arg direction="out" name="id" type="u"/>
		</method>
		<method name="CloseNotification">
			<arg direction="in" name="id" type="u"/>
		</method>
		<method name="GetServerInformation">
			<arg direction="out" name="name" type="s"/>
			<arg direction="out" name="vendor" type="s"/>
			<arg direction="out" name="version" type="s"/>
			<arg direction="out" name="spec_version" type="s"/>
		</method>
		<signal name="NotificationClosed">
			<arg name="id" type="u"/>
			<arg name="reason" type="u"/>
		</signal>
		<signal name="ActionInvoked">
			<arg name="id" type="u"/>
			<arg name="action_key" type="s"/>
		</signal>
	</interface>
</node>)";

NotificationWidget::NotificationWidget(const NotificationData& d, bool alert)
	: data(d), is_alert(alert), is_expanded(false), lbl_body(nullptr), btn_expand(nullptr) {
	get_style_context()->add_class("notification");
	set_transition_duration(400);
	set_transition_type(Gtk::RevealerTransitionType::SLIDE_DOWN);
	build_ui();
}

void NotificationWidget::build_ui() {
	box_main.set_orientation(Gtk::Orientation::HORIZONTAL);

	if (data.image_data) {
		auto* img = Gtk::make_managed<Gtk::Image>();
		img->get_style_context()->add_class("image");
		img->set(data.image_data);
		img->set_pixel_size(64); // TODO: Make this user customizable
		box_main.append(*img);
	}

	auto* box_content = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
	box_content->set_hexpand(true);

	auto* box_header = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
	box_header->get_style_context()->add_class("header");
	
	auto* lbl_summary = Gtk::make_managed<Gtk::Label>();
	lbl_summary->get_style_context()->add_class("summary");
	lbl_summary->set_markup(sanitize_markup(data.summary));
	lbl_summary->set_halign(Gtk::Align::START);
	lbl_summary->set_hexpand(true);
	lbl_summary->set_wrap(true);
	lbl_summary->set_max_width_chars(35);
	lbl_summary->set_ellipsize(Pango::EllipsizeMode::END);
	box_header->append(*lbl_summary);

	char time_buf[6];
	strftime(time_buf, sizeof(time_buf), "%H:%M", &data.timestamp);
	auto* lbl_time = Gtk::make_managed<Gtk::Label>(time_buf);
	lbl_time->get_style_context()->add_class("time");
	box_header->append(*lbl_time);

	if (!data.body.empty() && data.body.length() > 150) {
		btn_expand = Gtk::make_managed<Gtk::Button>();
		btn_expand->set_icon_name("expand-symbolic");
		btn_expand->add_css_class("flat");
		btn_expand->set_tooltip_text("Expand");
		btn_expand->set_focusable(false);
		btn_expand->signal_clicked().connect(sigc::mem_fun(*this, &NotificationWidget::toggle_expand));
		box_header->append(*btn_expand);
	}

	box_content->append(*box_header);

	if (!data.body.empty()) {
		build_body(box_content);
	}

	if (!data.actions.empty()) {
		build_actions(box_content);
	}
	
	box_main.append(*box_content);
	set_child(box_main);
}

void NotificationWidget::build_body(Gtk::Box* parent) {
	auto* box_body = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
	
	lbl_body = Gtk::make_managed<Gtk::Label>();
	lbl_body->get_style_context()->add_class("body");
	lbl_body->set_markup(sanitize_markup(data.body));
	lbl_body->set_halign(Gtk::Align::START);
	lbl_body->set_wrap(true);
	lbl_body->set_xalign(0.0);
	lbl_body->set_yalign(0.0);
	lbl_body->set_hexpand(true);
	box_body->append(*lbl_body);

	if (data.body.length() > 150) {
		lbl_body->set_max_width_chars(40);
		lbl_body->set_lines(3);
		lbl_body->set_ellipsize(Pango::EllipsizeMode::END);
	} else {
		lbl_body->set_max_width_chars(40);
	}
	
	parent->append(*box_body);
}

void NotificationWidget::toggle_expand() {
	if (!btn_expand) return;
	
	is_expanded = !is_expanded;
	
	if (is_expanded) {
		lbl_body->set_lines(0);
		lbl_body->set_ellipsize(Pango::EllipsizeMode::NONE);
		btn_expand->set_icon_name("collapse");
		btn_expand->set_tooltip_text("Collapse");
	} else {
		lbl_body->set_lines(3);
		lbl_body->set_ellipsize(Pango::EllipsizeMode::END);
		btn_expand->set_icon_name("expand");
		btn_expand->set_tooltip_text("Expand");
	}
}

void NotificationWidget::build_actions(Gtk::Box* parent) {
	auto* box_actions = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
	box_actions->add_css_class("actions");

	for (size_t i = 0; i < data.actions.size(); i += 2) {
		if (i + 1 >= data.actions.size()) break;
		
		const auto& key = data.actions[i];
		const auto& label = data.actions[i + 1];
		
		if (key == "default" || label.empty()) continue;
		
		auto* btn = Gtk::make_managed<Gtk::Button>(label);
		btn->set_hexpand(true);
		btn->signal_clicked().connect([this, key]() {
			signal_action.emit(key);
		});
		btn->add_css_class("action");
		box_actions->append(*btn);
	}
	
	if (box_actions->get_first_child()) {
		parent->append(*box_actions);
	}
}

Glib::ustring NotificationWidget::sanitize_markup(const Glib::ustring& text) {
	Glib::ustring result = text;

	size_t pos = 0;
	while ((pos = result.find("&amp;", pos)) != Glib::ustring::npos) {
		result.replace(pos, 5, "&");
		pos += 1;
	}
	pos = 0;
	while ((pos = result.find("&lt;", pos)) != Glib::ustring::npos) {
		result.replace(pos, 4, "<");
		pos += 1;
	}
	pos = 0;
	while ((pos = result.find("&gt;", pos)) != Glib::ustring::npos) {
		result.replace(pos, 4, ">");
		pos += 1;
	}

	const char* unsupported[] = {"<img", "<a "};
	for (const auto* tag : unsupported) {
		while ((pos = result.find(tag)) != Glib::ustring::npos) {
			auto end = result.find(">", pos);
			if (end != Glib::ustring::npos) {
				result.erase(pos, end - pos + 1);
			} else {
				break;
			}
		}
	}

	while ((pos = result.find("</a>")) != Glib::ustring::npos) {
		result.erase(pos, 4);
	}
	
	return result;
}

void NotificationWidget::start_timeout(std::function<void()> callback) {
	int timeout_ms = data.expire_timeout;
	if (timeout_ms == 0) timeout_ms = 5000;
	else if (timeout_ms < 0) timeout_ms = 10000;
	
	timeout_conn = Glib::signal_timeout().connect([callback]() {
		callback();
		return false;
	}, timeout_ms);
}

void NotificationWidget::stop_timeout() {
	timeout_conn.disconnect();
}

module_notifications::module_notifications(sysbar* window, const bool& icon_on_start)
	: module(window, icon_on_start), next_id(1) {
	get_style_context()->add_class("module_notifications");
	image_icon.set_from_icon_name("notification-symbolic");
	set_tooltip_text("No notifications");
	label_info.hide();
	
	std::string cfg_cmd = win->config_main["notification"]["command"];
	if (!cfg_cmd.empty()) command = cfg_cmd;
	
	setup_ui();
	setup_dbus();
}

void module_notifications::setup_ui() {
	flowbox_list.set_max_children_per_line(1);
	flowbox_list.set_selection_mode(Gtk::SelectionMode::NONE);
	flowbox_list.set_valign(Gtk::Align::START);
	
	flowbox_list.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
		auto* revealer = dynamic_cast<NotificationWidget*>(child->get_child());
		if (revealer) {
			guint32 id = revealer->get_id();
			if (!notifications[id].actions.empty() && notifications[id].actions[0] == "default") {
				send_action_signal(id, "default", notifications[id].sender);
			}
			remove_notification(id, 2);
		}
	});
	
	scroll_list.set_child(flowbox_list);
	scroll_list.set_vexpand();
	scroll_list.set_propagate_natural_height();
	scroll_list.set_visible(false);

	box_header.get_style_context()->add_class("notifications_header");
	box_header.set_visible(false);
	box_header.append(label_count);
	label_count.set_halign(Gtk::Align::START);
	label_count.set_hexpand(true);
	
	button_clear.set_halign(Gtk::Align::END);
	button_clear.set_icon_name("user-trash-symbolic");
	button_clear.signal_clicked().connect(sigc::mem_fun(*this, &module_notifications::clear_all));
	box_header.append(button_clear);

	if (win->position / 2) {
		win->sidepanel_end->box_sidepanel.prepend(box_header);
		win->sidepanel_end->box_sidepanel.prepend(scroll_list);
	} else {
		win->sidepanel_end->box_sidepanel.append(box_header);
		win->sidepanel_end->box_sidepanel.append(scroll_list);
	}

	window_alert.set_name("sysbar_notifications");
	window_alert.set_decorated(false);
	window_alert.set_default_size(350, -1);
	window_alert.set_hide_on_close(true);
	window_alert.remove_css_class("background");
	
	gtk_layer_init_for_window(window_alert.gobj());
	gtk_layer_set_namespace(window_alert.gobj(), "sysbar-notifications");
	gtk_layer_set_keyboard_mode(window_alert.gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
	gtk_layer_set_layer(window_alert.gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_set_anchor(window_alert.gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);

	flowbox_alert.set_max_children_per_line(1);
	flowbox_alert.set_selection_mode(Gtk::SelectionMode::NONE);
	flowbox_alert.set_valign(Gtk::Align::START);
	
	flowbox_alert.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
		auto* revealer = dynamic_cast<NotificationWidget*>(child->get_child());
		if (revealer) {
			guint32 id = revealer->get_id();
			if (!notifications[id].actions.empty() && notifications[id].actions[0] == "default") {
				send_action_signal(id, "default", notifications[id].sender);
			}
			revealer->signal_close.emit(2);
		}
	});
	
	scroll_alert.set_child(flowbox_alert);
	scroll_alert.set_propagate_natural_height(true);
	scroll_alert.set_max_content_height(400);
	window_alert.set_child(scroll_alert);

	win->overlay_window.signal_show().connect(sigc::mem_fun(*this, &module_notifications::clear_alerts));
}

void module_notifications::setup_dbus() {
	auto introspection = Gio::DBus::NodeInfo::create_for_xml(introspection_xml);
	Gio::DBus::own_name(
		Gio::DBus::BusType::SESSION,
		"org.freedesktop.Notifications",
		sigc::mem_fun(*this, &module_notifications::on_bus_acquired),
		{}, {},
		Gio::DBus::BusNameOwnerFlags::REPLACE
	);
}

void module_notifications::on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& conn, const Glib::ustring&) {
	auto introspection = Gio::DBus::NodeInfo::create_for_xml(introspection_xml);
	object_id = conn->register_object("/org/freedesktop/Notifications", introspection->lookup_interface(), vtable);
	connection = conn;
}

void module_notifications::on_method_call(
	const Glib::RefPtr<Gio::DBus::Connection>&,
	const Glib::ustring& sender, const Glib::ustring&,
	const Glib::ustring&, const Glib::ustring& method,
	const Glib::VariantContainerBase& params,
	const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) {
	
	if (method == "GetServerInformation") {
		auto info = Glib::Variant<std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>>::create(
			{"sysbar", "funky.sys64", "1.0.0", "1.2"}
		);
		invocation->return_value(info);
	}
	else if (method == "GetCapabilities") {
		auto caps = Glib::Variant<std::tuple<std::vector<Glib::ustring>>>::create(
			{{"actions", "body", "body-markup", "persistence"}}
		);
		invocation->return_value(caps);
	}
	else if (method == "CloseNotification") {
		Glib::VariantIter iter(params);
		Glib::VariantBase child;
		iter.next_value(child);
		guint32 id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();
		close_notification(id, 3);
		invocation->return_value({});
	}
	else if (method == "Notify") {
		guint32 id = handle_notify(sender, params);
		auto result = Glib::VariantContainerBase::create_tuple(Glib::Variant<guint32>::create(id));
		invocation->return_value(result);
	}
}

NotificationData module_notifications::parse_notification(
	guint32 id, const Glib::ustring& sender, const Glib::VariantContainerBase& params) {
	
	NotificationData data{};
	data.id = id;
	data.sender = sender;
	
	if (!params.is_of_type(Glib::VariantType("(susssasa{sv}i)"))) {
		return data;
	}
	
	Glib::VariantIter iter(params);
	Glib::VariantBase child;
	
	iter.next_value(child);
	data.app_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();
	
	iter.next_value(child);
	data.replaces_id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();
	
	iter.next_value(child);
	data.app_icon = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();
	
	iter.next_value(child);
	data.summary = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();
	
	iter.next_value(child);
	data.body = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(child).get();
	
	iter.next_value(child);
	data.actions = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::ustring>>>(child).get();
	
	iter.next_value(child);
	auto hints = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>>(child).get();
	
	iter.next_value(child);
	data.expire_timeout = Glib::VariantBase::cast_dynamic<Glib::Variant<int32_t>>(child).get();
	
	parse_hints(data, hints);

	if (!data.app_icon.empty() && std::filesystem::exists(std::filesystem::path(data.app_icon.c_str()))) {
		try {
			data.image_data = Gdk::Pixbuf::create_from_file(data.app_icon);
		} catch (...) {}
	}

	auto now = std::chrono::system_clock::now();
	auto time_t_now = std::chrono::system_clock::to_time_t(now);
	data.timestamp = *std::localtime(&time_t_now);
	
	return data;
}

void module_notifications::parse_hints(NotificationData& data, const std::map<Glib::ustring, Glib::VariantBase>& hints) {
	for (const auto& [key, value] : hints) {
		if (key == "transient") {
			try {
				data.is_transient = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value).get();
			} catch (...) {}
		}
		else if (key == "image-data" || key == "image_data" || key == "icon_data") {
			try {
				auto container = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(value);
				int w = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(0)).get();
				int h = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(1)).get();
				int rs = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(2)).get();
				bool alpha = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(container.get_child(3)).get();
				int bps = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(container.get_child(4)).get();
				auto pixels = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<uint8_t>>>(container.get_child(6)).get();
				
				if (!pixels.empty() && w > 0 && h > 0) {
					data.image_data = Gdk::Pixbuf::create(Gdk::Colorspace::RGB, alpha, bps, w, h);
					if (data.image_data) {
						guint8* dst = data.image_data->get_pixels();
						int dst_stride = data.image_data->get_rowstride();
						const guint8* src = pixels.data();
						
						for (int y = 0; y < h; ++y) {
							memcpy(dst + y * dst_stride, src + y * rs, std::min(rs, dst_stride));
						}
					}
				}
			} catch (...) {}
		}
	}
}

guint32 module_notifications::handle_notify(const Glib::ustring& sender, const Glib::VariantContainerBase& params) {
	Glib::VariantIter iter(params);
	Glib::VariantBase child;
	iter.next_value(child);
	iter.next_value(child);
	guint32 replaces_id = Glib::VariantBase::cast_dynamic<Glib::Variant<guint32>>(child).get();
	
	guint32 id;
	if (replaces_id != 0 && notifications.count(replaces_id) > 0) {
		id = replaces_id;
		remove_notification(id, 0);
	} else {
		id = next_id++;
	}
	
	auto data = parse_notification(id, sender, params);
	notifications[id] = data;
	show_notification(data);
	
	if (!command.empty()) {
		std::thread([this]() { (void)system(command.c_str()); }).detach();
	}
	
	return id;
}

void module_notifications::show_notification(const NotificationData& data) {
	auto* list_widget = Gtk::make_managed<NotificationWidget>(data, false);
	list_widgets[data.id] = list_widget;
	
	list_widget->signal_close.connect([this, id = data.id](guint32 reason) {
		remove_notification(id, reason);
	});
	
	list_widget->signal_action.connect([this, id = data.id, sender = data.sender](Glib::ustring action) {
		send_action_signal(id, action, sender);
		remove_notification(id, 2);
	});
	
	flowbox_list.prepend(*list_widget);
	
	Glib::signal_idle().connect_once([list_widget]() {
		list_widget->set_reveal_child(true);
	});

	if (!win->overlay_window.is_visible() && !data.is_transient) {
		auto* alert_widget = Gtk::make_managed<NotificationWidget>(data, true);
		alert_widgets[data.id] = alert_widget;
		
		alert_widget->signal_close.connect([this, id = data.id](guint32 reason) {
			if (alert_widgets.count(id)) {
				auto* w = alert_widgets[id];
				w->stop_timeout();
				w->set_reveal_child(false);
				Glib::signal_timeout().connect([this, w, id]() {
					if (w->get_parent()) {
						flowbox_alert.remove(*w->get_parent());
					}
					alert_widgets.erase(id);
					if (!flowbox_alert.get_first_child()) {
						window_alert.hide();
					}
					return false;
				}, w->get_transition_duration());
			}
		});
		
		alert_widget->signal_action.connect([this, id = data.id, sender = data.sender](Glib::ustring action) {
			send_action_signal(id, action, sender);
			if (alert_widgets.count(id)) {
				alert_widgets[id]->signal_close.emit(2);
			}
		});
		
		flowbox_alert.prepend(*alert_widget);
		
		if (!window_alert.is_visible()) {
			window_alert.show();
		}
		
		Glib::signal_idle().connect_once([alert_widget]() {
			alert_widget->set_reveal_child(true);
			alert_widget->start_timeout([alert_widget]() {
				alert_widget->signal_close.emit(1);
			});
		});
	}
	
	update_ui();
}

void module_notifications::remove_notification(guint32 id, guint32 reason) {
	if (notifications.count(id) == 0) return;
	
	notifications.erase(id);
	
	if (list_widgets.count(id)) {
		auto* w = list_widgets[id];
		w->set_reveal_child(false);
		Glib::signal_timeout().connect([this, w, id]() {
			if (w->get_parent()) {
				flowbox_list.remove(*w->get_parent());
			}
			list_widgets.erase(id);
			
			if (notifications.empty() && list_widgets.empty()) {
				update_ui();
			}
			return false;
		}, w->get_transition_duration());
	}

	if (alert_widgets.count(id)) {
		alert_widgets[id]->signal_close.emit(reason);
	}
	
	if (reason > 0) {
		send_closed_signal(id, reason);
	}
	
	if (!notifications.empty()) {
		update_ui();
	}
}

void module_notifications::send_closed_signal(guint32 id, guint32 reason) {
	if (!connection) return;
	
	std::vector<Glib::VariantBase> params;
	params.push_back(Glib::Variant<guint32>::create(id));
	params.push_back(Glib::Variant<guint32>::create(reason));
	
	connection->emit_signal(
		"/org/freedesktop/Notifications",
		"org.freedesktop.Notifications",
		"NotificationClosed",
		{},
		Glib::VariantContainerBase::create_tuple(params)
	);
}

void module_notifications::send_action_signal(guint32 id, const Glib::ustring& action, const Glib::ustring& sender) {
	if (!connection) return;
	
	std::vector<Glib::VariantBase> params;
	params.push_back(Glib::Variant<guint32>::create(id));
	params.push_back(Glib::Variant<Glib::ustring>::create(action));
	
	connection->emit_signal(
		"/org/freedesktop/Notifications",
		"org.freedesktop.Notifications",
		"ActionInvoked",
		sender,
		Glib::VariantContainerBase::create_tuple(params)
	);
}

void module_notifications::close_notification(guint32 id, guint32 reason) {
	remove_notification(id, reason);
}

void module_notifications::update_ui() {
	size_t count = notifications.size();
	
	if (count == 0) {
		box_header.set_visible(false);
		scroll_list.set_visible(false);
		set_tooltip_text("No notifications");
		image_icon.set_from_icon_name("notification-symbolic");
	} else {
		box_header.set_visible(true);
		scroll_list.set_visible(true);
		label_count.set_text(std::to_string(count) + " Notification" + (count > 1 ? "s" : ""));
		set_tooltip_text(label_count.get_text());
		image_icon.set_from_icon_name("notification-new-symbolic");
	}
}

void module_notifications::clear_all() {
	auto ids = notifications;
	for (const auto& [id, _] : ids) {
		remove_notification(id, 2);
	}
}

void module_notifications::clear_alerts() {
	auto ids = alert_widgets;
	for (const auto& [id, widget] : ids) {
		widget->signal_close.emit(2);
	}
}