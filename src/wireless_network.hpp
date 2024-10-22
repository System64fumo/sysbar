#pragma once
#include "config.hpp"
#ifdef MODULE_NETWORK

#include <string>

class wireless_manager {
	public:
		wireless_manager();
		~wireless_manager();

		struct wireless_info {
			std::string bssid;
			int signal_dbm;
			int signal_percentage;
			double frequency;
		};

		wireless_info* get_wireless_info(const std::string&);

	private:
		struct nl80211_state {
			struct nl_sock *socket;
			int nl80211_id;
		};

		nl80211_state state;
		wireless_info info;

		int convert_signal_strength(const int&);
		static int nl_socket_modify_cb(struct nl_msg*, void*);
};
#endif
