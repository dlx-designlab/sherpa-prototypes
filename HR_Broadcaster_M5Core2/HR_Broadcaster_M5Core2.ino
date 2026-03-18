// M5Stack Arduino BLE Scanner Example
// This code initializes the M5Stack Core2, sets up a BLE scanner using NimBLE
#include <M5Unified.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <OSCMessage.h>
#include <esp_now.h>


const char* WIFI_SSID     = "A101ZTa-7DD8A9"; // Your WiFi SSID
const char* WIFI_PASSWORD = "0089656a";
const char* HR_DEVICE_NAME = "TICKR";

const char* OSC_HOST = "10.55.248.224"; // OSC destination IP
const int   OSC_PORT = 8000;            // OSC destination port

static uint32_t lastBatteryCheck = 0;

WiFiUDP udp;
uint8_t espNowBroadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static NimBLEUUID HR_SERVICE_UUID("180D");
static NimBLEUUID HR_CHAR_UUID("2A37");

static NimBLEAdvertisedDevice* foundDevice = nullptr;
static NimBLEClient* pClient = nullptr;
static bool scanning = true;
static bool connected = false;

// --- Notification callback ---
void notifyCallback(NimBLERemoteCharacteristic* pChar,
                    uint8_t* pData, size_t length, bool isNotify) {
    if (length < 2) return;
    bool is16bit = (pData[0] & 0x01);
    int bpm = is16bit ? (pData[1] | (pData[2] << 8)) : pData[1];

    Serial.printf("Heart Rate: %d BPM\n", bpm);

    // Send BPM via OSC
    // OSCMessage msg("/heart_rate");
    // msg.add((int32_t)bpm);
    // udp.beginPacket(OSC_HOST, OSC_PORT);
    // msg.send(udp);
    // udp.endPacket();
    // msg.empty();
    
    // Send BPM via ESP-NOW to all receivers
    int32_t bpmToSend = (int32_t)bpm;
    esp_now_send(espNowBroadcast, (uint8_t*)&bpmToSend, sizeof(bpmToSend));    

    M5.Display.fillRect(160, 120, 160, 120, BLACK); // Clear right half
    M5.Display.setCursor(160, 120);
    M5.Display.setTextSize(8);
    M5.Display.setTextColor(RED);
    M5.Display.printf("%d", bpm);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(WHITE);
    M5.Display.setCursor(160, 200);
    M5.Display.print("BPM");
}

// --- Scanner callback ---
class MyAdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
        std::string name = advertisedDevice->getName();
        std::string address = advertisedDevice->getAddress().toString();
        int rssi = advertisedDevice->getRSSI();

        if (rssi > -70) {
            Serial.printf("Found: [%s] Name: %s | RSSI: %d\n",
                        address.c_str(),
                        name.empty() ? "Unknown" : name.c_str(),
                        rssi);
            M5.Display.startWrite();
            M5.Display.setScrollRect(0, 120, 320, 120, BLACK); // limit scroll to bottom half
            M5.Display.scroll(0, -20);
            M5.Display.setCursor(0, 220);
            M5.Display.setTextSize(1);
            M5.Display.printf("-> %s (%d)\n", name.empty() ? address.c_str() : name.c_str(), rssi);
            M5.Display.endWrite();
        }

        // Detect HR device and stop scan
        if (name.find(HR_DEVICE_NAME) != std::string::npos) {
            Serial.printf("HR Sensor found! Stopping scan...\n");
            foundDevice = new NimBLEAdvertisedDevice(*advertisedDevice);
            NimBLEDevice::getScan()->stop();
            scanning = false;
        }
    }
};

// --- Connect and subscribe ---
bool connectToHRM() {
    
    // M5.Display.fillScreen(BLACK);
    M5.Display.fillRect(0, 120, 320, 120, BLACK);
    M5.Display.setCursor(0, 120);
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(CYAN);
    M5.Display.println("HR Sensor Found!");
    M5.Display.setTextColor(WHITE);
    M5.Display.println("Connecting...");

    pClient = NimBLEDevice::createClient();
    if (!pClient->connect(foundDevice->getAddress())) {
        M5.Display.println("Connect FAILED");
        return false;
    }
    M5.Display.println("Connected!");

    NimBLERemoteService* pService = pClient->getService(HR_SERVICE_UUID);
    if (!pService) { Serial.println("HR Service not found"); pClient->disconnect(); return false; }

    NimBLERemoteCharacteristic* pChar = pService->getCharacteristic(HR_CHAR_UUID);
    if (!pChar) { Serial.println("HR Char not found"); pClient->disconnect(); return false; }

    if (!pChar->canNotify()) { Serial.println("Cannot notify"); pClient->disconnect(); return false; }

    pChar->subscribe(true, notifyCallback);
    Serial.println("Subscribed to HR notifications");
    M5.Display.println("Reading HR...");
    return true;
}

void drawBattery() {
    int level = M5.Power.getBatteryLevel();
    bool charging = M5.Power.isCharging();

    M5.Display.fillRect(220, 0, 100, 12, BLACK); // clear top-right corner
    M5.Display.setCursor(220, 0);
    M5.Display.setTextSize(1.5);

    if (charging) {
        M5.Display.setTextColor(GREEN);
        M5.Display.printf("BAT:%d%%CHG", level);
    } else if (level > 20) {
        M5.Display.setTextColor(WHITE);
        M5.Display.printf("BAT:%d%%", level);
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.printf("BAT:%d%%LOW", level);
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);
    
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(CYAN);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("*************************", 160, 90);
    M5.Display.drawString("* ARTIST HR BROADCASTER *", 160, 110);
    M5.Display.drawString("*          v0.1         *", 160, 130);
    M5.Display.drawString("*************************", 160, 150);
    M5.Display.setTextDatum(top_left); // reset to default for everything after
    delay(2000);
    
    M5.Display.fillScreen(BLACK);

    // Connect to WiFi hotspot
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(1.5);
    M5.Display.printf("WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int wifiRetries = 0;
    while (WiFi.status() != WL_CONNECTED && wifiRetries < 20) {
        delay(500);
        Serial.print(".");
        wifiRetries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi connected: %s\n", WiFi.localIP().toString().c_str());
        M5.Display.setTextColor(GREEN);
        M5.Display.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        udp.begin(OSC_PORT);
        
        // Send test OSC message
        // OSCMessage testMsg("/test");
        // testMsg.add((int32_t)42);
        // udp.beginPacket(OSC_HOST, OSC_PORT);
        // testMsg.send(udp);
        // udp.endPacket();
        // testMsg.empty();
        // Serial.printf("OSC test sent to %s:%d\n", OSC_HOST, OSC_PORT);
        // M5.Display.println("OSC test sent!");

        // Initialize ESP-NOW (must be after WiFi connects to share same channel)
        esp_now_init();
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, espNowBroadcast, 6);
        peerInfo.channel = 0;  // 0 = use current WiFi channel
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);
        M5.Display.println("ESP-NOW ready!");        

    } else {
        Serial.println("\nWiFi connection failed");
        M5.Display.setTextColor(RED);
        M5.Display.println("Can't connect to WiFi");
    }
    M5.Display.setTextColor(WHITE);

    // Initial battery status check
    delay(500);
    drawBattery();
    delay(1000);

    // Initialize NimBLE
    NimBLEDevice::init("M5Core2_Scanner");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    
    // Using your version-specific method name
    pScan->setScanCallbacks(new MyAdvertisedDeviceCallbacks());
    
    pScan->setActiveScan(true); // Active scan gets more data (like names)
    pScan->setInterval(100);
    pScan->setWindow(99);
    
    // Scan for 0 (forever)
    pScan->start(0, false); 
}

void loop() {
    M5.update();

    // Once scan stops and device is found, connect
    if (!scanning && !connected && foundDevice != nullptr) {
        connected = connectToHRM();
    }

    // Reconnect if dropped
    if (connected && pClient && !pClient->isConnected()) {
        connected = false;
        Serial.println("Disconnected. Reconnecting...");
        M5.Display.println("Disconnected...");
        delay(2000);
        connected = connectToHRM();
    }

    // Battery status update every 60 seconds
    if (millis() - lastBatteryCheck >= 60000) { // every 60 seconds
        lastBatteryCheck = millis();
        drawBattery();
    }
}