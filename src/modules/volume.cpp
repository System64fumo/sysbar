#include "volume.hpp"

#include <thread>

module_volume::module_volume(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_volume");
	volume_icons[0] = "audio-volume-low-symbolic";
	volume_icons[1] = "audio-volume-medium-symbolic";
	volume_icons[2] = "audio-volume-high-symbolic";
	label_info.hide();


	if (win->config_main["volume"]["show-label"] == "true")
		label_info.show();


	dispatcher_callback.connect(sigc::mem_fun(*this, &module_volume::update_info));

	// Why is this even necessary??
	// Why does audio not work if not initialized from another thread?
	std::thread([&]() {
		usleep(100 * 1000); // WHY!
		sys_wp = new syshud_wireplumber(nullptr, &dispatcher_callback);
	}).detach();

	setup_widget();
}

void module_volume::setup_widget() {
	Gtk::Box *box_widget = Gtk::make_managed<Gtk::Box>();

	box_widget->get_style_context()->add_class("widget_volume");

	image_widget_icon.set_pixel_size(24);

	scale_volume.set_hexpand(true);
	scale_volume.set_range(0, 100);
	scale_volume.set_sensitive(false); // Setting volume is currently unsupported

	box_widget->append(image_widget_icon);
	box_widget->append(scale_volume);
	win->grid_widgets_end.attach(*box_widget, 0, 4, 4, 1);
}

void module_volume::update_info() {
	label_info.set_text(std::to_string(sys_wp->volume));
	if (sys_wp->muted)
		image_icon.set_from_icon_name("audio-volume-muted-blocking-symbolic");
	else
		image_icon.set_from_icon_name(volume_icons[sys_wp->volume / 35]);

	// Widget
	image_widget_icon.set_from_icon_name(image_icon.get_icon_name());
	scale_volume.set_value(sys_wp->volume);
}
