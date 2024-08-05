#include "../config_parser.hpp"
#include "../config.hpp"
#include "taskbar.hpp"

#include <gdk/wayland/gdkwayland.h>
#include <iostream>

void handle_manager_toplevel(void *data, zwlr_foreign_toplevel_manager_v1 *manager,
	zwlr_foreign_toplevel_handle_v1 *toplevel) {

	std::cout << "New toplevel appeared" << std::endl;
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
	setup_proto();
}

void module_taskbar::setup_proto() {
	auto gdk_display = gdk_display_get_default();
	auto display = gdk_wayland_display_get_wl_display(gdk_display);
	auto registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, this);
	wl_display_roundtrip(display);
}
