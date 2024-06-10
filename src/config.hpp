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

// Build time configuration
#define RUNTIME_CONFIG								// Allow the use of runtime arguments
