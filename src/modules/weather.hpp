#pragma once
#ifdef MODULE_WEATHER

#include "../module.hpp"
#include <nlohmann/json.hpp>

class module_weather : public module {
	public:
		module_weather(const config &cfg, const bool &icon_on_start = false, const bool &clickable = false);

	private:
		nlohmann::json json_data;
		char unit = 'c';
		std::string tempC;
		std::string tempF;
		std::string weatherDesc;
		std::string weather_file;
		std::string weather_file_url = "https://wttr.in/?format=j1";

		bool update_info();
		void download_file();
		void get_weather_data(const std::string &date, const std::string &time);
};

#endif
