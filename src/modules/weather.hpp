#pragma once
#ifdef MODULE_WEATHER

#include "../module.hpp"
#include <nlohmann/json.hpp>

class module_weather : public module {
	public:
		module_weather(const config_bar &cfg, const bool &icon_on_start = false, const bool &clickable = false);

	private:
		struct weather_info {
			std::string feels_like_C;
			std::string feels_like_F;
			std::string temp_C;
			std::string temp_F;
			std::string humidity;
			std::string weatherDesc;
		} weather_info_current;

		nlohmann::json json_data;
		char unit = 'c';
		std::string weather_file;
		std::string weather_file_url = "https://wttr.in/?format=j1";

		bool update_info();
		void download_file();
		void get_weather_data(const std::string &date, const std::string &time);
};

#endif
