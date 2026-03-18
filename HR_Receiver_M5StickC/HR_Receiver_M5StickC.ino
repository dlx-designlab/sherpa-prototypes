// M5StickC Plus2 HR Receiver
// Receives BPM via ESP-NOW from HR_Broadcaster_M5Core2
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>

const char* WIFI_SSID     = "A101ZTa-7DD8A9";
const char* WIFI_PASSWORD = "0089656a";

static uint32_t lastBatteryCheck = 0;

static int32_t currentBpm = 0;
static uint32_t lastBeatTime = 0;
static uint32_t vibrateUntil = 0;

void onDataReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    if (len != sizeof(int32_t)) return;
    currentBpm = *(int32_t*)data;

    uint16_t bgColor;
    if (currentBpm < 80) {
        bgColor = BLUE;          // resting
    } else if (currentBpm < 100) {
        bgColor = YELLOW;         // light activity
    } else if (currentBpm < 130) {
        bgColor = ORANGE;        // moderate
    } else {
        bgColor = RED;           // high intensity
    }

    M5.Display.fillScreen(bgColor);
    
    // drawHeart(160, 120, 60, M5.Display.color565(180, 0, 0));    
    
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(12);
    M5.Display.drawString(String((int)currentBpm), 160, 120);
    M5.Display.setTextDatum(top_left);

    drawBattery();

}

void drawHeart(int cx, int cy, int r, uint16_t color) {
    M5.Display.fillCircle(cx - r/2, cy - r/4, r/2, color);
    M5.Display.fillCircle(cx + r/2, cy - r/4, r/2, color);
    M5.Display.fillTriangle(cx - r, cy - r/4, cx + r, cy - r/4, cx, cy + r, color);
}

void drawBattery() {
    static int cachedLevel = -1;
    static bool cachedCharging = false;

    if (cachedLevel == -1 || millis() - lastBatteryCheck >= 60000) {
        lastBatteryCheck = millis();
        cachedLevel = M5.Power.getBatteryLevel();
        cachedCharging = M5.Power.isCharging();
    }

    M5.Display.fillRect(220, 0, 100, 12, BLACK);
    M5.Display.setCursor(220, 0);
    M5.Display.setTextSize(1.5);

    if (cachedCharging) {
        M5.Display.setTextColor(GREEN);
        M5.Display.printf("BAT:%d%%CHG", cachedLevel);
    } else if (cachedLevel > 20) {
        M5.Display.setTextColor(WHITE);
        M5.Display.printf("BAT:%d%%", cachedLevel);
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.printf("BAT:%d%%LOW", cachedLevel);
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(YELLOW);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("************************", 160, 90);
    M5.Display.drawString("* AUDIENCE HR RECEIVER *", 160, 110);
    M5.Display.drawString("*          v0.1        *", 160, 130);
    M5.Display.drawString("************************", 160, 150);
    M5.Display.setTextDatum(top_left); // reset to default for everything after
    delay(2000);
    
    M5.Display.fillScreen(BLACK);    

    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(WHITE);
    M5.Display.printf("WiFi: %s\n", WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int wifiRetries = 0;
    while (WiFi.status() != WL_CONNECTED && wifiRetries < 20) {
        delay(500);
        wifiRetries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        M5.Display.setTextColor(GREEN);
        M5.Display.printf("IP: %s\n", WiFi.localIP().toString().c_str());

        if (esp_now_init() == ESP_OK) {
            esp_now_register_recv_cb(onDataReceive);
            M5.Display.setTextColor(CYAN);
            M5.Display.println("ESP-NOW ready!");
        } else {
            M5.Display.setTextColor(RED);
            M5.Display.println("ESP-NOW init failed");
        }
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.println("WiFi failed");
    }

    delay(1000);
    M5.Display.fillScreen(BLACK);
    drawBattery();
    delay(500);

    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(20, 60);
    M5.Display.println("Waiting for HR...");
}

void loop() {
    M5.update();

    if (currentBpm > 0) {
        uint32_t beatInterval = 60000 / currentBpm; // ms between beats
        uint32_t now = millis();

        if (now - lastBeatTime >= beatInterval) {
            lastBeatTime = now;
            vibrateUntil = now + 80; // 80ms pulse per beat
        }
    
        M5.Power.setVibration(now < vibrateUntil ? 220 : 0);
    } else {
        M5.Power.setVibration(0);
    }
    
}