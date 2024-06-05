#include "network.hpp"

#include <iostream>
#include <net/if.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <filesystem>

module_network::module_network(bool icon_on_start, bool clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_network");
	image_icon.set_from_icon_name("network-wired-symbolic");
	label_info.hide();

	// Set up networking stuff
	if (!setup_netlink())
		return;

	std::thread thread_network(&module_network::interface_thread, this);
	thread_network.detach();
}

module_network::~module_network() {
	close(nl_socket);
}

void module_network::interface_thread() {
	while (true) {
		int len = recv(nl_socket, buffer, sizeof(buffer), 0);
		if (len < 0) {
			std::cerr << "Error receiving netlink message" << std::endl;
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
		// TODO: Trigger a dispatcher to update UI
	}
}

void module_network::update_info() {
	// TODO: Figure out what the network type of the default adapter is
	// and show the appropiate icon
}

bool module_network::setup_netlink() {
	nl_socket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (nl_socket < 0) {
		std::cerr << "Failed to open netlink socket" << std::endl;
		return false;
	}

	struct sockaddr_nl sa;
	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE;

	if (bind(nl_socket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		std::cerr << "Failed to bind netlink socket" << std::endl;
		close(nl_socket);
		return false;
	}

	// Request current addresses
	request_dump(nl_socket, RTM_GETADDR);
	return true;
}

void module_network::request_dump(int nl_socket, int type) {
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

void module_network::process_message(struct nlmsghdr *nlh) {
	struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(nlh);
	struct rtattr *rth = IFA_RTA(ifa);
	int rtl = IFA_PAYLOAD(nlh);
	char if_name[IF_NAMESIZE];
	char if_addr[INET_ADDRSTRLEN];
	std::string if_type;
	unsigned int if_index = 0;

	for (; RTA_OK(rth, rtl); rth = RTA_NEXT(rth, rtl)) {

		// Get interface type
		if (if_indextoname(ifa->ifa_index, if_name) != nullptr) {
			std::string interface_path = "/sys/class/net/";
			interface_path.append(if_name);
			interface_path.append("/wireless");
			bool wireless = std::filesystem::exists(interface_path);

			// TODO: Add more types
			if (wireless)
				if_type = "Wireless";
			else
				if_type = "Ethernet";
		}

		// Get interface index
		if_index = if_nametoindex(if_name);

		// Skip anything we don't need
		if (rth->rta_type == RTA_OIF)
			default_if = if_index;
		else if (rth->rta_type != IFA_LOCAL)
			continue;
		else if (strcmp(if_name, "lo") == 0)
			continue;

		// TODO: Check if an interface already exists and update it accordingly
		// Alternatively, Delete it and re add it (Sounds easier but is very stupid)

		// Has an interface been updated?
		if (nlh->nlmsg_type == RTM_NEWADDR)
			inet_ntop(AF_INET, RTA_DATA(rth), if_addr, sizeof(if_addr));
		else
			strcpy(if_addr, "null");

		// Add the interface to the list
		if (rth->rta_type == IFA_LOCAL) {
			std::cout << "Interface: " << if_name << std::endl;
			std::cout << "Type: " << if_type << std::endl;
			std::cout << "Address: " << if_addr << std::endl;
			std::cout << "Index: " << if_index << "\n" << std::endl;
			network_adapter adapter;
			adapter.interface = if_name;
			adapter.type = if_type;
			adapter.ipv4 = if_addr;
			adapters.push_back(adapter);
		}
	}
}
