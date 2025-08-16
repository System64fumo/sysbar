#include "window.hpp"

Gtk::Revealer* sidepanel::create_header(const bool& position_start) {
	Gtk::Revealer* revealer_header = Gtk::make_managed<Gtk::Revealer>();
	Gtk::Box* box_header = Gtk::make_managed<Gtk::Box>();
	Gtk::Label* label_header = Gtk::make_managed<Gtk::Label>();
	Gtk::Button* button_return = Gtk::make_managed<Gtk::Button>();
	Gtk::Revealer* revealer_return = Gtk::make_managed<Gtk::Revealer>();

	revealer_header->set_child(*box_header);
	box_header->append(*revealer_return);
	box_header->append(*label_header);
	if (position_start)
		label_header->set_text("Start");
	else
		label_header->set_text("End");

	revealer_return->set_child(*button_return);
	button_return->set_image_from_icon_name("back");
	button_return->signal_clicked().connect([&, position_start]() {
		if (position_start)
			window->sidepanel_start->stack_pages.set_visible_child("main");
		else
			window->sidepanel_end->stack_pages.set_visible_child("main");
	});

	return revealer_header;
}

sidepanel::sidepanel(sysbar* window, const bool& position_start) : window(window) {
	std::string pos = position_start ? "end" : "start";
	std::string cfg_sidepanel_start_size = window->config_main["main"]["sidepanel-" + pos + "-size"];
	if (!cfg_sidepanel_start_size.empty())
		window->default_size_start = std::stoi(cfg_sidepanel_start_size);

	// Header
	// TODO: Re-Enable headers
	// revealer_header = create_header(true);
	// box_widgets.append(*revealer_header);
	// revealer_header->set_reveal_child(true);

	// Content
	set_child(box_widgets);
	set_kinetic_scrolling(false);
	stack_pages.get_style_context()->add_class("widgets_" + pos);
	stack_pages.add(grid_main, "main");
	stack_pages.set_vexpand_set();
	stack_pages.set_transition_type(Gtk::StackTransitionType::SLIDE_LEFT_RIGHT);
	box_widgets.get_style_context()->add_class("sidepanel_" + pos);
	box_widgets.append(stack_pages);
}