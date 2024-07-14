#include "../config_parser.hpp"
#include "../config.hpp"
#include "weather.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <curl/curl.h>
#include <iomanip>
#include <ctime>
#include <thread>

module_weather::module_weather(const config_bar &cfg, const bool &icon_on_start, const bool &clickable) : module(cfg, icon_on_start, clickable) {
	get_style_context()->add_class("module_weather");
	image_icon.set_from_icon_name("content-loading-symbolic");
	label_info.hide();

	#ifdef CONFIG_FILE
	config_parser config(std::string(getenv("HOME")) + "/.config/sys64/bar/config.conf");

	std::string cfg_url = config.get_value("weather", "url");
	if (!cfg_url.empty())
		weather_file_url = cfg_url;

	std::string cfg_unit = config.get_value("weather", "unit");
	if (!cfg_unit.empty())
		unit = cfg_unit[0];

	#endif

	std::thread update_thread(&module_weather::update_info, this);
	update_thread.detach();

	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_weather::update_info), 60 * 60 * 1000); // Update every hour
}

bool module_weather::update_info() {
	// TODO: Ideally this should run on another thread,
	// You know.. Juuuust in case it blocks ui updataes

	std::string home_dir = getenv("HOME");
	weather_file = home_dir + "/.cache/sysbar-weather.json";

	if (!std::filesystem::exists(weather_file)) {
		// TODO: Check for internet before trying to download
		download_file();
	}
	if (std::filesystem::file_size(weather_file) == 0) {
		download_file();
	}

	try {
		std::ifstream file(weather_file);
		file >> json_data;
		file.close();
	}
	catch (...) {
		image_icon.set_from_icon_name("weather-none-available-symbolic");
		std::cout << "Failed to parse weather data" << std::endl;
		std::filesystem::remove(weather_file);
		return true;
	}

	// Get time and date
	std::time_t t = std::time(nullptr);
	std::tm* now = std::localtime(&t);

	std::ostringstream date_stream;
	date_stream << std::put_time(now, "%Y-%m-%d");

	std::string date = date_stream.str();
	std::string time = std::to_string((now->tm_hour / 3) * 300);

	get_weather_data(date, time);

	if (unit == 'c')
		label_info.set_text(weather_info_current.temp_C);\
	else if (unit == 'f')
		label_info.set_text(weather_info_current.temp_F);
	else
		std::cerr << "Unknown unit: " << unit << std::endl;

	// Add more cases, Snow, Storms, ect ect
	std::map<std::string, std::string> icon_from_desc = {
		{"Sunny", "weather-clear-symbolic"},
		{"Clear", "weather-clear-symbolic"},
		{"Partly Cloudy", "weather-few-clouds-symbolic"},
		{"Cloudy", "weather-clouds-symbolic"},
		{"Overcast", "weather-overcast-symbolic"},
		{"Patchy rain nearby", "weather-showers-scattered-symbolic"},
		{"Patchy rain possible", "weather-showers-scattered-symbolic"},
		{"Patchy light rain", "weather-showers-scattered-symbolic"},
		{"Light rain", "weather-showers-scattered-symbolic"},
	};

	// Set icon according to weather description
	if (icon_from_desc.find(weather_info_current.weatherDesc) != icon_from_desc.end())
		image_icon.set_from_icon_name(icon_from_desc[weather_info_current.weatherDesc]);
	else
		image_icon.set_from_icon_name("weather-none-available-symbolic");

	// TODO: For some reason this flickers?
	set_tooltip_text(weather_info_current.weatherDesc);

	return true;
}

void module_weather::download_file() {
	CURL *curl;
	FILE *fp;
	CURLcode res;
	curl = curl_easy_init();
	if (!curl) {
		image_icon.set_from_icon_name("weather-none-available-symbolic");
		std::cerr << "Error: unable to initialize curl." << std::endl;
		return;
	}

	fp = fopen(weather_file.c_str(), "wb");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_URL, weather_file_url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(fp);

	if (res != CURLE_OK) {
		image_icon.set_from_icon_name("weather-none-available-symbolic");
		std::cerr << "Error: curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		return;
	}
}

void module_weather::get_weather_data(const std::string &date, const std::string &time) {
	auto weatherArray = json_data["weather"];

	// Iterate over each date in the weather array
	for (const auto& dailyWeather : weatherArray) {
		if (dailyWeather["date"] == date) {
			auto hourlyArray = dailyWeather["hourly"];

			// Iterate over each hourly section
			for (const auto& hourly : hourlyArray) {
				if (hourly["time"] == time) {
					weather_info_current.feels_like_C = hourly["FeelsLikeC"];
					weather_info_current.feels_like_F = hourly["FeelsLikeF"];
					weather_info_current.temp_C = hourly["tempC"];
					weather_info_current.temp_F = hourly["tempF"];
					weather_info_current.humidity = hourly["humidity"];
					weather_info_current.weatherDesc = hourly["weatherDesc"][0]["value"];
					 // For whatever reason, sometimes the last character is a space
					if (weather_info_current.weatherDesc.back() == ' ')
						weather_info_current.weatherDesc.pop_back();
					label_info.show();
					return;
				}
			}
		}
	}

	// If we reach this point then the file is probably out of date
	download_file();
}
