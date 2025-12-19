# Features
Sysbar offers modules which could be displayed on the bar to provide real time information about various things.<br>
It also offers sidepanels which contain widgets that show additional information from the loaded modules.<br>

Sidebars can be accessed by either clicking or tapping and dragging on either end of the bar.<br>
(Touch gestures are supported)<br>

<details closed>
  <summary>List of supported features</summary>

* clock
  * Module: Show time and date
  * Widget: Clendar (Planned: Show events and holidays)

* weather
  * Module: Show weather
  * Widget: (Planned: Show more detailed weather info)

* tray
  * Module: Show running tray items

* hyprland
  * Module: Show window title (Planned: Workspace indicator)

* volume
  * Module: Show audio output volume level
  * Widget: Same as the module (Planned: Set volume level)

* network
  * Module: Show network type and status (Ethernet, Wireless, Cellular)
  * Control: (Planned: Show nearby wireless networks)

* battery
  * Module: Show battery level and status (Charging, Discharging)
  * Control: (Planned: Show additional power stuff, Set power plan)

* notification
  * Widget: Show notifications

* taskbar
  * Module: Show running toplevels

* backlight
  * Module: Show backlight level
  * Widget: Show and set backlight brightness levels

* menu
  * Module: Shows a simple button to open or close an app launcher

* mpris
  * Module: Show currently playing song
  * Widget: Same as the above but with controls and album art

* bluetooth
  * Module: Show bluetooth status (Connected, Disconnected)
  * Control: (Planned: Show nearby bluetooth devices)

* cellular
  * Module: Show cellular signal strength
  * Control: (Planned: Ability to connect/disconnect to/from cellular networks)
</details>

# Configuration
Sysbar offers compile time and runtime configuration options.<br>
Undesired features can be disabled by ommiting them from `src/config.hpp`<br>
The config system is INI based and can be configured by editing `~/.config/sys64/bar/config.conf`<br>

<details closed>
  <summary>Runtime file based config</summary>

| section              | default                     | description                                        |
|----------------------|-----------------------------|----------------------------------------------------|
| [main]               |                             | Primary configuration                              |
| position             | 0                           | 0 = top 1 = right 2 = bottom 3 = left              |
| size                 | 40                          | Height or width depending on position              |
| layer                | 2                           | Background = 0, Bottom = 1, Top = 2, Overlay = 3   |
| autohide             | true                        | Hides the bar automatically when going fullscreen  |
| exclusive            | true                        | Exclude part of the screen for the bar             |
| verbose              | false                       | Verbose output (For debugging)                     |
| main-monitor         | HDMI-A-1                    | Monitor output name (DP-1, HDMI-A-1, ect..)        |
|&nbsp;                |                             |                                                    |
| [modules]            |                             |                                                    |
| start                | clock,weather,tray          | Modules shown at the start of the bar (Left/Top)   |
| center               | hyprland                    | Modules shown in the middle of the bar             |
| end                  | volume,network,notification | Modules shown at the end of the bar (Right/Bottom) |
| icon-size            | 16                          | Icon size of modules                               |
|&nbsp;                |                             |                                                    |
| [sidepanels]         |                             |                                                    |
| start-size           | 350                         | Width or Height of the Left/Top sidepanel          |
| start-label          |                             | Default/Main page header text (Empty to hide)      |
| end-size             | 350                         | Width or Height of the Right/Bottom sidepanel      |
| end-label            | Quick settings              | Default/Main page header text (Empty to hide)      |
|&nbsp;                |                             |                                                    |
| [controls]           |                             |                                                    |
| columns              | 2                           | Column count in the controls box                   |
|&nbsp;                |                             |                                                    |
| [clock]              |                             | Clock module configuration                         |
| interval             | 1000                        | How long (in ms) to refresh the time               |
| label-format         | %H:%M                       | [Label format](https://www.man7.org/linux/man-pages/man1/date.1.html) |
| tooltip-format       | %Y/%m/%d                    | Same as the above but for the tooltip              |
| widget-layout        | 0044                        | [XYWH (Single digit values to position the widget)](https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Grid.html#a2f3d5ceb9a1c2f491b541aa56ebdc1e8)  |
|&nbsp;                |                             |                                                    |
| [weather]            |                             | Weather module configuration                       |
| url                  | https://wttr.in/?format=j1  | [wttr.in](https://github.com/chubin/wttr.in) Cool project, Consider supporting the dev out |
| unit                 | f                           | Temperature unit **C**elsius or **F**ahrenheit     |
|&nbsp;                |                             |                                                    |
| [hyprland]           |                             | Hyprland module configuration                      |
| character-limit      | 128                         | Label character limit so the text won't get funky  |
|&nbsp;                |                             |                                                    |
| [volume]             |                             | Volume module configuration                        |
| show-label           | false                       | Show the volume level as text                      |
| widget-layout        | 0441                        | [XYWH (Single digit values to position the widget)](https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Grid.html#a2f3d5ceb9a1c2f491b541aa56ebdc1e8)  |
|&nbsp;                |                             |                                                    |
| [network]            |                             | Network module configuration                       |
| show-label           | false                       | Show signal strength as text                       |
|&nbsp;                |                             |                                                    |
| [battery]            |                             | Battery module configuration                       |
| show-label           | false                       | Show charge level as text                          |
|&nbsp;                |                             |                                                    |
| [bluetooth]          |                             | Bluetooth module configuration                     |
| show-icon            | false                       | Show the icon                                      |
|&nbsp;                |                             |                                                    |
| [notification]       |                             | Notification widget configuration                  |
| command              | ffplay /usr/share/..        | Command to run whenever you recieve a notification |
| show-control         | true                        | Show Do Not Disturb control                        |
|&nbsp;                |                             |                                                    |
| [backlight]          |                             | Backlight module configuration                     |
| path                 |                             | Path to backlight (/sys/class/backlight/panel)     |
| show-icon            | true                        | Show brightness level as an icon                   |
| show-label           | true                        | Show brightness level as text                      |
| widget-layout        | 0341                        | [XYWH (Single digit values to position the widget)](https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Grid.html#a2f3d5ceb9a1c2f491b541aa56ebdc1e8)  |
|&nbsp;                |                             |                                                    |
| [menu]               |                             | Menu module configuration                          |
| show-icon            | true                        | Show the icon                                      |
| show-label           | false                       | Show the label                                     |
| icon-name            | start-here                  | Icon name from your GTK icon theme                 |
| label-text           | Applications                | Text to show on the menu button                    |
|&nbsp;                |                             |                                                    |
| [taskbar]            |                             | Taskbar module configuration                       |
| text-length          | 14                          | Window title length                                |
| icon-size            | 32                          | Size of the icons                                  |
|&nbsp;                |                             |                                                    |
| [mpris]              |                             | Mpris module configuration                         |
| show-icon            | true                        | Show player status as an icon                      |
| show-label           | true                        | Show album name as text                            |
| album-rounding       | 10                          | Album art rounding                                 |
| album-size           | 96                          | Size of album art cover                            |
| widget-layout        | 0142                        | [XYWH (Single digit values to position the widget)](https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Grid.html#a2f3d5ceb9a1c2f491b541aa56ebdc1e8)  |
|&nbsp;                |                             |                                                    |
| [cellular]           |                             | Cellular module configuration                      |
| show-icon            | true                        | Show signal strength as an icon                    |
| show-label           | false                       | Show signal strength as text                       |
</details>

<details closed>
  <summary>Runtime launch arguments</summary>

```
  -p	Set position
  -s	Set start modules (modules on the left side)
  -c	Set center modules (modules in the middle)
  -e	Set end modules (modules on the right side)
  -S	Set bar size (Height or Width depending on position)
  -V	Be more verbose
  -v	Prints version info
```
</details>

# Styling
By default sysbar will follow your GTK4 theme.<br>
However it can also load Custom Style Sheets from `~/.config/sys64/bar/style.css`
