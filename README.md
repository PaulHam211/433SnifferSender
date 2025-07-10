# 🎯 RF433 Sniffer & Transmitter

A comprehensive ESP32-based device for capturing, storing, and replaying 433MHz RF signals with a beautiful responsive web interface. Perfect for home automation, security research, and RF signal analysis.

![ESP32 RF433 Device](https://img.shields.io/badge/ESP32-RF433%20Device-blue)
![Version](https://img.shields.io/badge/version-1.0.0-green)
![License](https://img.shields.io/badge/license-MIT-blue)

## ✨ Features

### 🎧 **Signal Capture & Storage**
- **Real-time 433MHz signal sniffing** with automatic detection
- **Intelligent storage** of up to **1,000 signals** with persistent memory
- **Duplicate detection** prevents storing identical signals
- **Automatic cleanup** removes oldest 20% of non-favorite signals when storage reaches 95%
- **Signal metadata** including protocol, bit length, timestamp, and custom names

### 📱 **Web Interface**
- **Beautiful responsive design** optimized for mobile and desktop
- **Real-time status updates** showing sniffing, buzzer, and LED states
- **Visual storage indicators** with color-coded progress bars
- **Signal visualization** with binary pattern representation
- **Advanced filtering** (All signals, Favorites only, Recent captures)
- **Signal management** with rename, favorite, and delete functions

### 🔊 **Audio & Visual Feedback**
- **Piezo buzzer** with distinct tones for different events:
  - Startup: Three ascending tones
  - Signal received: 1000Hz → 1500Hz beeps
  - Signal transmitted: 2000Hz → 1500Hz beeps
- **LED indicators** with customizable flash patterns
- **Enable/disable controls** for both audio and visual feedback

### 📡 **Signal Transmission**
- **One-click signal replay** from the web interface
- **Protocol preservation** ensures accurate signal reproduction
- **Real-time transmission feedback** with audio/visual confirmation

### 🛠 **Advanced Management**
- **Manual cleanup tools** for storage optimization
- **Age-based cleanup** (remove signals older than X days)
- **Favorite system** protects important signals from auto-cleanup
- **Export-ready format** for signal analysis

## 🔧 Hardware Requirements

| Component | Description | Notes |
|-----------|-------------|-------|
| **ESP32 Development Board** | Any ESP32 dev board | Tested with ESP32-WROOM-32 |
| **RF433 Transmitter Module** | OPEN-SMART or compatible | 5V power recommended |
| **RF433 Receiver Module** | OPEN-SMART or compatible | 3.3V compatible |
| **Piezo Buzzer** | Active or passive | For audio feedback |
| **Jumper Wires** | Male-to-female | For connections |
| **Breadboard** (Optional) | For prototyping | Makes wiring easier |

## 📋 Wiring Diagram

```
ESP32 Pin    →    Component
=========================================
3.3V         →    RF433 Receiver VCC
GND          →    RF433 Receiver GND & Buzzer GND
GPIO 4       →    RF433 Receiver DATA

VIN (5V)     →    RF433 Transmitter VCC
GND          →    RF433 Transmitter GND
GPIO 2       →    RF433 Transmitter DATA

GPIO 5       →    Piezo Buzzer Positive
GND          →    Piezo Buzzer Negative

GPIO 2       →    Built-in LED (shared with transmitter)
```

### 🔌 **Detailed Pin Configuration**

```cpp
#define RF_TRANSMITTER_PIN 2  // GPIO 2 - RF433 Transmitter DATA
#define RF_RECEIVER_PIN 4     // GPIO 4 - RF433 Receiver DATA  
#define PIEZO_BUZZER_PIN 5    // GPIO 5 - Piezo Buzzer
#define LED_BUILTIN 2         // GPIO 2 - Built-in LED
```

## 🚀 Installation & Setup

### **Step 1: Clone/Download Project**
```bash
git clone <repository-url>
cd RF433SnifferSender
```

### **Step 2: Hardware Assembly**
1. Connect components according to the wiring diagram above
2. Double-check all connections, especially power (3.3V vs 5V)
3. Ensure good antenna connections for optimal RF performance

### **Step 3: Software Setup**
```bash
# Using PlatformIO
pio run --target upload          # Upload firmware
pio run --target uploadfs        # Upload web interface
```

### **Step 4: First Boot**
1. Power on the ESP32
2. Listen for startup tones (3 ascending beeps)
3. Look for WiFi network: **`RF433_Sniffer`**
4. Connect with password: **`password123`**
5. Navigate to: **`192.168.4.1`**

## 📱 Usage Guide

### **Initial Setup**
1. **Connect to WiFi**: Join the `RF433_Sniffer` network
2. **Open Web Interface**: Navigate to `192.168.4.1` in your browser
3. **Check Status**: Verify all indicators show device is ready

### **Capturing Signals**
1. **Start Sniffing**: Click the "Start Sniffing" button
2. **Trigger RF Devices**: Use garage remotes, car keys, etc.
3. **Monitor Capture**: Signals appear in real-time with audio/visual feedback
4. **Organize Signals**: Rename important captures and mark as favorites

### **Managing Signals**
- **Rename**: Click "Rename" to give signals meaningful names
- **Favorite**: Star important signals to protect from auto-cleanup
- **Transmit**: Click "Transmit" to replay any captured signal
- **Delete**: Remove unwanted signals individually
- **Bulk Cleanup**: Use "Clean Old" or "Auto Clean" for maintenance

### **Storage Management**
- **Monitor Usage**: Watch the storage progress bar (green/yellow/red)
- **Auto Cleanup**: Activates at 95% capacity, removes oldest 20%
- **Manual Cleanup**: Remove signals older than specified days
- **Favorites Protection**: Starred signals are never auto-deleted

## 🌐 Web Interface Features

### **Status Dashboard**
- Real-time sniffing status (active/inactive)
- Audio feedback toggle (buzzer on/off)
- Visual feedback toggle (LED on/off)
- Storage usage with visual progress bar
- Signal count and favorites count

### **Signal Management**
- **Grid View**: Cards showing signal details and binary visualization
- **Filtering**: All, Favorites, or Recent signals
- **Actions**: Transmit, Rename, Favorite, Delete for each signal
- **Bulk Operations**: Clear all or cleanup tools

### **Mobile Optimization**
- Responsive design adapts to phone screens
- Touch-friendly buttons and controls
- Optimized layout prevents button cutoffs
- Fast loading and smooth interactions

## 🔍 API Reference

The device exposes a RESTful API for advanced integration:

### **Status & Control**
- `GET /api/status` - Get device status and statistics
- `POST /api/sniffing` - Enable/disable signal capturing
- `POST /api/buzzer` - Toggle audio feedback
- `POST /api/led` - Toggle LED feedback

### **Signal Management**
- `GET /api/signals` - Retrieve all stored signals
- `POST /api/transmit` - Transmit a specific signal by ID
- `DELETE /api/signals` - Delete a signal by ID
- `POST /api/signals/rename` - Rename a signal
- `POST /api/signals/favorite` - Toggle favorite status

### **Storage Management**
- `POST /api/clear` - Clear all signals
- `POST /api/cleanup` - Perform automatic cleanup
- `POST /api/cleanup/old` - Remove signals older than X days

## ⚙️ Configuration

### **WiFi Settings**
```cpp
WiFi.softAP("RF433_Sniffer", "password123");
```

### **Pin Assignments**
```cpp
#define RF_TRANSMITTER_PIN 2
#define RF_RECEIVER_PIN 4  
#define PIEZO_BUZZER_PIN 5
```

### **Storage Limits**
```cpp
const int MAX_SIGNALS = 1000;
const int AUTO_CLEANUP_THRESHOLD = 950;  // 95% capacity
```

### **Audio Customization**
```cpp
// Receive sound: 1000Hz → 1500Hz
// Transmit sound: 2000Hz → 1500Hz  
// Startup: 800Hz, 1000Hz, 1200Hz
```

## 🔧 Troubleshooting

### **Common Issues**

| Problem | Solution |
|---------|----------|
| **No startup tone** | Check buzzer wiring and power connections |
| **Can't find WiFi network** | Verify ESP32 power and check serial monitor |
| **HTTP 500 error** | Upload filesystem: `pio run --target uploadfs` |
| **No signal capture** | Check receiver wiring (GPIO 4) and 3.3V power |
| **Transmitter not working** | Verify transmitter wiring (GPIO 2) and 5V power |
| **Serial monitor blank** | Check baud rate (115200) and COM port |

### **Signal Quality Tips**
- Use proper antennas (17.3cm wire for 433MHz)
- Keep transmitter and receiver separated
- Avoid interference from WiFi routers
- Test with known working 433MHz remotes

### **Performance Optimization**
- Regularly use cleanup tools to maintain performance
- Mark important signals as favorites before auto-cleanup
- Monitor storage usage to prevent data loss
- Keep firmware updated for best performance

## 📊 Technical Specifications

- **Frequency**: 433MHz (ISM band)
- **Protocols**: All RC-Switch supported protocols
- **Storage**: 1,000 signals with persistent memory
- **Interface**: Responsive web UI + REST API
- **Power**: 5V via USB or external supply
- **Range**: Depends on antenna and RF modules (typically 10-100m)
- **Memory**: ESP32 Flash + NVS for persistence

## 🏗️ Development

### **Building from Source**
```bash
# Prerequisites: PlatformIO installed
git clone <repository-url>
cd RF433SnifferSender
pio run                    # Compile
pio run --target upload    # Flash firmware  
pio run --target uploadfs  # Flash web interface
```

### **Project Structure**
```
├── src/
│   └── main.cpp          # Main application code
├── data/
│   └── index.html        # Web interface
├── platformio.ini        # Build configuration
└── README.md            # This file
```

### **Libraries Used**
- ESP32 Arduino Framework
- ESPAsyncWebServer (Web interface)
- ArduinoJson (API responses)
- RC-Switch (RF communication)
- Preferences (Persistent storage)

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **RC-Switch Library** - For reliable 433MHz communication
- **ESPAsyncWebServer** - For the powerful web server capabilities
- **Arduino/ESP32 Community** - For extensive documentation and support
- **Open Source Hardware** - For making this project possible

## 📞 Support

- **Issues**: Use GitHub Issues for bug reports
- **Discussions**: Join the community discussions
- **Documentation**: Check the wiki for additional guides
- **Updates**: Watch the repository for new releases

---

**⚠️ Disclaimer**: This device is for educational and personal use only. Ensure compliance with local RF regulations and laws. Use responsibly and respect others' privacy and property.
