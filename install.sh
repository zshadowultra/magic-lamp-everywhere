#!/bin/bash
set -e

echo "=== Genie Open Effect Installer ==="
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then 
    echo "Error: Don't run this script with sudo"
    echo "It will ask for sudo password when needed"
    exit 1
fi

# Install dependencies
echo "Step 1: Installing build dependencies..."
if command -v apt &> /dev/null; then
    sudo apt update
    sudo apt install -y cmake extra-cmake-modules build-essential \
        libkf6config-dev libkf6coreaddons-dev libkf6windowsystem-dev \
        kwin-dev qt6-base-dev
elif command -v dnf &> /dev/null; then
    sudo dnf install -y cmake extra-cmake-modules gcc-c++ \
        kf6-kconfig-devel kf6-kcoreaddons-devel kf6-kwindowsystem-devel \
        kwin-devel qt6-qtbase-devel
elif command -v pacman &> /dev/null; then
    sudo pacman -S --needed cmake extra-cmake-modules base-devel \
        kconfig kcoreaddons kwindowsystem kwin qt6-base
else
    echo "Error: Unsupported package manager. Install dependencies manually."
    exit 1
fi

# Build
echo ""
echo "Step 2: Building effect..."
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Install
echo ""
echo "Step 3: Installing effect..."
sudo make install

# Enable
echo ""
echo "Step 4: Enabling effect..."
kwriteconfig6 --file kwinrc --group Plugins --key kwin_genie_openEnabled true
qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect "kwin_genie_open" 2>/dev/null
sleep 1
qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect "kwin_genie_open" 2>/dev/null || dbus-send --session --dest=org.kde.KWin /KWin org.kde.KWin.reconfigure

# Keep legacy qt path in sync
sudo cp /usr/lib/qt6/plugins/kwin/effects/plugins/kwin_genie_open.so \
        /usr/lib/qt/plugins/kwin/effects/plugins/kwin_genie_open.so 2>/dev/null || true

echo ""
echo "=== Installation Complete! ==="
echo "The Genie Open effect is now active."
echo "You can disable it in: System Settings → Window Management → Desktop Effects"
