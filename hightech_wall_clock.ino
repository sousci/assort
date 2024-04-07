#include <Adafruit_NeoPixel.h>
#include <WiFi.h> // Wi-Fiライブラリをインクルード
#include "time.h"

#define PIN 27 // INが接続されているピンを指定
#define NUMPIXELS 74 // LEDの数を指定

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800); // 800kHzでNeoPixelを駆動

const char* ssid = "yourSSID"; // Wi-FiのSSID
const char* password = "yourPassword"; // Wi-Fiのパスワード

const char* ntpServer = "jp.pool.ntp.org"; // 日本のNTPサーバー
const long gmtOffset_sec = 3600 * 9; // GMTからのオフセット（日本はUTC+9）
const int daylightOffset_sec = 0; // 夏時間の設定はここでは0

unsigned long lastSyncTime = 0; // 最後にNTP同期を行った時刻
unsigned long lastUpdateTime = 0; // 最後に時刻を更新した時刻
const long syncInterval = 3600000; // 同期間隔（1時間）

bool colonState = false; // コロンの状態（点灯/消灯）

struct tm timeinfo; // 時刻情報を格納する構造体

// 各桁のセグメント対応（4桁目基準。1始まり）
const int digitSegments[10][18] = {
  {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}, // 0
  {6, 7, 8, 9, 10, 11}, // 1
  {4, 5, 6, 7, 8, 12, 13, 14, 15, 16, 17, 18}, // 2
  {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 17, 18}, // 3
  {1, 2, 3, 6, 7, 8, 9, 10, 11, 17, 18}, // 4
  {1, 2, 3, 4, 5, 9, 10, 11, 12, 13, 17, 18}, // 5
  {1, 2, 3, 4, 5, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}, // 6
  {4, 5, 6, 7, 8, 9, 10, 11}, // 7
  {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}, // 8
  {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 17, 18} // 9
};

void setup() {
  Serial.begin(115200);
  pixels.begin(); // NeoPixelを開始
  connectToWifi(); // Wi-Fiに接続
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // NTPサーバー設定
  getLocalTime(&timeinfo); // 初期時刻の取得
  lastSyncTime = millis(); // 初期同期時刻を設定
}

void loop() {
  unsigned long currentTime = millis();
  
  // 毎秒更新処理
  if (currentTime - lastUpdateTime >= 1000) {
    lastUpdateTime = currentTime;
    timeinfo.tm_sec += 1;
    mktime(&timeinfo); // timeinfoを正規化

    // 毎正時にNTP同期
    if (timeinfo.tm_min == 0 && timeinfo.tm_sec == 0 && currentTime - lastSyncTime >= syncInterval) {
      getLocalTime(&timeinfo);
      lastSyncTime = currentTime;
    }

    // コロンの点滅処理
    colonState = !colonState;
    showTime(timeinfo.tm_hour, timeinfo.tm_min, colonState);
  }
}

void showTime(int hour, int minute, bool colonState) {
  pixels.clear(); // NeoPixelの出力をリセット
  int numberToShow = hour * 100 + minute; // 4桁の数値として結合
  showNumber(numberToShow);
  lightColon(colonState); // コロンの制御
  pixels.show(); // LEDに色を反映
}

void showNumber(int num) {
  int digits[4] = {num / 1000, (num / 100 % 10), (num / 10 % 10), num % 10};
  int offsets[4] = {0, 18, 38, 56}; // 各桁の開始LED番号

  for (int digit = 0; digit < 4; digit++) {
    int number = digits[digit];
    for (int seg = 0; digitSegments[number][seg] != 0 && seg < 18; seg++) {
      lightSegment(offsets[digit] + digitSegments[number][seg] - 1);
    }
  }
}

void lightSegment(int index) {
  pixels.setPixelColor(index, pixels.Color(255, 255, 255)); // 白色で点灯
}

void lightColon(bool state) {
  int colonPixels[2] = {37, 38}; // コロンのLED位置
  for (int i = 0; i < 2; i++) {
    pixels.setPixelColor(colonPixels[i] - 1, state ? pixels.Color(255, 255, 255) : pixels.Color(0, 0, 0));
  }
}

void connectToWifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to the WiFi network");
}
