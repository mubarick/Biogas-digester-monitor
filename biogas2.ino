/*
 * mq4 ---- A0
 * mq137 ---- A1
 * mq136 ---- A2
 * pressure ---- A3
 * led ---- 3
 * sd card module ---- spi
 */

#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>

#include <MQUnifiedsensor.h>
/************************Hardware Related Macros************************************/
#define         Board                   ("Arduino UNO")
#define         Pin                     (A0)  //Analog input 4 of your arduino
#define         pin                     (A2)
/***********************Software Related Macros************************************/
#define         Type                    ("MQ-4") //MQ4
#define         type                    ("MQ-136") //MQ4
#define         Voltage_Resolution      (5)
#define         ADC_Bit_Resolution      (10) // For arduino UNO/MEGA/NANO
//#define         RatioMQ4CleanAir        (4.4) //RS / R0 = 60 ppm 
/*****************************Globals***********************************************/
//Declare Sensor
MQUnifiedsensor MQ4(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type);

MQUnifiedsensor MQ136(Board, Voltage_Resolution, ADC_Bit_Resolution, pin, type);


////////////////////////////////////////////////////////////////////////////////////
#define RL 10  //The value of resistor RL is 47K
#define m -0.263 //Enter calculated Slope 
#define b 0.42 //Enter calculated intercept
#define Ro 20 //Enter found Ro value
#define MQ137 A0 //Sensor is connected to A0
////////////////////////////////////////////////////////////////////////////////

#define RL2 10  //The value of resistor RL is 47K
#define m2 -0.260094 //Enter calculated Slope 
#define b2 -0.549225 //Enter calculated intercept
#define Ro2 20 //Enter found Ro value
//#define MQ136 A2 //Sensor is connected to A4
///////////////////////////////////////////////////////////////////////////////
const int pressureInput = A3; //select the analog input pin for the pressure transducer
const int pressureZero = 101.5;//102.4; //analog reading of pressure transducer at 0psi
const int pressureMax = 921.6; //analog reading of pressure transducer at 100psi
const int pressuretransducermaxPSI = 200; //psi value of transducer being used
const int sensorreadDelay = 250; //constant integer to set the sensor read delay in milliseconds
float pressureValue = 0; //variable to store the value coming from the pressure transducer
///////////////////////////////////////////////////////////////////////////////////////


SoftwareSerial myserial(8, 9);
const int chipSelect = 2;
const byte led = 3;

long millisNow = 0;
long millisThen = 0;
const long interval = 10000;

String temp, level, CO2, methane,ammonia, h2s, presure, logTime, _date, _time, hum;

void setup() {
  Serial.begin(9600);
  myserial.begin(9600);
  pinMode(led, OUTPUT);

  MQ4.setRegressionMethod(1);
  MQ4.init();
  MQ4.setR0(10);

  MQ136.setRegressionMethod(1);
  MQ136.init();
  MQ136.setR0(6.7);
  
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  File dataFile = SD.open("DATA.txt", FILE_WRITE);

/*
  if (dataFile) {

    dataFile.println(F("Date,Time,Temperature,Humidity,CO2,methane,ammonia,H2S,pressure,level")); 
    dataFile.close();
    Serial.println("heading written succesfully...");
  }

  else
   Serial.println("there is a problem ooo");
*/
  for(int i=60*10; i>0; i--)
  {
    digitalWrite(led, 1);
    delay(500);
    digitalWrite(led, 0);
    delay(500);
  }

}

void loop() {

  millisNow = millis();

  if(millisNow - millisThen >= interval)
  {
    methane = String(getMethane());
    ammonia = String(getAmmonia());
    h2s = String(getH2S(), 4);
    presure = String(getPressure());
    
    Serial.println("mq4:" + String(getMethane()));
    Serial.println("mq137:" + String(getAmmonia()));
    Serial.println("mq136:" + String(getH2S()));
    Serial.println("pressure:" + String(getPressure()));
    Serial.println();

    String data = methane + "*" + ammonia + "#" + h2s + "&" + presure;
    Serial.println(data);
    myserial.print(data);
    millisThen = millisNow;
  }

  if(myserial.available() > 0)
  {
    String logData = myserial.readString();

    Serial.println(logData);
 /*   
    if(logData == "stop")
    {
      while(1)
      {
        if(myserial.available() > 0)
          String data = myserial.readString();

          if(data == "resume")
            break;
      }
    }

  */
    temp = logData.substring(0, logData.indexOf("*"));
    level = logData.substring(logData.indexOf("*")+1, logData.indexOf("#"));
    CO2 = logData.substring(logData.indexOf("#")+1, logData.indexOf("&"));
    methane = logData.substring(logData.indexOf("&")+1, logData.indexOf("@"));
    ammonia = logData.substring(logData.indexOf("@")+1, logData.indexOf("$"));
    h2s = logData.substring(logData.indexOf("$")+1, logData.indexOf("%"));
    presure = logData.substring(logData.indexOf("%")+1, logData.indexOf("+"));
    logTime = logData.substring(logData.indexOf("+")+1, logData.indexOf("!"));
    hum = logData.substring(logData.indexOf("!")+1);
    
    _time = logTime.substring(0, logTime.indexOf("|"));
    _date = logTime.substring(logTime.indexOf("|")+1);

    Serial.println("temperature: " + temp);
    Serial.println("humidity: " + hum);
    Serial.println("level: " + level);
    Serial.println("CO2: " + CO2);
    Serial.println("methane: " + methane);
    Serial.println("ammonia: " + ammonia);
    Serial.println("h2s: " + h2s);
    Serial.println("pressure: " + presure);
    Serial.println("time: " + _time);
    Serial.println("date: " + _date);
    
    log_data();
    
    delay(50000);
  }
  
}

float getMethane()
{
  MQ4.update();
  MQ4.setA(1012.7); MQ4.setB(-2.786);
  float CH4 = MQ4.readSensor();
  return CH4;
}


float getH2S()
{
  MQ136.update();
  MQ136.setA(36.737); MQ136.setB(-3.536);
  float h2s = MQ136.readSensor();
  return h2s;
}


float getAmmonia()
{
  float VRL; //Voltage drop across the MQ sensor
  float Rs; //Sensor resistance at gas concentration 
  float ratio; //Define variable for ratio

  VRL = analogRead(MQ137)*(5.0/1023.0); //Measure the voltage drop and convert to 0-5V
  Rs = ((5.0*RL)/VRL)-RL; //Use formula to get Rs value

  ratio = Rs/Ro;  // find ratio Rs/Ro

  float ppm = pow(10, ((log10(ratio)-b)/m)); //use formula to calculate ppm
  return ppm;
}

float getPressure()
{
   for(byte i=0; i<100; i++)
    {
      pressureValue += analogRead(pressureInput); 
      delay(10);
    } 
    //reads value from input pin and assigns to variable
    //Serial.println("total: " + String(pressureValue));
    pressureValue = pressureValue/100;
    //Serial.println("average: " + String(pressureValue));
    //float voltage = (pressureValue* 5.04/1023.0);
    //Serial.println(pressureValue);
    //Serial.println("volt: " + String(voltage, 3));
    pressureValue = ((pressureValue-pressureZero)*pressuretransducermaxPSI)/(pressureMax-pressureZero);
    //Serial.print(pressureValue);
    //Serial.println("psi");
    //delay(1000);
    if(pressureValue < 0)
      pressureValue *= -1;
      
    return pressureValue; 
}

void log_data()
{
  //("Date,Time,Temperature,Humidity,CO2,methane,ammonia,H2S,pressure,level")
  File dataFile = SD.open("DATA.txt", FILE_WRITE);
 
  if (dataFile) {
    dataFile.print(_date);
    dataFile.print(","); 
    dataFile.print(_time); 
    dataFile.print(","); 
    dataFile.print(temp); 
    dataFile.print(",");
    dataFile.print(hum);
    dataFile.print(",");
    dataFile.print(CO2); 
    dataFile.print(","); 
    dataFile.print(methane);
    dataFile.print(",");
    dataFile.print(ammonia); 
    dataFile.print(","); 
    dataFile.print(h2s); 
    dataFile.print(","); 
    dataFile.print(presure); 
    dataFile.print(","); 
    dataFile.print(level); 
    dataFile.println(); 
    dataFile.close(); 

  }
  
  else
  Serial.println("OOPS!! SD card writing failed");
}

