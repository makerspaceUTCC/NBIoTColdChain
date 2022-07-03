// =============NPKpH===========//
// ----------------NB IoT AIS display ssd1327 and Grafana---------//
// --------------RS485 module -------------//
// --------------auto 485 module RED board ===//
// ---- need delay time 50 ms ---------------------//
// ----- GPS serial0---------------------------//
// ---- 11 june 2022 02:04 am --------------------------//
#include <SoftwareSerial.h>
#include "crc16.h"
#include <Wire.h>
#include <ClosedCube_HDC1080.h>
#include "AIS_SIM7020E_API.h"
#include <U8g2lib.h>
#include <TinyGPS++.h>
float GPS_lat,GPS_lng=0;
void SendNPK(unsigned int Soil_N, unsigned int Soil_P,unsigned int Soil_K,unsigned int Soil_pH);

U8G2_SSD1327_MIDAS_128X128_F_HW_I2C u8g2( // note "F_HW" version gave best results
/* No Rotation*/U8G2_R0,
/* reset=/ U8X8_PIN_NONE,
/ clock=/ 25, // use NON-default I2C pins for NodeMCU-32S
/ data=*/ 26); // data pin

//static const int RXPin = 3, TXPin = 1;
static const uint32_t GPSBaud = 4800;
// The TinyGPS++ object
TinyGPSPlus gps;
//SoftwareSerial GPss(RXPin, TXPin);

bool LCD = true;  //show LCD
SoftwareSerial SWSerial(32,33);   // (RX,TX)  //new pcbv1.0

int Count = 0;           // Save data on the RTC memory


AIS_SIM7020E_API nb;

#define MQTT_SERVER   "MQTT Server" //
#define MQTT_PORT     "1883"
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_NAME     ""

#define SQL1 "== SQL command===,"
#define SQL2 ","
#define SQL3 ",null)"
#define SID 2007  /*ใส่ SID ที่ใช้ ตั้งแต่1007-1008*/
#define SID2 1009  /*ใส่ SID ที่ใช้ ตั้งแต่1007-1009*/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}

byte Adrr=0x01, Fcode=0x03; // Fcode=0x03
//const int EnTxPin =  6;
//const int EnRxPin =  7;
//int counter = 0;
float MTemp,MHum;
byte values[11];
int regval[2];

void setup() {

  Serial.begin(9600);
  while (!Serial);
  //GPss.begin(GPSBaud);
  SWSerial.begin(9600);
 if (LCD==true){
     u8g2.begin();
     u8g2.setBusClock(10000000); // seems to work reliably, and very fast
     
 }
 
  String StrSID=String(SID);
  nb.begin();           //Init Magellan LIB

  while (nb.connectMQTT(MQTT_SERVER,MQTT_PORT,StrSID,MQTT_USERNAME,MQTT_PASSWORD)==false)
  {
    Serial.print(". ");
    delay(100);
  }
  Serial.println("MQ connectec ");

  
}
unsigned long Timer1 =millis();
unsigned long Timer2 =millis();
void loop() {

if(millis()-Timer1 >= 5000){
     Show();
     GPS_display();
  Timer1 =millis();   
}
 if(millis()-Timer2 >= 300){
    GPS_call();//GPS don't need time delay
    Timer2 =millis();    
  }
}

unsigned int modRead(byte reg){

  byte RegisterH=0x00,RegisterL=reg, len =0x01;
  byte x[6] = { Adrr, Fcode,RegisterH,RegisterL,0x00,len};
  uint16_t u16CRC=0xffff;
  for (int i = 0; i < 6; i++)
    {
        u16CRC = crc16_update(u16CRC, x[i]);

    }   //Serial.print(Adrr,HEX);Serial.print(",");Serial.print(Fcode,HEX);Serial.print(",");Serial.print(len,HEX);Serial.print(",");Serial.print(lowByte(u16CRC),HEX); Serial.print(",");
        //Serial.println(highByte(u16CRC),HEX);
  byte request[8] = { Adrr, Fcode,RegisterH,RegisterL,0x00,len, lowByte(u16CRC),highByte(u16CRC)};
  
  //digitalWrite(EnTxPin, HIGH); digitalWrite(EnRxPin, HIGH); 
  //delay(50);
  if(SWSerial.write(request,sizeof(request))==8)
  {
     //digitalWrite(EnTxPin, LOW); digitalWrite(EnRxPin, LOW); 
     delay(50);
     for(byte i=0;i<7;i++){
        
        values[i] = SWSerial.read();
        //Serial.print(" 0x");
        //Serial.print(values[i],HEX);
        
     }
     //Serial.println("");
     //Serial.flush();
  }
   unsigned int val = (values[3]<<8)| values[4];
     
   //Serial.print(val);
  return val;
}

void Show(void){

  MTemp = (165*modRead(0x00)/1650.0)-40.0;
  MHum = modRead(0x01)/10.0;

  if (LCD==true){
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.setFontDirection(0);
  //u8g2.clearBuffer();
  u8g2.setCursor(2, 22);//(พิกัดแกน x ,พิกัดแกน y)
  u8g2.print("Truck ID 001");
  u8g2.drawLine(0, 23, 127, 23); /*  - bottom*/
  
  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setFontDirection(0);
  u8g2.setCursor(0, 50);
  u8g2.print("Temp :             ");
  u8g2.setCursor(80, 50);
  u8g2.print(MTemp,1);

  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setFontDirection(0);
  u8g2.setCursor(0, 72);
  u8g2.print("Humidity%:              ");
  u8g2.setFontDirection(0);
  u8g2.setCursor(96, 72);
  u8g2.print(MHum,1);

  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setFontDirection(0);
  u8g2.setCursor(0, 92);
  u8g2.print("Lat,Lon:");
  u8g2.setCursor(20, 112);
  u8g2.print(GPS_lat);
  u8g2.setCursor(80, 112);
  u8g2.print(GPS_lng);

  u8g2.sendBuffer();
  u8g2.clearBuffer();
  delay(1000);  
}}

void GPS_display(void){
  
  GPS_lat=gps.location.lat();
  GPS_lng=gps.location.lng();
    Serial.print("\n Publish message: ");
    char sql[256];
    snprintf(sql, sizeof sql, "%s%d%s%f%s%f%s%f%s%f%s", SQL1, SID2, SQL2, GPS_lat, SQL2, GPS_lng, SQL2, MTemp, SQL2, MHum, SQL3);    
    if (nb.publish(sql, "xxxx") == true) { 
      Serial.print("Success sending: ");
      Serial.println(sql);
    } else {
      Serial.println("Fail sending");
    }
    //Serial.println(sql);
  delay(100); 

}

void GPS_call(void){
  while (Serial.available() > 0)
    if (gps.encode(Serial.read()))
      displayInfo();

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    //while(true);

  }
  }

void displayInfo()
{
  //Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    GPS_lat=gps.location.lat();
    GPS_lng=gps.location.lng();
   // Serial.print(gps.location.lat(), 6);
   // Serial.print(F(","));
    //Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}
