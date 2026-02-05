// Libraries
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <LiquidCrystal_I2C.h>

// Wi-Fi and Firebase configuration
#define WIFI_SSID "hotspot"
#define WIFI_PASSWORD "blablabla"
#define API_KEY "AIzaSyDCr8brbhoxqbOtwgJnuxgdi53ShK4KY84"
#define DATABASE_URL "https://ble-wb-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"
#define BUILTIN_LED 10

// BLE and Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
BLEScan* pBLEScan;

// Timing and thresholds
unsigned long sendDataPrevMillis = 0;
unsigned long saldoUser = 150000;      // Default balance
int RSSI_THRESHOLD = -65;              // Signal strength threshold
bool device_found = false;
int scanTime = 1;                       // Scan duration in seconds

// LCD setup
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// BLE callback class
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getServiceUUID().toString().c_str()) {
      device_found = true;
    } else {
      device_found = false;
    }
  }
};

void setup() {
  Serial.begin(115200);
  initiate_device();
  connect_to_wifi();
  connect_to_database();
  delay(3000);
}

void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    FirebaseJson json;

    digitalWrite(BUILTIN_LED, LOW);
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Selamat Datang");

    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);

    for (int i = 0; i < foundDevices.getCount(); i++) {
      BLEAdvertisedDevice device = foundDevices.getDevice(i);
      int rssi = device.getRSSI();
      String catchuuid = device.getServiceUUID().toString().c_str();

      Serial.print("UUID: ");
      Serial.println(catchuuid);
      Serial.print("RSSI: ");
      Serial.println(rssi);

      if (rssi > RSSI_THRESHOLD && device_found) {
        if (!Firebase.RTDB.get(&fbdo, "BLE/" + catchuuid)) {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("User tidak");
          lcd.setCursor(3, 1);
          lcd.print("ditemukan!");
          digitalWrite(BUILTIN_LED, LOW);
          delay(1000);
        }

        if (Firebase.RTDB.get(&fbdo, "BLE/" + catchuuid)) {
          saldoUser = String(fbdo.to<int>()).toInt();

          if (saldoUser >= 15000) {
            json.add(catchuuid, saldoUser - 15000);
            Firebase.RTDB.updateNode(&fbdo, "BLE", &json);
            digitalWrite(BUILTIN_LED, HIGH);
            lcd.clear();
            lcd.setCursor(1, 0);
            lcd.print("Silahkan lewat");
            lcd.setCursor(1, 1);
            lcd.print("Saldo: ");
            lcd.print(saldoUser - 15000);
          } else {
            digitalWrite(BUILTIN_LED, LOW);
            lcd.clear();
            lcd.setCursor(3, 0);
            lcd.print("Saldo anda");
            lcd.setCursor(2, 1);
            lcd.print("tidak cukup!");
          }
        }
      }
    }

    pBLEScan->clearResults();
  }
}

// Initialize device
void initiate_device() {
  pinMode(BUILTIN_LED, OUTPUT);

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  lcd.begin();
  lcd.backlight();
}

// Connect to Wi-Fi
void connect_to_wifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wifi Connected!");
}

// Connect to Firebase
void connect_to_database() {
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Login success!");
    lcd.setCursor(0, 1);
    lcd.print("Database OK!");
    delay(1000);
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);
}
