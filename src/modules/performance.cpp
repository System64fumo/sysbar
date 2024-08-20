#include "../config_parser.hpp"
#include "performance.hpp"

#include <fstream>
#include <iomanip>

module_performance::module_performance(sysbar *window, const bool &icon_on_start) : module(window, icon_on_start) {
	get_style_context()->add_class("module_performance");
	image_icon.set_from_icon_name("cpu-symbolic");

	// TODO: Add config support
	// TODO: Add more metrics (Ram, Disk, Temp, Ect..)

	prev_stats = get_cpu_stats();
	label_info.set_text("0");
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &module_performance::update_info), interval);
}

bool module_performance::update_info() {
	cpu_stats curr_stats = get_cpu_stats();
	double cpu_load = calculate_cpu_load(prev_stats, curr_stats);
	prev_stats = curr_stats;

	std::ostringstream out;
	out << std::fixed << std::setprecision(precision) << cpu_load;
	label_info.set_text(out.str());
	return true;
}

cpu_stats module_performance::get_cpu_stats() {
	std::ifstream file("/proc/stat");
	std::string line;
	cpu_stats stats = {};

	if (file.is_open()) {
		std::getline(file, line);
		std::sscanf(line.c_str(), "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
					&stats.user, &stats.nice, &stats.system, &stats.idle,
					&stats.iowait, &stats.irq, &stats.softirq, &stats.steal);
		file.close();
	}

	return stats;
}

double module_performance::calculate_cpu_load(const cpu_stats &prev, const cpu_stats &curr) {
	unsigned long long prev_idle = prev.idle + prev.iowait;
	unsigned long long curr_idle = curr.idle + curr.iowait;

	unsigned long long prev_non_idle = prev.user + prev.nice + prev.system + prev.irq + prev.softirq + prev.steal;
	unsigned long long curr_non_idle = curr.user + curr.nice + curr.system + curr.irq + curr.softirq + curr.steal;

	unsigned long long prev_total = prev_idle + prev_non_idle;
	unsigned long long curr_total = curr_idle + curr_non_idle;

	unsigned long long totald = curr_total - prev_total;
	unsigned long long idled = curr_idle - prev_idle;

	double cpu_load = (totald - idled) / static_cast<double>(totald);
	return cpu_load * 100.0;
}
