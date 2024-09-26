#include "wireless_network.hpp"
#ifdef MODULE_NETWORK
#include <sstream>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <net/if.h>
#include <linux/nl80211.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

wireless_manager::wireless_manager() {
	state.socket = nl_socket_alloc();
	if (!state.socket) {
		std::fprintf(stderr, "Failed to allocate netlink socket\n");
		return;
	}

	if (genl_connect(state.socket)) {
		std::fprintf(stderr, "Failed to connect to generic netlink\n");
		nl_socket_free(state.socket);
		return;
	}

	state.nl80211_id = genl_ctrl_resolve(state.socket, "nl80211");
	if (state.nl80211_id < 0) {
		std::fprintf(stderr, "nl80211 not found\n");
		nl_socket_free(state.socket);
		return;
	}
}

wireless_manager::~wireless_manager() {
	if (state.socket) {
		nl_socket_free(state.socket);
	}
}

int wireless_manager::convert_signal_strength(const int& signal_strength_dbm) {
	const int min_dbm = -90;
	const int max_dbm = -30;

	if (signal_strength_dbm <= min_dbm)
		return 0;
	else if (signal_strength_dbm >= max_dbm)
		return 100;
	else
		return std::round((signal_strength_dbm - min_dbm) * 100.0 / (max_dbm - min_dbm));
}

int wireless_manager::nl_socket_modify_cb(struct nl_msg *msg, void *arg) {
	wireless_manager *manager = static_cast<wireless_manager*>(arg);
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	auto *gnlh = static_cast<genlmsghdr*>(nlmsg_data(nlmsg_hdr(msg)));

	nla_parse(tb, NL80211_ATTR_MAX, (struct nlattr*)genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), nullptr);

	if (!tb[NL80211_ATTR_BSS]) {
		std::fprintf(stderr, "No BSS information found\n");
		return NL_SKIP;
	}

	nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], nullptr);

	if (bss[NL80211_BSS_BSSID]) {
		auto ies = static_cast<char *>(nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]));
		auto ies_len = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
		int hdr_len = 2;

		while (ies_len > hdr_len) {
			uint8_t element_id = static_cast<uint8_t>(ies[0]);
			uint8_t element_len = static_cast<uint8_t>(ies[1]);

			if (ies_len < element_len + hdr_len)
				break;

			if (element_id == 0x00) {
				if (element_len > 0)
					manager->info.bssid.assign(ies + hdr_len, ies + hdr_len + element_len);
				else
					manager->info.bssid = "<hidden>";
				break;
			}
	
			ies_len -= element_len + hdr_len;
			ies += element_len + hdr_len;
		}
	}

	if (bss[NL80211_BSS_SIGNAL_MBM]) {
		manager->info.signal_dbm = (int)nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]) / 100;
		manager->info.signal_percentage = manager->convert_signal_strength(manager->info.signal_dbm);
	}

	if (bss[NL80211_BSS_FREQUENCY]) {
		double frequency = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);
		manager->info.frequency = frequency / 1000.0;
	}

	return NL_SKIP;
}

wireless_manager::wireless_info* wireless_manager::get_wireless_info(const std::string& interface_name) {
	auto *msg = nlmsg_alloc();
	genlmsg_put(msg, 0, 0, state.nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);

	int if_index = if_nametoindex(interface_name.c_str());
	if (if_index == 0) {
		std::fprintf(stderr, "Interface %s not found\n", interface_name.c_str());
		nlmsg_free(msg);
		return nullptr;
	}

	nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);
	
	nl_cb_set(nl_socket_get_cb(state.socket), NL_CB_VALID, NL_CB_CUSTOM, wireless_manager::nl_socket_modify_cb, this);

	if (nl_send_auto(state.socket, msg) < 0) {
		std::fprintf(stderr, "Failed to send message to nl80211\n");
		nlmsg_free(msg);
		return nullptr;
	}

	nl_recvmsgs_default(state.socket);
	nlmsg_free(msg);

	return &info;
}
#endif
