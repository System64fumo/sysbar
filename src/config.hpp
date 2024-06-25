#pragma once
#include <string>
#include <map>
#include <string>

/*
	Default config.
	Can be configured instead of using launch arguments.
*/

// Current													Default
inline int position = 0;									// 0
inline int size = 40;										// 40
inline bool verbose = false;								// false
inline std::string m_start = "clock,weather,tray";			// "clock,weather,tray"
inline std::string m_center = "hyprland";					// ""
inline std::string m_end = "volume,network,notification";	// "volume,network,notification"

// Build time configuration		Description
#define RUNTIME_CONFIG			// Allow the use of runtime arguments
#define MODULE_CLOCK			// Include the clock module
#define MODULE_WEATHER			// Include the weather module
#define MODULE_TRAY				// Include the tray module
#define MODULE_HYPRLAND			// Include the tray module
#define MODULE_VOLUME			// Include the volume module
#define MODULE_NETWORK			// Include the network module
#define MODULE_NOTIFICATION		// Include the notifications module

// INI parser
class config_parser {
	public:
		config_parser(const std::string &filename);
		std::string get_value(const std::string &section, const std::string &key);

	private:
		std::map<std::string, std::map<std::string, std::string>> data;
		std::string trim(const std::string &str);
};
