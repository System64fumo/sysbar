#include "weather.hpp"

#include <glibmm/dispatcher.h>
#include <fstream>
#include <curl/curl.h>
#include <iomanip>
#include <ctime>
#include <thread>

module_weather::module_weather(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_weather");
	image_icon.set_from_icon_name("content-loading-symbolic");
	label_info.hide();

	std::string cfg_url = win->config_main["weather"]["url"];
	if (!cfg_url.empty())
		weather_file_url = cfg_url;

	std::string cfg_unit = win->config_main["weather"]["unit"];
	if (!cfg_unit.empty())
		unit = cfg_unit[0];


	std::thread update_thread(&module_weather::update_info, this);
	update_thread.detach();

	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_weather::update_info), 60 * 60 * 1000); // Update every hour
}

bool module_weather::update_info() {
	// TODO: Ideally this should run on another thread,
	// You know.. Juuuust in case it blocks ui updataes

	std::string home_dir = getenv("HOME");
	weather_file = std::move(home_dir) + "/.cache/sysbar-weather.json";

	std::ifstream file(weather_file, std::ios::ate);

	// Check if the file is Ok
	if (file.tellg() < 10) {
		file.close();
		download_file();
		file.open(weather_file, std::ios::ate);
	}

	// The file is not okay
	if (!file.is_open() || file.tellg() < 10) {
		image_icon.set_from_icon_name("weather-none-available-symbolic");
		std::fprintf(stderr, "Failed to parse weather data\n");
		return true;
	}

	// Sanity check
	file.seekg(0);
	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (!Json::parseFromStream(builder, file, &root, &errs)) {
		image_icon.set_from_icon_name("weather-none-available-symbolic");
		std::fprintf(stderr, "The weather file does not seem to be valid: %s\n", errs.c_str());
		return false;
	}

	// Reset the file pointer and load the data
	file.seekg(0, std::ios::beg);
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

	if (unit == 'c')
		label_info.set_text(weather_info_current.temp_C);\
	else if (unit == 'f')
		label_info.set_text(weather_info_current.temp_F);
	else
		std::fprintf(stderr, "Unknown unit: %c\n", unit);

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
		std::fprintf(stderr, "Error: unable to initialize curl.\n");
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
		std::fprintf(stderr, "Error: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		return;
	}
}

void module_weather::get_weather_data(const std::string& date, const std::string& time) {
	Json::Value weatherArray = json_data["weather"];

	// Iterate over each date in the weather array
	for (const auto& dailyWeather : weatherArray) {
		if (dailyWeather["date"].asString() == date) {
			Json::Value hourlyArray = dailyWeather["hourly"];

			// Iterate over each hourly section
			for (const auto& hourly : hourlyArray) {
				if (hourly["time"].asString() == time) {
					weather_info_current.feels_like_C = hourly["FeelsLikeC"].asString();
					weather_info_current.feels_like_F = hourly["FeelsLikeF"].asString();
					weather_info_current.temp_C = hourly["tempC"].asString();
					weather_info_current.temp_F = hourly["tempF"].asString();
					weather_info_current.humidity = hourly["humidity"].asString();
					weather_info_current.weatherDesc = hourly["weatherDesc"][0]["value"].asString();
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
