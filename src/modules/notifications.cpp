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
	add_css_class("notification");
	set_transition_duration(400);
	set_transition_type(Gtk::RevealerTransitionType::SLIDE_DOWN);
	build_ui();
}

void NotificationWidget::build_ui() {
	box_main.set_orientation(Gtk::Orientation::HORIZONTAL);

	if (data.image_data) {
		auto* img = Gtk::make_managed<Gtk::Image>();
		img->add_css_class("image");
		img->set(data.image_data);
		img->set_pixel_size(64);
		box_main.append(*img);
	}

	auto* box_content = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
	box_content->set_hexpand(true);

	auto* box_header = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
	box_header->add_css_class("header");
	
	auto* lbl_summary = Gtk::make_managed<Gtk::Label>();
	lbl_summary->add_css_class("summary");
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
	lbl_time->add_css_class("time");
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
	lbl_body->add_css_class("body");
	lbl_body->set_markup(sanitize_markup(data.body));
	lbl_body->set_halign(Gtk::Align::START);
	lbl_body->set_wrap(true);
	lbl_body->set_xalign(0.0);
	lbl_body->set_yalign(0.0);
	lbl_body->set_hexpand(true);
	lbl_body->set_max_width_chars(40);
	
	if (data.body.length() > 150) {
		lbl_body->set_lines(3);
		lbl_body->set_ellipsize(Pango::EllipsizeMode::END);
	}
	
	box_body->append(*lbl_body);
	parent->append(*box_body);
}

void NotificationWidget::toggle_expand() {
	if (!btn_expand || !lbl_body) return;
	
	is_expanded = !is_expanded;
	
	if (is_expanded) {
		lbl_body->set_lines(0);
		lbl_body->set_ellipsize(Pango::EllipsizeMode::NONE);
		btn_expand->set_icon_name("collapse-symbolic");
		btn_expand->set_tooltip_text("Collapse");
	} else {
		lbl_body->set_lines(3);
		lbl_body->set_ellipsize(Pango::EllipsizeMode::END);
		btn_expand->set_icon_name("expand-symbolic");
		btn_expand->set_tooltip_text("Expand");
	}
}

void NotificationWidget::build_actions(Gtk::Box* parent) {
	auto* box_actions = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
	box_actions->add_css_class("actions");

	for (size_t i = 0; i + 1 < data.actions.size(); i += 2) {
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
	
	auto replace_all = [&result](const char* from, const char* to, size_t from_len) {
		size_t pos = 0;
		while ((pos = result.find(from, pos)) != Glib::ustring::npos) {
			result.replace(pos, from_len, to);
			pos += strlen(to);
		}
	};

	replace_all("&amp;", "&", 5);
	replace_all("&lt;", "<", 4);
	replace_all("&gt;", ">", 4);
	replace_all("<br>", "\n", 4);
	replace_all("<br/>", "\n", 5);
	replace_all("<br />", "\n", 6);

	for (const char* tag : {"<img", "<a "}) {
		size_t pos;
		while ((pos = result.find(tag)) != Glib::ustring::npos) {
			auto end = result.find(">", pos);
			if (end != Glib::ustring::npos) {
				result.erase(pos, end - pos + 1);
			} else {
				break;
			}
		}
	}

	size_t pos;
	while ((pos = result.find("</a>")) != Glib::ustring::npos) {
		result.erase(pos, 4);
	}
	
	return result;
}

void NotificationWidget::start_timeout(std::function<void()> callback) {
	int timeout_ms = data.expire_timeout <= 0 ? (data.expire_timeout == 0 ? 5000 : 10000) : data.expire_timeout;
	
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
	add_css_class("module_notifications");
	image_icon.set_from_icon_name("notification-symbolic");
	set_tooltip_text("No notifications");
	label_info.hide();
	
	std::string cfg_cmd = win->config_main["notification"]["command"];
	if (!cfg_cmd.empty()) command = cfg_cmd;
	
	setup_ui();
	setup_dbus();

	if (win->config_main["notification"]["show-control"] == "true") {
		setup_control();
	}
}

#ifdef MODULE_CONTROLS
void module_notifications::setup_control() {
	if (!win->box_controls)
		return;

	auto container = static_cast<module_controls*>(win->box_controls);
	// TODO: Add support for control levels (Put the controls in the notifications tab)
	control_notifications = Gtk::make_managed<control>(win, "weather-clear-night-symbolic", false, "notifications", false);
	container->flowbox_controls.append(*control_notifications);

	// Do Not Disturb button
	control_notifications->button_action.signal_toggled().connect([this]() {
		if (control_notifications->button_action.get_active()) {
			notification_level = 0;
		}
		else {
			notification_level = 3;
		}
		update_ui();
	});
	control_notifications->label_title.set_text("Do Not Disturb");
}
#endif

NotificationWidget* module_notifications::find_widget_by_id(Gtk::FlowBox& flowbox, guint32 id) {
	auto* child = flowbox.get_first_child();
	while (child) {
		auto* flowbox_child = dynamic_cast<Gtk::FlowBoxChild*>(child);
		if (flowbox_child) {
			auto* widget = dynamic_cast<NotificationWidget*>(flowbox_child->get_child());
			if (widget && widget->get_id() == id) {
				return widget;
			}
		}
		child = child->get_next_sibling();
	}
	return nullptr;
}

void module_notifications::setup_ui() {
	flowbox_list.set_max_children_per_line(1);
	flowbox_list.set_selection_mode(Gtk::SelectionMode::NONE);
	flowbox_list.set_vexpand(true);

	if (win->position % 2)
		flowbox_list.set_valign(Gtk::Align::START);
	else
		flowbox_list.set_valign(Gtk::Align::END);
	
	flowbox_list.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
		auto* revealer = dynamic_cast<NotificationWidget*>(child->get_child());
		if (!revealer) return;
		
		const NotificationData& data = revealer->get_data();
		if (!data.actions.empty() && data.actions[0] == "default") {
			send_action_signal(data.id, "default", data.sender);
		}
		remove_notification(data.id, 2);
	});
	
	scroll_list.set_child(flowbox_list);
	scroll_list.set_vexpand();
	scroll_list.set_propagate_natural_height();
	scroll_list.set_visible(false);

	box_header.add_css_class("notifications_header");
	box_header.set_visible(false);
	box_header.append(label_count);
	label_count.set_halign(Gtk::Align::START);
	label_count.set_hexpand(true);
	
	button_clear.set_halign(Gtk::Align::END);
	button_clear.set_icon_name("user-trash-symbolic");
	button_clear.signal_clicked().connect(sigc::mem_fun(*this, &module_notifications::clear_all));
	box_header.append(button_clear);

	auto& target = win->sidepanel_end->box_sidepanel;
	if (win->position / 2) {
		target.prepend(box_header);
		target.prepend(scroll_list);
	} else {
		target.append(box_header);
		target.append(scroll_list);
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
		if (!revealer) return;
		
		const NotificationData& data = revealer->get_data();
		if (!data.actions.empty() && data.actions[0] == "default") {
			send_action_signal(data.id, "default", data.sender);
		}
		revealer->signal_close.emit(2);
	});
	
	scroll_alert.set_child(flowbox_alert);
	scroll_alert.set_propagate_natural_height(true);
	scroll_alert.set_max_content_height(400);
	window_alert.set_child(scroll_alert);

	win->overlay_window.signal_show().connect(sigc::mem_fun(*this, &module_notifications::clear_alerts));
}

void module_notifications::setup_dbus() {
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
		invocation->return_value(Glib::Variant<std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>>::create(
			{"sysbar", "funky.sys64", "1.0.0", "1.2"}
		));
	}
	else if (method == "GetCapabilities") {
		invocation->return_value(Glib::Variant<std::tuple<std::vector<Glib::ustring>>>::create(
			{{"actions", "body", "body-markup", "persistence"}}
		));
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
		invocation->return_value(Glib::VariantContainerBase::create_tuple(Glib::Variant<guint32>::create(id)));
	}
}

template<typename T>
T extract_variant(Glib::VariantIter& iter) {
	Glib::VariantBase child;
	iter.next_value(child);
	return Glib::VariantBase::cast_dynamic<Glib::Variant<T>>(child).get();
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
	
	data.app_name = extract_variant<Glib::ustring>(iter);
	data.replaces_id = extract_variant<guint32>(iter);
	data.app_icon = extract_variant<Glib::ustring>(iter);
	data.summary = extract_variant<Glib::ustring>(iter);
	data.body = extract_variant<Glib::ustring>(iter);
	data.actions = extract_variant<std::vector<Glib::ustring>>(iter);
	auto hints = extract_variant<std::map<Glib::ustring, Glib::VariantBase>>(iter);
	data.expire_timeout = extract_variant<int32_t>(iter);
	
	parse_hints(data, hints);

	if (!data.app_icon.empty() && std::filesystem::exists(data.app_icon.c_str())) {
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
	auto it = hints.find("transient");
	if (it != hints.end()) {
		try {
			data.is_transient = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(it->second).get();
		} catch (...) {}
	}

	for (const char* key : {"image-data", "image_data", "icon_data"}) {
		it = hints.find(key);
		if (it == hints.end()) continue;
		
		try {
			auto container = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(it->second);
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
			break;
		} catch (...) {}
	}
}

guint32 module_notifications::handle_notify(const Glib::ustring& sender, const Glib::VariantContainerBase& params) {
	Glib::VariantIter iter(params);
	Glib::VariantBase unused;
	iter.next_value(unused);
	guint32 replaces_id = extract_variant<guint32>(iter);
	
	guint32 id = (replaces_id != 0 && find_widget_by_id(flowbox_list, replaces_id)) ? replaces_id : next_id++;
	
	if (replaces_id != 0 && id == replaces_id) {
		remove_notification(id, 0);
	}
	
	auto data = parse_notification(id, sender, params);
	show_notification(data);
	
	if (!command.empty() && notification_level != 0) {
		std::thread([cmd = command]() { (void)system(cmd.c_str()); }).detach();
	}
	
	return id;
}

void module_notifications::show_notification(const NotificationData& data) {
	auto* list_widget = Gtk::make_managed<NotificationWidget>(data, false);
	
	list_widget->signal_close.connect([this, id = data.id](guint32 reason) {
		remove_notification(id, reason);
	});
	
	list_widget->signal_action.connect([this, id = data.id](Glib::ustring action) {
		auto* widget = find_widget_by_id(flowbox_list, id);
		if (widget) {
			const NotificationData& widget_data = widget->get_data();
			send_action_signal(id, action, widget_data.sender);
		}
		remove_notification(id, 2);
	});
	
	flowbox_list.prepend(*list_widget);
	
	Glib::signal_idle().connect_once([list_widget]() {
		list_widget->set_reveal_child(true);
	});

	if (!win->overlay_window.is_visible() && !data.is_transient && notification_level != 0) {
		auto* alert_widget = Gtk::make_managed<NotificationWidget>(data, true);
		
		alert_widget->signal_close.connect([this, id = data.id](guint32 reason) {
			auto* w = find_widget_by_id(flowbox_alert, id);
			if (!w) return;
			
			w->stop_timeout();
			w->set_reveal_child(false);
			
			auto duration = w->get_transition_duration();
			
			Glib::signal_timeout().connect([this, id]() {
				auto* widget = find_widget_by_id(flowbox_alert, id);
				if (widget) {
					auto parent = widget->get_parent();
					if (parent && parent->get_parent()) {
						flowbox_alert.remove(*parent);
					}
				}
				if (!flowbox_alert.get_first_child()) {
					window_alert.hide();
				}
				return false;
			}, duration);
		});
		
		alert_widget->signal_action.connect([this, id = data.id](Glib::ustring action) {
			auto* widget = find_widget_by_id(flowbox_alert, id);
			if (widget) {
				const NotificationData& widget_data = widget->get_data();
				send_action_signal(id, action, widget_data.sender);
			}
			auto* w = find_widget_by_id(flowbox_alert, id);
			if (w) {
				w->signal_close.emit(2);
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
	auto* w = find_widget_by_id(flowbox_list, id);
	if (w) {
		w->set_reveal_child(false);
		Glib::signal_timeout().connect([this, id]() {
			auto* widget = find_widget_by_id(flowbox_list, id);
			if (widget) {
				auto parent = widget->get_parent();
				if (parent && parent->get_parent()) {
					flowbox_list.remove(*parent);
				}
			}
			update_ui();
			return false;
		}, w->get_transition_duration());
	}

	auto* alert_w = find_widget_by_id(flowbox_alert, id);
	if (alert_w) {
		alert_w->signal_close.emit(reason);
	}
	
	if (reason > 0) {
		send_closed_signal(id, reason);
	}
	
	if (w) {
		update_ui();
	}
}

void module_notifications::send_closed_signal(guint32 id, guint32 reason) {
	if (!connection) return;
	
	std::vector<Glib::VariantBase> params{
		Glib::Variant<guint32>::create(id),
		Glib::Variant<guint32>::create(reason)
	};
	
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
	
	std::vector<Glib::VariantBase> params{
		Glib::Variant<guint32>::create(id),
		Glib::Variant<Glib::ustring>::create(action)
	};
	
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
	auto* child = flowbox_list.get_first_child();
	size_t count = 0;
	
	while (child) {
		count++;
		child = child->get_next_sibling();
	}

	std::string icon = "notification";
	if (notification_level != 3)
		icon += "-disabled";

	if (count == 0) {
		box_header.set_visible(false);
		scroll_list.set_visible(false);
		set_tooltip_text("No notifications");
	} else {
		box_header.set_visible(true);
		scroll_list.set_visible(true);
		label_count.set_text(std::to_string(count) + " Notification" + (count > 1 ? "s" : ""));
		set_tooltip_text(label_count.get_text());
		icon += "-new";
	}
	image_icon.set_from_icon_name(icon + "-symbolic");
}

void module_notifications::clear_all() {
	std::vector<NotificationWidget*> widgets;
	auto* child = flowbox_list.get_first_child();
	while (child) {
		auto* flowbox_child = dynamic_cast<Gtk::FlowBoxChild*>(child);
		if (flowbox_child) {
			auto* widget = dynamic_cast<NotificationWidget*>(flowbox_child->get_child());
			if (widget) widgets.push_back(widget);
		}
		child = child->get_next_sibling();
	}
	
	for (auto* widget : widgets) {
		remove_notification(widget->get_id(), 2);
	}
}

void module_notifications::clear_alerts() {
	std::vector<NotificationWidget*> alerts;
	auto* child = flowbox_alert.get_first_child();
	while (child) {
		auto* flowbox_child = dynamic_cast<Gtk::FlowBoxChild*>(child);
		if (flowbox_child) {
			auto* widget = dynamic_cast<NotificationWidget*>(flowbox_child->get_child());
			if (widget) alerts.push_back(widget);
		}
		child = child->get_next_sibling();
	}
	
	for (auto* widget : alerts) {
		widget->signal_close.emit(2);
	}
}