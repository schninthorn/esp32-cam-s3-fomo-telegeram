
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// ใส่ชื่อ Wi-Fi ของคุณ
const char* ssid = "ROSLabStudio";
const char* password = "w973hPPaATh6MSTi";

// ใส่ Bot Token ที่ได้จาก BotFather
#define BOT_TOKEN "9122398397:AAG_TilY1K0u0hnZ4TCNfBPlK6qDSlalWDM"

// Chat ID ของคุณ
#define CHAT_ID "8765654908"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");

  // ใช้สำหรับทดสอบ HTTPS
  client.setInsecure();

  bool sent = bot.sendMessage(
    CHAT_ID,
    "Hello from ESP32-S3 CAM!",
    ""
  );

  if (sent) {
    Serial.println("Telegram message sent!");
  } else {
    Serial.println("Telegram send failed!");
  }
}

void loop() {
}
