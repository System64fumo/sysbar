#include "../config.hpp"
#include "hyprland.hpp"

#include <iostream>
#include <filesystem>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>

module_hyprland::module_hyprland(const bool &icon_on_start, const bool &clickable) : module(icon_on_start, clickable) {
	get_style_context()->add_class("module_hyprland");
	image_icon.hide();
	label_info.set_margin_end(size / 3);

	if (position %2 == 0) {
		std::thread socket_thread(&module_hyprland::socket_listener, this);
		socket_thread.detach();
	}
}

void module_hyprland::update_info() {
	if (data.find("activewindow>>") != std::string::npos) {
		std::string active_window_data = data.substr(14);
		int pos = active_window_data.find(',');

		std::string active_window = active_window_data.substr(0, pos);
		std::string active_window_title = active_window_data.substr(pos + 1);

		label_info.set_text(active_window_title);
	}
}

void module_hyprland::socket_listener() {
	std::filesystem::path xdg_runtime_dir = std::filesystem::path(getenv("XDG_RUNTIME_DIR"));
	std::filesystem::path hyprland_socket = xdg_runtime_dir / "hypr";

	if (!std::filesystem::exists(hyprland_socket)) {
		std::cout << "Socket not found at: " << hyprland_socket << std::endl;
		return;
	}

	std::string instance_signature = getenv("HYPRLAND_INSTANCE_SIGNATURE");

	if (instance_signature.empty()) {
		std::cout << "Hyprland instance signature not found, Is hyprland running?" << hyprland_socket << std::endl;
		return;
	}


	std::filesystem::path socket_path = hyprland_socket / instance_signature / ".socket2.sock";

	struct sockaddr_un addr;
	std::vector<char> buffer(1024);
	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		std::cout << "socket connection failed" << std::endl;
		close(sockfd);
		return;
	}

	std::string temp_buff;

	while (true) {
		ssize_t numBytes = read(sockfd, buffer.data(), buffer.size());
		if (numBytes < 0) {
			std::cerr << "Read error" << std::endl;
			close(sockfd);
			return;
		} else if (numBytes == 0) {
			std::cout << "Connection closed by peer" << std::endl;
			break;
		}

		temp_buff.append(buffer.data(), numBytes);
		size_t pos = 0;

		// On new line
		while ((pos = temp_buff.find('\n')) != std::string::npos) {
			data = temp_buff.substr(0, pos);
			temp_buff.erase(0, pos + 1);
			update_info();
			if (verbose)
				std::cout << data << std::endl;
		}
	}

	close(sockfd);
}
