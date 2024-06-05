#include "weather.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <curl/curl.h>
#include <iomanip>
#include <ctime>

module_weather::module_weather(bool icon_on_start, bool clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_weather");

	// TODO: Set icon according to weather
	image_icon.set_from_icon_name("weather-cloudy-symbolic");

	weather_file_url = "https://wttr.in/?format=j1";
	update_info();
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_weather::update_info), 60 * 60 * 1000); // Update every hour
}

bool module_weather::update_info() {
	// TODO: Ideally this should run on another thread,
	// You know.. Juuuust in case it blocks ui updataes

	std::string home_dir = getenv("HOME");
	weather_file = home_dir + "/.cache/sysbar-weather.json";

	if (!std::filesystem::exists(weather_file)) {
		// TODO: Check for internet before trying to download
		// Maybe add a way to set a custom URL later?
		download_file();
	}

	std::ifstream file(weather_file);
	file >> json_data;
	file.close();

	// Get time and date
	std::time_t t = std::time(nullptr);
	std::tm* now = std::localtime(&t);

	std::ostringstream date_stream;
	date_stream << std::put_time(now, "%Y-%m-%d");

	std::string date = date_stream.str();
	std::string time = std::to_string((now->tm_hour / 3) * 300);

	get_weather_data(date, time);

	// TODO: Add option to pick between  celsius or fahrenheit
	label_info.set_text(tempC);
	return true;
}

void module_weather::download_file() {
	CURL *curl;
	FILE *fp;
	CURLcode res;
	curl = curl_easy_init();
	if (!curl) {
		std::cerr << "Error: unable to initialize curl." << std::endl;
		return;
	}

	fp = fopen(weather_file.c_str(), "wb");
	curl_easy_setopt(curl, CURLOPT_URL, weather_file_url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(fp);

	if (res != CURLE_OK) {
		std::cerr << "Error: curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		return;
	}
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

	// If we reach this point then the file is probably out of date
	download_file();
}
