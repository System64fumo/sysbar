# Sysbar
Sysbar is a modular status bar for wayland written in gtkmm4<br>
![preview](https://github.com/System64fumo/sysbar/blob/main/preview.jpg "preview")

> [!NOTE]
> sysbar is work in progress and thus is missing a lot of features.<br>
> Audio stuff is provided by wireplumber only for now<br>

# Modules
* clock
* weather
* tray
* hyprland
* volume
* network
* battery (WIP)
* notification (WIP)
* taskbar (WIP)
* backlight (WIP)

# Configuration
sysbar can be configured in 3 ways<br>
1: By changing config.hpp and recompiling (Suckless style)<br>
2: Using a config file (~/.config/sys64/bar/config.conf)<br>
3: Using launch arguments<br>
```
arguments:
  -p	Set position
  -s	Set start modules (modules on the left side)
  -c	Set center modules (modules in the middle)
  -e	Set end modules (modules on the right side)
  -S	Set bar size (Height or Width depending on position)
  -V	Be more verbose
  -v	Prints version info
```

# config.hpp
sysbar offers some features you can enable/disable via config.hpp<br>
By default all features are enabled, However if you wish to disable something,<br>
You can do so by deleting the line that contains `#define FEATURE_NAME`<br>

# Signals
``pkill -10 sysbar`` to show.<br>
``pkill -12 sysbar`` to hide.<br>
``pkill -34 sysbar`` to toggle.<br>

# Theming
sysbar uses your gtk4 theme by default, However it can be also load custom css,<br>
Just copy the included style.css file to ~/.config/sys64/bar/style.css<br>

# Credits
[wttr.in](https://github.com/chubin/wttr.in) for their weather service<br>
[waybar](https://github.com/Alexays/Waybar) for showing how to write wireplumber stuff<br>
[wf-shell](https://github.com/WayfireWM/wf-shell) for showing how to do system tray stuff<br>

# Also check out
[waybar](https://github.com/Alexays/Waybar)<br>
[wf-shell](https://github.com/WayfireWM/wf-shell)<br>
