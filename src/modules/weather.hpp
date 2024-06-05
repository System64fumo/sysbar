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

		bool update_info();
		void get_weather_data(std::string date, std::string time);
};
