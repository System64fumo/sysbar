#include "cellular.hpp"

#include <giomm/dbusconnection.h>

module_cellular::module_cellular(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_cellular");
	image_icon.set_from_icon_name("network-cellular-acquiring-symbolic");
	label_info.set_text("0");

	#ifdef CONFIG_FILE
		std::string cfg_icon = config->get_value("cellular", "show-icon");
		if (cfg_icon != "true") {
			image_icon.hide();
			label_info.set_margin_end(config_main.size / 3);
		}

		std::string cfg_label = config->get_value("cellular", "show-label");
		if (cfg_label != "true")
			label_info.hide();
	#endif

	setup();
}

void module_cellular::setup() {
	proxy = Gio::DBus::Proxy::create_sync(
		Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM),
		"org.freedesktop.ModemManager1",
		"/org/freedesktop/ModemManager1/Modem/0",
		"org.freedesktop.DBus.Properties");

	proxy->signal_signal().connect(
		sigc::mem_fun(*this, &module_cellular::on_properties_changed));

	// TODO: Add a manual trigger
}

void module_cellular::on_properties_changed(
	const Glib::ustring& sender_name,
	const Glib::ustring& signal_name,
	const Glib::VariantContainerBase& parameters) {

	Glib::VariantIter iter(parameters);
	Glib::VariantBase child;
	iter.next_value(child); // Ignore this for now "org.freedesktop.ModemManager1.Modem"
	iter.next_value(child);

	auto variant_dict = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>>(child);
	std::map<Glib::ustring, Glib::VariantBase> map_hints = variant_dict.get();
	for (const auto& [key, value] : map_hints) {
		if (key == "SignalQuality") {
			auto tuple = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(value);
			uint32_t signal = Glib::VariantBase::cast_dynamic<Glib::Variant<uint32_t>>(tuple.get_child(0)).get();
			//uint32_t boolean = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(tuple.get_child(1)).get();
			label_info.set_text(std::to_string(signal));

			std::string icon;
			if (signal > 80)
				icon = "network-cellular-signal-excellent-symbolic";
			else if (signal > 60)
				icon = "network-cellular-signal-good-symbolic";
			else if (signal > 40)
				icon = "network-cellular-signal-ok-symbolic";
			else if (signal > 20)
				icon = "network-cellular-signal-weak-symbolic";
			else
				icon = "network-cellular-signal-none-symbolic";
			image_icon.set_from_icon_name(icon);
		}
	}
}
