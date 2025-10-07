# Solar Inverter Dashboard (ESP32/ESP8266)

Complete system to monitor a solar inverter via Modbus RTU using ESP32/ESP8266 and a responsive web dashboard.

## Features
- Modbus RTU communication with inverter
- Responsive web dashboard with energy flow visualization
- Real-time updates (every 5s by default)
- Animated lines showing energy flow with speed based on power (W)
- OTA (Over-The-Air) updates
- Embedded web server (serves dashboard and data.json)

## Required Hardware
- ESP32 or ESP-01 (ESP8266)
- RS485 transceiver/module
- Connections (ESP32 example):
  - GPIO16 → RX (RS485)
  - GPIO17 → TX (RS485)
  - GND → GND (RS485)
- ESP-01: TX=GPIO1, RX=GPIO3 (use RS485 transceiver with auto-direction)

## Project Structure
```
inverter-dashboard/
├── platformio.ini          # PlatformIO configuration
├── src/
│   └── main.cpp            # Firmware source
├── data/                   # Web files (SPIFFS)
│   ├── index.html          # Dashboard
│   ├── style.css           # Styles
│   └── script.js           # Dashboard JS
└── README.md               # This document
```

## Setup

### 1) Configure WiFi
Edit `src/main.cpp`:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 2) Configure Security Token
IMPORTANT: Change the default token:
```cpp
#define AUTH_TOKEN "CHANGE_ME_LONG_RANDOM_TOKEN"
```
Generate a strong token (≥32 chars with letters/numbers/symbols) and use the same token in:
- `src/main.cpp`
- `data/script.js`

### 3) Configure Modbus
Check settings in `src/main.cpp`:
```cpp
#define MODBUS_BAUD 9600
#define INVERTER_ADDRESS 0x01
```

### 4) Build and Upload
```bash
# Install dependencies
pio lib install

# Build
pio run

# Upload firmware
pio run --target upload

# Upload web files to SPIFFS
pio run --target uploadfs
```

## Accessing the Dashboard
After upload the device will:
1. Connect to WiFi
2. Start the web server (port 80)
3. Read inverter data via Modbus every 5 seconds

Open in your browser: `http://DEVICE_IP`

## Monitored Data (Registers)
| Register | Description                 | Unit | Type       |
|---------:|-----------------------------|------|------------|
|     4067 | PV Total Power              | W    | U_DWORD_R  |
|     5401 | Measured Power (Grid)       | W    | S_WORD     |
|    10008 | Total Consumption Power     | W    | U_WORD     |
|    10022 | Battery Power               | W    | S_WORD     |
|    10023 | Battery Level               | %    | U_WORD     |
|    10024 | Battery Health              | %    | U_WORD     |

## Dashboard Features
- Colored circles for Solar, Grid, House, Battery
- Curved lines connecting components
- Animated dots showing energy flow
- Dynamic dot speed based on power
- Directional arrows:
  - Grid: `◀` (import) / `▶` (export)
  - Battery: `▼` (charging) / `▲` (discharging)
- Battery bar with dynamic color
- Real-time connection status

## Security
### Token Authentication
- Protected endpoint: `/data.json` requires token
- Methods:
  - Header: `Authorization: Bearer <token>`
  - Query: `?token=<token>`
- Default token (example): `inverter_2024_secure_token_xyz789` (CHANGE IT!)

### Recommendations
1. Change the default token
2. Use HTTPS if possible (requires cert proxy)
3. Restrict external access with firewall
4. Use strong tokens and rotate periodically

### Test
```bash
# With token (success)
curl "http://DEVICE_IP/data.json?token=YOUR_TOKEN"

# Without token (401)
curl "http://DEVICE_IP/data.json"
```

## OTA Updates
- Hostname: `inverter-modbus-esp01`
- Password: `12345678`
- Port: 8266 (ArduinoOTA default)

## Troubleshooting
### Modbus
1. Check RS485 wiring
2. Confirm inverter address (0x01)
3. Verify baud rate (9600)
4. Watch Serial Monitor for errors

### WiFi
1. Check credentials
2. Ensure the device is in range
3. Ensure 2.4GHz is available

### Dashboard
1. Upload SPIFFS content (`uploadfs`)
2. Confirm web server is running
3. Check device IP on Serial Monitor

## Logs & Debug
Serial output provides progress and error messages for WiFi, OTA, Modbus and HTTP.

## Customization
Edit `data/style.css` to tweak colors, sizes and layout.

## License
Open-source. Modify and distribute freely.

## Contributions
1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push the branch
5. Open a Pull Request
