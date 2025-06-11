#include <Arduino.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);


void lcd_i2c(String text = "", int kolom = 0, int baris = 0) {
  byte bar[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
  };
  if (text == "") {
    lcd.init();  //jika error pakai lcd.init();
    lcd.backlight();
    lcd.createChar(0, bar);
    lcd.setCursor(0, 0);
    lcd.print("Loading..");
    for (int i = 0; i < 16; i++) {
      lcd.setCursor(i, 1);
      lcd.write(byte(0));
      delay(100);
    }
    delay(50);
    lcd.clear();
  } else {
    lcd.setCursor(kolom, baris);
    lcd.print(text + "                ");
  }
}


#define TRIG_PIN A1
#define ECHO_PIN A0
int port_tombol = 8;

SoftwareSerial mySerial(13, 12);  // RX, TX untuk DFPlayer
DFRobotDFPlayerMini myDFPlayer;

int heightInt = 0;  // Variabel tinggi badan global
int buttonPressCount = 0;
bool genderSelected = false;          // Status pemilihan gender
bool isLocked = false;                // Status penguncian nilai
bool lastButtonState = HIGH;          // Status tombol sebelumnya
String category = "Tidak Diketahui";  // Kategori tinggi badan

void tinggi() {
  if (isLocked) return;  // Jangan ubah data jika terkunci

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  if (duration == 0) {
    Serial.println("0, Tidak Terdeteksi, Unlocked");
    return;
  }

  float distance = duration * 0.034 / 2;
  heightInt = (distance >= 200) ? 0 : (200 - (int)distance);

  // Kategorisasi tinggi (hanya disimpan, tidak langsung dicetak)
  if (heightInt < 150) category = "Sangat Pendek";
  else if (heightInt < 160) category = "Pendek";
  else if (heightInt < 170) category = "Ideal";
  else if (heightInt < 180) category = "Tinggi";
  else category = "Sangat Tinggi";
}

void handleButtonPress() {
  static unsigned long lastPressTime = 0;
  unsigned long currentTime = millis();
  bool buttonState = digitalRead(port_tombol);

  // Deteksi tombol ditekan (LOW) dan dilepas (HIGH)
  if (buttonState == LOW && lastButtonState == HIGH) {
    if (currentTime - lastPressTime > 300) {  // Debounce 300ms
      buttonPressCount++;

      if (buttonPressCount == 1) {
        isLocked = true;
        genderSelected = true;  // Aktifkan pemutaran suara
        // Output ke Serial Monitor
        Serial.print(heightInt);
        Serial.print(", ");
        Serial.print(category);
        Serial.print(", ");
        Serial.println(isLocked ? "Locked" : "Unlocked");
      } else if (buttonPressCount == 2) {
        isLocked = false;
        heightInt = 0;
        category = "Tidak Diketahui";  // Reset kategori
        buttonPressCount = 0;
        //Serial.println("RESET, Data Direset");
      }
      lastPressTime = currentTime;
      
    }
  }

  lastButtonState = buttonState;  // Simpan status tombol terakhir

  // Mainkan suara hanya sekali setelah tombol ditekan 2 kali
  if (genderSelected && isLocked) {
    if (heightInt < 150) {
      myDFPlayer.play(5);
    } else if (heightInt >= 150 && heightInt <= 159) {
      myDFPlayer.play(2);
    } else if (heightInt >= 160 && heightInt <= 169) {
      myDFPlayer.play(3);
    } else if (heightInt >= 170 && heightInt <= 179) {
      myDFPlayer.play(1);
    } else {
      myDFPlayer.play(4);
    }
    delay(1500);             // Tunggu beberapa detik sebelum mengubah status
    genderSelected = false;  // Reset setelah memainkan suara
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(port_tombol, INPUT_PULLUP);

  lcd_i2c();

  mySerial.begin(9600);
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("ERROR, Gagal terhubung ke DFPlayer");
  }
  myDFPlayer.volume(30);
  Serial.println("Height (cm), Category");
}


void loop() {
  tinggi();
  handleButtonPress();
  lcd_i2c("TB: " + String(heightInt), 0, 0);
  lcd_i2c(category, 0, 1);
  // int tombol = digitalRead(port_tombol);
  // Serial.println("TOMBOL : " + (String)tombol);
  delay(1500);
}
