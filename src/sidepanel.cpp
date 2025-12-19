#include "window.hpp"

void sidepanel::setup_header() {
	revealer_header.set_child(box_header);

	box_header.add_css_class("header");
	box_header.append(revealer_return);
	box_header.append(label_header);
	label_header.add_css_class("title-4");
	label_header.set_margin_start(10);

	revealer_return.set_child(button_return);
	revealer_return.set_transition_type(Gtk::RevealerTransitionType::SLIDE_RIGHT);
	button_return.set_margin(5);
	button_return.set_image_from_icon_name("back");
	button_return.signal_clicked().connect([&]() {
		set_page("main");
	});
}

sidepanel::sidepanel(sysbar* window, const bool& position_start) : box_sidepanel(Gtk::Orientation::VERTICAL), window(window), position_start(position_start) {
	std::string pos = position_start ? "start" : "end";
	std::string cfg_sidepanel_start_size = window->config_main["main"]["sidepanel-" + pos + "-size"];
	if (!cfg_sidepanel_start_size.empty())
		window->default_size_start = std::stoi(cfg_sidepanel_start_size);

	// Header
	setup_header();
	box_widgets.append(revealer_header);
	revealer_header.set_reveal_child(true);

	// Content
	set_child(box_sidepanel);
	set_kinetic_scrolling(false);
	stack_pages.add(grid_main, "main");
	stack_pages.set_vexpand_set();
	stack_pages.set_transition_type(Gtk::StackTransitionType::SLIDE_LEFT_RIGHT);
	stack_pages.add_css_class("control_page_main");
	box_widgets.append(stack_pages);
	box_widgets.add_css_class("sidepanel");
	box_sidepanel.append(box_widgets);

	set_page("main");
}

void sidepanel::set_page(const std::string& destination) {
	stack_pages.set_visible_child(destination);
	revealer_return.set_reveal_child(destination != "main");
	std::map<std::string, std::string> pages; // TODO: Add translations

	// EN_US
	if (position_start)
		pages["main"] = window->config_main["sidepanels"]["start-label"];
	else
		pages["main"] = window->config_main["sidepanels"]["end-label"];
	pages["battery"] = "Battery";
	pages["bluetooth"] = "Bluetooth";
	pages["network"] = "Network";

	label_header.set_text(pages[destination]);
	revealer_header.set_reveal_child(!pages[destination].empty());
}