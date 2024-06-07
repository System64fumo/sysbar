#include "volume.hpp"
#include <thread>

module_volume::module_volume(const bool &icon_on_start, const bool &clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_volume");
	image_icon.set_from_icon_name("audio-volume-high-symbolic");

	update_info();
	dispatcher_callback.connect(sigc::mem_fun(*this, &module_volume::update_info));

	// Why is this even necessary??
	// Why does audio not work if not initialized from another thread?
	std::thread thread_audio = std::thread([this]() {
		sys_wp = new sys_wireplumber(&dispatcher_callback);
	});
	thread_audio.join();
}

void module_volume::update_info() {
	label_info.set_text(std::to_string(sys_wp->volume));
}
