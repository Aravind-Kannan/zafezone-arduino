#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <SoftwareSerial.h>
#include<ArduinoJson.h>
#include <TinyGPS.h>
#define I2C_ADDRESS 0x3C
#define RST_PIN -1

SSD1306AsciiAvrI2c display;
TinyGPS gps;
SoftwareSerial gsm(2, 3);
SoftwareSerial gpsSerial(6, 7);

String Date_Time;
String Date;
String Time;

float gpslat, gpslon;
unsigned long age;
int year;
byte month, day, hour, minute, second, hundredths;
int led = 13;
const int buzzer = 11;
int sizeOfData;
int m = 0, flag = 1, f1 = 0;
String phoneNum = "6384354006";
boolean emg = false;

void setup() {
  Serial.begin(9600);
#if RST_PIN >= 0
  display.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  display.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0
  display.setFont(Adafruit5x7);
  display.clear();
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, HIGH);
  pinMode(led, OUTPUT);
  gsm.begin(9600);
  gpsSerial.begin(9600);
  gsm.listen();
  display.print(F("\n"));
  display.set2X();
  display.print(F("-----------"));
  display.print(F("\n<Zafe-Zone>\n"));
  display.print(F("___________"));
  delay(2000);
  exec("AT+HTTPTERM");
  initGSM();
  initGPRS();
  exec("AT+CIFSR");
  initHTTP();
  display.clear();

}

void loop() {

  digitalWrite(buzzer, HIGH);
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5 / 1023.0);
  gpsSerial.listen();
  while (gpsSerial.available()) {
    int c = gpsSerial.read();
    if (gps.encode(c))
    {
      gps.f_get_position(&gpslat, &gpslon);
      gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    }
  }
  Date_Time = String(year) + "-" + String(month) + "-" + String(day) + " " + String(hour) + ":" + String(minute) + ":" + String(second) ;
  Date = "  " + String(day) + "/" + String(month) + "/" + String(year);
  Time = "  " + String(hour) + ":" + String(minute) + ":" + String(second);

  if (m != int(minute)) {
    flag = 1;
  }
  String sendData;
  if (voltage > 4.45 && f1 == 1) {

    DynamicJsonBuffer jBuffer;
    JsonObject& root = jBuffer.createObject();
    root["phone_number"] = phoneNum;
    root["longitude"] = String(gpslon, 6);
    root["latitude"] = String(gpslat, 6);
    root["emergency"] = true;
    root["sent_at"] = Date_Time;
    root.printTo(sendData);
    sizeOfData = sendData.length();
    display.clear();
    digitalWrite(led, HIGH);
    digitalWrite(buzzer, LOW);
    gsm.listen();
    display.set2X();
    display.println(F(""));
    display.println(F(" Button ON"));
    display.println(F(" Sending"));
    Serial.println("HTTP POST ...");
    postHTTP(root, sendData);
    MakeCall();
    HangupCall();

  }

  if (minute % 2 == 1 && flag == 1) {
    flag = 0;
    DynamicJsonBuffer jBuffer;
    JsonObject& root = jBuffer.createObject();
    m = int(minute);
    digitalWrite(led, HIGH);
    display.set1X();
    root["phone_number"] = phoneNum;
    root["longitude"] = String(gpslon, 6);
    root["latitude"] = String(gpslat, 6);
    root["emergency"] = false;
    root["sent_at"] = Date_Time;
    root.printTo(sendData);
    sizeOfData = sendData.length();
    gsm.listen();
    postHTTP(root, sendData);
  }


  else {
    f1 = 1;
    display.clear();
    display.set1X();
    display.println("\n");
    display.print(Date);
    display.set2X();
    display.print("\n");
    display.print(Time);
    display.println(F("\n"));
    display.set1X();
    display.print(     voltage);
    digitalWrite(led, LOW);
    delay(500);
  }

}


void initGSM() {
  display.clear();
  display.set1X();
  display.print("\n");
  display.println(F("Executing Setup:-\n"));
  display.println(F("GSM  Initialization"));
  connectGSM("AT", "OK");
  connectGSM("ATE1", "OK");
  connectGSM("AT+CPIN?", "READY");
}

void initGPRS() {
  display.println(F("GPRS Initialization"));
  connectGSM("AT+CIPSHUT", "OK");
  connectGSM("AT+CGATT=1", "OK");
  connectGSM("AT+CSTT=\"www\",\"\",\"\"", "OK");
  connectGSM("AT+CIICR", "OK");



}


void initHTTP() {
  display.println(F("HTTP Initialization"));
  connectGSM("AT+SAPBR=3,1,\"APN\",\"CMNET\"", "OK");
  connectGSM("AT+SAPBR=1,1", "OK");
  connectGSM("AT+HTTPINIT", "OK");
  connectGSM("AT+HTTPPARA=\"CID\",1", "OK");
  connectGSM("AT+HTTPPARA= \"URL\",\"zafezone-flask.herokuapp.com/locs\"", "OK");
}

void postHTTP(JsonObject& root, String sendData) {
  connectGSM("AT+HTTPDATA=" + String(sizeOfData) + ",10000", "DOWNLOAD");
  display.print(F(" ."));
  delay(3000);
  //connectGSM(sendData, "OK");
  root.prettyPrintTo(Serial);
  Serial.println();
  gsm.println(sendData);
  display.print(F(" ."));
  delay(1000);
  connectGSM("AT+HTTPACTION=1", "OK");
  display.print(F(" ."));
  delay(10000);
  String c;
  c = gsm.readString();
  Serial.println(c);
  //display.set1X();
  delay(1000);
  display.print(     c.substring(41, 44));
  delay(1500);

  sizeOfData = 0;
}

void connectGSM (String cmd, char* res)
{
  while (1)
  {
    if (cmd == "AT+SAPBR=1,1") {
      exec("AT+SAPBR=0,1");
      delay(1000);
    }
    Serial.println(cmd);
    gsm.println(cmd);
    delay(1000);
    while (gsm.available() > 0)
    {
      if (gsm.find(res))
      {
        delay(1000);
        return;
      }
    }
    delay(1000);
  }

}

void exec(String s) {

  gsm.println(s);
  delay(3000);
  Serial.println(gsm.readString());
}

void MakeCall()
{
  gsm.println("ATD+919445886543;");

  Serial.println("MakeCall");
  delay(8000);
}

void HangupCall()
{
  gsm.println("ATH");
  Serial.println("Hangup Call");
  delay(1000);
}
