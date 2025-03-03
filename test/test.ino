#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <AltSoftSerial.h>

// Khai báo LCD 16x2 với địa chỉ I2C 0x21
LiquidCrystal_I2C lcd(0x21, 16, 2);

// Khai báo cảm biến vân tay (TX -> A0, RX -> A1)
SoftwareSerial fingerSerial(A0, A1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// Khai báo Bluetooth HC-05 (TX 10, RX 11) dùng AltSoftSerial
AltSoftSerial BTSerial;

// Cấu hình Keypad 4x4
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {11, 10, 7, 6}; 
byte colPins[COLS] = {5, 4, 3, 2}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Chân Relay
const int relayPin = A2;

// Cảm biến SRF05
const int trigPin = 12;
const int echoPin = 13;

// Mật khẩu đúng
String correctPassword = "1234";  
String inputPassword = "";

// Hàm đo khoảng cách bằng SRF05
float getDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034 / 2;
    return distance; // Khoảng cách tính bằng cm
}

// Hàm kiểm tra bật/tắt LCD theo khoảng cách
void updateLCDState() {
    float distance = getDistance();
    if (distance < 200) { // Nếu khoảng cách < 200cm (2m), bật LCD
        lcd.backlight();
    } else { // Nếu xa hơn 2m, tắt LCD
        lcd.noBacklight();
    }
}

// Hàm kiểm tra mật khẩu
void checkPassword() {
    if (inputPassword == correctPassword) {
        unlockDoor();
    } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wrong Password!");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Keypad or Finger");
    }
    inputPassword = ""; // Xóa mật khẩu nhập vào
}

// Hàm mở khóa cửa
void unlockDoor() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Granted!");
    digitalWrite(relayPin, HIGH); // Bật relay (mở khóa)
    delay(3000); // Giữ relay 3s
    digitalWrite(relayPin, LOW);  // Tắt relay (khóa lại)
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Keypad or Finger");
}

// Hàm kiểm tra vân tay
void checkFingerprint() {
    if (finger.getImage() == FINGERPRINT_OK) {
        if (finger.image2Tz() == FINGERPRINT_OK && finger.fingerFastSearch() == FINGERPRINT_OK) {
            unlockDoor();
        } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Fingerprint Fail!");
            delay(2000);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Keypad or Finger");
        }
    }
}

// Hàm kiểm tra tín hiệu từ Bluetooth
void checkBluetooth() {
    if (BTSerial.available()) {  // Nếu có dữ liệu từ Bluetooth
        char command = BTSerial.read();  // Đọc ký tự
        Serial.print("Bluetooth nhận: ");
        Serial.println(command);

        if (command == 'O') {  // Nhận ký tự 'O' (Open)
            unlockDoor();
        } else if (command == 'C') {  // Nhận ký tự 'C' (Close)
            digitalWrite(relayPin, LOW);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Door Locked");
            delay(2000);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Keypad or Finger");
        }
    }
}

void setup() {
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW); 

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Keypad or Finger");

    Serial.begin(9600);
    BTSerial.begin(9600);  // Khởi động Bluetooth HC-05
    fingerSerial.begin(57600); // Khởi động vân tay

    if (finger.verifyPassword()) {
        Serial.println("Fingerprint sensor OK!");
    } else {
        Serial.println("Fingerprint sensor NOT found!");
        while (1);
    }
}

void loop() {
    updateLCDState(); // Kiểm tra khoảng cách và bật/tắt LCD

    char key = keypad.getKey();
    if (key) {
        if (key == '#') { 
            checkPassword();
        } else if (key == '*') { 
            inputPassword = "";
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Keypad or Finger");
        } else {
            inputPassword += key;
            lcd.setCursor(0, 1);
            lcd.print(inputPassword);
        }
    }

    checkFingerprint(); // Kiểm tra vân tay liên tục
    checkBluetooth();   // Kiểm tra Bluetooth để mở/đóng khóa
}
