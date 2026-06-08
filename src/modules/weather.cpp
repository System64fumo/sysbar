#include "weather.hpp"

#include <glibmm/dispatcher.h>
#include <fstream>
#include <curl/curl.h>
#include <iomanip>
#include <ctime>
#include <thread>
#include <algorithm>

module_weather::module_weather(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start) {
	add_css_class("module_weather");
	image_icon.set_from_icon_name("content-loading-symbolic");
	label_info.hide();

	std::string cfg_url = win->config_main["weather"]["url"];
	if (!cfg_url.empty())
		weather_file_url = cfg_url;

	std::string cfg_unit = win->config_main["weather"]["unit"];
	if (!cfg_unit.empty())
		unit = cfg_unit[0];

	dispatcher.connect(sigc::mem_fun(*this, &module_weather::update_info));

	std::thread fetch_thread(&module_weather::fetch_data, this);
	fetch_thread.detach();

	Glib::signal_timeout().connect([this]() {
		std::thread(&module_weather::fetch_data, this).detach();
		return true;
	}, 60 * 60 * 1000); // Update every hour
}

bool module_weather::fetch_data() {
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
		std::fprintf(stderr, "Failed to parse weather data\n");
		return true;
	}

	// Sanity check
	file.seekg(0);
	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (!Json::parseFromStream(builder, file, &root, &errs)) {
		std::fprintf(stderr, "The weather file does not seem to be valid: %s\n", errs.c_str());
		return false;
	}

	// Reset the file pointer and load the data
	file.seekg(0, std::ios::beg);
	file >> json_data;
	file.close();

	// Get time and date
	const std::time_t& t = std::time(nullptr);
	std::tm* now = std::localtime(&t);

	std::ostringstream date_stream;
	date_stream << std::put_time(now, "%Y-%m-%d");

	const std::string& date = date_stream.str();
	const std::string& time = std::to_string((now->tm_hour / 3) * 300);

	get_weather_data(date, time);

	return true;
}

void module_weather::update_info() {
	weather_info info;
	{
		std::lock_guard<std::mutex> lock(data_mutex);
		if (!data_ready) {
			image_icon.set_from_icon_name("weather-none-available-symbolic");
			return;
		}
		info = pending_weather;
		data_ready = false;
	}

	// Apply the weather data to the UI (main thread only)
	weather_info_current = info;

	if (unit == 'c')
		label_info.set_text(weather_info_current.temp_C);
	else if (unit == 'f')
		label_info.set_text(weather_info_current.temp_F);
	else
		std::fprintf(stderr, "Unknown unit: %c\n", unit);

	// Add more cases, Snow, Storms, ect ect
	const std::map<std::string, std::string> icon_from_desc = {
		{"sunny", "weather-clear-symbolic"},
		{"clear", "weather-clear-symbolic"},
		{"partly cloudy", "weather-few-clouds-symbolic"},
		{"cloudy", "weather-clouds-symbolic"},
		{"overcast", "weather-overcast-symbolic"},
		{"patchy rain nearby", "weather-showers-scattered-symbolic"},
		{"patchy rain possible", "weather-showers-scattered-symbolic"},
		{"patchy light rain", "weather-showers-scattered-symbolic"},
		{"light rain", "weather-showers-scattered-symbolic"},
		{"light rain shower", "weather-showers-scattered-symbolic"},
	};

	// Set icon according to weather description
	auto it = icon_from_desc.find(weather_info_current.weatherDesc);
	if (it != icon_from_desc.end())
		image_icon.set_from_icon_name(it->second);
	else
		image_icon.set_from_icon_name("weather-none-available-symbolic");

	set_tooltip_text(weather_info_current.weatherDesc);
	label_info.show();
}

void module_weather::download_file() {
	CURL *curl;
	FILE *fp;
	CURLcode res;
	curl = curl_easy_init();
	if (!curl) {
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
					weather_info info;
					info.feels_like_C = hourly["FeelsLikeC"].asString();
					info.feels_like_F = hourly["FeelsLikeF"].asString();
					info.temp_C = hourly["tempC"].asString();
					info.temp_F = hourly["tempF"].asString();
					info.humidity = hourly["humidity"].asString();
					info.weatherDesc = hourly["weatherDesc"][0]["value"].asString();
					std::transform(info.weatherDesc.begin(),
						info.weatherDesc.end(),
						info.weatherDesc.begin(),
						[](unsigned char c) { return std::tolower(c); });

					// For whatever reason, sometimes the last character is a space
					if (info.weatherDesc.back() == ' ')
						info.weatherDesc.pop_back();

					// Store the data and signal the main thread
					{
						std::lock_guard<std::mutex> lock(data_mutex);
						pending_weather = info;
						data_ready = true;
					}
					dispatcher.emit();
					return;
				}
			}
		}
	}

	// If we reach this point then the file is probably out of date
	download_file();
}
