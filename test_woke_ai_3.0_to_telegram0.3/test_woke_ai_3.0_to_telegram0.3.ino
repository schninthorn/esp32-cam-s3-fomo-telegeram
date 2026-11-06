/*
  ESP32-S3 CAM (PSRAM)
  - รัน Edge Impulse FOMO แบบ code 2 (EloquentEsp32cam)
  - ถ่ายเป็น JPEG แล้วส่ง Telegram ด้วย HTTPClient แบบ code 1
  - ใช้เฟรมเดียวกันทั้ง inference และส่งรูป (ไม่ต้อง fmt2jpg)
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <esp_camera.h>
#include <obj_inferencing.h>
#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/edgeimpulse/fomo.h>

using eloq::camera;
using eloq::ei::fomo;

// Wi-Fi
const char* ssid     = "ROSLabStudio";
const char* password = "w973hPPaATh6MSTi";

// Telegram
#define BOT_TOKEN "9122398397:AAG_TilY1K0u0hnZ4TCNfBPlK6qDSlalWDM"
#define CHAT_ID   "8765654908"

bool photoSent = false;

// ==========================
// WiFi + Telegram test message
// ==========================
void testTelegram()
{
    Serial.println("[WiFi] Connecting...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[WiFi] Connection failed!");
        return;
    }

    Serial.print("[WiFi] Connected! IP: ");
    Serial.println(WiFi.localIP());

    WiFiClientSecure client;
    client.setInsecure();
    UniversalTelegramBot bot(BOT_TOKEN, client);

    Serial.println("[Telegram] Sending test message...");
    bool sent = bot.sendMessage(CHAT_ID, "ESP32-S3 CAM: Telegram connection test", "");
    Serial.println(sent ? "[Telegram] Message sent successfully!"
                        : "[Telegram] Message sending failed!");
}

// ==========================
// ส่ง JPEG frame ปัจจุบันไป Telegram (วิธีเดียวกับ code 1)
// ==========================
bool sendPhotoTelegram()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[Telegram] WiFi not connected!");
        return false;
    }

    camera_fb_t *fb = camera.frame;
    if (fb == nullptr || fb->len < 4)
    {
        Serial.println("[Telegram] No frame!");
        return false;
    }

    // ตรวจว่าเป็น JPEG จริง (เริ่มด้วย FF D8)
    if (fb->buf[0] != 0xFF || fb->buf[1] != 0xD8)
    {
        Serial.println("[Telegram] Frame is not JPEG! (check camera.pixformat.jpeg())");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = "https://api.telegram.org/bot";
    url += BOT_TOKEN;
    url += "/sendPhoto";

    if (!http.begin(client, url))
    {
        Serial.println("[Telegram] HTTPS connection failed!");
        return false;
    }
    http.setTimeout(15000);

    String boundary = "ESP32TelegramBoundary";

    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
    head += CHAT_ID;
    head += "\r\n";
    head += "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"photo\"; filename=\"capture.jpg\"\r\n";
    head += "Content-Type: image/jpeg\r\n\r\n";

    String tail = "\r\n--" + boundary + "--\r\n";

    size_t totalLen = head.length() + fb->len + tail.length();

    // ใช้ PSRAM ถ้ามี
    uint8_t *body = (uint8_t *)ps_malloc(totalLen);
    if (!body) body = (uint8_t *)malloc(totalLen);
    if (!body)
    {
        Serial.println("[Telegram] Memory allocation failed!");
        http.end();
        return false;
    }

    size_t pos = 0;
    memcpy(body + pos, head.c_str(), head.length());  pos += head.length();
    memcpy(body + pos, fb->buf, fb->len);             pos += fb->len;
    memcpy(body + pos, tail.c_str(), tail.length());

    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    Serial.printf("[Telegram] Sending photo (%u bytes)...\n", (unsigned)fb->len);
    int code = http.POST(body, totalLen);
    free(body);

    bool ok = false;
    if (code > 0)
    {
        Serial.printf("[Telegram] HTTP Response: %d\n", code);
        Serial.println(http.getString());
        ok = (code == 200);
        Serial.println(ok ? "[Telegram] Photo sent successfully!"
                          : "[Telegram] Photo sending failed!");
    }
    else
    {
        Serial.printf("[Telegram] HTTP Error: %s\n", http.errorToString(code).c_str());
    }

    http.end();
    return ok;
}

void setup()
{
    delay(3000);
    Serial.begin(115200);
    Serial.println("__EDGE IMPULSE FOMO (PSRAM, JPEG)__");

    Serial.printf("PSRAM found: %s\n", psramFound() ? "YES" : "NO");
    Serial.printf("PSRAM size: %u\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %u\n", ESP.getFreePsram());

    testTelegram();

    // camera settings
    camera.pinout.wroom_s3();
    camera.resolution.vga();      // ปรับได้ เช่น qvga() ถ้าอยากเร็วขึ้น
    camera.pixformat.jpeg();

    while (!camera.begin().isOk())
        Serial.println(camera.exception.toString());

    // แก้ภาพกลับหัว (vflip + hmirror = หมุน 180 องศา) และตั้งค่าสี/แสงอัตโนมัติ
    sensor_t *s = esp_camera_sensor_get();
    if (s != nullptr)
    {
        s->set_vflip(s, 1);
        s->set_hmirror(s, 1);
        s->set_whitebal(s, 1);        // auto white balance
        s->set_awb_gain(s, 1);
        s->set_exposure_ctrl(s, 1);   // auto exposure
        s->set_gain_ctrl(s, 1);       // auto gain
    }
    else
    {
        Serial.println("ERROR: Cannot get camera sensor");
    }

    Serial.println("Camera OK");
    Serial.println("Put object in front of camera");
}

void loop()
{
    // capture picture (JPEG)
    if (!camera.capture().isOk())
    {
        Serial.println(camera.exception.toString());
        return;
    }

    // run FOMO
    if (!fomo.run().isOk())
    {
        Serial.println(fomo.exception.toString());
        return;
    }

    // รอให้ auto white balance / exposure นิ่งก่อน (ข้าม ~15 เฟรมแรก)
    static int frameCount = 0;
    if (frameCount < 15) frameCount++;

    // ส่งรูปครั้งแรกครั้งเดียว (เฟรมเดียวกับที่ใช้ inference)
    if (!photoSent && frameCount >= 15)
    {
        Serial.println("[Test] Sending first camera image...");
        if (sendPhotoTelegram())
        {
            photoSent = true;
            Serial.println("[Test] Photo sent!");
        }
    }

    Serial.printf("Found %d object(s) in %dms\n", fomo.count(), fomo.benchmark.millis());

    if (!fomo.foundAnyObject())
        return;

    Serial.printf(
        "Found %s at (x = %d, y = %d) (size %d x %d). Proba is %.2f\n",
        fomo.first.label, fomo.first.x, fomo.first.y,
        fomo.first.width, fomo.first.height, fomo.first.proba);

    if (fomo.count() > 1)
    {
        fomo.forEach([](int i, bbox_t bbox) {
            Serial.printf(
                "#%d) Found %s at (x = %d, y = %d) (size %d x %d). Proba is %.2f\n",
                i + 1, bbox.label, bbox.x, bbox.y,
                bbox.width, bbox.height, bbox.proba);
        });
    }
}