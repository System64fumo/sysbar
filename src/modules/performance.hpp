#pragma once
#include "../module.hpp"
#ifdef MODULE_PERFORMANCE

struct cpu_stats {
	unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
};

class module_performance : public module {
	public:
		module_performance(sysbar *window, const bool &icon_on_start = true);

	private:
		int precision = 0;
		int interval = 1000;
		cpu_stats prev_stats;
		bool update_info();
		cpu_stats get_cpu_stats();
		double calculate_cpu_load(const cpu_stats &prev, const cpu_stats &curr);
};

#endif
