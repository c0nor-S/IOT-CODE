#include "DHT.h"
#define DHT_SENSOR_PIN 14
#define DHT_SENSOR_TYPE DHT11

#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024
#include "TinyGsmClient.h"

#define MODEM_RST 23
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 5
#define MODEM_TX 17
#define MODEM_RX 16
#define I2C_SDA 21
#define I2C_SCL 22

#define SerialMon Serial
#define SerialAT Serial1

#include "Wire.h"

#define RELAY_FAN_PIN 13
#define TEMP_UPPER_FAN 20
#define TEMP_LOWER_FAN 8

#define RELAY_HEAT_PIN 34
#define TEMP_UPPER_HEAT 17
#define TEMP_LOWER_HEAT 8


#define SMS_TARGET "+353851834364"

#ifdef DUMP_AT_COMMANDS
  #include "StreamDebugger.h"
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

#define IP5306_ADDR         0x75
#define IP5306_REG_SYS_CTL0  0x00

bool setPowerBoostKeepOn(int en){
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if(en){
    Wire.write(0x37);
  } else {
    Wire.write(0x35);
  }
  return Wire.endTransmission() == 0;
}

DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

const int TRIG = 27;
const int ECHO = 35;
const int LED_1 = 26;
const int LED_2 = 25;
long duration, cm;
const char simPIN[] = "";

void setup() {
  Serial.begin (115200);
  
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(RELAY_FAN_PIN, OUTPUT);
  pinMode(RELAY_HEAT_PIN, OUTPUT);

  dht.begin();


//SIM800       SMS TEXT MESSAGE//
  Wire.begin(I2C_SDA, I2C_SCL);
  bool isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAILED"));

  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  modem.restart();

  if(strlen(simPIN) && modem.getSimStatus() != 3){
    modem.simUnlock(simPIN);
  }

  String smsMessage = "SMS MESSAGE : HELLO!\n\n";
  if(modem.sendSMS(SMS_TARGET, smsMessage)){
    SerialMon.println(smsMessage);
  } else {
    SerialMon.println("SMS Failed To Send");
  }
}

void loop() {

  //HC-SR04       ULTRASONIC SENSOR STATUS//
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  pinMode(ECHO, INPUT);
  duration = pulseIn(ECHO, HIGH);

  cm = (duration/2) / 29.1;

  Serial.print("-Distance: ");
  Serial.print(cm);
  Serial.print("cm\n");
  
  //LED STATUS//
  if(cm < 20){
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, HIGH);
    Serial.print("-LED STATUS: ON\n");
  } 
  else{
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    Serial.print("-LED STATUS: OFF\n");
  }

  //DHT11       TEMP & HUMIDITY STATUS//
  float h = dht.readHumidity();
  float t = dht.readTemperature();

    Serial.print(F("•Humidity: "));
    Serial.print(h);
    Serial.print(F("%\n•Temperature: "));
    Serial.print(t);
    Serial.print(F("°C\n"));

  //DC FAN & HEATING ELEMENT//
  if (isnan(h) || isnan(t)){
    Serial.println(F("ERROR: Failed To Read From Sensor\n"));
    return;
  } else {
    if (t > TEMP_UPPER_FAN){
      Serial.println("--Fan Status: ON--\n");
      digitalWrite(RELAY_FAN_PIN, HIGH);
    } else if (t < TEMP_LOWER_FAN){
      Serial.println("--Fan Status: OFF--\n");
      digitalWrite(RELAY_FAN_PIN, LOW);
    } else if(t > TEMP_UPPER_HEAT){
      Serial.println("--Heating Status: OFF--\n");
      digitalWrite(RELAY_HEAT_PIN, LOW);
    } else if(t < TEMP_LOWER_HEAT){
      Serial.println("--Heating Status: ON--\n");
      digitalWrite(RELAY_HEAT_PIN, HIGH);
    }
  }

  delay(1000);
}