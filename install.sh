#!/bin/bash
set -e

echo "=== Magic Lamp Everywhere - Installer ==="
echo ""

# Dependencies
echo "[1/4] Installing dependencies..."
sudo pacman -S --needed --noconfirm cmake extra-cmake-modules \
    kwin qt6-base kf6-kcmutils kf6-config kf6-coreaddons kf6-i18n 2>/dev/null || \
sudo apt-get install -y cmake extra-cmake-modules build-essential \
    libkf6config-dev libkf6coreaddons-dev libkf6kcmutils-dev \
    kwin-dev qt6-base-dev 2>/dev/null || \
echo "  (Skipping auto-install — install dependencies manually if build fails)"

# Build
echo ""
echo "[2/4] Building..."
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DQML_IMPORT_PATH="" -Wno-dev
make -j$(nproc)
cd ..

# Install
echo ""
echo "[3/4] Installing..."
cd build && sudo make install
# Keep legacy qt path in sync (some distros use /usr/lib/qt instead of /usr/lib/qt6)
sudo cp /usr/lib/qt6/plugins/kwin/effects/plugins/kwin_genie_open.so \
        /usr/lib/qt/plugins/kwin/effects/plugins/kwin_genie_open.so 2>/dev/null || true
cd ..

# Enable
echo ""
echo "[4/4] Enabling effect..."
kwriteconfig6 --file kwinrc --group Plugins --key kwin_genie_openEnabled true

echo ""
echo "=== Done! ==="
echo ""
echo "[*] Restarting KWin to apply..."
systemctl --user restart plasma-kwin_wayland.service 2>/dev/null && echo "KWin restarted." || echo "Please restart KWin manually: nohup kwin_wayland --replace &>/dev/null & disown"
echo ""
echo "To configure: System Settings → Desktop Effects → Magic Lamp Everywhere → gear icon"
