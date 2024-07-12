#pragma once
#include "config.hpp"
#include "window.hpp"

#include <gtkmm/application.h>

config config_main;
inline Glib::RefPtr<Gtk::Application> app;
inline int timeout = 1;
sysbar* win;

typedef sysbar* (*sysbar_create_func)(const config &cfg);
sysbar_create_func sysbar_create_ptr;
