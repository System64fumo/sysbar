#pragma once
#include <string>

// Build time configuration		Description
#define CONFIG_RUNTIME			// Allow the use of runtime arguments
#define CONFIG_FILE				// Allow the use of a config file
#define MODULE_CLOCK			// Include the clock module
#define MODULE_WEATHER			// Include the weather module
#define MODULE_TRAY				// Include the tray module
#define MODULE_HYPRLAND			// Include the tray module
#define MODULE_VOLUME			// Include the volume module
#define MODULE_NETWORK			// Include the network module
#define MODULE_BATTERY			// Include the battery module
#define MODULE_NOTIFICATION		// Include the notifications module
#define MODULE_PERFORMANCE		// Include the performance module
#define MODULE_TASKBAR			// Include the taskbar module
#define MODULE_BACKLIGHT		// Include the backlight module
#define MODULE_MPRIS			// Include the mpris module
#define MODULE_BLUETOOTH		// Include the bluetooth module
#define MODULE_CONTROLS			// Include the controls module (technically widget only)
#define MODULE_CELLULAR			// Include the cellular module
#define FEATURE_WIRELESS		// Support for wireless networks

// Default config
struct config_bar {
	int position = 0;
	int size = 40;
	bool verbose = false;
	int main_monitor = 0;
	std::string m_start = "clock,weather,tray";
	std::string m_center = "hyprland";
	std::string m_end = "volume,network,notification";
};
