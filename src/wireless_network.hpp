#pragma once
#include <string>
#include <sstream>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>
#include <net/if.h>

class wireless_manager {
	public:
		wireless_manager();
		~wireless_manager();
		
		int init_nl80211();
		int get_wireless_info(const std::string& interface_name);
	
		struct wireless_info {
			std::string bssid;
			int signal_dbm;
			int signal_percentage;
			double frequency;
		};

		const wireless_info& get_info() const;

	private:
		struct nl80211_state {
			struct nl_sock *socket;
			int nl80211_id;
		};
	
		int convert_signal_strength(int signal_strength_dbm);
		static int nl_socket_modify_cb(struct nl_msg *msg, void *arg);
		
		nl80211_state state;
		wireless_info info;
};
