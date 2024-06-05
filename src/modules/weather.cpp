#include "weather.hpp"

#include <iostream>
#include <fstream>

module_weather::module_weather(bool icon_on_start, bool clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_weather");

	// TODO: Set icon according to weather
	image_icon.set_from_icon_name("weather-cloudy-symbolic");

	update_info();
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_weather::update_info), 60 * 60 * 1000); // Update every hour
}

bool module_weather::update_info() {
	// TODO: Ideally this should run on another thread,
	// You know.. Juuuust in case it blocks ui updataes

	std::string home_dir = getenv("HOME");
	std::string weather_file = home_dir + "/.cache/sysbar-weather.json";
	std::ifstream file(weather_file);

	// TODO: Add way to download the weather file

	if (!file.is_open()) {
		std::cerr << "Could not open weather file!" << std::endl;
		return false;
	}

	file >> json_data;
	file.close();

	// TODO: Get time and date automatically
	// For now use temporary test data
	std::string date = "2024-06-06";
	std::string time = "0";

	get_weather_data(date, time);

	label_info.set_text(tempC);
	return true;
}

void module_weather::get_weather_data(std::string date, std::string time) {
	auto weatherArray = json_data["weather"];

	// Iterate over each date in the weather array
	for (const auto& dailyWeather : weatherArray) {
		if (dailyWeather["date"] == date) {
			auto hourlyArray = dailyWeather["hourly"];

			// Iterate over each hourly section
			for (const auto& hourly : hourlyArray) {
				if (hourly["time"] == time) {
					tempC = hourly["tempC"];
					tempF = hourly["tempF"];
					weatherDesc = hourly["weatherDesc"][0]["value"];
					return;
				}
			}
		}
	}

	// TODO: Handle out of bounds or invalid dates
	// Aka re download the file with a more up to date version
}
