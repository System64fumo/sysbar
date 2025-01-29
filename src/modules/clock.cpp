#include "clock.hpp"
#include "../config_parser.hpp"

#include <glibmm/dispatcher.h>
#include <filesystem>
#include <ctime>

module_clock::module_clock(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_clock");
	image_icon.set_from_icon_name("preferences-system-time-symbolic");

	std::string cfg_label_format = win->config_main["clock"]["label-format"];
	if (!cfg_label_format.empty())
		tooltip_format = cfg_label_format;

	std::string cfg_tooltip_format = win->config_main["clock"]["tooltip-format"];
	if (!cfg_tooltip_format.empty())
		tooltip_format = cfg_tooltip_format;

	std::string cfg_interval = win->config_main["clock"]["interval"];
	if (!cfg_interval.empty())
		interval = std::stoi(cfg_interval);

	std::string cfg_layout = win->config_main["clock"]["widget-layout"];
	if (!cfg_layout.empty()) {
		widget_layout.clear();
		for (char c : cfg_layout) {
			widget_layout.push_back(c - '0');
		}
	}

	update_info();
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_clock::update_info), interval);
	window->overlay_window.signal_show().connect(sigc::mem_fun(*this, &module_clock::on_overlay_change));
	setup_widget();
}

bool module_clock::update_info() {
	std::time_t now = std::time(nullptr);
	std::tm* local_time = std::localtime(&now);

	char label_buffer[32];
	std::strftime(label_buffer, sizeof(label_buffer), label_format.c_str(), local_time);
	label_info.set_text(label_buffer);

	char tooltip_buffer[32];
	std::strftime(tooltip_buffer, sizeof(tooltip_buffer), tooltip_format.c_str() , local_time);
	set_tooltip_text(tooltip_buffer);

	return true;
}

void module_clock::setup_widget() {
	Gtk::Box* box_widget = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
	box_widget->append(calendar);
	box_widget->append(revealer_events);
	calendar.set_hexpand(true);
	label_event.set_xalign(0);
	label_event.get_style_context()->add_class("event_label");
	revealer_events.set_child(label_event);
	revealer_events.set_transition_type(Gtk::RevealerTransitionType::SLIDE_DOWN);
	revealer_events.set_transition_duration(500);
	win->grid_widgets_start.attach(*box_widget, widget_layout[0], widget_layout[1], widget_layout[2], widget_layout[3]);

	std::string events_path;
	const std::string user_events = std::string(getenv("HOME")) + "/.config/sys64/bar/calendar.conf";
	const std::string system_events = "/usr/share/sys64/bar/calendar.conf";

	if (std::filesystem::exists(user_events))
		events_path = user_events;
	else
		events_path = system_events;

	config_parser events(events_path);
	calendar_events = events.data;

	calendar.signal_day_selected().connect(sigc::mem_fun(*this, &module_clock::check_for_events));
	calendar.signal_prev_month().connect(sigc::mem_fun(*this, &module_clock::on_calendar_change));
	calendar.signal_next_month().connect(sigc::mem_fun(*this, &module_clock::on_calendar_change));
	calendar.signal_prev_year().connect(sigc::mem_fun(*this, &module_clock::on_calendar_change));
	calendar.signal_next_year().connect(sigc::mem_fun(*this, &module_clock::on_calendar_change));
	on_calendar_change();
}

void module_clock::on_calendar_change() {
	// TODO: Add event types
	auto it = calendar_events.find("events");
	if (it == calendar_events.end())
		return;

	calendar.clear_marks();
	for (const auto& [event_date, event_name] : it->second) {
		module_clock::date_time date = parse_date_time(event_date);

		const bool& same_year = date.year == calendar.get_year() || date.year == 0;
		const bool& same_month = date.month - 1 == calendar.get_month() || date.month == 0;
		if (same_year && same_month) {
			calendar.mark_day(date.day);
		}
	}

	check_for_events();
}

void module_clock::check_for_events() {
	revealer_events.set_reveal_child(false);
	if (!event_class.empty()) {
		win->get_style_context()->remove_class(event_class);
		win->grid_widgets_start.get_style_context()->remove_class(event_class);
		win->grid_widgets_end.get_style_context()->remove_class(event_class);
		event_class = "";
	}

	if (!calendar.get_day_is_marked(calendar.get_day()))
		return;
	auto datetime = calendar.get_date();
	std::string date_str = datetime.format("%Y-%m-%d");
	std::string event = calendar_events["events"][date_str];
	if (event.empty())
		event = calendar_events["events"]["*" + date_str.substr(4)];

	// What?
	if (event.empty())
		return;

	label_event.set_text(event);
	revealer_events.set_reveal_child(true);

	// TODO: Check if today is the day of the event
	for (char c : event) {
		if (std::isalpha(c))
			event_class.push_back(std::tolower(c));
	}
	event_class = "event_" + event_class;
	win->get_style_context()->add_class(event_class);
	win->grid_widgets_start.get_style_context()->add_class(event_class);
	win->grid_widgets_end.get_style_context()->add_class(event_class);
}

module_clock::date_time module_clock::parse_date_time(const std::string& date_str) {
	date_time date;

	std::istringstream ss(date_str);
	std::string year_str, month_str, day_str;
	std::getline(ss, year_str, '-');
	std::getline(ss, month_str, '-');
	std::getline(ss, day_str, ' ');

	if (year_str != "*")
		date.year = std::stoi(year_str);
	if (month_str != "*")
		date.month = std::stoi(month_str);
	if (day_str != "*")
		date.day = std::stoi(day_str);

	return date;
}

void module_clock::on_overlay_change() {
	// Focus calendar to current day when the overlay is shown
	calendar.select_day(Glib::DateTime::create_now_local());
}
