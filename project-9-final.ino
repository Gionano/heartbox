#include <Arduino.h>
#include "ESP8266WiFi/src/ESP8266WiFi.h"
#include "ESP8266WebServer/src/ESP8266WebServer.h"
#include "DNSServer/src/DNSServer.h"
#include "WiFiManager/WiFiManager.h"      // Library untuk konfigurasi WiFi via web portal
#include "PubSubClient/src/PubSubClient.h"     // Library untuk MQTT
#include "LiquidCrystal595/LiquidCrystal595.h" // Library untuk LCD dengan shift register
#include "EEPROM/EEPROM.h"           // Library EEPROM

// ======================================================================================
// --- PENGATURAN YANG BISA DIUBAH ---
// ======================================================================================

// --- Pengaturan Portal WiFi ---
// Password untuk Access Point "HeartBox-Setup" saat mode konfigurasi.
// Minimal 8 karakter. Atur menjadi "" jika tidak ingin menggunakan password.
// Password rumah kamu, atau bisa menggunakan hotspot dari Handphone
const char* ap_password = "";

// --- Pengaturan MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
// BUAT TOPIK UNIK ANDA! Hindari penggunaan topik umum.
// Contoh: "almei/heartbox/123xyz" atau "hadiahUntukAlmei/pesan"
// PERINGATAN : HARUS SESUAI DENGAN DI PAGE INDEX.HTML!
const char* mqtt_topic = "(Repositori MQTT)";

// ======================================================================================
// --- Definisi Pin (Tidak perlu diubah) ---
// ======================================================================================
const int LCD_DATA_PIN = D6;
const int LCD_LATCH_PIN = D7;
const int LCD_CLOCK_PIN = D8;
const int BUTTON_PIN = D4;
const int LED_PIN_1 = D2;
const int LED_PIN_2 = D3;

// ======================================================================================
// --- GAYA TAMPILAN (CSS) UNTUK PORTAL WIFI (VERSI FINAL) ---
// ======================================================================================
const char* custom_css = R"rawliteral(
<style>
/* Import Font */
@import url('https://fonts.googleapis.com/css2?family=Poppins:wght@400;700&display=swap');

/* General Body Styling */
body {
  background: linear-gradient(to right, #ffdde1, #ee9ca7);
  font-family: 'Poppins', sans-serif;
  color: #4A4A4A;
  text-align: center;
  margin: 0;
  padding: 20px;
}

/* Main content container styling */
.c, div {
  border: none;
  border-radius: 15px;
  background-color: rgba(255, 255, 255, 0.85);
  box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);
  backdrop-filter: blur(5px);
  -webkit-backdrop-filter: blur(5px);
  padding: 30px;
  max-width: 400px;
  margin: 20px auto;
}

/* Header styling */
h1 {
  font-size: 2.5em;
  color: #d6336c;
  margin-bottom: 10px;
}
h1::before { content: '❤️ '; }
h1::after { content: ' ❤️'; }

/* Paragraph & general text styling */
p {
  font-size: 1.1em;
  margin-bottom: 25px;
}

/* Input field styling */
input[type="text"], input[type="password"] {
  width: calc(100% - 22px);
  padding: 12px 10px;
  margin-bottom: 15px;
  border: 1px solid #ddd;
  border-radius: 8px;
  background-color: #fff;
  font-size: 1em;
  transition: box-shadow 0.3s;
}
input[type="text"]:focus, input[type="password"]:focus {
  outline: none;
  box-shadow: 0 0 8px #d6336c;
}

/* Button styling (targets default buttons and custom class .b) */
button, .b {
  width: 100%;
  padding: 14px;
  margin-bottom: 10px; /* Added margin for spacing between buttons */
  border: none;
  border-radius: 8px;
  background-color: #d6336c;
  color: white;
  font-size: 1.2em;
  font-weight: bold;
  cursor: pointer;
  transition: background-color 0.3s, transform 0.2s;
  text-decoration: none; /* Remove underline from button links */
  display: inline-block; /* Ensure proper rendering */
  box-sizing: border-box; /* Ensure padding doesn't break layout */
}
button:hover, .b:hover {
  background-color: #a61e4d;
  transform: translateY(-2px);
}

/* Link styling */
a {
  color: #a61e4d;
  text-decoration: none;
  font-weight: bold;
}
a:hover {
  text-decoration: underline;
}

/* WiFi Quality icon styling */
.q {
  float: right;
  height: 16px;
  margin-top: -10px;
}
</style>
)rawliteral";


// --- Variabel Global ---
LiquidCrystal595 lcd(LCD_DATA_PIN, LCD_LATCH_PIN, LCD_CLOCK_PIN);
const int SCREEN_WIDTH = 16;
const int SCREEN_HEIGHT = 2;

const int EEPROM_MESSAGE_ADDR = 0; // Alamat awal untuk pesan di EEPROM
const int EEPROM_SIZE = 128;       // Ukuran total EEPROM

String loveMessage = "Belum ada pesan!"; // Pesan default
bool newMessageReceived = false;
bool isDisplayingMessage = false;

int scrollIndex = 0;
unsigned long lastScrollTime = 0;
const int SCROLL_DELAY = 300;

// Variabel untuk teks berjalan saat idle
// Anda bisa memasukkan link website yang untuk index.html
// Contoh: String idleMessage = "https://example.com/for-someone";
String idleMessage = "";
int idleScrollIndex = 0;
unsigned long lastIdleScrollTime = 0;

int ledBrightness = 0;
int fadeAmount = 5;

unsigned long lastBlinkTime = 0;
const int BLINK_INTERVAL = 1200;
bool isShowingReadyState = true;

// Variabel untuk logika retry MQTT
int mqtt_retry_count = 0;
const int MQTT_MAX_RETRIES = 5; // Batas maksimal percobaan koneksi MQTT

byte heart[8] = {
  B00000,
  B01010,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};

// --- Inisialisasi Klien WiFi & MQTT ---
WiFiClient espClient;
PubSubClient client(espClient);

// --- Deklarasi Fungsi ---
void handleLedFade();
void displayScrollingMessage();
void displayIdleScroll();
void showReadyState();
void showNewMessageNotification();
void handleBlinkingNotification();
void loadMessageFromEEPROM();
void saveMessageToEEPROM(String message);
void setup_wifi_manager();
void configModeCallback(WiFiManager *myWiFiManager);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void mqtt_reconnect();

// --- Fungsi Setup Utama ---
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nMemulai HeartBox v4 (MQTT Retry Logic)...");

  EEPROM.begin(EEPROM_SIZE);

  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  lcd.begin(SCREEN_WIDTH, SCREEN_HEIGHT);
  lcd.createChar(0, heart);
  lcd.home();

  // Cek apakah tombol ditekan saat boot untuk mereset pengaturan WiFi
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Tombol ditekan saat boot. Menghapus pengaturan WiFi.");
    lcd.clear();
    lcd.print("Reset WiFi...");
    WiFiManager wifiManager;
    wifiManager.resetSettings(); // Menghapus pengaturan WiFi yang tersimpan
    Serial.println("Pengaturan WiFi dihapus.");
    lcd.clear();
    lcd.print("Restarting...");
    delay(2000);
    ESP.restart();
  }

  loadMessageFromEEPROM(); // Coba muat pesan lama
    
  setup_wifi_manager(); // Mulai WiFi Manager untuk koneksi

  // Konfigurasi koneksi MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  if (!newMessageReceived) {
    showReadyState();
  } else {
    showReadyState();
    isShowingReadyState = true;
    lastBlinkTime = millis();
  }
}

// --- Fungsi Loop Utama ---
void loop() {
  // Jaga koneksi MQTT tetap hidup
  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop(); // Penting! Proses pesan masuk MQTT

  int buttonState = digitalRead(BUTTON_PIN);

  if (newMessageReceived) {
    handleLedFade();
    handleBlinkingNotification();

    if (buttonState == LOW) {
      Serial.println("Tombol ditekan. Menampilkan pesan.");
      newMessageReceived = false;
      isDisplayingMessage = true;
      scrollIndex = 0;
      lastScrollTime = millis();
      digitalWrite(LED_PIN_1, LOW);
      digitalWrite(LED_PIN_2, LOW);
      analogWrite(LED_PIN_1, 0); // Pastikan LED mati
      analogWrite(LED_PIN_2, 0);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
      lcd.print("PESAN  SPESIAL");
      lcd.setCursor(15, 0);
      lcd.write(byte(0));
      delay(300); // Debounce
    }
  } else if (isDisplayingMessage) {
    displayScrollingMessage();
    if (buttonState == LOW) {
      Serial.println("Tombol ditekan lagi. Menghentikan pesan.");
      isDisplayingMessage = false;
      showReadyState();
      delay(300); // Debounce
    }
  } else {
    // Kondisi idle: tidak ada pesan baru, tidak sedang menampilkan pesan.
    // Tampilkan URL berjalan.
    displayIdleScroll();
  }
}


// --- Fungsi-Fungsi Pendukung ---

/**
 * @brief Menjalankan WiFi Manager untuk konfigurasi dan koneksi WiFi.
 */
void setup_wifi_manager() {
  WiFiManager wifiManager;

  // =================================================================
  // === INI BAGIAN PENTING UNTUK MENAMBAHKAN GAYA TAMPILAN (CSS) ===
  // =================================================================
  wifiManager.setCustomHeadElement(custom_css);
  // =================================================================

  // Menampilkan info di LCD saat masuk mode konfigurasi
  wifiManager.setAPCallback(configModeCallback);
    
  // Mengatur timeout koneksi, jika gagal masuk mode AP
  wifiManager.setConnectTimeout(20);

  // Nama Access Point yang akan dibuat
  // Ganti dengan nama AP yang diinginkan
  // NOTE : AP INI HANYA UNTUK MEMINTA CREDENTIALS WIFI KAMU, UNTUK MENGONLINE-KAN HEARTBOX
  const char* ap_name = "(Nama AP)";

  lcd.clear();
  lcd.print("Connecting...");
    
  // Mencoba menghubungkan. Jika gagal, akan memulai AP dengan password yang sudah diatur.
  if (!wifiManager.autoConnect(ap_name, ap_password)) {
    Serial.println("Gagal terhubung dan timeout.");
    lcd.clear();
    lcd.print("Config Failed!");
    lcd.setCursor(0,1);
    lcd.print("Restarting...");
    delay(3000);
    ESP.restart();
  }

  // Jika berhasil terhubung:
  Serial.println("\nWiFi terhubung!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
  showReadyState(); // Tampilkan status siap di LCD
}

/**
 * @brief Fungsi callback yang dipanggil saat WiFiManager masuk ke mode AP.
 * @param myWiFiManager Pointer ke instance WiFiManager.
 */
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Masuk ke mode konfigurasi (AP)");
  Serial.print("Nama AP: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.print("IP AP: ");
  Serial.println(WiFi.softAPIP());

  lcd.clear();
  lcd.print("Setup WiFi:");
  lcd.setCursor(0, 1);
  lcd.print(myWiFiManager->getConfigPortalSSID());
}


/**
 * @brief Fungsi ini dipanggil setiap kali ada pesan masuk dari MQTT.
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Pesan diterima dari topik: ");
  Serial.println(topic);

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Isi pesan: ");
  Serial.println(message);

  if (message.length() > 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pesan Diterima!");
    lcd.setCursor(0, 1);
    lcd.print("Menyimpan...");
    delay(50);

    saveMessageToEEPROM(message);

    loveMessage = message;
    newMessageReceived = true;
    isDisplayingMessage = false;
      
    showReadyState();
    isShowingReadyState = true;
    lastBlinkTime = millis();
  }
}

/**
 * @brief Menghubungkan kembali ke broker MQTT jika koneksi terputus.
 * Jika gagal 5x, akan mereset pengaturan WiFi dan restart.
 */
void mqtt_reconnect() {
  while (!client.connected()) {
    // PERUBAHAN DIMULAI DI SINI
    // Cek apakah sudah melebihi batas percobaan
    if (mqtt_retry_count >= MQTT_MAX_RETRIES) {
      Serial.println("Gagal terhubung ke server MQTT setelah beberapa kali percobaan.");
      Serial.println("Mereset pengaturan WiFi dan memulai ulang...");

      lcd.clear();
      lcd.print("Server Gagal!");
      lcd.setCursor(0, 1);
      lcd.print("Cek/Ganti WiFi");
      delay(4000); // Tampilkan pesan sebentar

      WiFiManager wifiManager;
      wifiManager.resetSettings(); // Hapus kredensial WiFi yang tersimpan
      
      lcd.clear();
      lcd.print("Restarting...");
      delay(2000);
      ESP.restart(); // Restart ESP. Ini akan memaksa WiFiManager berjalan lagi saat setup.
    }
    // PERUBAHAN SELESAI DI SINI

    Serial.print("Mencoba koneksi MQTT... (Percobaan ke-");
    Serial.print(mqtt_retry_count + 1);
    Serial.println(")");
    lcd.clear();
    lcd.print("Hubungkan Server");

    String clientId = "(Client ID)";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("terhubung!");
      client.subscribe(mqtt_topic);
      Serial.print("Berlangganan ke topik: ");
      Serial.println(mqtt_topic);
      showReadyState();
      mqtt_retry_count = 0; // PENTING: Reset penghitung jika berhasil
    } else {
      Serial.print("gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik");
      lcd.setCursor(0,1);
      lcd.print("Gagal! Coba lagi");
      
      mqtt_retry_count++; // Tambah penghitung jika gagal
      
      delay(5000);
    }
  }
}


/**
 * @brief Menyimpan string pesan ke dalam memori EEPROM.
 * Diperbaiki agar tidak menghapus seluruh EEPROM.
 */
void saveMessageToEEPROM(String message) {
  Serial.println("Menyimpan pesan ke EEPROM...");
  // Tulis karakter pesan
  for (unsigned int i = 0; i < message.length() && i < (EEPROM_SIZE - 1); i++) {
    EEPROM.write(EEPROM_MESSAGE_ADDR + i, message[i]);
  }
  // Tulis karakter null terminator untuk menandai akhir string
  EEPROM.write(EEPROM_MESSAGE_ADDR + message.length(), '\0');

  if (EEPROM.commit()) {
    Serial.println("Pesan berhasil disimpan.");
  } else {
    Serial.println("Gagal menyimpan pesan.");
  }
}

/**
 * @brief Memuat pesan dari EEPROM saat startup.
 */
void loadMessageFromEEPROM() {
  Serial.println("Mencoba memuat pesan dari EEPROM...");
  String storedMessage = "";
  char c;
  for (int i = 0; i < (EEPROM_SIZE - 1); i++) {
    c = EEPROM.read(EEPROM_MESSAGE_ADDR + i);
    if (c == '\0' || c == 255) { // Berhenti jika menemukan null terminator atau byte kosong
      break;
    }
    storedMessage += c;
  }

  if (storedMessage.length() > 0) {
    loveMessage = storedMessage;
    newMessageReceived = true; // Anggap pesan lama sebagai pesan baru saat startup
    Serial.print("Pesan ditemukan di memori: ");
    Serial.println(loveMessage);
  } else {
    Serial.println("Tidak ada pesan di memori. Menggunakan pesan default.");
  }
}

/**
 * @brief Menangani efek berkedip pelan (pulsating fade) untuk LED notifikasi.
 */
void handleLedFade() {
  analogWrite(LED_PIN_1, ledBrightness);
  analogWrite(LED_PIN_2, ledBrightness);

  ledBrightness += fadeAmount;

  if (ledBrightness <= 0 || ledBrightness >= 255) {
    fadeAmount = -fadeAmount;
  }
  delay(30);
}

/**
 * @brief Mengelola pergeseran pesan di baris kedua LCD.
 */
void displayScrollingMessage() {
  if (millis() - lastScrollTime >= SCROLL_DELAY) {
    lastScrollTime = millis();
    String scrollableMessage = loveMessage + "                "; // 16 spasi
    lcd.setCursor(0, 1);
    String displayString = "";
    for (int i = 0; i < SCREEN_WIDTH; i++) {
      displayString += scrollableMessage[(scrollIndex + i) % scrollableMessage.length()];
    }
    lcd.print(displayString);
    scrollIndex++;
    if (scrollIndex >= scrollableMessage.length()) {
      scrollIndex = 0;
    }
  }
}

/**
 * @brief Mengelola pergeseran URL di baris kedua LCD saat idle.
 */
void displayIdleScroll() {
  if (millis() - lastIdleScrollTime >= SCROLL_DELAY) {
    lastIdleScrollTime = millis();
    
    String scrollableMessage = idleMessage + "                "; // Padding untuk loop
    lcd.setCursor(0, 1);
    String displayString = "";
    for (int i = 0; i < SCREEN_WIDTH; i++) {
      displayString += scrollableMessage[(idleScrollIndex + i) % scrollableMessage.length()];
    }
    lcd.print(displayString);
    
    idleScrollIndex++;
    if (idleScrollIndex >= scrollableMessage.length()) {
      idleScrollIndex = 0;
    }
  }
}


/**
 * @brief Menyiapkan layar untuk status "siap" (idle).
 */
void showReadyState() {
  lcd.clear();
  lcd.print("HeartBox Online!");
  // Mereset posisi scroll untuk teks berjalan saat idle
  idleScrollIndex = 0;
  lastIdleScrollTime = 0;
}

/**
 * @brief Menampilkan notifikasi ada pesan baru di LCD.
 */
void showNewMessageNotification() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ada Pesan Nih");
  lcd.setCursor(0, 1);
  lcd.print("Untuk Kamu! ");
  lcd.write(byte(0));
}

/**
 * @brief Mengelola layar notifikasi yang berkedip.
 */
void handleBlinkingNotification() {
  if (millis() - lastBlinkTime >= BLINK_INTERVAL) {
    lastBlinkTime = millis();
    if (isShowingReadyState) {
      showNewMessageNotification();
      isShowingReadyState = false;
    } else {
      // Menampilkan pesan statis saat berkedip, bukan memanggil showReadyState()
      lcd.clear();
      lcd.print("HeartBox Online!");
      lcd.setCursor(0, 1);
      lcd.print("Tekan tombolnya!");
      isShowingReadyState = true;
    }
  }
}
