/**
 * target
 *     * ESP32
 * dependencies
 *     * PubSubClient, ArduinoJson
 */
#include <WiFi.h>
#include <PubSubClient.h>
#include <M5Stack.h>
#include <ArduinoJson.h>

char *ssid = "aterm05-1401";
char *password = "09061271401";

// MQTTの接続先のIP
const char *endpoint = "192.168.179.7";
// MQTTのポート
const int port = 1883;

// デバイスID
char *deviceID = "M5Stack";
// トピック
char *akabekoTopic = "/sub/bigakabeko";

char blowerPin = 2;

////////////////////////////////////////////////////////////////////////////////

WiFiClient httpsClient;
PubSubClient mqttClient(httpsClient);

void setup() {
    Serial.begin(115200);
    pinMode(blowerPin, OUTPUT);

    // Initialize the M5Stack object
    M5.begin();

    // START
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.printf("START");

    // Start WiFi
    Serial.println("Connecting to ");
    Serial.print(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // WiFi Connected
    Serial.println("\nWiFi Connected.");
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("WiFi Connected.");

    mqttClient.setServer(endpoint, port);
    mqttClient.setCallback(mqttCallback);

    connectMQTT();
}

void connectMQTT() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect(deviceID)) {
            Serial.println("Connected.");
            int qos = 0;
            mqttClient.subscribe(akabekoTopic, qos);
            Serial.println("Subscribed.");
        } else {
            Serial.print("Failed. Error state=");
            Serial.print(mqttClient.state());
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

/**
 * MQTTから信号を受信
 */
void mqttCallback (char* topic, byte* payload, unsigned int length) {
    Serial.println("Recieve Topic");
    // メッセージを読み込み
    String str = "";
    for (int i = 0; i < length; i++) {
        str += (char)payload[i];
    }
    // JSONにパース
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(str);
    // パースが成功したか確認
    if (!root.success()) {
        Serial.println("ParseError");
        return;
    }

    Serial.printf("Topic: %s\n", topic);
    if (topic == "/sub/bigakabeko") {
        bool enable = root["value"];
        int time = root["time"];
        blower(enable, time);
    }
}

/**
 * ブロワーを制御
 */
void blower(bool enable, int time) {
    if (enable) {
        digitalWrite(blowerPin, HIGH);
        // time == 0 の場合は自動OFFしない
        if (time != 0) {
            delay(time);
            digitalWrite(blowerPin, LOW);
        }
    } else {
        digitalWrite(blowerPin, LOW);
    }
}

void mqttLoop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
}

void loop() {
    // 接続確認
    mqttLoop();
}
