#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>

// Inisialisasi RTC, LCD, dan Servo ini meruoakan hasil dari kode program alat pakan otomatis
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myServo;

// Pin push button dan servo
const int selectTimePin = 2;  // Tombol untuk memilih waktu pemberian makan atau delay
const int adjustTimePin = 3;  // Tombol untuk mengatur waktu atau nilai delay
const int confirmPin = 4;     // Tombol untuk menyimpan pengaturan
const int deletePin = 5;      // Tombol untuk menghapus waktu
const int addTimePin = 6;     // Tombol untuk menambah waktu baru
const int servoPin = 9;       // Pin servo

// Array waktu pemberian makan (jam, menit)
int feedTimes[3][2];          // Waktu pemberian makan
int selectedTimeIndex = 0;    // Indeks waktu yang dipilih (0, 1, 2)
int adjustmentMode = 0;       // Mode pengaturan: 0 = jam, 1 = menit
int feedDelay = 5;            // Delay pemberian makan (dalam detik)
bool delayMode = false;       // Apakah sedang di mode pengaturan delay
bool servoActivated[3] = {false, false, false}; // Status servo aktif per waktu

// Fungsi untuk membaca data dari EEPROM
void readFeedTimesFromEEPROM() {
    for (int i = 0; i < 3; i++) {
        feedTimes[i][0] = EEPROM.read(i * 4);       // Jam (setiap waktu memerlukan 4 byte)
        feedTimes[i][1] = EEPROM.read(i * 4 + 1);   // Menit
        if (feedTimes[i][0] > 23 || feedTimes[i][1] > 59) {
            feedTimes[i][0] = -1;
            feedTimes[i][1] = -1;
        }
    }
    feedDelay = EEPROM.read(12); // Baca nilai delay
    if (feedDelay < 0 || feedDelay > 60) {
        feedDelay = 5; // Reset ke default jika nilai tidak valid
    }
}

// Fungsi untuk menyimpan data ke EEPROM
void saveFeedTimesToEEPROM() {
    for (int i = 0; i < 3; i++) {
        EEPROM.write(i * 4, feedTimes[i][0]);       // Jam
        EEPROM.write(i * 4 + 1, feedTimes[i][1]);   // Menit
    }
    EEPROM.write(12, feedDelay); // Simpan nilai delay
}

// Fungsi untuk mereset perangkat I2C
void resetI2CDevices() {
    Wire.begin();
    Wire.endTransmission(); // Mengakhiri transmisi untuk me-reset perangkat
    delay(100);  // Beri waktu perangkat untuk stabil
}

void setup() {
    // Reset perangkat I2C
    resetI2CDevices();

    // Inisialisasi LCD dan Servo
    lcd.begin(16, 2);
    lcd.backlight();
    myServo.attach(servoPin);
    myServo.write(0);

    // Konfigurasi tombol sebagai input pull-up
    pinMode(selectTimePin, INPUT_PULLUP);
    pinMode(adjustTimePin, INPUT_PULLUP);
    pinMode(confirmPin, INPUT_PULLUP);
    pinMode(deletePin, INPUT_PULLUP);
    pinMode(addTimePin, INPUT_PULLUP);

    // Inisialisasi RTC
    if (!rtc.begin()) {
        lcd.setCursor(0, 0);
        lcd.print("RTC Error!");
        while (1);
    }

    if (rtc.lostPower()) {
        rtc.adjust(DateTime(__DATE__, __TIME__));
    }

    // Baca data dari EEPROM itonmdfnm pakan otromasi
    readFeedTimesFromEEPROM();

    lcd.setCursor(0, 0);
    lcd.print("KKNR9844 UNY");
    lcd.setCursor(0, 1);
    lcd.print("ROGATE MANGIWA");
    delay(5000);
    lcd.clear();
}

void loop() {
    DateTime now = rtc.now();

    // Tampilkan waktu saat ini di LCD
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    lcd.print(now.hour());
    lcd.print(":");
    if (now.minute() < 10) lcd.print("0");
    lcd.print(now.minute());

    // Tampilkan waktu pemberian makan atau delay di LCD
    lcd.setCursor(0, 1);
    if (delayMode) {
        lcd.print("Delay: ");
        lcd.print(feedDelay);
        lcd.print(" s      ");
    } else {
        lcd.print("Set: ");
        if (feedTimes[selectedTimeIndex][0] == -1) {
            lcd.print("--:--");
        } else {
            int hour = feedTimes[selectedTimeIndex][0];
            int minute = feedTimes[selectedTimeIndex][1];
            if (hour < 10) lcd.print("0");
            lcd.print(hour);
            lcd.print(":");
            if (minute < 10) lcd.print("0");
            lcd.print(minute);
        }
    }

    // Pilih waktu pemberian makan atau mode delay
    if (digitalRead(selectTimePin) == LOW) {
        if (delayMode) {
            delayMode = false;
        } else {
            selectedTimeIndex++;
            if (selectedTimeIndex > 2) {
                selectedTimeIndex = 0;
                delayMode = true;
            }
        }
        delay(100);
    }

    // Atur waktu atau delay
    if (digitalRead(adjustTimePin) == LOW) {
        if (delayMode) {
            feedDelay++;
            if (feedDelay > 15) feedDelay = 1; // Maksimal 60 detik
        } else if (feedTimes[selectedTimeIndex][0] != -1) {
            if (adjustmentMode == 0) {
                feedTimes[selectedTimeIndex][0]++;
                if (feedTimes[selectedTimeIndex][0] > 23) feedTimes[selectedTimeIndex][0] = 0;
            } else {
                feedTimes[selectedTimeIndex][1]++;
                if (feedTimes[selectedTimeIndex][1] > 59) feedTimes[selectedTimeIndex][1] = 0;
            }
        }
        delay(250);
    }

    // Ganti mode pengaturan (jam/minute)
    if (digitalRead(confirmPin) == LOW && !delayMode) {
        adjustmentMode = !adjustmentMode;
        lcd.setCursor(0, 1);
        lcd.print(adjustmentMode == 0 ? "Set: Hour      " : "Set: Minute    ");
        delay(1000);
        lcd.clear();
    }

    // Hapus waktu pemberian makan
    if (digitalRead(deletePin) == LOW) {
        if (!delayMode) {
            feedTimes[selectedTimeIndex][0] = -1;
            feedTimes[selectedTimeIndex][1] = -1;
            saveFeedTimesToEEPROM(); // Simpan perubahan ke EEPROM
            lcd.setCursor(0, 1);
            lcd.print("Waktu Dihapus  ");
            delay(1000);
            lcd.clear();
        }
    }

    // Tambah waktu baru setelah dihapus
    if (digitalRead(addTimePin) == LOW && feedTimes[selectedTimeIndex][0] == -1 && !delayMode) {
        feedTimes[selectedTimeIndex][0] = 0;
        feedTimes[selectedTimeIndex][1] = 0;
        adjustmentMode = 0; // Mulai dari jam
        lcd.setCursor(0, 1);
        lcd.print("Tambahkan Waktu:  ");
        saveFeedTimesToEEPROM(); // Simpan perubahan ke EEPROM
        delay(1000);
        lcd.clear();
    }

    // Aktifkan servo jika waktu saat ini sesuai
    for (int i = 0; i < 3; i++) {
        if (feedTimes[i][0] == -1) continue;
        if (now.hour() == feedTimes[i][0] && now.minute() == feedTimes[i][1] && !servoActivated[i]) {
            lcd.setCursor(0, 1);
            lcd.print("Memberi makan       ");
            myServo.write(180);
            delay(feedDelay * 1000); // Gunakan nilai delay
            lcd.clear();
            myServo.write(0);
            servoActivated[i] = true;
        }
        if (now.minute() != feedTimes[i][1]) {
            servoActivated[i] = false;
        }
    }

    delay(200);
}
