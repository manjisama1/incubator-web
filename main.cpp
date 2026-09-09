#include <DHT.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <AccelStepper.h>

#define DHTPIN        4
#define DHTTYPE      DHT22
#define RELAY_HEATER 18
#define RELAY_FAN    19

#define MOTOR_IN1    13
#define MOTOR_IN2    12
#define MOTOR_IN3    14
#define MOTOR_IN4    27

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

DHT dht(DHTPIN, DHTTYPE);
AccelStepper stepper(AccelStepper::HALF4WIRE, MOTOR_IN1, MOTOR_IN3, MOTOR_IN2, MOTOR_IN4);

bool mainPower = true;
bool autoTempMode = true;

float targetTempSet = 37.50;

bool fanAlwaysOn = true;
bool fanManualState = true;
unsigned long fanOnInterval = 300000;
unsigned long fanOffInterval = 60000;

bool autoTurnEnabled = true;
unsigned long turnInterval = 600000; 
unsigned long turnDurationMs = 4000;  
int turnDirection = 1;                 
float motorSpeed = 100.0;             

bool heaterState = false;
bool fanState = true;

float validTemp = -999.0;
float lastValidTemp = -999.0;
float smoothedRate = 0.0;
float heaterDutyCycle = 0.0;

float Kp = 0.80;  
float Kd = 0.40;  

unsigned long lastReadTime = 0;
unsigned long lastLogTime = 0;
unsigned long lastTurnTime = 0;
unsigned long lastFanToggleTime = 0;
unsigned long pwmWindowStart = 0;
const unsigned long PWM_WINDOW_MS = 10000;

bool manualJogActive = false;
int manualJogDir = 1;

bool autoTurnActive = false;
unsigned long turnStartMs = 0;

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic = NULL;
BLECharacteristic *pRxCharacteristic = NULL;
bool deviceConnected = false;

void stopMotor() {
  stepper.stop();
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
}

void startEggTrayTurn(int dir, float speed) {
  if (!mainPower) return;
  stepper.setMaxSpeed(speed);
  stepper.setSpeed(dir * speed);
  turnStartMs = millis();
  autoTurnActive = true;
}

void parseCommand(String cmd) {
  cmd.trim();
  if (cmd == "PWR_ON") {
    mainPower = true;
  }
  else if (cmd == "PWR_OFF") {
    mainPower = false;
    autoTempMode = false;
    heaterState = false;
    fanState = false;
    manualJogActive = false;
    autoTurnActive = false;
    stopMotor();
  }
  else if (cmd == "AUTO_ON") {
    autoTempMode = true;
    mainPower = true;
    heaterState = true;
    manualJogActive = false;
  }
  else if (cmd == "AUTO_OFF") {
    autoTempMode = false;
  }
  else if (cmd == "HEATER_ON") {
    heaterState = true;
    autoTempMode = false;
  }
  else if (cmd == "HEATER_OFF") {
    heaterState = false;
    autoTempMode = false;
  }
  else if (cmd == "FAN_ON") {
    fanState = true;
    fanAlwaysOn = true;
  }
  else if (cmd == "FAN_OFF") {
    fanState = false;
    fanAlwaysOn = false;
  }
  else if (cmd == "AUTO_TURN_ON") {
    autoTurnEnabled = true;
  }
  else if (cmd == "AUTO_TURN_OFF") {
    autoTurnEnabled = false;
  }
  else if (cmd.startsWith("SET_TEMP:")) {
    targetTempSet = cmd.substring(9).toFloat();
  }
  else if (cmd.startsWith("SET_SPEED:")) {
    motorSpeed = cmd.substring(10).toFloat();
  }
  else if (cmd.startsWith("SET_TURN_DUR:")) {
    turnDurationMs = cmd.substring(13).toInt();
  }
  else if (cmd.startsWith("SET_TURN_INT:")) {
    turnInterval = cmd.substring(13).toInt();
  }
  else if (cmd == "JOG_FWD") {
    manualJogActive = true;
    autoTurnActive = false;
    manualJogDir = 1;
  }
  else if (cmd == "JOG_REV") {
    manualJogActive = true;
    autoTurnActive = false;
    manualJogDir = -1;
  }
  else if (cmd == "JOG_STOP") {
    manualJogActive = false;
    autoTurnActive = false;
    stopMotor();
  }
  else if (cmd == "TURN_TRAY") {
    startEggTrayTurn(turnDirection, motorSpeed);
  }
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      BLEDevice::startAdvertising();
    }
};

class MyRxCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue().c_str();
      if (rxValue.length() > 0) parseCommand(rxValue);
    }
};

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(RELAY_HEATER, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);

  digitalWrite(RELAY_FAN, RELAY_ON);
  digitalWrite(RELAY_HEATER, RELAY_OFF);

  BLEDevice::init("Manjis_Incubator");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new MyRxCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  lastTurnTime = millis();
  lastFanToggleTime = millis();
  pwmWindowStart = millis();

  Serial.println("--- FILTERED PREDICTIVE CONTROL START ---");
  Serial.println("sec,temp,rate,duty,heater,fan");
}

void loop() {
  unsigned long now = millis();

  if (mainPower) {
    if (manualJogActive) {
      stepper.setMaxSpeed(motorSpeed);
      stepper.setSpeed(manualJogDir * motorSpeed);
      stepper.runSpeed();
    } else if (autoTurnActive) {
      if (now - turnStartMs < turnDurationMs) {
        stepper.runSpeed();
      } else {
        autoTurnActive = false;
        stopMotor();
      }
    } else if (autoTurnEnabled && (now - lastTurnTime >= turnInterval)) {
      lastTurnTime = now;
      startEggTrayTurn(turnDirection, motorSpeed);
    }

    if (fanAlwaysOn) {
      fanState = true;
    } else {
      if (fanState && (now - lastFanToggleTime >= fanOnInterval)) {
        fanState = false;
        lastFanToggleTime = now;
      } else if (!fanState && (now - lastFanToggleTime >= fanOffInterval)) {
        fanState = true;
        lastFanToggleTime = now;
      }
    }

    if (now - lastReadTime >= 2000) {
      float timeDeltaMin = (now - lastReadTime) / 60000.0;
      lastReadTime = now;

      float rawTemp = dht.readTemperature();
      float hum = dht.readHumidity();

      if (!isnan(rawTemp) && rawTemp > 10.0 && rawTemp < 60.0) {
        if (validTemp < -100.0 || abs(rawTemp - validTemp) < 3.0) {
          validTemp = rawTemp;

          if (lastValidTemp > -100.0) {
            float instantRate = (validTemp - lastValidTemp) / timeDeltaMin;
            smoothedRate = (0.25 * instantRate) + (0.75 * smoothedRate);
          }
          lastValidTemp = validTemp;

          if (autoTempMode) {
            float error = targetTempSet - validTemp;
            
            float pOutput = Kp * error;
            float dOutput = Kd * smoothedRate;
            
            heaterDutyCycle = 0.50 + pOutput - dOutput;

            if (validTemp >= 37.75) heaterDutyCycle = 0.0;
            if (validTemp <= 37.35) heaterDutyCycle = 1.0;

            if (heaterDutyCycle > 1.0) heaterDutyCycle = 1.0;
            if (heaterDutyCycle < 0.0) heaterDutyCycle = 0.0;
          }
        }
      }

      if (isnan(hum)) {
        hum = 0.0;
      }

      if (now - lastLogTime >= 5000) {
        lastLogTime = now;
        Serial.print(now / 1000);
        Serial.print(",");
        Serial.print(validTemp, 2);
        Serial.print(",");
        Serial.print(smoothedRate, 2);
        Serial.print(",");
        Serial.print(heaterDutyCycle, 2);
        Serial.print(",");
        Serial.print(heaterState ? 1 : 0);
        Serial.print(",");
        Serial.println(fanState ? 1 : 0);
      }

      if (deviceConnected) {
        String jsonOutput = "{\"temp\":" + String(validTemp, 2) +
                            ",\"hum\":" + String(hum, 1) +
                            ",\"rate\":" + String(smoothedRate, 2) +
                            ",\"pwr\":" + String(mainPower ? "true" : "false") +
                            ",\"autoTemp\":" + String(autoTempMode ? "true" : "false") +
                            ",\"autoTurn\":" + String(autoTurnEnabled ? "true" : "false") +
                            ",\"heater\":" + String(heaterState ? "true" : "false") +
                            ",\"fan\":" + String(fanState ? "true" : "false") + "}";
        pTxCharacteristic->setValue(jsonOutput.c_str());
        pTxCharacteristic->notify();
      }
    }

    if (autoTempMode) {
      if (now - pwmWindowStart >= PWM_WINDOW_MS) {
        pwmWindowStart = now;
      }
      unsigned long activeMs = (unsigned long)(heaterDutyCycle * PWM_WINDOW_MS);
      heaterState = ((now - pwmWindowStart) < activeMs);
    }

  } else {
    heaterState = false;
    fanState = false;
    stopMotor();
  }

  digitalWrite(RELAY_HEATER, heaterState ? RELAY_ON : RELAY_OFF);
  digitalWrite(RELAY_FAN, fanState ? RELAY_ON : RELAY_OFF);
}
