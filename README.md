# AirPods Linux-iPhone Seamless Handoff

Daemon for seamless audio handoff between Linux and iPhone using AirPods.

## Requirements

- Qt6 (Core, Bluetooth, DBus)
- PulseAudio or PipeWire
- AirPods paired and connected to Linux

## Install Dependencies

**Arch Linux:**
```bash
sudo pacman -S qt6-base qt6-connectivity cmake make
```

**Debian/Ubuntu:**
```bash
sudo apt install qt6-base-dev libqt6bluetooth6-dev qt6-connectivity-dev cmake build-essential
```

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install qt6-qtbase-devel qt6-qtconnectivity-devel cmake gcc-c++
```

**openSUSE:**
```bash
sudo zypper install qt6-base-devel qt6-connectivity-devel cmake gcc-c++
```

**Gentoo:**
```bash
sudo emerge -av dev-qt/qtbase dev-qt/qtconnectivity dev-util/cmake
```

**Void Linux:**
```bash
sudo xbps-install -S qt6-base-devel qt6-connectivity-devel cmake gcc
```

## Building

```bash
git clone https://github.com/xatuke/handoff.git
cd handoff && mkdir build && cd build
cmake ..
make
```

## Usage

```bash
# Get your AirPods MAC address
bluetoothctl devices

# Run the handoff daemon
./airpods-handoff 34:0E:22:49:C4:73
```

Replace `34:0E:22:49:C4:73` with your AirPods Bluetooth MAC address.

## Running at Startup

To run automatically on login:

```bash
# Create systemd user service
mkdir -p ~/.config/systemd/user
cat > ~/.config/systemd/user/airpods-handoff.service << EOF
[Unit]
Description=AirPods Linux-iPhone Handoff
After=bluetooth.target

[Service]
Type=simple
ExecStart=/path/to/handoff/build/airpods-handoff YOUR_AIRPODS_MAC
Restart=on-failure

[Install]
WantedBy=default.target
EOF

# Enable and start
systemctl --user enable airpods-handoff
systemctl --user start airpods-handoff

# Check status
systemctl --user status airpods-handoff
```

## Logs

The app outputs to stdout/stderr. To see logs:

```bash
# If running manually
./airpods-handoff 34:0E:22:49:C4:73

# If running as systemd service
journalctl --user -u airpods-handoff -f
```

## Troubleshooting

**Audio doesn't switch:**
- Check AirPods are connected: `bluetoothctl info YOUR_MAC`
- Verify A2DP profile is active: `pactl list cards | grep -A 50 bluez`
- Check logs for errors

**Permission denied:**
- Add user to `bluetooth` group: `sudo usermod -a -G bluetooth $USER`
- Logout and login again

## License

MIT
