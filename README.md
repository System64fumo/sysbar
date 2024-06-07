# Sysbar
Sysbar is a modular status bar for wayland written in gtkmm4<br>

> [!NOTE]
> sysbar is work in progress and thus is missing a lot of features.<br>
> At the moment there is no way to customize the layout besides editing the code.<br>
> Audio stuff is provided by wireplumber only for now<br>

# Modules
* clock
* weather (WIP)
* volume
* network (WIP)

# Configuration
sysbar can be configured in 2 ways<br>
1: By changing config.hpp and recompiling (Suckless style)<br>
2: Using launch arguments<br>
```
arguments:
  -p	Set position
  -V	Be more verbose
  -v	Prints version info
  -s	Set bar size (Height or Width depending on position)
```

# Theming
sysbar uses your gtk4 theme by default, However it can be also load custom css,<br>
Just copy the included menu.css file to ~/.config/sys64/bar.css<br>

# Credits
[wttr.in](https://github.com/chubin/wttr.in) for their weather service<br>
[waybar](https://github.com/Alexays/Waybar) for showing how to write wireplumber stuff<br>

# Also check out
[waybar](https://github.com/Alexays/Waybar)<br>
[wf-shell](https://github.com/WayfireWM/wf-shell)<br>
