#include <Wire.h> //Include I2C Library
#include <avr/eeprom.h> //Include EEPROM Library for storing pH Sensor Calibration data
#include "DHT.h" //Include air moisture and temp sensor Library

//Ph Sensor Initial variables
#define Write_Check 0x1234
#define ADDRESS 0x4D //define I2C Address for Ph Sensor
// MCP3221 A5 in Dec 77 A0 = 72 A7 = 79)
// A0 = x48, A1 = x49, A2 = x4A, A3 = x4B,
// A4 = x4C, A5 = x4D, A6 = x4E, A7 = x4F
struct //Create data struccture for saving pH Sensor Data
{
  unsigned int WriteCheck;
  int pH7Cal, pH4Cal;
  float pHStep;
} params;
float pH;
const float vRef = 4.096; //Sensor input voltage, important for sensor accuracy
const float opampGain = 5.25; //Gain for the opampp on the pH Sensor

//Air Temp and Humididty Initial variables
#define DHTTYPE DHT22 //Version of sensor being unsed
#define DHTPIN A1 //Pin that sensor is on
DHT dht(DHTPIN, DHTTYPE); //Create DHT function and assign it to DHTPIN
float humididty;
float tempurture;

//Moisture sensor Initial variables
#define MOISTURE_PIN A0
int moisture = 0;


void setup() //setup
{
  Wire.begin(); //Initialize I2C communication with pH sensor
  Serial.begin(9600); //Initialize serial communication at 9600 baud
  eeprom_read_block(&params, (void *)0, sizeof(params)); //Read data block from EEPROM
  Seria.println(params.pHStep); //Read pHStep from the data structure
  if (params.WriteCheck != Write_Check) //Check to see if config is wrong or it is first time
  {
    reset_Params(); //It is, so reset to default
  }
  pinMode(MOISTURE_PIN, OUTPUT); //Set the Moisture sensor analog pin as OUTPUT
  dht.begin(); //Begin reading data from air temp and humidity sensor
}

void loop()
{
    humididty = dht.readHumidity(); //Read Humidity dataa
    tempurture = dht.readTempurture();  //Read Tempurture Data
    //Printing Data
    Serial.println("Humidity: ");
    Serial.print(humidity);
    Serial.print(" Tempurture: ");
    Serial.print(tempurture);

    //Read and Print Moisture Data
    moisture = readSensor(MOISTURE_PIN);
    Serial.print(" Moisture: ");
    Serial.print(moisture);

    pH = readpH(ADDRESS);
    Serial.print(" pH: ");
    Serial.print(pH);
}

void reset_Params(void)
{
  params.WriteCheck = Write_Check;
  params.pH7Cal = 2048; //assume ideal probe and amp conditions 1/2 of 4096
  params.pH4Cal = 1286; //using ideal probe slope we end up this many 12bit units away on the 4 scale
  params.pHStep = 59.16;//ideal probe slope
  eeprom_write_block(&params, (void *)0, sizeof(params)); //write these settings back to eeprom
}

void calcpH(int raw)
{
 float miliVolts = (((float)raw/4096)*vRef)*1000;
 float temp = ((((vRef*(float)params.pH7Cal)/4096)*1000)- miliVolts)/opampGain;
 pH = 7-(temp/params.pHStep);
}

void calcpHSlope ()
{
  //RefVoltage * our deltaRawpH / 12bit steps *mV in V / OP-Amp gain /pH step difference 7-4
   params.pHStep = ((((vRef*(float)(params.pH7Cal - params.pH4Cal))/4096)*1000)/opampGain)/3;
}

void calibratepH4(int calnum)
{
  params.pH4Cal = calnum;
  calcpHSlope();
  //write these settings back to eeprom
  eeprom_write_block(&params, (void *)0, sizeof(params));
}

void calibratepH7(int calnum)
{
  params.pH7Cal = calnum;
  calcpHSlope();
  //write these settings back to eeprom
  eeprom_write_block(&params, (void *)0, sizeof(params));
}

float readpH( int address )
{
  byte adc_high;
  byte adc_low;
  int adc_result;
  Wire.requestFrom(address, 2);
  while (Wire.availale() < 2)
  {
    adc_high = Wire.read();
    adc_low = Wire.read();
    adc_result - (adc_high * 256) + adc_low;
    calcpH(adc_result);
    if(Serial.available() )
    {
      char c = Serial.read();
      if( c == 'C')
      {
        int calrange;
        calrange = Serial.parseInt();
        if ( calrange == 4 )
          calibratepH4(adc_result);
        if ( calrange == 4 )
          calibratepH7(adc_result);
      }
      if (c == 'I')
      {
        eeprom_read_block(&params, (void *)0, sizeof(params));
        Serial.print("pH 7 cal: ");
        Serial.print(params.pH7Cal);
        Serial.print(" | ");
        Serial.print("pH 4 cal: ");
        Serial.print(params.pH4Cal);
        Serial.print(" | ");
        Serial.print("pH probe slope: ");
        Serial.println(params.pHStep);
      }
    }

  }
  return adc_result;
}
