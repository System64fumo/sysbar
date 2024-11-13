#include "network.hpp"

#include <net/if.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <thread>
#include <filesystem>
#include <fstream>

module_network::module_network(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start), default_if_index(0) {
	get_style_context()->add_class("module_network");

	if (win->config_main["network"]["show-label"] != "true")
		label_info.hide();

	// Set up networking stuff
	if (!setup_netlink())
		return;

	dispatcher.connect(sigc::mem_fun(*this, &module_network::update_info));
	std::thread thread_network(&module_network::interface_thread, this);
	thread_network.detach();

	Glib::signal_timeout().connect([&]() {
		update_info();
		return true;
	}, 10000);

	#ifdef MODULE_CONTROLS
	setup_control();
	#endif
}

module_network::~module_network() {
	close(nl_socket);
}

void module_network::interface_thread() {
	while (true) {
		int len = recv(nl_socket, buffer, sizeof(buffer), 0);
		if (len < 0) {
			std::fprintf(stderr, "Error receiving netlink message");
			return;
		}
	
		for (struct nlmsghdr *nlh = (struct nlmsghdr *)buffer; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
			if (nlh->nlmsg_type == NLMSG_DONE) {
				break;
			}
			if (nlh->nlmsg_type == RTM_NEWADDR || nlh->nlmsg_type == RTM_DELADDR || nlh->nlmsg_type == RTM_GETADDR) {
				process_message(nlh);
			}
		}
		dispatcher.emit();
	}
}

void module_network::update_info() {
	// Get primary interface
	uint if_index = default_if_index;
	auto default_if = std::find_if(adapters.begin(), adapters.end(), [if_index](const network_adapter& a) { return a.index == if_index; });
	if (default_if == adapters.end()) {
		std::fprintf(stderr, "No interface found\n");
		image_icon.set_from_icon_name("network-error-symbolic");
		return;
	}

	if (win->verbose)
		std::printf("Default interface is %s\n", default_if->interface.c_str());

	if (default_if->type == "Ethernet") {
		image_icon.set_from_icon_name("network-wired-symbolic");
		label_info.set_text("");
	}
	#ifdef FEATURE_WIRELESS
	else if (default_if->type == "Wireless") {
		auto info = manager.get_wireless_info(default_if->interface);
		label_info.set_text(std::to_string(info->signal_percentage));

		std::string icon;
		if (info->signal_percentage > 80)
			icon = "network-wireless-signal-excellent-symbolic";
		else if (info->signal_percentage > 60)
			icon = "network-wireless-signal-good-symbolic";
		else if (info->signal_percentage > 40)
			icon = "network-wireless-signal-ok-symbolic";
		else if (info->signal_percentage > 20)
			icon = "network-wireless-signal-weak-symbolic";
		else
			icon = "network-wireless-signal-none-symbolic";

		image_icon.set_from_icon_name(icon);
	}
	else if (default_if->type == "Cellular")
		image_icon.set_from_icon_name("network-cellular-connected-symbolic");
	#endif
	else
		image_icon.set_from_icon_name("network-error-symbolic");

	set_tooltip_text("Adapter: " + default_if->interface + "\nAddress: " + default_if->ipv4);
}

bool module_network::setup_netlink() {
	nl_socket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (nl_socket < 0) {
		std::fprintf(stderr, "Failed to open netlink socket\n");
		return false;
	}

	struct sockaddr_nl sa;
	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE;

	if (bind(nl_socket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		std::fprintf(stderr, "Failed to bind netlink socket\n");
		close(nl_socket);
		return false;
	}

	// Request current addresses
	request_dump(RTM_GETADDR);
	return true;
}

void module_network::request_dump(const int& type) {
	struct nlmsghdr nlh;
	struct rtgenmsg rtg;

	memset(&nlh, 0, sizeof(nlh));
	nlh.nlmsg_len = NLMSG_LENGTH(sizeof(rtg));
	nlh.nlmsg_type = type;
	nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh.nlmsg_seq = 1;
	nlh.nlmsg_pid = getpid();

	memset(&rtg, 0, sizeof(rtg));
	rtg.rtgen_family = AF_INET;

	struct iovec iov = { &nlh, nlh.nlmsg_len };
	struct sockaddr_nl sa = { .nl_family = AF_NETLINK };

	struct msghdr msg = {
		.msg_name = &sa,
		.msg_namelen = sizeof(sa),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};

	sendmsg(nl_socket, &msg, 0);
}

void module_network::process_message(struct nlmsghdr* nlh) {
	struct ifaddrmsg* ifa = (struct ifaddrmsg*)NLMSG_DATA(nlh);
	struct rtattr* rth = IFA_RTA(ifa);
	int rtl = IFA_PAYLOAD(nlh);
	char if_name[IF_NAMESIZE];
	char if_addr[INET_ADDRSTRLEN];
	std::string if_type;
	unsigned int if_index = 0;

	for (; RTA_OK(rth, rtl); rth = RTA_NEXT(rth, rtl)) {
		// Get interface type
		if (if_indextoname(ifa->ifa_index, if_name) != nullptr) {
			std::string interface_path = "/sys/class/net/";
			std::ifstream file(interface_path + if_name + "/type");
			bool wireless = std::filesystem::exists(interface_path + if_name + "/wireless");
			std::ostringstream type_buffer;
			type_buffer << file.rdbuf();
			std::string type = type_buffer.str();
			type.pop_back();

			// TODO: Add more types
			// Hotspot and VPN types are left
			if (wireless && (type == "1"))
				if_type = "Wireless";
			else if (type == "519")
				if_type = "Cellular";
			else
				if_type = "Ethernet";
		}

		// Get interface index
		if_index = if_nametoindex(if_name);

		// Skip anything we don't need
		if (rth->rta_type == RTA_OIF)
			default_if_index = if_index;
		else if (rth->rta_type != IFA_LOCAL)
			continue;
		else if (strcmp(if_name, "lo") == 0)
			continue;

		// Has an interface been updated?
		if (nlh->nlmsg_type == RTM_NEWADDR)
			inet_ntop(AF_INET, RTA_DATA(rth), if_addr, sizeof(if_addr));
		else
			strcpy(if_addr, "null");

		// Add the interface to the list
		if (rth->rta_type == IFA_LOCAL) {

			/// Find the interface if it already exists
			auto it = std::find_if(adapters.begin(), adapters.end(), [if_index](const network_adapter& s) {
				return s.index == if_index;
			});
			if (it != adapters.end()) {
				it->ipv4 = if_addr;
				continue;
			}

			// Create a new interface if needed
			if (win->verbose)
				std::printf("%s has been added to the list\n", if_name);

			network_adapter adapter;
			adapter.interface = if_name;
			adapter.type = if_type;
			adapter.ipv4 = if_addr;
			adapter.index = if_index;
			adapters.push_back(adapter);
		}
	}
}

#ifdef MODULE_CONTROLS
void module_network::setup_control() {
	if (!win->box_controls)
		return;

	auto container = static_cast<module_controls*>(win->box_controls);
	control_network = Gtk::make_managed<control>("network-wireless-symbolic", true);
	container->flowbox_controls.append(*control_network);
}
#endif
