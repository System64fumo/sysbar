#include "hyprland.hpp"

#include <filesystem>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <algorithm>

module_hyprland::module_hyprland(sysbar* window, const bool& icon_on_start) : module(window, icon_on_start), new_workspace(0), character_limit(128) {
	get_style_context()->add_class("module_hyprland");
	image_icon.hide();
	label_info.set_margin_end(win->size / 3);

	window_active = nullptr;
	monitor_active = nullptr;

	std::string cfg_char_limit = win->config_main["hyprland"]["character-limit"];
	if (!cfg_char_limit.empty())
		character_limit = std::stoi(cfg_char_limit);


	if (win->position %2 == 0) {
		dispatcher.connect(sigc::mem_fun(*this, &module_hyprland::update_info));
		std::thread socket_thread(&module_hyprland::socket_listener, this);
		socket_thread.detach();
	}
}

void module_hyprland::update_info() {
	std::lock_guard<std::mutex> lock(mutex);
	std::string data = data_queue.front();
	data_queue.pop();

	//std::printf("Data: %s\n", data.c_str());

	// Amazing if else chain of states
	if (data.find("openwindow>>") != std::string::npos ||
		data.find("closewindow>>") != std::string::npos) {
		uint8_t start = data.find(">>") + 2;
		uint8_t end = data.find(",", start);
		std::string window_id = data.substr(start, end - start);

		// TODO: Handle process sleep states
		// If sysbar or sysshell go to sleep we may lose track of windows
		// Maybe add a garbage collector/resetter thing?
		if (data.find("openwindow>>") != std::string::npos) {
			windows[window_id] = window {
				window_id
			};
			window_active = &windows[window_id];
		}
		else if (data.find("closewindow>>") != std::string::npos) {
			windows.erase(window_id);
		}
	}

	else if (data.find("activewindowv2>>") != std::string::npos) {
		// TODO: Verify existance of window otherwise return nullptr
		std::string window_id = data.substr(16, 12);
		if (window_id == "")
			window_active = nullptr;
		else 
			window_active = &windows[window_id];

		update_fullscreen_status();
	}

	else if (data.find("fullscreen>>") != std::string::npos) {
		bool fullscreen_state = data.substr(12, 1) == "1";
		if (window_active == nullptr)
			return;

		window_active->fullscreen = fullscreen_state;
		update_fullscreen_status();
	}

	// TODO: Get current active window and monitor on startup
	else if (data.find("activewindow>>") != std::string::npos) {
		std::string active_window_data = data.substr(14);
		int pos = active_window_data.find(',');

		std::string active_window = active_window_data.substr(0, pos);
		Glib::ustring active_window_title = active_window_data.substr(pos + 1);

		if ((int)active_window_title.size() > character_limit)
			active_window_title = active_window_title.substr(0, character_limit + 3) + "...";

		label_info.set_text(active_window_title);
	}

	else if (data.find("focusedmon>>") != std::string::npos) {
		uint8_t start = data.find(">>") + 2;
		uint8_t end = data.find(",", start);
		std::string monitor_connector = data.substr(start, end - start);
		if (!monitors.count(monitor_connector)) {
			monitors[monitor_connector] = {
				monitor_connector
			};
		}

		monitor_active = &monitors[monitor_connector];
	}
}

void module_hyprland::socket_listener() {
	std::filesystem::path xdg_runtime_dir = std::filesystem::path(getenv("XDG_RUNTIME_DIR"));
	std::filesystem::path hyprland_socket = xdg_runtime_dir / "hypr";

	if (!std::filesystem::exists(hyprland_socket)) {
		std::fprintf(stderr, "Socket not found at: %s\n", hyprland_socket.c_str());
		return;
	}

	const char* is = getenv("HYPRLAND_INSTANCE_SIGNATURE");

	if (is == nullptr) {
		std::fprintf(stderr, "Hyprland instance signature not found, Is hyprland running?\n");
		return;
	}

	std::string instance_signature = is;

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

void module_hyprland::update_fullscreen_status() {
	if (win->config_main["main"]["autohide"] != "true")
		return;

	if (monitor_active == nullptr)
		return;

	if (monitor_active->connector != win->config_main["main"]["main-monitor"])
		return;
	
	bool window_active_state;
	if (window_active != nullptr)
		window_active_state = window_active->fullscreen;

	if (window_active_state && win->visible) {
		win->handle_signal(12);
	}
	else if (!window_active_state && !(win->visible)) {
		win->handle_signal(10);
	}
}