#include "DHT.h"
#define DHT_SENSOR_PIN 14
#define DHT_SENSOR_TYPE DHT11

#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024 //Defines Buffer Size For GSM Modem
#include "TinyGsmClient.h"

#define MODEM_RST 23 //Reset Pin
#define MODEM_PWKEY 4 // Power Key
#define MODEM_POWER_ON 5 //Power Pin
#define MODEM_TX 17 //Transmit Pin
#define MODEM_RX 16 //Recieve Pin
#define I2C_SDA 21 //SDA Pin
#define I2C_SCL 22 //SCL Pin

#define SerialMon Serial //Serial Monitor For Debugging
#define SerialAT Serial1 //Serial Port For Modem Communication

#include "Wire.h"

#define RELAY_FAN_PIN 13
#define TEMP_UPPER_FAN 20
#define TEMP_LOWER_FAN 8

#define RELAY_HEAT_PIN 34
#define TEMP_UPPER_HEAT 17
#define TEMP_LOWER_HEAT 8


#define SMS_TARGET "+353851834364" //Phone Number SMS Will Be Sent To

#ifdef DUMP_AT_COMMANDS
  #include "StreamDebugger.h"
  StreamDebugger debugger(SerialAT, SerialMon); //Initialises The Debugger
  TinyGsm modem(debugger); //Initialises The Debugger
#else
  TinyGsm modem(SerialAT); //Initialises GSM Modem Without Debugging
#endif

#define IP5306_ADDR 0x75 //I2C Address
#define IP5306_REG_SYS_CTL0  0x00 //Register Address

bool setPowerBoostKeepOn(int en){
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if(en){
    Wire.write(0x37); //Turns On Pwer Boost
  } else {
    Wire.write(0x35); //Turns Off Power Boost
  }
  return Wire.endTransmission() == 0; //Returns true If Successful
}

DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

const int TRIG = 27;
const int ECHO = 35;
const int LED_1 = 26;
const int LED_2 = 25;
long duration, cm;
const char simPIN[] = "";
bool fanStatus = false;
bool heaterStatus = false;

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
  bool isOk = setPowerBoostKeepOn(1); //Turns Power Boost On
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAILED"));

  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX); //Starts Communication With GSM Modem
  modem.restart(); //Restarts Modem

  if(strlen(simPIN) && modem.getSimStatus() != 3){ //Checks If SIM PIN Is Defined & SIM Is Not Unlocked
    modem.simUnlock(simPIN); //Unlocks SIM Card If Locked 
  }

  String smsMessage = "SMS MESSAGE : HELLO!\n\n"; //Message To Be Sent Via SMS
  if(modem.sendSMS(SMS_TARGET, smsMessage)){ //Sends SMS To Phone Number
    SerialMon.println(smsMessage); //Prints SMS To Serial Monitor If SMS Was Sent Successfully
  } else {
    SerialMon.println("SMS Failed To Send"); //Prints Error Message If SMS Failed 
  }
}

void loop() {

  Serial.print("\n\n--- Dynamic Smart Desk - STATUS ---\n");

  //HC-SR04       ULTRASONIC SENSOR STATUS//
  digitalWrite(TRIG, LOW); //Sends 10 MicroSecond Pulse To TRIG PIN To Initiate Distance Measuring
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  pinMode(ECHO, INPUT);
  duration = pulseIn(ECHO, HIGH); //Time In MicroSeconds

  cm = (duration/2) / 29.1; //Divided By Two Because It Measuring The Time To Travel To The Object And Back
                            //Divided By 29.1 - Converts MicroSeconds To Centimeters - Comes From The Speed Of Sound In Air (343 M/S)
  Serial.print("\n-Distance: ");
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
  if (isnan(h) || isnan(t)){ //Checks If The Temp. & Humi. Reading Are 'Not A Number' - Invalid Data'
    Serial.println(F("ERROR: Failed To Read From Sensor\n"));
    return;
  } else {
    if (t > TEMP_UPPER_FAN){
      if(!fanStatus){ //Checks If Fan Status Is OFF
        Serial.println("--Fan Status: ON--\n\n");
        digitalWrite(RELAY_FAN_PIN, HIGH); //Turn On Fan Relay
        fanStatus = true; //Updates Fan Status
      }
    } else if (t < TEMP_LOWER_FAN){
      if(fanStatus){ //Checks If Fan Status Is ON
        Serial.println("--Fan Status: OFF--\n\n");
        digitalWrite(RELAY_FAN_PIN, LOW); //Turn Off Fan Relay
        fanStatus = false; //Updates Fan Status
      }
    }
    
    if(t > TEMP_UPPER_HEAT){
      if(!heaterStatus){ //Checks If Heater Status Is OFF
        Serial.println("--Heating Status: OFF--\n\n");
        digitalWrite(RELAY_HEAT_PIN, LOW); //Turn Off HeaterRelay
        heaterStatus = false; //Updates Heater Status
      }
    } else if(t < TEMP_LOWER_HEAT){
      if(heaterStatus){ //Checks If Heater Status Is ON
        Serial.println("--Heating Status: ON--\n\n");
        digitalWrite(RELAY_HEAT_PIN, HIGH); //Turn On Heater Relay
        heaterStatus = true; //Updates Heater Status
      }
    } 
  }

  delay(2000); //Repeats Loop Every 2 Seconds
}