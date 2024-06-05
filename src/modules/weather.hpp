#include "../module.hpp"
#include <nlohmann/json.hpp>

class module_weather : public module {
	public:
		module_weather(bool icon_on_start = false, bool clickable = false);

	private:
		nlohmann::json json_data;
		std::string tempC;
		std::string tempF;
		std::string weatherDesc;
		std::string weather_file;
		const char *weather_file_url;

		bool update_info();
		void download_file();
		void get_weather_data(std::string date, std::string time);
};
