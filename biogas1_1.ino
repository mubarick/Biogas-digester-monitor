/*
 * buzzer ---- 10
 * scd30 ---- i2c
 * ds18b20 ---- 3
 * ultrasonic: echo ---- 4, trigger ---- 5
 * lcd ---- i2c
 * rtc ---- i2c
 * gsm: tx ---- 6, rx ---- 7
 */

// tweak the getLevel() function so it returns the actual level
// fix issue with the display messing up after working for some time

#include <NewPing.h> // ultrasonic sensor
#include "SparkFun_SCD30_Arduino_Library.h" //co2
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>  // i2c
#include <OneWire.h> // temperature sensor
#include <DallasTemperature.h>  // temperature sensor
#include "RTClib.h" //real time clock

// define pins for the ultrasonic sensor
#define TRIGGER_PIN  5 
#define ECHO_PIN     4  
#define MAX_DISTANCE 200

// create object for the sensor
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// define the pin for the ds18b20 temperature sensor
#define ONE_WIRE_BUS 3

//creat object for co2 sensor
SCD30 airSensor; 

//creat object for the realtime clock
RTC_DS1307 rtc;

// declare variable to hold date and time
static DateTime now;

// create object of onewire protocol
OneWire oneWire(ONE_WIRE_BUS);

// create object of the temperature sensor
DallasTemperature sensors(&oneWire);

// create lcd object
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// create gsm module object
SoftwareSerial gsm(6, 7);

//create object for serial communiction between the controllers
SoftwareSerial myserial(8, 9);

float temp = 0.0;
int level = 0;
int CO2 = 0;
unsigned int uS = 0;
int distance = 0;

String methane = "0.0", ammonia = "0.0", H2S = "0.0",  pressure = "0.0";

const byte buzzer = 10;

long millisNow = 0;
long millisThen = 0;
const long interval = 20000;
float humidity = 0.0;
byte cnt = 0;

void setup() {
  //init serial (hardware) communication
  Serial.begin(9600);
  //configure buzzer pin as output
  pinMode(buzzer, OUTPUT);

  //init temp sensor
  sensors.begin();

  //init gsm module serial communication
  gsm.begin(9600);

  //init serial communication for controllers
  myserial.begin(9600);

  //init i2c protocol
  Wire.begin();

  //init the rtc
  rtc.begin();

  //rtc.adjust(DateTime(2022, 12, 4, 14, 6, 00));
 
  now = rtc.now();

  //init lcd as a 20 by 4 display
  lcd.begin(20, 4);
  //turn on backlight
  lcd.setBacklight(1);
  lcd.clear();
  
  lcd.home();
  lcd.print(F("  BIOGAS MONITOR"));

  // check the co2 sensor
  if(airSensor.begin() == false)
  {
    Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    while (1)
    ;
  }
  
  digitalWrite(buzzer, 1);
  delay(1000);
  digitalWrite(buzzer, 0);

  lcd.setCursor(0,1);
  lcd.print("PLEASE WAIT!");
  lcd.setCursor(0,2);
  lcd.print("SYSTEM WARMING UP..");

  lcd.setCursor(0,3);
  lcd.print("UENR-RCEES");

  // wait for ten mins for sensors to warm up
  for(int i=60*10; i>0; i--)
  {
    lcd.setCursor(14, 3);
    lcd.print(String(i) + "  ");
    delay(1000);
  }
  
  lcd.clear();
  
  myserial.listen();

  lcd.setCursor(0,0);

  lcd.print("waiting for data...");

}

void loop() {

  // get the current run time
  millisNow = millis();
  
  temp = get_temp();
  level = getLevel(200);
  _getCO2();

  //showTime(12);
  
  if(myserial.available()>0)
  {
    cnt++;
    
    String data = myserial.readString();
    data.trim();

    Serial.println(data);
    
    methane = data.substring(0, data.indexOf("*"));
    ammonia = data.substring(data.indexOf("*")+1, data.indexOf("#"));
    H2S = data.substring(data.indexOf("#")+1, data.indexOf("&"));
    pressure = data.substring(data.indexOf("&")+1);

    float h = H2S.toFloat();

    H2S = String(h);
    
    Serial.println("*************************************");
    Serial.println("temperature: " + String(temp));
    Serial.println("humidity: " + String(humidity));
    Serial.println("level: " + String(level));
    Serial.println("CO2: " + String(CO2));
    Serial.println("methane: " + methane);
    Serial.println("ammonia: " + ammonia);
    Serial.println("H2S: " + H2S);
    Serial.println("pressure: " + pressure);
    Serial.println("*************************************");
    lcd.clear();
    showTime(12);

    float co_p = (float)CO2/1000000;
    co_p = co_p * 100;

    float methane_p = methane.toFloat();
    methane_p = methane_p/1000000;
    methane_p = methane_p * 100;

    float ammonia_p = ammonia.toFloat();
    ammonia_p = ammonia_p/1000000;
    ammonia_p = ammonia_p * 100;

    float h2s_p = H2S.toFloat();
    h2s_p = h2s_p/1000000;
    h2s_p = h2s_p * 100;
   
    
  lcd.setCursor(0,0);
  //lcd.print("P:" + pressure + " ");
  lcd.print("CH4:" + String(methane_p, 2)+ "% ");

  lcd.setCursor(0,1);
  //lcd.print("T:" + String(temp) + "  ");
  lcd.print("NH3:" + String(ammonia_p, 2) + "% ");
  lcd.setCursor(12,1);
  lcd.print("H:" + String(humidity));

  lcd.setCursor(0,2);
  lcd.print("CO2:" + String(co_p, 2) + "% ");
  lcd.setCursor(12,2);
  //lcd.print("CH4:" + methane);
  lcd.print("P:" + pressure + " ");
  
  lcd.setCursor(0,3);
  lcd.print("H2S:" + String(h2s_p, 2) + "% ");
  //lcd.print("NH3:" + ammonia + " ");
  lcd.setCursor(12,0);
  lcd.print("T:" + String(temp) + " ");

  delay(1000);
  lcd.setCursor(12, 3);
  lcd.print("L:" + String(level) + "  ");
  
    if(cnt == 6)
    {
      String logTime = getTimeStamp();
      String logData = String(temp) + "*" + String(level)+ "#" + String(CO2) + "&" + methane +
                     "@" + ammonia + "$" + H2S + "%" + pressure + "+" + logTime + "!" + String(humidity);
      myserial.print(logData);
    
      Serial.println(logData);
      sendToThingspeak();

      cnt = 0;
    }    
  }
/*
  if(millisNow - millisThen >= interval)
  {
    String logTime = getTimeStamp();
    String logData = String(temp) + "*" + String(level)+ "#" + String(CO2) + "&" + methane +
                     "@" + ammonia + "$" + H2S + "%" + pressure + "+" + logTime + "!" + String(humidity);
    myserial.print(logData);
    
    Serial.println(logData);
    millisThen = millisNow;
  }
*/
  //delay(500);
}

float get_temp()
{
  sensors.requestTemperatures(); 

  float tempC = sensors.getTempCByIndex(0);

  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    return tempC;
  } 
  else
  {
    Serial.println("Error: Could not read temperature data");
  }
  
}

int getLevel(int depth)
{
  distance = 0;
  for(byte i = 0; i < 10; i++)
  {
    delay(50);                      
    uS = sonar.ping(); 
    distance += uS / US_ROUNDTRIP_CM;   
  }

  distance = distance/10;

  return (depth - distance);
}

void _getCO2()
{
  if(airSensor.dataAvailable())
  {
    CO2 = airSensor.getCO2();
    humidity = airSensor.getHumidity();
  }
}


//void sendToThingspeak()
//{
//  gsm.println("AT+CPIN?");
//  delay(1000);
//  updateSerial();
//  gsm.println("AT+CREG?");
//  delay(1000);
//  updateSerial();
//  gsm.println("AT+CGATT?");
//  delay(1000);
//  updateSerial();
//  gsm.println("AT+CIPSHUT");
//  delay(1000);
//  gsm.println("AT+CIPSTATUS");
//  delay(2000);
//  gsm.println("AT+CIPMUX=0");
//  delay(2000);
//  gsm.println("AT+CSTT=\"MTN\"");
//  delay(1000);
//  gsm.println("AT+CIICR");
//  delay(3000);
//  gsm.println("AT+CIFSR");
//  delay(2000);
//  gsm.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\", \"80\"");
//  delay(6000);
//  gsm.println("AT+CIPSEND");
//  delay(4000);
//  //Serial.println("GET http://api.thingspeak.com/update?key=W68QUVX7MAZT3NXY&field1=" + String(lati));
//  //gsm.println("GET http://api.thingspeak.com/update?key=EUK8JZB9CJ2Z6VUH&field1=" + String(t) +"&field2=" + String(h) + "&field3=" +String(s));
//  gsm.println("GET http://api.thingspeak.com/update?key=IZYH9YHF34T5SJNW&field1=" + String(temp) +"&field2=" + String(humidity) + "&field3=" +String(CO2) 
//  +"&field4=" + String(methane) + "&field5=" +String(ammonia) +"&field6=" + String(H2S) + "&field7=" +String(pressure) + "&field8=" +String(level));
//  delay(4000);
//  gsm.println((char)26);
//  delay(5000);
//  gsm.println("AT+CIPSHUT");
//  delay(100);
//}

void sendToThingspeak()
{
  gsm.println("AT");
  delay(200);
  updateSerial();
  gsm.println("AT+CFUN=1");
  delay(200);
  updateSerial();
  gsm.println("AT+CPIN?");
  delay(200);
  updateSerial();
  gsm.println("AT+CSTT=\"MTN\",\"\",\"\"");
  delay(200);
  updateSerial();
  gsm.println("AT+CIICR");
  delay(500);
  updateSerial();
  gsm.println("AT+CIFSR");
  delay(500);
  updateSerial();
  gsm.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");
  delay(1000);
  updateSerial();
  gsm.println("AT+CIPSEND");
  delay(1000);
  updateSerial();
  gsm.println("GET http://api.thingspeak.com/update?api_key=IZYH9YHF34T5SJNW&field1=" + String(temp) +"&field2=" + String(humidity) + "&field3=" +String(CO2) 
  +"&field4=" + String(methane) + "&field5=" +String(ammonia) +"&field6=" + String(H2S) + "&field7=" +String(pressure) + "&field8=" +String(level));
  delay(1000);
  updateSerial();
  gsm.println((char)26);
  updateSerial();
  delay(300);
}

void showTime(int y)
{
  now = rtc.now();
  lcd.setCursor(y, 3);
  lcd.print(F("T:"));
  lcd.print(getHour(now.hour()) + ":" + getMinute(now.minute()));// + ":" + getSecond(now.second()));
  //time_ = getHour(now.hour()) +  getMinute(now.minute());
}

String getHour(int hour)
{

  if (hour < 10)
    return String("0" + String(hour));
  else
    return String(hour);

}

String getMinute(int minute)
{

  if (minute < 10)
    return String("0" + String(minute));
  else
    return String(minute);
}

String getSecond(int second)
{

  if (second < 10)
    return String("0" + String(second));
  else
    return String(second);
}

String getTimeStamp()
{
  now = rtc.now();
  String timeStamp = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + "|" +
  String(now.year()) + "/" + String(now.month()) + "/" + String(now.day());

  return timeStamp;
}

void updateSerial() {
  while(gsm.available() > 0)
      Serial.write(gsm.read());
  //while(Serial.available() > 0)
      //gsm.write(Serial.read());
}
