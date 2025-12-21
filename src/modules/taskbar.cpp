#include "taskbar.hpp"

#include <gdk/wayland/gdkwayland.h>
#include <gdkmm/seat.h>
#include <giomm/desktopappinfo.h>
#include <giomm/menu.h>
#include <giomm/simpleactiongroup.h>

std::vector<std::shared_ptr<Gio::AppInfo>> app_list;

std::string cleanup_string(const std::string &str) {
	std::string result;
	for (char c : str) {
		if (c != ' ' && c != '-' && c != '_') {
			result += std::tolower(c);
		}
	}
	return result;
}

Glib::RefPtr<Gio::AppInfo> find_app_info(const std::string& app_id) {
	if (app_id.empty()) return nullptr;
	
	std::string appid_lower = app_id;
	std::transform(appid_lower.begin(), appid_lower.end(), appid_lower.begin(), ::tolower);
	
	auto desktop_app = Gio::DesktopAppInfo::create(app_id + ".desktop");
	if (desktop_app) return desktop_app;
	
	desktop_app = Gio::DesktopAppInfo::create(appid_lower + ".desktop");
	if (desktop_app) return desktop_app;
	
	std::string appid_cleaned = cleanup_string(app_id);
	for (auto app : app_list) {
		std::string app_id_clean = cleanup_string(app->get_id());
		if (appid_cleaned == app_id_clean) return app;
		
		std::string app_name = cleanup_string(app->get_name());
		if (appid_cleaned == app_name) return app;
		
		std::string app_executable = cleanup_string(app->get_executable());
		if (appid_cleaned == app_executable) return app;
	}
	
	return nullptr;
}

void handle_toplevel_title(void* data, zwlr_foreign_toplevel_handle_v1* handle, const char* title) {
	auto toplevel_entry = static_cast<taskbar_item*>(data);
	toplevel_entry->handle = handle;

	std::string text = title;
	if (text.empty()) text = "Untitled Window";
	
	toplevel_entry->full_title = text;

	if (text.length() > toplevel_entry->text_length)
		text = text.substr(0, toplevel_entry->text_length - 2) + "..";

	toplevel_entry->toplevel_label.set_text(text);
	
	std::string tooltip = toplevel_entry->app_id;
	if (!tooltip.empty()) tooltip += "\n";
	tooltip += toplevel_entry->full_title;
	toplevel_entry->set_tooltip_text(tooltip);
}

void handle_toplevel_app_id(void* data, zwlr_foreign_toplevel_handle_v1*, const char* app_id) {
	auto toplevel_entry = static_cast<taskbar_item*>(data);
	
	toplevel_entry->app_id = app_id ? app_id : "";
	auto app_info = find_app_info(toplevel_entry->app_id);

	if (app_info && app_info->get_icon())
		toplevel_entry->image_icon.set(app_info->get_icon());
	else
		toplevel_entry->image_icon.set_from_icon_name("application-x-executable");
	
	std::string tooltip = toplevel_entry->app_id;
	if (!tooltip.empty() && !toplevel_entry->full_title.empty()) tooltip += "\n";
	if (!toplevel_entry->full_title.empty()) tooltip += toplevel_entry->full_title;
	toplevel_entry->set_tooltip_text(tooltip);
}

void handle_toplevel_output_enter(void* data, zwlr_foreign_toplevel_handle_v1*, wl_output* output) {}

void handle_toplevel_output_leave(void* data, zwlr_foreign_toplevel_handle_v1*, wl_output* output) {}

void handle_toplevel_state(void* data, zwlr_foreign_toplevel_handle_v1*, wl_array* state) {
	auto toplevel_entry = static_cast<taskbar_item*>(data);
	auto flowbox_child = static_cast<Gtk::FlowBoxChild*>(toplevel_entry->get_parent());
	auto flowbox = static_cast<Gtk::FlowBox*>(flowbox_child->get_parent());

	uint32_t* state_array = (uint32_t*)state->data;
	size_t count = state->size / sizeof(uint32_t);

	toplevel_entry->is_maximized = false;
	toplevel_entry->is_minimized = false;
	toplevel_entry->is_fullscreen = false;
	toplevel_entry->is_activated = false;

	for (size_t i = 0; i < count; ++i) {
		switch (state_array[i]) {
			case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED:
				toplevel_entry->is_maximized = true;
				break;
			case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED:
				toplevel_entry->is_minimized = true;
				break;
			case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED:
				toplevel_entry->is_activated = true;
				flowbox->select_child(*flowbox_child);
				break;
			case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN:
				toplevel_entry->is_fullscreen = true;
				break;
		}
	}
	
	if (toplevel_entry->is_activated)
		toplevel_entry->add_css_class("taskbar_item_active");
	else
		toplevel_entry->remove_css_class("taskbar_item_active");
}

void handle_toplevel_done(void* data, zwlr_foreign_toplevel_handle_v1*) {}

void handle_toplevel_closed(void* data, zwlr_foreign_toplevel_handle_v1* handle) {
	auto toplevel_entry = static_cast<taskbar_item*>(data);
	auto flowbox_child = static_cast<Gtk::FlowBoxChild*>(toplevel_entry->get_parent());
	auto flowbox = static_cast<Gtk::FlowBox*>(flowbox_child->get_parent());
	
	toplevel_entry->handle = nullptr;
	flowbox->remove(*toplevel_entry);
}

void handle_toplevel_parent(void* data, zwlr_foreign_toplevel_handle_v1* handle, zwlr_foreign_toplevel_handle_v1* parent) {}

zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_v1_impl = {
	.title = handle_toplevel_title,
	.app_id = handle_toplevel_app_id,
	.output_enter = handle_toplevel_output_enter,
	.output_leave = handle_toplevel_output_leave,
	.state = handle_toplevel_state,
	.done = handle_toplevel_done,
	.closed = handle_toplevel_closed,
	.parent = handle_toplevel_parent
};

void handle_manager_toplevel(void* data, zwlr_foreign_toplevel_manager_v1* manager,
	zwlr_foreign_toplevel_handle_v1* toplevel) {
	auto self = static_cast<module_taskbar*>(data);

	taskbar_item* toplevel_entry = Gtk::make_managed<taskbar_item>(self, self->cfg);
	
	zwlr_foreign_toplevel_handle_v1_add_listener(toplevel,
		&toplevel_handle_v1_impl, toplevel_entry);

	self->flowbox_main.append(*toplevel_entry);
}

zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_v1_impl = {
	.toplevel = handle_manager_toplevel,
};

void registry_handler(void* data, struct wl_registry* registry,
	uint32_t id, const char* interface, uint32_t version) {

	auto self = static_cast<module_taskbar*>(data);
	if (strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
		auto zwlr_toplevel_manager = (zwlr_foreign_toplevel_manager_v1*)
			wl_registry_bind(registry, id,
				&zwlr_foreign_toplevel_manager_v1_interface,
				std::min(version, 3u));
		zwlr_foreign_toplevel_manager_v1_add_listener(zwlr_toplevel_manager,
			&toplevel_manager_v1_impl, self);
	}
}

wl_registry_listener registry_listener = {
	&registry_handler
};

module_taskbar::module_taskbar(sysbar* window, const bool &icon_on_start) : module(window, icon_on_start) {
	remove_css_class("module");
	set_cursor(Gdk::Cursor::create("default"));
	image_icon.unparent();
	label_info.unparent();

	std::string cfg_text_length = win->config_main["taskbar"]["text-length"];
	if (!cfg_text_length.empty())
		cfg.text_length = std::stoi(cfg_text_length);

	std::string cfg_icon_size = win->config_main["taskbar"]["icon-size"];
	if (!cfg_icon_size.empty())
		cfg.icon_size = std::stoi(cfg_icon_size);

	cfg.show_icon = (win->config_main["taskbar"]["show-icon"] == "true");
	cfg.show_label = (win->config_main["taskbar"]["show-label"] == "true");

	if (win->position % 2 == 0) {
		box_container.set_halign(Gtk::Align::CENTER);
		flowbox_main.set_min_children_per_line(25);
		flowbox_main.set_max_children_per_line(25);
	} else {
		box_container.set_valign(Gtk::Align::CENTER);
	}

	flowbox_main.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
		auto toplevel_entry = static_cast<taskbar_item*>(child->get_child());
		if (!toplevel_entry->handle) return;
		
		auto gseat = Gdk::Display::get_default()->get_default_seat();
		auto seat = gdk_wayland_seat_get_wl_seat(gseat->gobj());
		
		if (toplevel_entry->is_minimized)
			zwlr_foreign_toplevel_handle_v1_unset_minimized(toplevel_entry->handle);
		else if (toplevel_entry->is_activated)
			zwlr_foreign_toplevel_handle_v1_set_minimized(toplevel_entry->handle);
		else
			zwlr_foreign_toplevel_handle_v1_activate(toplevel_entry->handle, seat);
	});

	box_container.add_css_class("module_taskbar");
	box_container.append(flowbox_main);

	scrolledwindow.set_child(box_container);
	scrolledwindow.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::NEVER);
	scrolledwindow.set_hexpand_set(true);
	scrolledwindow.set_vexpand_set(true);
	append(scrolledwindow);

	app_list = Gio::AppInfo::get_all();
	app_list.erase(
		std::remove_if(app_list.begin(), app_list.end(),
			[](const Glib::RefPtr<Gio::AppInfo>& app_info) {
				return !app_info->should_show();
			}),
		app_list.end()
	);

	setup_proto();
}

void module_taskbar::setup_proto() {
	auto gdk_display = gdk_display_get_default();
	auto display = gdk_wayland_display_get_wl_display(gdk_display);
	auto registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, this);
	wl_display_roundtrip(display);
}

taskbar_item::taskbar_item(module_taskbar* self, const module_taskbar::config_tb& cfg) {
	text_length = cfg.text_length;
	image_icon.set_pixel_size(cfg.icon_size);

	auto gesture_click = Gtk::GestureClick::create();
	gesture_click->set_button(0);
	gesture_click->signal_pressed().connect([this, self, gesture_click](int n_press, double x, double y) {
		auto button = gesture_click->get_current_button();
		
		if (button == 2) {
			if (handle) {
				zwlr_foreign_toplevel_handle_v1_close(handle);
			}
		} else if (button == 3) {
			show_context_menu(self);
		}
	});
	add_controller(gesture_click);

	append(image_icon);
	append(toplevel_label);

	image_icon.set_visible(cfg.show_icon);
	toplevel_label.set_visible(cfg.show_label);

	if (!cfg.show_label) {
		image_icon.set_hexpand(true);
		image_icon.set_vexpand(true);
	} else {
		if (self->flowbox_main.get_min_children_per_line() == 25)
			set_size_request(100, -1);
		else
			set_size_request(-1, 100);
		toplevel_label.set_margin_start(3);
	}

	setup_context_menu(self);
}

void taskbar_item::setup_context_menu(module_taskbar* self) {
	auto empty_menu = Gio::Menu::create();
	context_menu = Gtk::PopoverMenu(empty_menu);
	context_menu.set_parent(*this);
	context_menu.set_has_arrow(false);
	
	auto action_group = Gio::SimpleActionGroup::create();
	
	action_group->add_action("minimize", [this]() {
		if (!handle) return;
		if (is_minimized)
			zwlr_foreign_toplevel_handle_v1_unset_minimized(handle);
		else
			zwlr_foreign_toplevel_handle_v1_set_minimized(handle);
	});
	
	action_group->add_action("maximize", [this]() {
		if (!handle) return;
		if (is_maximized)
			zwlr_foreign_toplevel_handle_v1_unset_maximized(handle);
		else
			zwlr_foreign_toplevel_handle_v1_set_maximized(handle);
	});
	
	action_group->add_action("fullscreen", [this]() {
		if (!handle) return;
		if (is_fullscreen)
			zwlr_foreign_toplevel_handle_v1_unset_fullscreen(handle);
		else
			zwlr_foreign_toplevel_handle_v1_set_fullscreen(handle, nullptr);
	});
	
	action_group->add_action("close", [this]() {
		if (handle)
			zwlr_foreign_toplevel_handle_v1_close(handle);
	});
	
	insert_action_group("taskbar", action_group);
}

void taskbar_item::show_context_menu(module_taskbar* self) {
	if (!handle) return;
	
	auto menu_model = Gio::Menu::create();
	
	if (is_minimized)
		menu_model->append("Unminimize", "taskbar.minimize");
	else
		menu_model->append("Minimize", "taskbar.minimize");
	
	if (is_maximized)
		menu_model->append("Unmaximize", "taskbar.maximize");
	else
		menu_model->append("Maximize", "taskbar.maximize");
	
	if (is_fullscreen)
		menu_model->append("Exit Fullscreen", "taskbar.fullscreen");
	else
		menu_model->append("Fullscreen", "taskbar.fullscreen");
	
	menu_model->append("Close", "taskbar.close");
	
	context_menu.set_menu_model(menu_model);
	context_menu.popup();
}

taskbar_item::~taskbar_item() {
	context_menu.unparent();
}