# ⚡ Inverter Dashboard

A real-time energy flow visualization dashboard for solar inverters with Modbus communication.

## 🚀 Live Demo

**[View Live Demo on GitHub Pages](https://yourusername.github.io/inverter-dashboard/demo.html)**

## 📱 Features

- **Real-time Energy Flow**: Visual representation of energy flow between solar, battery, house, and grid
- **Interactive Controls**: Adjust values to see flow changes in real-time
- **Responsive Design**: Works perfectly on desktop and mobile devices
- **Animated Visualization**: Moving dots show energy flow direction and speed
- **Smart Indicators**: Directional arrows for grid import/export and battery charge/discharge
- **Modbus Integration**: ESP8266/ESP32 firmware for real inverter communication

## 🎮 Demo Controls

The demo includes interactive sliders to simulate different energy scenarios:

- **Solar Production**: 0-5000W
- **Battery Level**: 0-100%
- **Battery Power**: -2000W to +2000W (discharge/charge)
- **House Consumption**: 0-3000W
- **Grid Power**: -2000W to +2000W (export/import)

## 🔧 Hardware Setup

### ESP-01 (ESP8266) Configuration
```
VCC    → 3.3V
GND    → GND
TX     → RX (USB-Serial)
RX     → TX (USB-Serial)
CH_PD  → 3.3V
GPIO0  → GND (during upload)
```

### Modbus Connection
- **Baud Rate**: 9600 (configurable)
- **Address**: 1 (configurable)
- **Registers**: Configurable via web interface

## 📊 Modbus Registers

Default register mapping:
- **4067**: PV Total Power (W)
- **5401**: Grid Power (W)
- **10008**: House Consumption (W)
- **10022**: Battery Power (W)
- **10023**: Battery Level (%)
- **10024**: Battery Health (%)

## 🌐 Web Interface

### Dashboard (`/`)
- Real-time energy flow visualization
- Component status and values
- Battery level indicator
- System status and timestamp

### Configuration (`/config`)
- WiFi settings
- Modbus configuration
- Register mapping
- Security token management

## 🛠️ Development

### Local Testing
```bash
# Open demo.html in browser
open demo.html

# Or use a local server
python -m http.server 8000
```

### ESP8266 Development
```bash
# Install PlatformIO
pip install platformio

# Build and upload
pio run --target upload

# Monitor serial output
pio device monitor
```

## 📁 Project Structure

```
inverter-dashboard/
├── index.html          # Main dashboard
├── demo.html           # Interactive demo
├── style.css           # Styling
├── script.js           # Dashboard logic
├── data.json           # Sample data
├── src/main.cpp        # ESP8266 firmware
├── platformio.ini      # Build configuration
└── .github/workflows/  # GitHub Actions
```

## 🔐 Security

- **Token-based authentication** for data endpoints
- **Configurable security tokens** via web interface
- **Protected configuration** endpoints

## 📱 Mobile Support

- **Responsive design** for all screen sizes
- **Touch-friendly** controls
- **Optimized performance** for mobile devices

## 🚀 Deployment

### GitHub Pages
1. Enable GitHub Pages in repository settings
2. Push to main branch
3. Access via `https://username.github.io/inverter-dashboard/demo.html`

### ESP8266
1. Flash firmware to ESP-01
2. Connect to WiFi
3. Access via device IP address

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is open source and available under the [MIT License](LICENSE).

## 🆘 Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/inverter-dashboard/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/inverter-dashboard/discussions)
- **Documentation**: [Wiki](https://github.com/yourusername/inverter-dashboard/wiki)

## 🙏 Acknowledgments

- **Modbus-ESP8266** library for Modbus communication
- **ArduinoJson** for JSON handling
- **ESP8266WebServer** for web interface

---

**Made with ❤️ for the solar energy community**
