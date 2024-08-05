#include "../config_parser.hpp"
#include "../config.hpp"
#include "taskbar.hpp"

#include <gdk/wayland/gdkwayland.h>
#include <iostream>

// Placeholder functions
void handle_toplevel_title(void *data, zwlr_foreign_toplevel_handle_v1*, const char *title) {
	auto toplevel_entry = static_cast<Gtk::Box*>(data);
	std::string text = title;

	if (text.length() > 10)
		text = text.substr(0, 10 - 2) + "..";

	Gtk::Label *toplevel_label = Gtk::make_managed<Gtk::Label>(text);
	toplevel_entry->append(*toplevel_label);
	std::cout << title << std::endl;
}

void handle_toplevel_app_id(void *data, zwlr_foreign_toplevel_handle_v1*, const char *app_id) {}

void handle_toplevel_output_enter(void *data, zwlr_foreign_toplevel_handle_v1*, wl_output *output) {}

void handle_toplevel_output_leave(void *data, zwlr_foreign_toplevel_handle_v1*, wl_output *output) {}

void handle_toplevel_state(void *data, zwlr_foreign_toplevel_handle_v1*, wl_array *state) {}

void handle_toplevel_done(void *data, zwlr_foreign_toplevel_handle_v1*) {}

void handle_toplevel_closed(void *data, zwlr_foreign_toplevel_handle_v1* handle) {}

void handle_toplevel_parent(void *data, zwlr_foreign_toplevel_handle_v1* handle, zwlr_foreign_toplevel_handle_v1* parent) {}

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

void handle_manager_toplevel(void *data, zwlr_foreign_toplevel_manager_v1 *manager,
	zwlr_foreign_toplevel_handle_v1 *toplevel) {
	auto self = static_cast<module_taskbar*>(data);

	// Temporary placeholder
	Gtk::Box *toplevel_entry = Gtk::make_managed<Gtk::Box>();

	zwlr_foreign_toplevel_handle_v1_add_listener(toplevel,
		&toplevel_handle_v1_impl, toplevel_entry);

	self->flowbox_main.append(*toplevel_entry);
}

zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_v1_impl = {
	.toplevel = handle_manager_toplevel,
};

void registry_handler(void *data, struct wl_registry *registry,
							 uint32_t id, const char *interface, uint32_t version) {

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

module_taskbar::module_taskbar(const config_bar &cfg, const bool &icon_on_start, const bool &clickable) : module(cfg, icon_on_start, clickable) {
	image_icon.hide();
	label_info.hide();
	if (config_main.position %2 == 0) {
		flowbox_main.set_orientation(Gtk::Orientation::VERTICAL);
	}
	append(flowbox_main);
	setup_proto();
}

void module_taskbar::setup_proto() {
	auto gdk_display = gdk_display_get_default();
	auto display = gdk_wayland_display_get_wl_display(gdk_display);
	auto registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, this);
	wl_display_roundtrip(display);
}
