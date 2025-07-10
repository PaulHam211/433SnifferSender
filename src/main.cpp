#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <RCSwitch.h>
#include <SPIFFS.h>
#include <Preferences.h>

// Pin definitions
#define RF_TRANSMITTER_PIN 2
#define RF_RECEIVER_PIN 4
#define PIEZO_BUZZER_PIN 5
#define LED_BUILTIN 2

// RF433 instances
RCSwitch mySwitch = RCSwitch();
RCSwitch receiver = RCSwitch();

// Web server
AsyncWebServer server(80);

// Preferences for storing signals
Preferences preferences;

// Global variables
bool sniffingEnabled = false;
bool buzzerEnabled = true;
bool ledEnabled = true;
unsigned long lastSignalTime = 0;
int signalCount = 0;

// Signal storage structure
struct RFSignal {
  String name;
  unsigned long value;
  unsigned int bitLength;
  unsigned int protocol;
  unsigned long timestamp;
  bool isFavorite;
};

// In-memory signal storage (will be persisted to preferences)
std::vector<RFSignal> storedSignals;
const int MAX_SIGNALS = 1000;  // Increased to 1000 signals
const int AUTO_CLEANUP_THRESHOLD = 950;  // Start cleanup when reaching 95% capacity

// Function declarations
void handleReceivedSignal();
void transmitSignal(const RFSignal& signal);
bool isDuplicate(const RFSignal& newSignal);
void performAutoCleanup();
void playReceiveSound();
void playTransmitSound();
void playStartupSound();
void flashLED(int duration, int times);
void loadStoredSignals();
void saveStoredSignals();
void setupWebServer();

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIEZO_BUZZER_PIN, OUTPUT);
  
  // Setup LEDC for buzzer (ESP32 tone equivalent)
  ledcSetup(0, 1000, 8); // channel 0, 1000 Hz base freq, 8-bit resolution
  ledcAttachPin(PIEZO_BUZZER_PIN, 0);
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  // Initialize preferences
  preferences.begin("rf433", false);
  
  // Load persistent settings
  buzzerEnabled = preferences.getBool("buzzerEnabled", true);
  ledEnabled = preferences.getBool("ledEnabled", true);
  sniffingEnabled = preferences.getBool("sniffingEnabled", true);  // Auto-start sniffing by default
  
  Serial.println("Settings loaded:");
  Serial.println("  Buzzer: " + String(buzzerEnabled ? "ON" : "OFF"));
  Serial.println("  LED: " + String(ledEnabled ? "ON" : "OFF"));
  Serial.println("  Sniffing: " + String(sniffingEnabled ? "ON" : "OFF"));
  
  // Setup RF modules
  mySwitch.enableTransmit(RF_TRANSMITTER_PIN);
  receiver.enableReceive(digitalPinToInterrupt(RF_RECEIVER_PIN));
  
  // Setup WiFi Access Point
  WiFi.softAP("RF433_Sniffer", "password123");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Load stored signals
  loadStoredSignals();
  
  // Setup web server routes
  setupWebServer();
  
  // Start web server
  server.begin();
  
  Serial.println("RF433 Sniffer ready!");
  playStartupSound();
}

void loop() {
  // Check for received RF signals
  if (receiver.available() && sniffingEnabled) {
    handleReceivedSignal();
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

void handleReceivedSignal() {
  unsigned long value = receiver.getReceivedValue();
  unsigned int bitLength = receiver.getReceivedBitlength();
  unsigned int protocol = receiver.getReceivedProtocol();
  
  if (value != 0) {
    Serial.print("Received: ");
    Serial.print(value);
    Serial.print(" / ");
    Serial.print(bitLength);
    Serial.print("bit ");
    Serial.print("Protocol: ");
    Serial.println(protocol);
    
    // Create new signal
    RFSignal newSignal;
    newSignal.value = value;
    newSignal.bitLength = bitLength;
    newSignal.protocol = protocol;
    newSignal.timestamp = millis();
    newSignal.name = "Signal_" + String(signalCount++);
    newSignal.isFavorite = false;
    
    // Add to storage if not duplicate and under limit
    if (!isDuplicate(newSignal)) {
      // Check if we need to do cleanup
      if (storedSignals.size() >= AUTO_CLEANUP_THRESHOLD) {
        performAutoCleanup();
      }
      
      // Add the signal if there's still space
      if (storedSignals.size() < MAX_SIGNALS) {
        storedSignals.push_back(newSignal);
        saveStoredSignals();
        
        Serial.println("Signal stored (" + String(storedSignals.size()) + "/" + String(MAX_SIGNALS) + ")");
      } else {
        Serial.println("Storage full! Signal not saved.");
      }
    } else {
      Serial.println("Duplicate signal ignored.");
    }
    
    // Provide feedback
    if (buzzerEnabled) {
      playReceiveSound();
    }
    if (ledEnabled) {
      flashLED(100, 3);
    }
    
    lastSignalTime = millis();
  }
  
  receiver.resetAvailable();
}

void transmitSignal(const RFSignal& signal) {
  Serial.print("Transmitting: ");
  Serial.print(signal.value);
  Serial.print(" / ");
  Serial.print(signal.bitLength);
  Serial.print("bit ");
  Serial.print("Protocol: ");
  Serial.println(signal.protocol);
  
  mySwitch.setProtocol(signal.protocol);
  mySwitch.send(signal.value, signal.bitLength);
  
  // Provide feedback
  if (buzzerEnabled) {
    playTransmitSound();
  }
  if (ledEnabled) {
    flashLED(200, 2);
  }
}

bool isDuplicate(const RFSignal& newSignal) {
  for (const auto& signal : storedSignals) {
    if (signal.value == newSignal.value && 
        signal.bitLength == newSignal.bitLength &&
        signal.protocol == newSignal.protocol) {
      return true;
    }
  }
  return false;
}

void performAutoCleanup() {
  Serial.println("Storage nearly full, performing automatic cleanup...");
  
  // Sort signals by timestamp (oldest first)
  std::sort(storedSignals.begin(), storedSignals.end(), 
    [](const RFSignal& a, const RFSignal& b) {
      // Keep favorites, sort others by timestamp
      if (a.isFavorite && !b.isFavorite) return false;
      if (!a.isFavorite && b.isFavorite) return true;
      return a.timestamp < b.timestamp;
    });
  
  // Calculate how many signals to remove (remove oldest 20% of non-favorites)
  int signalsToRemove = MAX_SIGNALS * 0.2;  // Remove 20%
  int removedCount = 0;
  
  // Remove oldest non-favorite signals
  auto it = storedSignals.begin();
  while (it != storedSignals.end() && removedCount < signalsToRemove) {
    if (!it->isFavorite) {
      it = storedSignals.erase(it);
      removedCount++;
    } else {
      ++it;
    }
  }
  
  Serial.println("Cleanup complete: Removed " + String(removedCount) + " old signals");
  Serial.println("Storage now: " + String(storedSignals.size()) + "/" + String(MAX_SIGNALS));
  
  // Save the cleaned up signals
  saveStoredSignals();
}

void playReceiveSound() {
  ledcWriteTone(0, 1000);
  delay(100);
  ledcWriteTone(0, 0);
  delay(20);
  ledcWriteTone(0, 1500);
  delay(100);
  ledcWriteTone(0, 0);
}

void playTransmitSound() {
  ledcWriteTone(0, 2000);
  delay(150);
  ledcWriteTone(0, 0);
  delay(20);
  ledcWriteTone(0, 1500);
  delay(150);
  ledcWriteTone(0, 0);
}

void playStartupSound() {
  for (int i = 0; i < 3; i++) {
    ledcWriteTone(0, 800 + i * 200);
    delay(200);
    ledcWriteTone(0, 0);
    delay(50);
  }
}

void flashLED(int duration, int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(duration);
    digitalWrite(LED_BUILTIN, LOW);
    delay(duration);
  }
}

void loadStoredSignals() {
  // Load signals from preferences
  int count = preferences.getInt("signalCount", 0);
  signalCount = preferences.getInt("nextId", 0);
  
  for (int i = 0; i < count; i++) {
    RFSignal signal;
    String prefix = "sig" + String(i) + "_";
    
    signal.name = preferences.getString((prefix + "name").c_str(), "");
    signal.value = preferences.getULong((prefix + "val").c_str(), 0);
    signal.bitLength = preferences.getUInt((prefix + "bits").c_str(), 0);
    signal.protocol = preferences.getUInt((prefix + "proto").c_str(), 0);
    signal.timestamp = preferences.getULong((prefix + "time").c_str(), 0);
    signal.isFavorite = preferences.getBool((prefix + "fav").c_str(), false);
    
    if (signal.value != 0) {
      storedSignals.push_back(signal);
    }
  }
  
  Serial.println("Loaded " + String(storedSignals.size()) + " signals from storage");
}

void saveStoredSignals() {
  preferences.putInt("signalCount", storedSignals.size());
  preferences.putInt("nextId", signalCount);
  
  for (size_t i = 0; i < storedSignals.size(); i++) {
    String prefix = "sig" + String(i) + "_";
    const RFSignal& signal = storedSignals[i];
    
    preferences.putString((prefix + "name").c_str(), signal.name);
    preferences.putULong((prefix + "val").c_str(), signal.value);
    preferences.putUInt((prefix + "bits").c_str(), signal.bitLength);
    preferences.putUInt((prefix + "proto").c_str(), signal.protocol);
    preferences.putULong((prefix + "time").c_str(), signal.timestamp);
    preferences.putBool((prefix + "fav").c_str(), signal.isFavorite);
  }
}

void setupWebServer() {
  // Serve static files
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
  // API endpoints
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(300);
    doc["sniffing"] = sniffingEnabled;
    doc["buzzer"] = buzzerEnabled;
    doc["led"] = ledEnabled;
    doc["signalCount"] = storedSignals.size();
    doc["maxSignals"] = MAX_SIGNALS;
    doc["storageUsed"] = (float)storedSignals.size() / MAX_SIGNALS * 100;
    doc["lastSignal"] = lastSignalTime;
    
    // Count favorites
    int favoriteCount = 0;
    for (const auto& signal : storedSignals) {
      if (signal.isFavorite) favoriteCount++;
    }
    doc["favoriteCount"] = favoriteCount;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/api/sniffing", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("enabled", true)) {
      sniffingEnabled = request->getParam("enabled", true)->value() == "true";
      preferences.putBool("sniffingEnabled", sniffingEnabled);  // Save to preferences
      request->send(200, "text/plain", sniffingEnabled ? "Sniffing enabled" : "Sniffing disabled");
    } else {
      request->send(400, "text/plain", "Missing enabled parameter");
    }
  });
  
  server.on("/api/buzzer", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("enabled", true)) {
      buzzerEnabled = request->getParam("enabled", true)->value() == "true";
      preferences.putBool("buzzerEnabled", buzzerEnabled);  // Save to preferences
      request->send(200, "text/plain", buzzerEnabled ? "Buzzer enabled" : "Buzzer disabled");
    } else {
      request->send(400, "text/plain", "Missing enabled parameter");
    }
  });
  
  server.on("/api/led", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("enabled", true)) {
      ledEnabled = request->getParam("enabled", true)->value() == "true";
      preferences.putBool("ledEnabled", ledEnabled);  // Save to preferences
      request->send(200, "text/plain", ledEnabled ? "LED enabled" : "LED disabled");
    } else {
      request->send(400, "text/plain", "Missing enabled parameter");
    }
  });
  
  server.on("/api/signals", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(8192);
    JsonArray signals = doc.createNestedArray("signals");
    
    for (size_t i = 0; i < storedSignals.size(); i++) {
      JsonObject signal = signals.createNestedObject();
      signal["id"] = i;
      signal["name"] = storedSignals[i].name;
      signal["value"] = String(storedSignals[i].value);
      signal["bitLength"] = storedSignals[i].bitLength;
      signal["protocol"] = storedSignals[i].protocol;
      signal["timestamp"] = storedSignals[i].timestamp;
      signal["isFavorite"] = storedSignals[i].isFavorite;
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/api/transmit", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("id", true)) {
      int id = request->getParam("id", true)->value().toInt();
      if (id >= 0 && id < storedSignals.size()) {
        transmitSignal(storedSignals[id]);
        request->send(200, "text/plain", "Signal transmitted");
      } else {
        request->send(400, "text/plain", "Invalid signal ID");
      }
    } else {
      request->send(400, "text/plain", "Missing signal ID");
    }
  });
  
  server.on("/api/signals", HTTP_DELETE, [](AsyncWebServerRequest *request){
    if (request->hasParam("id", true)) {
      int id = request->getParam("id", true)->value().toInt();
      if (id >= 0 && id < storedSignals.size()) {
        storedSignals.erase(storedSignals.begin() + id);
        saveStoredSignals();
        request->send(200, "text/plain", "Signal deleted");
      } else {
        request->send(400, "text/plain", "Invalid signal ID");
      }
    } else {
      request->send(400, "text/plain", "Missing signal ID");
    }
  });
  
  server.on("/api/signals/rename", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("id", true) && request->hasParam("name", true)) {
      int id = request->getParam("id", true)->value().toInt();
      String name = request->getParam("name", true)->value();
      if (id >= 0 && id < storedSignals.size()) {
        storedSignals[id].name = name;
        saveStoredSignals();
        request->send(200, "text/plain", "Signal renamed");
      } else {
        request->send(400, "text/plain", "Invalid signal ID");
      }
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });
  
  server.on("/api/signals/favorite", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("id", true) && request->hasParam("favorite", true)) {
      int id = request->getParam("id", true)->value().toInt();
      bool favorite = request->getParam("favorite", true)->value() == "true";
      if (id >= 0 && id < storedSignals.size()) {
        storedSignals[id].isFavorite = favorite;
        saveStoredSignals();
        request->send(200, "text/plain", favorite ? "Signal marked as favorite" : "Signal unmarked as favorite");
      } else {
        request->send(400, "text/plain", "Invalid signal ID");
      }
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });
  
  server.on("/api/clear", HTTP_POST, [](AsyncWebServerRequest *request){
    storedSignals.clear();
    signalCount = 0;
    saveStoredSignals();
    request->send(200, "text/plain", "All signals cleared");
  });
  
  server.on("/api/cleanup", HTTP_POST, [](AsyncWebServerRequest *request){
    int originalCount = storedSignals.size();
    performAutoCleanup();
    int removedCount = originalCount - storedSignals.size();
    request->send(200, "text/plain", "Cleanup complete: Removed " + String(removedCount) + " signals");
  });
  
  server.on("/api/cleanup/old", HTTP_POST, [](AsyncWebServerRequest *request){
    // Remove signals older than specified days (default 7 days)
    int daysOld = 7;
    if (request->hasParam("days", true)) {
      daysOld = request->getParam("days", true)->value().toInt();
    }
    
    unsigned long cutoffTime = millis() - (daysOld * 24 * 60 * 60 * 1000UL);
    int originalCount = storedSignals.size();
    
    auto it = storedSignals.begin();
    while (it != storedSignals.end()) {
      if (!it->isFavorite && it->timestamp < cutoffTime) {
        it = storedSignals.erase(it);
      } else {
        ++it;
      }
    }
    
    int removedCount = originalCount - storedSignals.size();
    saveStoredSignals();
    request->send(200, "text/plain", "Removed " + String(removedCount) + " signals older than " + String(daysOld) + " days");
  });
}