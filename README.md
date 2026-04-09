# Genie Open Effect for KWin

A KWin effect that applies a macOS-style genie animation when windows open, triggered by the `windowAdded` signal.

## Quick Install (Recommended)

```bash
cd /home/zshadowultra/Projects/genie-kwin-open-close
./install.sh
```

The script will:
1. Install required dependencies (asks for sudo password)
2. Build the effect
3. Install it system-wide
4. Enable it automatically

## Manual Installation

If you prefer manual steps:

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install cmake extra-cmake-modules build-essential \
    libkf6config-dev libkf6coreaddons-dev libkf6windowsystem-dev \
    kwin-dev qt6-base-dev

# Build
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Install
sudo make install

# Enable
kwriteconfig6 --file kwinrc --group Plugins --key kwin_genie_openEnabled true
qdbus org.kde.KWin /KWin reconfigure
```

## Uninstall

```bash
sudo rm /usr/lib/*/qt6/plugins/kwin/effects/plugins/kwin_genie_open.so
sudo rm -rf /usr/share/kwin/effects/kwin_genie_open
kwriteconfig6 --file kwinrc --group Plugins --key kwin_genie_openEnabled false
qdbus org.kde.KWin /KWin reconfigure
```

## How it works

- Listens to `windowAdded` signal
- Filters to only animate normal windows
- Creates 40x40 mesh grid for smooth deformation
- Warps window from taskbar icon position (or bottom center) upward
- Bottom of window narrows toward icon while top stays wide
- Uses sine easing curve for smooth animation
- Duration: 250ms

## Troubleshooting

**Effect doesn't appear:**
- Check if enabled: System Settings → Window Management → Desktop Effects
- Restart KWin: `kwin_x11 --replace &` or `kwin_wayland --replace &`
- Check logs: `journalctl -f | grep kwin`

**Build errors:**
- Make sure you're on KDE Plasma 6 (not Plasma 5)
- Install all dependencies listed above

## Based on

- KWin Magic Lamp effect
- Yet Another Magic Lamp effect
