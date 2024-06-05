#include "../module.hpp"

struct network_adapter {
	std::string interface;
	std::string type;
	std::string ipv4;
};

class module_network : public module {
	public:
		module_network(bool icon_on_start = false, bool clickable = false);
		~module_network();

	private:
		int nl_socket;
		char buffer[4096];
		unsigned int default_if = 0;
		std::vector<network_adapter> adapters;

		bool setup_netlink();
		void interface_thread();
		void update_info();
		void process_message(struct nlmsghdr *nlh);
		void request_dump(int nl_socket, int type);
};
