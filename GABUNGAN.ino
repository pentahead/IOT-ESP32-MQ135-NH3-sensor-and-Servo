#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <MQ135.h>
#include <EEPROM.h>

const char* ssid = "Faruq";
const char* password = "1sampai66";
const char* serverNameStatus = "http://192.168.244.162/Monsafe/app/views/user/Pengurasan/control_status.php"; // URL endpoint untuk mengambil status
const char* serverNamePPM = "http://192.168.244.162/Monsafe/iot.php"; // URL endpoint untuk mengirim data PPM

Servo myservo;
String currentStatus = ""; // Variable to store the current status
const int MQ_PIN = 34;   // Pin analog untuk sensor MQ135       
const float RL = 10.0;   // Resistansi beban (ohm)
const float Ro = 0.54;   // Resistansi pada udara bersih (kohm)
const float m = -0.280;  // Koefisien m
const float b = 0.425;   // Koefisien b

String apiKeyValue = "12345678912";

int ledwifi = 2;

float ppm;  // Declare ppm as a global variable

void setup() {
  Serial.begin(115200);

  pinMode(ledwifi, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    digitalWrite(ledwifi, LOW);
  }

  Serial.println();
  Serial.print("Connected: ");
  Serial.println(WiFi.localIP());
  digitalWrite(ledwifi, HIGH);

  myservo.attach(14); // Pin yang digunakan untuk menghubungkan servo
  myservo.write(0); // Posisi awal servo di 0 derajat
}

void loop() {
  monitorGas();
  checkStatus();
  delay(1000); // Loop interval (1 detik)
}

void monitorGas() {
  float sensor_volt, RS_gas, RS_ratio;
  float analog_value = analogRead(MQ_PIN);  // Read analog value from the sensor
  Serial.print("Analog value: ");
  Serial.println(analog_value);

 
  sensor_volt = analog_value * (3.3 / 4095.0);  // Konversi nilai analog menjadi tegangan

  RS_gas = (RL * (3.3 - sensor_volt) / sensor_volt);
  RS_ratio = RS_gas / Ro;
  ppm = log10(RS_ratio) * m + b; // Calculate PPM (use the correct logarithmic formula)


if (ppm < 0) {
    ppm = 0;
  }
  Serial.print("PPM: ");
  Serial.println(ppm);

  
  Serial.println();
  
  sendDataToServer();
  delay(2000); // Kirim data setiap 2 detik
}


void sendDataToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverNamePPM);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String httpRequestData = "api_key=" + apiKeyValue + "&ppm_act=" + String(ppm);
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);

    int httpResponseCode = http.POST(httpRequestData);
    if (httpResponseCode == HTTP_CODE_OK) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("(http)Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void checkStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverNameStatus);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(payload);

      if (payload != currentStatus) {
        currentStatus = payload;  

        if (payload == "ON") {
          myservo.write(220); // Pindahkan servo ke 180 derajat
          Serial.println("Servo moved to 200 degrees (ON)");
        } else if (payload == "OFF") {
          myservo.write(0); // Pindahkan servo ke 0 derajat
          Serial.println("Servo moved to 0 degrees (OFF)");
        } else {
          Serial.println("Unknown status received");
        }
      }
    } else {
      Serial.print("(servo)Error on HTTP request: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}
