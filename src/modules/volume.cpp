#include "../config_parser.hpp"
#include "volume.hpp"

#include <thread>

module_volume::module_volume(const config_bar &cfg, const bool &icon_on_start, const bool &clickable) : module(cfg, icon_on_start, clickable) {
	get_style_context()->add_class("module_volume");
	volume_icons[0] = "audio-volume-low-symbolic";
	volume_icons[1] = "audio-volume-medium-symbolic";
	volume_icons[2] = "audio-volume-high-symbolic";
	label_info.hide();

	#ifdef CONFIG_FILE
	config_parser config(std::string(getenv("HOME")) + "/.config/sys64/bar/config.conf");

	if (config.available) {
		std::string cfg_label = config.get_value("volume", "label");
		if (cfg_label == "true")
			label_info.show(); 
	}
	#endif

	dispatcher_callback.connect(sigc::mem_fun(*this, &module_volume::update_info));

	// Why is this even necessary??
	// Why does audio not work if not initialized from another thread?
	std::thread thread_audio = std::thread([this]() {
		sys_wp = new sysvol_wireplumber(&dispatcher_callback);
	});
	thread_audio.join();
}

void module_volume::update_info() {
	label_info.set_text(std::to_string(sys_wp->volume));
	if (sys_wp->muted)
		image_icon.set_from_icon_name("audio-volume-muted-blocking-symbolic");
	else
		image_icon.set_from_icon_name(volume_icons[sys_wp->volume / 35]);
}
