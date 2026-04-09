# Magic Lamp Everywhere

A KWin effect that plays the **Magic Lamp / Genie animation when apps open** — not just when minimizing.

Based on KWin's built-in Magic Lamp effect. Works on **KDE Plasma 6** (Wayland & X11).

![KDE Plasma 6](https://img.shields.io/badge/KDE%20Plasma-6-blue)

---

## What it does

- Every app that opens animates with the genie effect from its taskbar icon
- Apps without a taskbar icon animate from the application launcher button
- Optionally: closing an app collapses it toward your cursor (toggle in settings)

---

## Install

```bash
git clone https://github.com/zshadowultra/magic-lamp-everywhere.git
cd magic-lamp-everywhere
chmod +x install.sh
./install.sh
```

Then **log out and log back in** (or run `kwin_wayland --replace & disown`).

---

## Configure

Go to **System Settings → Desktop Effects → Magic Lamp Everywhere** and click the **gear icon**.

| Option | Description |
|--------|-------------|
| Animation duration | Speed in ms. `0` = default (250ms). Higher = slower. |
| Animate window closing | When enabled, closing a window collapses it toward your cursor. |

---

## Uninstall

```bash
sudo rm /usr/lib/qt6/plugins/kwin/effects/plugins/kwin_genie_open.so
sudo rm /usr/lib/qt6/plugins/kwin/effects/configs/kwin_genieopen_config.so
kwriteconfig6 --file kwinrc --group Plugins --key kwin_genie_openEnabled false
```

Then log out and back in.

---

## Requirements

- KDE Plasma 6 (KWin 6.x)
- CMake, Qt6, KF6

> **Not compatible with Plasma 5.**
