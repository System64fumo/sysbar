#pragma once
#include "config.hpp"

config_bar config_main;
class sysbar {};
sysbar* win;

typedef sysbar* (*sysbar_create_func)(const config_bar &cfg);
sysbar_create_func sysbar_create_ptr;
typedef void (*sysbar_handle_signal_func)(sysbar*, int);
sysbar_handle_signal_func sysbar_handle_signal_ptr;
