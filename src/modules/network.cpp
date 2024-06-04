#include "network.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <string>
#include <thread>

int nl_socket;
char buffer[4096];

void process_interface_type(const std::string& if_name) {
	std::string type_path = "/sys/class/net/" + if_name + "/type";
	std::ifstream type_file(type_path);
	int if_type = 0;
	if (type_file.is_open()) {
		type_file >> if_type;
		type_file.close();

		switch (if_type) {
			case 1:
				std::cout << "Type: Ethernet" << std::endl;
				break;
			case 801:
				std::cout << "Type: Wireless" << std::endl;
				break;
			case 769:
				std::cout << "Type: WWAN" << std::endl;
				break;
			case 772:
				std::cout << "Type: Loopback" << std::endl;
				break;
			default:
				std::cout << "Type: Other (type " << if_type << ")" << std::endl;
		}
	} else {
		std::cerr << "Failed to read interface type for " << if_name << std::endl;
	}
}

void process_message(struct nlmsghdr *nlh) {
	struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(nlh);
	struct rtattr *rth = IFA_RTA(ifa);
	int rtl = IFA_PAYLOAD(nlh);

	for (; RTA_OK(rth, rtl); rth = RTA_NEXT(rth, rtl)) {
		if (rth->rta_type == IFA_LOCAL) {
			char if_name[IF_NAMESIZE];
			if (if_indextoname(ifa->ifa_index, if_name) != nullptr) {
			std::cout << "Interface: " << if_name << std::endl;
			process_interface_type(if_name);
			} else {
				std::cerr << "Failed to get interface name for index " << ifa->ifa_index << std::endl;
			}
			if (nlh->nlmsg_type != RTM_DELADDR) {
				char ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, RTA_DATA(rth), ip, sizeof(ip));
				std::cout << "Address: " << ip << "\n" << std::endl;
			}
			else {
				std::cout << "Address deleted" << std::endl;
			}
		}
	}
}

void request_dump(int nl_socket, int type) {
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

bool setup() {
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

module_network::module_network(bool icon_on_start, bool clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_network");
	image_icon.set_from_icon_name("network-wired-symbolic");
	label_info.hide();

	// Set up networking stuff
	if (!setup())
		return;

	std::thread thread_network(&module_network::update_info, this);
	thread_network.detach();
}

module_network::~module_network() {
	close(nl_socket);
}

bool module_network::update_info() {
	int len = recv(nl_socket, buffer, sizeof(buffer), 0);
	if (len < 0) {
		std::cerr << "Error receiving netlink message" << std::endl;
		return false;
	}

	for (struct nlmsghdr *nlh = (struct nlmsghdr *)buffer; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
		if (nlh->nlmsg_type == NLMSG_DONE) {
			break;
		}
		if (nlh->nlmsg_type == RTM_NEWADDR || nlh->nlmsg_type == RTM_DELADDR || nlh->nlmsg_type == RTM_GETADDR) {
			process_message(nlh);
		}
	}
	return true;
}
