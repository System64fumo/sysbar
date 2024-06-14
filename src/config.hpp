#pragma once
#include <string>

/*
	Default config.
	Can be configured instead of using launch arguments.
*/

// Current											Default
inline int position = 0;							// 0
inline int size = 40;								// 40
inline bool verbose = false;						// false
inline std::string m_start = "clock,weather,tray";	// "clock,weather,tray"
inline std::string m_center = "";					// ""
inline std::string m_end = "volume,network";		// "volume,network"

// Build time configuration		Description
#define RUNTIME_CONFIG			// Allow the use of runtime arguments
#define MODULE_CLOCK			// Include the clock module
#define MODULE_WEATHER			// Include the weather module
#define MODULE_TRAY				// Include the tray module
#define MODULE_VOLUME			// Include the volume module
#define MODULE_NETWORK			// Include the network module
