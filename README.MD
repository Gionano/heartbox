# HeartBox - Kotak Pesan Cinta Digital berbasis IoT Dengan ESP8266

<p align="center">
  <img src="src/img_heartbox" alt="Foto Proyek HeartBox" width="500"/>
</p>

**HeartBox** adalah sebuah proyek IoT (Internet of Things) yang romantis dan personal. Bayangkan sebuah kotak fisik yang bisa menerima pesan teks yang dikirim dari mana saja di seluruh dunia melalui internet. Pesan tersebut akan ditampilkan di layar LCD, lengkap dengan notifikasi lampu LED yang berkedip lembut. Ini adalah cara modern dan personal untuk mengirimkan "surat cinta" digital kepada orang terkasih.

## ✨ Fitur Utama

*   **Konfigurasi WiFi Mudah:** Menggunakan portal web (Captive Portal) untuk menghubungkan perangkat ke jaringan WiFi Anda tanpa perlu mengubah kode.
*   **Penerima Pesan MQTT:** Menerima pesan secara real-time menggunakan protokol MQTT yang ringan.
*   **Tampilan LCD 16x2:** Menampilkan pesan yang diterima dengan efek teks berjalan.
*   **Notifikasi LED:** Lampu LED akan berkedip dengan efek *fade* (redup-terang) saat ada pesan baru.
*   **Penyimpanan Pesan:** Pesan terakhir yang diterima akan disimpan di memori (EEPROM), sehingga tidak akan hilang meskipun listrik padam.
*   **Tombol Interaktif:** Satu tombol untuk menampilkan pesan baru dan menghentikan notifikasi.
*   **Fungsi Reset:** Mudah untuk menghapus konfigurasi WiFi dan memulai dari awal.
*   **Tampilan Portal Kustom:** Halaman portal konfigurasi memiliki tampilan yang menarik dan dapat disesuaikan.
*   **Logika Koneksi Ulang:** Jika gagal terhubung ke server pesan, perangkat akan mencoba beberapa kali sebelum mereset konfigurasi WiFi secara otomatis.

## ⚙️ Kebutuhan Hardware

*   ESP8266 (NodeMCU, Wemos D1 Mini, atau sejenisnya)
*   LCD 16x2
*   IC Shift Register 74HC595
*   Push Button (Tombol)
*   2x LED (warna apa saja, merah muda/merah disarankan)
*   Resistor (sesuai kebutuhan untuk LED)
*   Breadboard dan Kabel Jumper

## 📚 Kebutuhan Software & Library

1.  **[Arduino IDE](https://www.arduino.cc/en/software)**
2.  **Library (dapat diinstal melalui Library Manager di Arduino IDE):**
    *   `WiFiManager` oleh tzapu
    *   `PubSubClient` oleh Nick O'Leary
    *   `LiquidCrystal595` (Pastikan Anda memiliki library yang tepat untuk LCD dengan shift register)
    *   `EEPROM` (Sudah terpasang secara default di Arduino IDE)

## 🚀 Panduan Lengkap (Langkah-demi-Langkah)

### Langkah 1: Rakit Hardware
Hubungkan semua komponen sesuai dengan definisi pin yang ada di dalam file `project-9-final.ino`.

| Komponen          | Pin ESP8266 |
| ----------------- | ----------- |
| LCD Data Pin      | `D6` |
| LCD Latch Pin     | `D7` |
| LCD Clock Pin     | `D8` |
| Tombol            | `D4` |
| LED 1             | `D2` |
| LED 2             | `D3` |

### Langkah 2: Konfigurasi Kode (`project-9-final.ino`)
Buka file `.ino` di Arduino IDE dan ubah beberapa pengaturan di bagian atas file:

```cpp
// --- Pengaturan Portal WiFi ---
const char* ap_password = ""; // Atur password untuk AP, atau biarkan "" jika tidak ingin pakai password.
// --- Pengaturan MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
// PENTING! Ganti topik ini menjadi unik. HARUS SAMA DENGAN DI HALAMAN PENGIRIM!
const char* mqtt_topic = "(Repositori MQTT)"; // Contoh: "untuk/almei/pesan-cinta"
// --- Pengaturan Lainnya (Opsional) ---
String idleMessage = ""; // Teks berjalan saat idle. Bisa diisi link website pengirim.
const char* ap_name = "(Nama AP)"; // Nama Access Point untuk mode konfigurasi
String clientId = "(Client ID)"; // ID unik untuk koneksi ke server MQTT
```

*   **`mqtt_topic`**: Ini adalah bagian paling penting. Anda **HARUS** menggantinya dengan topik yang unik. Topik ini harus sama persis dengan yang digunakan di halaman web pengirim pesan (`index.html`). Contoh: `almei/hadiah/1` atau `untuk/pacarku/pesan-rahasia`.
*   **`ap_name`**: Ganti `(Nama AP)` dengan nama yang Anda inginkan untuk jaringan WiFi saat mode setup, misalnya `HeartBox-Almei`.
*   **`clientId`**: Ganti `(Client ID)` dengan nama unik untuk perangkat Anda, misalnya `HeartBox-Almei-Device`.

### 3. Upload Sketch
Pilih board (misal: "NodeMCU 1.0 (ESP-12E Module)") dan port yang benar di Arduino IDE, lalu upload sketch ke ESP8266 Anda.

## 🎮 Cara Penggunaan

### 1. Konfigurasi WiFi (Hanya Sekali)
*   Setelah sketch di-upload, nyalakan perangkat.
*   LCD akan menampilkan "Connecting...". Jika tidak ada jaringan WiFi yang tersimpan, perangkat akan membuat Access Point (AP) baru.
*   Gunakan ponsel atau laptop Anda untuk mencari jaringan WiFi bernama yang Anda atur di `ap_name` (misal: **"HeartBox-Almei"**).
*   Hubungkan ke jaringan tersebut. Jika Anda mengatur `ap_password`, masukkan passwordnya.
*   Sebuah halaman login (Captive Portal) akan muncul secara otomatis. Jika tidak, buka browser dan kunjungi alamat `192.168.4.1`.
*   Pilih jaringan WiFi rumah Anda dari daftar, masukkan passwordnya, lalu klik "Save".
*   Perangkat akan restart dan mencoba terhubung ke WiFi Anda. Jika berhasil, LCD akan menampilkan "HeartBox Online!".

### 2. Mengirim dan Menerima Pesan
*   Gunakan klien MQTT (seperti halaman `index.html` yang disediakan atau aplikasi lain) untuk mengirim pesan.
*   Pastikan Anda mengirim pesan ke **topik yang sama persis** dengan yang Anda atur di `mqtt_topic` dalam kode.
*   Saat pesan diterima, LCD akan menampilkan notifikasi dan kedua LED akan mulai berkedip dengan efek *fade*.

### 3. Membaca Pesan
*   Tekan tombol pada HeartBox.
*   Notifikasi akan berhenti, dan pesan yang Anda kirim akan mulai ditampilkan sebagai teks berjalan di baris kedua LCD.
*   Pesan ini akan tersimpan dan akan tetap ada bahkan setelah perangkat dimatikan dan dinyalakan kembali.

## 🛠️ Fungsi Khusus & Troubleshooting

*   **Reset Pengaturan WiFi:** Jika Anda ingin menghubungkan HeartBox ke jaringan WiFi yang berbeda, matikan perangkat. Tekan dan tahan tombol, lalu nyalakan perangkat sambil tetap menahan tombol. Pengaturan WiFi yang tersimpan akan dihapus, dan perangkat akan masuk kembali ke mode konfigurasi AP.

*   **Gagal Terhubung ke Server Pesan (MQTT):** Jika perangkat berhasil terhubung ke WiFi tetapi gagal terhubung ke server MQTT (misalnya, server sedang down atau salah konfigurasi), perangkat akan mencoba menghubungkan kembali sebanyak 5 kali. Jika masih gagal, perangkat akan menganggap ada masalah dengan koneksi internet, menghapus pengaturan WiFi, dan restart untuk memulai ulang proses konfigurasi. Ini untuk memastikan perangkat tidak "terjebak" dalam kondisi tidak bisa menerima pesan.

---


