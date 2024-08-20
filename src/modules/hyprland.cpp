#include "../config_parser.hpp"
#include "hyprland.hpp"

#include <filesystem>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>

module_hyprland::module_hyprland(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_hyprland");
	image_icon.hide();
	label_info.set_margin_end(config_main.size / 3);

	#ifdef CONFIG_FILE
	config_parser config(std::string(getenv("HOME")) + "/.config/sys64/bar/config.conf");

	if (config.available) {
		std::string cfg_char_limit = config.get_value("hyprland", "character-limit");
		if (cfg_char_limit != "empty")
			character_limit = std::stoi(cfg_char_limit);
	}
	#endif

	if (config_main.position %2 == 0) {
		dispatcher.connect(sigc::mem_fun(*this, &module_hyprland::update_info));
		std::thread socket_thread(&module_hyprland::socket_listener, this);
		socket_thread.detach();
	}
}

void module_hyprland::update_info() {
	std::lock_guard<std::mutex> lock(mutex);
	std::string data = data_queue.front();
	data_queue.pop();

	if (data.find("activewindow>>") != std::string::npos) {
		std::string active_window_data = data.substr(14);
		int pos = active_window_data.find(',');

		std::string active_window = active_window_data.substr(0, pos);
		Glib::ustring active_window_title = active_window_data.substr(pos + 1);

		if ((int)active_window_title.size() > character_limit)
			active_window_title = active_window_title.substr(0, character_limit + 3) + "...";

		label_info.set_text(active_window_title);
	}
}

void module_hyprland::socket_listener() {
	std::filesystem::path xdg_runtime_dir = std::filesystem::path(getenv("XDG_RUNTIME_DIR"));
	std::filesystem::path hyprland_socket = xdg_runtime_dir / "hypr";

	if (!std::filesystem::exists(hyprland_socket)) {
		std::fprintf(stderr, "Socket not found at: %s\n", hyprland_socket.c_str());
		return;
	}

	std::string instance_signature = getenv("HYPRLAND_INSTANCE_SIGNATURE");

	if (instance_signature.empty()) {
		std::fprintf(stderr, "Hyprland instance signature not found, Is hyprland running?\n");
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
		std::fprintf(stderr, "socket connection failed\n");
		close(sockfd);
		return;
	}

	std::string temp_buff;

	while (true) {
		ssize_t numBytes = read(sockfd, buffer.data(), buffer.size());
		if (numBytes < 0) {
			std::fprintf(stderr, "Read error\n");
			close(sockfd);
			return;
		} else if (numBytes == 0) {
			std::fprintf(stderr, "Connection closed by peer\n");
			break;
		}

		temp_buff.append(buffer.data(), numBytes);
		size_t pos = 0;

		// On new line
		while ((pos = temp_buff.find('\n')) != std::string::npos) {
			std::lock_guard<std::mutex> lock(mutex);
			data_queue.push(temp_buff.substr(0, pos));
			temp_buff.erase(0, pos + 1);

			dispatcher.emit();
		}
	}

	close(sockfd);
}
