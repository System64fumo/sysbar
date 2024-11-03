#pragma once
#include "../module.hpp"
#ifdef MODULE_NETWORK
#include "../wireless_network.hpp"
#include "controls.hpp"

#include <glibmm/dispatcher.h>

struct network_adapter {
	std::string interface;
	std::string type;
	std::string ipv4;
	uint index;
};

class module_network : public module {
	public:
		module_network(sysbar*, const bool&);
		~module_network();

	private:
		Glib::Dispatcher dispatcher;
		#ifdef FEATURE_WIRELESS
		wireless_manager manager;
		#endif

		#ifdef MODULE_CONTROLS
		control* control_network;
		void setup_control();
		#endif

		int nl_socket;
		char buffer[4096];
		uint default_if_index;
		std::vector<network_adapter> adapters;

		bool setup_netlink();
		void interface_thread();
		void update_info();
		void process_message(struct nlmsghdr*);
		void request_dump(const int&);
};

#endif
