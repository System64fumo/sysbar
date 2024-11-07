#pragma once
#include "../module.hpp"
#ifdef MODULE_CLOCK

#include <gtkmm/calendar.h>
#include <gtkmm/revealer.h>

class module_clock : public module {
	public:
		module_clock(sysbar*, const bool&);

	private:
		struct date_time {
			int year = 0;
			int month = 0;
			int day = 0;
		};

		Gtk::Calendar calendar;
		Gtk::Revealer revealer_events;
		Gtk::Label label_event;

		int interval = 1000;
		std::string label_format = "%H:%M";
		std::string tooltip_format = "%Y/%m/%d";
		std::vector<int> widget_layout = {0, 0, 4, 4};
		std::map<std::string, std::map<std::string, std::string>> calendar_events;
		std::string event_class;

		bool update_info();
		void setup_widget();
		void on_calendar_change();
		void check_for_events();
		date_time parse_date_time(const std::string& date_str);
};

#endif
