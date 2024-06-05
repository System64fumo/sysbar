#include "../module.hpp"

struct network_adapter {
	std::string interface;
	std::string type;
	std::string ipv4;
	uint index;
};

class module_network : public module {
	public:
		module_network(bool icon_on_start = false, bool clickable = false);
		~module_network();

	private:
		int nl_socket;
		char buffer[4096];
		uint default_if_index = 0;
		std::vector<network_adapter> adapters;
		Glib::Dispatcher dispatcher;

		bool setup_netlink();
		void interface_thread();
		void update_info();
		void process_message(struct nlmsghdr *nlh);
		void request_dump(int nl_socket, int type);
};
