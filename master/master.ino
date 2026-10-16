#include <BLEDevice.h>

#define SERVICE_UUID "2276d988-c975-455e-af70-df99a8a93b25"
#define CHAR_UUID    "bbb60622-862e-4bbc-9639-efdc2144c610"

const int NUM_SLAVES = 2;
const char *SLAVE_ADDR[NUM_SLAVES] = {
  "20:9b:a9:87:63:82",   // Slave 1
  "f4:65:0b:e8:77:7e"    // Slave 2
};

const int BUTTON_PIN = 14;

BLEClient *clients[NUM_SLAVES] = {nullptr, nullptr};
BLERemoteCharacteristic *chars[NUM_SLAVES] = {nullptr, nullptr};
BLEAdvertisedDevice *targets[NUM_SLAVES] = {nullptr, nullptr};

bool ledOn = false;
bool lastButton = HIGH;
unsigned long lastPress = 0;

bool isConnected(int i) {
  return clients[i] != nullptr && clients[i]->isConnected();
}

class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) {
    String addr = String(dev.getAddress().toString().c_str());
    addr.toLowerCase();
    Serial.println(addr);   // debug: ipakita ang tanang BLE device nga makit-an
    for (int i = 0; i < NUM_SLAVES; i++) {
      if (addr == SLAVE_ADDR[i] && targets[i] == nullptr) {
        Serial.print("Found Slave ");
        Serial.println(i + 1);
        targets[i] = new BLEAdvertisedDevice(dev);
      }
    }
  }
};

bool connectToSlave(int i) {
  clients[i] = BLEDevice::createClient();
  if (!clients[i]->connect(targets[i])) return false;

  BLERemoteService *svc = clients[i]->getService(SERVICE_UUID);
  if (!svc) { clients[i]->disconnect(); return false; }

  chars[i] = svc->getCharacteristic(CHAR_UUID);
  if (!chars[i]) { clients[i]->disconnect(); return false; }

  Serial.print("Connected to Slave ");
  Serial.println(i + 1);
  return true;
}

void sendCommand(int i, const char *cmd) {
  chars[i]->writeValue((uint8_t *)cmd, strlen(cmd), true);
  Serial.print("Sent to Slave ");
  Serial.print(i + 1);
  Serial.print(": ");
  Serial.println(cmd);
}

void scanAndConnect() {
  for (int i = 0; i < NUM_SLAVES; i++) {
    if (!isConnected(i)) {
      delete targets[i];
      targets[i] = nullptr;
    }
  }

  Serial.println("Scanning...");
  BLEScan *scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new ScanCallbacks());
  scan->setActiveScan(true);
  scan->start(5, false);
  scan->clearResults();

  for (int i = 0; i < NUM_SLAVES; i++) {
    if (!isConnected(i) && targets[i] != nullptr) {
      if (!connectToSlave(i)) {
        Serial.print("Failed to connect to Slave ");
        Serial.println(i + 1);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  BLEDevice::init("MASTER");
}

void loop() {
  bool allConnected = true;
  for (int i = 0; i < NUM_SLAVES; i++) {
    if (!isConnected(i)) allConnected = false;
  }
  if (!allConnected) scanAndConnect();

  bool reading = digitalRead(BUTTON_PIN);
  if (lastButton == HIGH && reading == LOW && millis() - lastPress > 250) {
    lastPress = millis();
    ledOn = !ledOn;
    for (int i = 0; i < NUM_SLAVES; i++) {
      if (isConnected(i)) sendCommand(i, ledOn ? "ON" : "OFF");
    }
  }
  lastButton = reading;
}
