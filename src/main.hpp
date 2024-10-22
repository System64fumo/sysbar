#pragma once
#include "config.hpp"
#include <map>
#include <string>

class sysbar {};
sysbar* win;

typedef sysbar* (*sysbar_create_func)(const std::map<std::string, std::map<std::string, std::string>> &cfg);
sysbar_create_func sysbar_create_ptr;
typedef void (*sysbar_handle_signal_func)(sysbar*, int);
sysbar_handle_signal_func sysbar_handle_signal_ptr;
