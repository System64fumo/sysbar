#pragma once
#include "../module.hpp"
#ifdef MODULE_WEATHER

#include <json/json.h>

class module_weather : public module {
	public:
		module_weather(sysbar*, const bool&);

	private:
		struct weather_info {
			std::string feels_like_C;
			std::string feels_like_F;
			std::string temp_C;
			std::string temp_F;
			std::string humidity;
			std::string weatherDesc;
		} weather_info_current;

		Json::Value json_data;
		char unit = 'c';
		std::string weather_file;
		std::string weather_file_url = "https://wttr.in/?format=j1";

		bool update_info();
		void download_file();
		void get_weather_data(const std::string&, const std::string&);
};

#endif
