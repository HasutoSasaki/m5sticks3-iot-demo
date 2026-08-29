#include <FS.h>
#include <LittleFS.h>
#include <M5Unified.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

namespace {

WiFiClientSecure tlsClient;
PubSubClient mqttClient(tlsClient);

constexpr char AWS_IOT_CLIENT_ID[] = "m5sticks3-iot-demo";
constexpr char AWS_IOT_TELEMETRY_TOPIC[] = "m5sticks3-iot-demo/telemetry";
constexpr char AWS_IOT_COMMAND_TOPIC[] = "m5sticks3-iot-demo/command";
constexpr char AWS_IOT_STATUS_TOPIC[] = "m5sticks3-iot-demo/status";

String awsIoTEndpoint;
unsigned long lastTelemetryAt = 0;
unsigned int messageNumber = 0;

void showMessage(const char* firstLine, const char* secondLine = "") {
  M5.Display.clear();
  M5.Display.setCursor(8, 12);
  M5.Display.println(firstLine);
  M5.Display.println();
  M5.Display.println(secondLine);
}

void connectWiFi() {
  showMessage("Wi-Fi connecting", "Saved settings");
  WiFi.mode(WIFI_STA);
  WiFi.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  showMessage("Wi-Fi connected", WiFi.localIP().toString().c_str());
}

void synchronizeTime() {
  configTime(9 * 60 * 60, 0, "ntp.nict.jp", "pool.ntp.org");

  time_t now = time(nullptr);
  while (now < 1700000000) {
    delay(500);
    now = time(nullptr);
  }
}

bool loadEndpoint() {
  File endpointFile = LittleFS.open("/aws-iot/endpoint.txt", FILE_READ);
  if (!endpointFile) {
    Serial.println("endpoint file not found");
    return false;
  }

  awsIoTEndpoint = endpointFile.readString();
  endpointFile.close();
  awsIoTEndpoint.trim();

  if (awsIoTEndpoint.isEmpty()) {
    Serial.println("endpoint file is empty");
    return false;
  }
  return true;
}

bool loadTlsFiles() {
  File caFile = LittleFS.open("/aws-iot/amazon-root-ca.pem", FILE_READ);
  if (!caFile || caFile.size() == 0 || !tlsClient.loadCACert(caFile, caFile.size())) {
    Serial.println("failed to load Amazon Root CA");
    return false;
  }
  caFile.close();

  File certificateFile = LittleFS.open("/aws-iot/device-certificate.pem", FILE_READ);
  if (!certificateFile || certificateFile.size() == 0 ||
      !tlsClient.loadCertificate(certificateFile, certificateFile.size())) {
    Serial.println("failed to load device certificate");
    return false;
  }
  certificateFile.close();

  File privateKeyFile = LittleFS.open("/aws-iot/private-key.pem", FILE_READ);
  if (!privateKeyFile || privateKeyFile.size() == 0 ||
      !tlsClient.loadPrivateKey(privateKeyFile, privateKeyFile.size())) {
    Serial.println("failed to load private key");
    return false;
  }
  privateKeyFile.close();

  return true;
}

bool loadAwsIoTConfiguration() {
  if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mount failed");
    return false;
  }
  return loadEndpoint() && loadTlsFiles();
}

void publishTelemetry() {
  char payload[160];
  snprintf(payload, sizeof(payload),
           "{\"deviceId\":\"%s\",\"messageNumber\":%u,\"rssi\":%d}",
           AWS_IOT_CLIENT_ID, messageNumber++, WiFi.RSSI());

  if (mqttClient.publish(AWS_IOT_TELEMETRY_TOPIC, payload)) {
    Serial.printf("telemetry published: %s\n", payload);
    showMessage("Telemetry published", payload);
  } else {
    Serial.println("telemetry publish failed");
    showMessage("Publish failed");
  }
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  char message[161] = {};
  const unsigned int copyLength = min(length, static_cast<unsigned int>(sizeof(message) - 1));
  memcpy(message, payload, copyLength);

  Serial.printf("command received: topic=%s payload=%s\n", topic, message);
  showMessage("Command received", message);

  char status[224];
  snprintf(status, sizeof(status),
           "{\"deviceId\":\"%s\",\"event\":\"command-received\",\"messageLength\":%u}",
           AWS_IOT_CLIENT_ID, copyLength);
  mqttClient.publish(AWS_IOT_STATUS_TOPIC, status);
}

void connectMqtt() {
  while (!mqttClient.connected()) {
    showMessage("AWS IoT connecting");
    Serial.println("connecting to AWS IoT Core");

    if (mqttClient.connect(AWS_IOT_CLIENT_ID)) {
      mqttClient.subscribe(AWS_IOT_COMMAND_TOPIC);
      Serial.printf("subscribed: %s\n", AWS_IOT_COMMAND_TOPIC);
      showMessage("AWS IoT connected", AWS_IOT_COMMAND_TOPIC);
      return;
    }

    Serial.printf("MQTT connect failed: %d\n", mqttClient.state());
    delay(3000);
  }
}

}  // namespace

void setup() {
  auto config = M5.config();
  M5.begin(config);
  M5.Display.setTextSize(1);
  Serial.begin(115200);

  connectWiFi();
  synchronizeTime();

  if (!loadAwsIoTConfiguration()) {
    showMessage("AWS files failed", "Upload LittleFS data");
    while (true) {
      delay(1000);
    }
  }

  mqttClient.setServer(awsIoTEndpoint.c_str(), 8883);
  mqttClient.setCallback(onMessage);
  mqttClient.setBufferSize(512);
}

void loop() {
  if (!mqttClient.connected()) {
    connectMqtt();
  }
  mqttClient.loop();

  const unsigned long now = millis();
  if (now - lastTelemetryAt >= 10000) {
    lastTelemetryAt = now;
    publishTelemetry();
  }
}
