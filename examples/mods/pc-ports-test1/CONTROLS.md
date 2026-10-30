# Controls and status

Hold **L + R**, tap the mod's menu button, then release the shortcut. D-Pad
navigates, A selects and B closes. The right-side badges show whether each mod
is enabled, waiting for a requirement or actively running a mission. Turn off
an individual badge in its menu; holding L + R still reveals its shortcut.

If another mod uses the same shortcut, create
`switch/gtav-scripthook/settings/<plugin-id>/controls.ini`:

```ini
[controls]
menu_button=0
show_status=true
```

`menu_button` selects: **0 Left, 1 Up, 2 Right, 3 ZL, 4 ZR, 5 X, 6 A, 7 B,
8 left-stick click, 9 right-stick click**. Choose different buttons for mods
you run together. Missing or invalid values keep that mod's default. Read once
when the plugin starts; Kill Process and Load Now applies file changes.

The saved in-menu badge preference takes priority over `show_status`. No
configuration file is required, and install ZIPs do not replace saved settings.
