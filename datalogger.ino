//#include <SPI.h>
#include "SD.h"
#include <Wire.h>
#include "RTClib.h"
#include <OneWire.h>
#include "SoftwareSerial.h"
#include <DallasTemperature.h>

//RTC time
RTC_DS1307 RTC; // define the Real Time Clock object

//SD card
File logfile; // the logging file
char filename[] = "M_WQUA00.CSV";
const int chipSelect = 10; //digital pin 10 for SD card

//OPR
#define VOLTAGE 5.00    //system voltage
#define OFFSET 6        //zero drift voltage
double orpValue;
#define ArrayLenth  40    //times of collection
#define orpPin 1          //orp meter output,connect to Arduino controller ADC pin
int orpArray[ArrayLenth];
int orpArrayIndex=0;
//OPR
double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    printf("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}

//EC
#define EC_PIN A2
float voltage,ecValue,temperature;
float ECcurrent;
 
//Temperature chip
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//pH
const int analogInPin = A0; 
int sensorValue = 0;         //pH meter Analog output to Arduino Analog Input 0
unsigned long int avgValue;  //Store the average value of the sensor feedback
float b;
int buf[10],temp;

//anemometer_windVane_rain
#define uint  unsigned int
#define ulong unsigned long
#define PIN_ANEMOMETER  2     // Digital 2
#define PIN_RAINGAUGE  3     // Digital 3
#define PIN_VANE        5     // Analog 5
volatile int numRevsAnemometer; // Incremented in the interrupt
volatile int numDropsRainGauge; // Incremented in the interrupt
ulong nextCalcSpeed;                // When we next calc the wind speed
ulong nextCalcDir;                  // When we next calc the direction
ulong nextCalcRain;                  // When we next calc the rain drop
ulong time;                         // Millis() at each start of loop().
/*#define NUMDIRS 8
ulong   adc[NUMDIRS] = {26, 45, 77, 118, 161, 196, 220, 256};
char *strVals[NUMDIRS] = {"W","NW","N","SW","NE","S","SE","E"};
byte dirOffset=0;
*/
void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);

  while(1);
}

void setup() {
 // initialize serial communication with computer:
  Serial.begin(9600);
  sensors.begin();
  
  //SD Card
//   while (!Serial) {
//    ; // wait for serial port to connect. Needed for native USB port only
//  }
  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    digitalWrite(8, HIGH); 
    digitalWrite(9, HIGH); 
    delay(900000);
    return;
  }
  Serial.println("initialization done.");
  // create a new file
  Serial.print("Creating File...");
 // char filename [] = "M_WQUA00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  if (! logfile) {
   Serial.println("couldnt create file");
   digitalWrite(8, HIGH); 
   digitalWrite(9, HIGH); 
   delay(900000);
   return;
  }
  Serial.print("Logging to: ");
  Serial.println(filename);
  logfile.close();
  
  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
    digitalWrite(8, HIGH); 
    digitalWrite(9, HIGH); 
    delay(900000);
#endif  //ECHO_TO_SERIAL
  }

/*
//anemometer_windVane_rain
//Anemometer
   pinMode(PIN_ANEMOMETER, INPUT);
   digitalWrite(PIN_ANEMOMETER, HIGH);
   attachInterrupt (0, countAnemometer, RISING); 
//Rain_Gauge_Bucket
   pinMode(PIN_RAINGAUGE, INPUT);
   digitalWrite(PIN_RAINGAUGE, HIGH);
   attachInterrupt(1, countRainGauge, FALLING);
   sei();
*/
}

void loop() {
  //mosfet
/*
  //fast_mode (for testing)
 digitalWrite(5, LOW); //turn off the power to sensors
 digitalWrite(8, HIGH);
  delay(5000);
 digitalWrite(8, LOW);
  digitalWrite(5, HIGH); //turn on the power to sensors
 digitalWrite(9, HIGH);
  delay(1000); //wait to warm up the sensors
 digitalWrite(9, LOW);
*/

  //logging_mode (for field data logging)
  //every 15 minutes, record date, time and all sensors reading
  //30 seconds before reading sensors, turn on power to sensors (warm up)
  digitalWrite(5, LOW); //turn off the power to sensors
  digitalWrite(8, HIGH);
  delay(840000);
  digitalWrite(8, LOW);
  digitalWrite(5, HIGH); //turn on the power to sensors
  digitalWrite(9, HIGH);
  delay(60000); //wait to warm up the sensors
  digitalWrite(9, LOW);

  logfile = SD.open(filename, FILE_WRITE);
  
  //Turbidity sensor
  int sensorValueTur = analogRead(A3);
  float voltageTur = sensorValueTur * (5.0 / 1024.0);
  
  DateTime now;
  now = RTC.now();
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print(" ");
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print(" ");

  //LED
  digitalWrite(9, HIGH); 
  delay(100); 
  digitalWrite(9, LOW);
 
  //EC
    static unsigned long timepoint = millis();
    while ((millis() - timepoint) < 1000) {
  }
      voltage = analogRead(EC_PIN)/1024.0*5000;   // read the voltage
      float TempCoefficient=1.0+0.0185*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.0185*(fTP-25.0));
      float CoefficientVolatge=(float)voltage/TempCoefficient;   
      if(CoefficientVolatge<150)Serial.println("too clean!");   //25^C 1413us/cm<-->about 216mv  if the voltage(compensate)<150,that is <1ms/cm,out of the range
      else if(CoefficientVolatge>3300)Serial.println("Out of the range!");  //>20ms/cm,out of the range
      else
      { 
      if(CoefficientVolatge<=448)ECcurrent=6.84*CoefficientVolatge-64.32;   //1ms/cm<EC<=3ms/cm
      else if(CoefficientVolatge<=1457)ECcurrent=6.98*CoefficientVolatge-127;  //3ms/cm<EC<=10ms/cm
      else ECcurrent=5.3*CoefficientVolatge+2278;                           //10ms/cm<EC<20ms/cm
      ECcurrent/=1000;    //convert us/cm to ms/cm
     

    Serial.print(" ");
    logfile.print(" ");
    Serial.print("EC_Volt(mV):");
    logfile.print("EC_Volt(mV):");
    logfile.print(" ");
    Serial.print(voltage,2);  //millivolt average,from 0mv to 4995mV
    logfile.print(voltage,2);
    Serial.print(" ");
    logfile.print(" ");
    Serial.print("EC(ms/cm):");
    logfile.print("EC(ms/cm):");
    logfile.print(" ");
    Serial.print(ECcurrent,2);  //two decimal
    logfile.print(ECcurrent,2);
      }
      
    Serial.print("EC_Volt(mV):");
    logfile.print("EC_Volt(mV):");
    logfile.print(" ");
    Serial.print(voltage,2);  //millivolt average,from 0mv to 4995mV
    logfile.print(voltage,2);
    Serial.print(" ");
    logfile.print(" ");
    Serial.print("EC(ms/cm):");
    logfile.print("EC(ms/cm):");
    logfile.print(" ");
    Serial.print(ECcurrent,2);  //two decimal
    logfile.print(ECcurrent,2);
    Serial.print(" ");
    logfile.print(" ");
    Serial.print("Temp(^C):");
    logfile.print("Temp(^C):");
    logfile.print(" ");
    sensors.requestTemperatures();  
    Serial.print(sensors.getTempCByIndex(0));
    logfile.print(sensors.getTempCByIndex(0));

  //LED
  digitalWrite(9, HIGH); 
  delay(100); 
  digitalWrite(9, LOW);

//pH
 for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    buf[i]=analogRead(analogInPin);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
  avgValue+=buf[i];
  float phVol=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
  float phValue=12.7643 * phVol - 17.9263;                      //convert the millivolt int
  
 Serial.print(" ");
 logfile.print(" ");
 Serial.print("pHVol:");
 logfile.print("pHVol:");
 logfile.print(" ");
 Serial.print(phVol);
 Serial.print(" ");
 logfile.print(phVol);
 logfile.print(" ");
 Serial.print("pH:");
 logfile.print("pH:");
 logfile.print(" ");
 Serial.print(phValue,2);
 logfile.print(phValue);
 Serial.print(" ");
 Serial.print("Tur(mV):");
 Serial.print(voltageTur);
 logfile.print(" ");
 logfile.print("Tur(mV):");
 logfile.print(" ");
 logfile.print(voltageTur);

  //LED
  digitalWrite(9, HIGH); 
  delay(100); 
  digitalWrite(9, LOW);

 //OPR
  static unsigned long orpTimer=millis();   //analog sampling interval
  static unsigned long printTime=millis();
  if(millis() >= orpTimer)
  {
    orpTimer=millis()+20;
    orpArray[orpArrayIndex++]=analogRead(orpPin);    //read an analog value every 20ms
    if (orpArrayIndex==ArrayLenth) {
      orpArrayIndex=0;
    }   
    orpValue=((30*(double)VOLTAGE*1000)-(75*avergearray(orpArray, ArrayLenth)*VOLTAGE*1000/1024))/75-OFFSET;   //convert the analog value to orp according the circuit
  }
  if(millis() >= printTime)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  {
    printTime=millis()+800;
    
    Serial.print(" ");
    logfile.print(" ");
    Serial.print("ORP(mV):");
    logfile.print("ORP(mV):");
    logfile.print(" ");
    Serial.print((int)orpValue);
    logfile.print((int)orpValue);
    Serial.print(" ");
    logfile.print(" ");
  }

//Anemometer
   nextCalcRain = millis() + 30000;
   nextCalcSpeed = millis() + 5000;
   numRevsAnemometer = 0;         // Reset counter
   numDropsRainGauge = 0;         // Reset counter
   while(millis() < nextCalcSpeed) {
  
      //LED
    digitalWrite(9, HIGH);   
   }
   time = millis();
   calcWindSpeed();
   Serial.print(" ");
   logfile.print(" ");
   while(millis() < nextCalcRain) {
    
    //LED
   digitalWrite(9, HIGH);  
   }
   time = millis();
   calcRainFall();
   
   Serial.println();
   logfile.println();
   
   logfile.close();

   //LED
  digitalWrite(9, HIGH); 
  delay(2000); 
  digitalWrite(9, LOW);

}

/*
ch=0,let the DS18B20 start the convert;ch=1,MCU read the current temperature from the DS18B20.
*/

//anemometer_windVane_rain
 void countAnemometer() {
   numRevsAnemometer++;
}
void countRainGauge() {
   numDropsRainGauge++;
}
/*void calcWindDir() {
   int val;
   byte x, reading;
   val = analogRead(PIN_VANE);
   val >>=2;                        // Shift to 255 range
   reading = val;
   for (x=0; x<NUMDIRS; x++) {
      if (adc[x] >= reading)
         break;
   }
   x = (x + dirOffset) % 8;   // Adjust for orientation
   Serial.print("  Dir: ");
   Serial.println(strVals[x]);
}
*/
void calcWindSpeed() {
   int x, iSpeed;
   long speed = 24011;
   speed *= numRevsAnemometer;
   speed /= 5000;
   iSpeed = speed;         // Need this for formatting below
   Serial.print("Wind_Speed:");
   logfile.print("Wind_Speed:");
   logfile.print(" ");
   x = iSpeed / 10;
   Serial.print(x);
   logfile.print(x);
   Serial.print('.');
   logfile.print('.');
   x = iSpeed % 10;
   Serial.print(x);
   logfile.print(x);
   logfile.print(" ");

     //LED
   digitalWrite(9, HIGH); 
   delay(100); 
   digitalWrite(9, LOW);
}
void calcRainFall() {
   int x, iVol;
   long vol = 2794; // 0.2794 mm
   vol *= numDropsRainGauge;
   vol /= 30000;
   iVol = vol;         // Need this for formatting below
   Serial.print("Rain_Fall:");
   logfile.print("Rain_Fall:");
   logfile.print(" ");
   x = iVol / 10000;
   Serial.print(x);
   logfile.print(x);
   Serial.print('.');
   logfile.print('.');
   x = iVol % 10000;
   Serial.print(x);
   logfile.print(x);
   
     //LED
   digitalWrite(9, HIGH); 
   delay(100); 
   digitalWrite(9, LOW);
}
