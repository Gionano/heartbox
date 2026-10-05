/*
 * HeartBox v4
 * ESP8266 + MQTT + LCD 16x2 (shift register) + WiFiManager + EEPROM
 *
 * Alur:
 *   Idle           -> URL berjalan di baris 2
 *   NewMessage     -> LED berdenyut + layar berkedip, tunggu tombol ditekan
 *   ShowingMessage -> pesan berjalan di baris 2, tombol lagi untuk kembali ke Idle
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>      // Konfigurasi WiFi via web portal
#include <PubSubClient.h>     // MQTT
#include <LiquidCrystal595.h> // LCD dengan shift register
#include <EEPROM.h>

// ======================================================================================
// KONFIGURASI
// ======================================================================================
namespace cfg {
  // --- Portal WiFi ---
  constexpr const char* AP_NAME     = "HeartBox-Setup";
  constexpr const char* AP_PASSWORD = "";   // Min. 8 karakter, "" = tanpa password
  constexpr uint8_t     WIFI_CONNECT_TIMEOUT_S = 20;

  // --- MQTT ---
  constexpr const char*   MQTT_SERVER = "broker.hivemq.com";
  constexpr uint16_t      MQTT_PORT   = 1883;
  constexpr const char*   MQTT_TOPIC  = "topik-unik-anda/heartbox/pesan"; // Buat unik!
  constexpr uint8_t       MQTT_MAX_RETRIES       = 5;
  constexpr unsigned long MQTT_RETRY_INTERVAL_MS = 5000;

  // --- Tampilan & timing ---
  constexpr const char*   IDLE_TEXT = "YOUR_WEBSITE";
  constexpr unsigned long SCROLL_DELAY_MS   = 300;
  constexpr unsigned long BLINK_INTERVAL_MS = 1200;
  constexpr unsigned long LED_FADE_STEP_MS  = 30;
  constexpr unsigned long DEBOUNCE_MS       = 50;
  constexpr int           LED_FADE_AMOUNT   = 5;

  // --- EEPROM ---
  constexpr int EEPROM_SIZE         = 128;
  constexpr int EEPROM_MESSAGE_ADDR = 0;
  constexpr int MAX_MESSAGE_LEN     = EEPROM_SIZE - 1; // sisakan 1 byte untuk '\0'
}

// ======================================================================================
// PIN & LCD
// ======================================================================================
namespace pins {
  constexpr uint8_t LCD_DATA  = D6;
  constexpr uint8_t LCD_LATCH = D7;
  constexpr uint8_t LCD_CLOCK = D8;
  constexpr uint8_t BUTTON    = D4;
  constexpr uint8_t LED_1     = D2;
  constexpr uint8_t LED_2     = D3;
}

constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;

byte heartChar[8] = {
  B00000, B01010, B11111, B11111,
  B01110, B00100, B00000, B00000
};

// ======================================================================================
// CSS PORTAL WIFI (minimalis)
// ======================================================================================
constexpr const char* CUSTOM_CSS = R"rawliteral(
<style>
  * { box-sizing: border-box; }

  body {
    background-color: #f5f5f7;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    color: #1d1d1f;
    text-align: center;
    margin: 0;
    padding: 20px;
    display: flex;
    justify-content: center;
    align-items: center;
    min-height: 100vh;
  }

  .c, div {
    background-color: #ffffff;
    border-radius: 12px;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
    padding: 30px;
    width: 100%;
    max-width: 380px;
    margin: 0 auto;
    text-align: left;
  }
  div { text-align: center; }

  h1 {
    font-size: 22px;
    font-weight: 600;
    margin: 0 0 20px;
    color: #1d1d1f;
    text-align: center;
  }

  input[type="text"], input[type="password"] {
    width: 100%;
    padding: 12px;
    margin-bottom: 15px;
    border: 1px solid #d2d2d7;
    border-radius: 8px;
    background-color: #fafafa;
    font-size: 14px;
    transition: all 0.2s ease;
    outline: none;
  }
  input[type="text"]:focus, input[type="password"]:focus {
    background-color: #fff;
    border-color: #0071e3;
    box-shadow: 0 0 0 3px rgba(0, 113, 227, 0.1);
  }

  button, .b {
    width: 100%;
    padding: 12px;
    margin: 10px 0;
    border: none;
    border-radius: 8px;
    background-color: #1d1d1f;
    color: white;
    font-size: 14px;
    font-weight: 500;
    cursor: pointer;
    transition: background-color 0.2s;
  }
  button:hover, .b:hover { background-color: #3a3a3c; }

  a {
    color: #6e6e73;
    text-decoration: none;
    font-size: 12px;
    margin-top: 10px;
    display: inline-block;
  }
  a:hover { color: #1d1d1f; text-decoration: underline; }

  .q { float: right; opacity: 0.5; }
</style>
)rawliteral";

// ======================================================================================
// STATE
// ======================================================================================
enum class State : uint8_t { Idle, NewMessage, ShowingMessage };

struct Scroller {
  unsigned int  index    = 0;
  unsigned long lastTick = 0;
  void reset() { index = 0; lastTick = 0; }
};

LiquidCrystal595 lcd(pins::LCD_DATA, pins::LCD_LATCH, pins::LCD_CLOCK);
WiFiClient       espClient;
PubSubClient     mqtt(espClient);

State  state       = State::Idle;
String loveMessage = "Belum ada pesan!";

Scroller idleScroll;
Scroller messageScroll;

// LED fade
int           ledBrightness = 0;
int           fadeStep      = cfg::LED_FADE_AMOUNT;
unsigned long lastFadeTime  = 0;

// Notifikasi berkedip
bool          showingReadyScreen = true;
unsigned long lastBlinkTime      = 0;

// Retry MQTT
uint8_t       mqttRetryCount   = 0;
unsigned long lastMqttAttempt  = 0;

// ======================================================================================
// TAMPILAN LCD
// ======================================================================================
void showReadyState() {
  lcd.clear();
  lcd.print("HeartBox Online!");
  idleScroll.reset();
}

void showNewMessageNotification() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ada Pesan Nih");
  lcd.setCursor(0, 1);
  lcd.print("Untuk Kamu ");
  lcd.write(byte(0));
}

void showPressPrompt() {
  lcd.clear();
  lcd.print("HeartBox Online!");
  lcd.setCursor(0, 1);
  lcd.print("Tekan tombolnya!");
}

void showMessageHeader() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(0));
  lcd.print("PESAN  SPESIAL");
  lcd.setCursor(15, 0);
  lcd.write(byte(0));
}

// Teks berjalan generik di satu baris LCD.
void scrollRow(Scroller& s, const String& text, uint8_t row) {
  if (millis() - s.lastTick < cfg::SCROLL_DELAY_MS) return;
  s.lastTick = millis();

  const String padded = text + String("                "); // 16 spasi sebagai jeda loop
  const unsigned int len = padded.length();

  String view;
  view.reserve(LCD_COLS);
  for (uint8_t i = 0; i < LCD_COLS; i++) {
    view += padded[(s.index + i) % len];
  }
  lcd.setCursor(0, row);
  lcd.print(view);

  s.index = (s.index + 1) % len;
}

void updateNotificationBlink() {
  if (millis() - lastBlinkTime < cfg::BLINK_INTERVAL_MS) return;
  lastBlinkTime = millis();

  if (showingReadyScreen) showNewMessageNotification();
  else                    showPressPrompt();
  showingReadyScreen = !showingReadyScreen;
}

// ======================================================================================
// LED
// ======================================================================================
void setLeds(int brightness) {
  analogWrite(pins::LED_1, brightness);
  analogWrite(pins::LED_2, brightness);
}

// Efek berdenyut tanpa delay() agar MQTT & tombol tetap responsif.
void updateLedFade() {
  if (millis() - lastFadeTime < cfg::LED_FADE_STEP_MS) return;
  lastFadeTime = millis();

  setLeds(ledBrightness);
  ledBrightness = constrain(ledBrightness + fadeStep, 0, 255);
  if (ledBrightness == 0 || ledBrightness == 255) fadeStep = -fadeStep;
}

// ======================================================================================
// EEPROM
// ======================================================================================
void saveMessageToEEPROM(const String& message) {
  size_t len = message.length();
  if (len > cfg::MAX_MESSAGE_LEN) len = cfg::MAX_MESSAGE_LEN;

  for (size_t i = 0; i < len; i++) {
    EEPROM.write(cfg::EEPROM_MESSAGE_ADDR + i, message[i]);
  }
  EEPROM.write(cfg::EEPROM_MESSAGE_ADDR + len, '\0');

  Serial.println(EEPROM.commit() ? F("Pesan berhasil disimpan.") : F("Gagal menyimpan pesan."));
}

// Mengembalikan true jika ada pesan tersimpan.
bool loadMessageFromEEPROM() {
  String stored;
  for (int i = 0; i < cfg::MAX_MESSAGE_LEN; i++) {
    const uint8_t c = EEPROM.read(cfg::EEPROM_MESSAGE_ADDR + i);
    if (c == 0x00 || c == 0xFF) break; // null terminator / byte kosong
    stored += (char)c;
  }

  if (stored.length() == 0) {
    Serial.println(F("Tidak ada pesan di memori. Menggunakan pesan default."));
    return false;
  }

  loveMessage = stored;
  Serial.print(F("Pesan ditemukan di memori: "));
  Serial.println(loveMessage);
  return true;
}

// ======================================================================================
// PERPINDAHAN STATE
// ======================================================================================
void enterNewMessageState() {
  state = State::NewMessage;
  showReadyState();
  showingReadyScreen = true;
  lastBlinkTime = millis();
}

void enterShowingMessageState() {
  state = State::ShowingMessage;
  setLeds(0);
  messageScroll.reset();
  showMessageHeader();
}

void enterIdleState() {
  state = State::Idle;
  showReadyState();
}

// ======================================================================================
// TOMBOL (edge-triggered + debounce, tanpa delay)
// ======================================================================================
bool buttonJustPressed() {
  static bool wasDown = false;
  static unsigned long lastChange = 0;

  const bool isDown = (digitalRead(pins::BUTTON) == LOW);
  if (isDown != wasDown && millis() - lastChange > cfg::DEBOUNCE_MS) {
    lastChange = millis();
    wasDown = isDown;
    return isDown;
  }
  return false;
}

// ======================================================================================
// WIFI
// ======================================================================================
void resetWifiAndRestart() {
  WiFiManager wm;
  wm.resetSettings(); // Hapus kredensial WiFi tersimpan

  lcd.clear();
  lcd.print("Restarting...");
  delay(2000);
  ESP.restart();
}

void onConfigMode(WiFiManager* wm) {
  Serial.println(F("Masuk ke mode konfigurasi (AP)"));
  Serial.print(F("Nama AP: "));
  Serial.println(wm->getConfigPortalSSID());
  Serial.print(F("IP AP: "));
  Serial.println(WiFi.softAPIP());

  lcd.clear();
  lcd.print("Setup WiFi:");
  lcd.setCursor(0, 1);
  lcd.print(wm->getConfigPortalSSID());
}

void connectWifi() {
  WiFiManager wm;
  wm.setCustomHeadElement(CUSTOM_CSS);
  wm.setAPCallback(onConfigMode);
  wm.setConnectTimeout(cfg::WIFI_CONNECT_TIMEOUT_S);

  lcd.clear();
  lcd.print("Connecting...");

  if (!wm.autoConnect(cfg::AP_NAME, cfg::AP_PASSWORD)) {
    Serial.println(F("Gagal terhubung dan timeout."));
    lcd.clear();
    lcd.print("Config Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.print(F("WiFi terhubung! IP: "));
  Serial.println(WiFi.localIP());
}

// ======================================================================================
// MQTT
// ======================================================================================
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("Pesan diterima dari topik: "));
  Serial.println(topic);

  if (length == 0) return;
  if (length > cfg::MAX_MESSAGE_LEN) length = cfg::MAX_MESSAGE_LEN; // muat di EEPROM

  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print(F("Isi pesan: "));
  Serial.println(message);

  saveMessageToEEPROM(message);
  loveMessage = message;
  enterNewMessageState();
}

// Non-blocking: dipanggil tiap loop, mencoba konek ulang tiap MQTT_RETRY_INTERVAL_MS.
void maintainMqtt() {
  if (mqtt.connected()) {
    mqtt.loop();
    return;
  }
  if (WiFi.status() != WL_CONNECTED) return; // tunggu WiFi pulih, jangan hitung sebagai gagal

  const unsigned long now = millis();
  if (mqttRetryCount > 0 && now - lastMqttAttempt < cfg::MQTT_RETRY_INTERVAL_MS) return;

  // Gagal berkali-kali: reset WiFi supaya portal konfigurasi muncul lagi.
  if (mqttRetryCount >= cfg::MQTT_MAX_RETRIES) {
    Serial.println(F("Gagal terhubung ke server MQTT. Mereset WiFi dan restart..."));
    lcd.clear();
    lcd.print("Server Gagal!");
    lcd.setCursor(0, 1);
    lcd.print("Cek/Ganti WiFi");
    delay(4000);
    resetWifiAndRestart();
    return;
  }

  lastMqttAttempt = now;
  const bool showStatus = (state == State::Idle); // jangan timpa layar notifikasi/pesan

  Serial.print(F("Mencoba koneksi MQTT... (Percobaan ke-"));
  Serial.print(mqttRetryCount + 1);
  Serial.println(")");
  if (showStatus) {
    lcd.clear();
    lcd.print("Hubungkan Server");
  }

  const String clientId = "HeartBox-" + String(ESP.getChipId(), HEX);

  if (mqtt.connect(clientId.c_str())) {
    Serial.print(F("Terhubung! Berlangganan ke topik: "));
    Serial.println(cfg::MQTT_TOPIC);
    mqtt.subscribe(cfg::MQTT_TOPIC);
    mqttRetryCount = 0;
    if (showStatus) showReadyState();
  } else {
    Serial.print(F("Gagal, rc="));
    Serial.println(mqtt.state());
    mqttRetryCount++;
    if (showStatus) {
      lcd.setCursor(0, 1);
      lcd.print("Gagal! Coba lagi");
    }
  }
}

// ======================================================================================
// SETUP & LOOP
// ======================================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\nMemulai HeartBox v5..."));

  EEPROM.begin(cfg::EEPROM_SIZE);

  pinMode(pins::LED_1, OUTPUT);
  pinMode(pins::LED_2, OUTPUT);
  pinMode(pins::BUTTON, INPUT_PULLUP);

  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.createChar(0, heartChar);
  lcd.home();

  // Tombol ditekan saat boot -> reset pengaturan WiFi
  if (digitalRead(pins::BUTTON) == LOW) {
    Serial.println(F("Tombol ditekan saat boot. Menghapus pengaturan WiFi."));
    lcd.clear();
    lcd.print("Reset WiFi...");
    resetWifiAndRestart();
  }

  const bool hasStoredMessage = loadMessageFromEEPROM();

  connectWifi();

  mqtt.setServer(cfg::MQTT_SERVER, cfg::MQTT_PORT);
  mqtt.setCallback(onMqttMessage);

  // Pesan lama dianggap sebagai pesan baru saat startup
  if (hasStoredMessage) enterNewMessageState();
  else                  showReadyState();
}

void loop() {
  maintainMqtt();

  const bool pressed = buttonJustPressed();

  switch (state) {
    case State::Idle:
      scrollRow(idleScroll, cfg::IDLE_TEXT, 1);
      break;

    case State::NewMessage:
      updateLedFade();
      updateNotificationBlink();
      if (pressed) enterShowingMessageState();
      break;

    case State::ShowingMessage:
      scrollRow(messageScroll, loveMessage, 1);
      if (pressed) enterIdleState();
      break;
  }
}
