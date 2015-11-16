
#include <Wire.h> //Include I2C Library
#include <avr/eeprom.h> //Include EEPROM Library for storing pH Sensor Calibration data
#include "DHT.h" //Include air moisture and temp sensor Library
#include <OneWire.h>
#include <DallasTemperature.h>


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
float humidity;
float temperature;

//Moisture sensor Initial variables
#define MOISTURE_PIN 3
int moisture = 0;

//Water Temp Stuff
#define WATER_TEMP_PIN 2
#define TEMPERATURE_PRECISION 9

OneWire oneWire(WATER_TEMP_PIN);
DallasTemperature sensors(&oneWire);
int numberOfDevices;
DeviceAddress tempDeviceAddress;


void setup() //setup
{
  Wire.begin(); //Initialize I2C communication with pH sensor
  Serial.begin(9600); //Initialize serial communication at 9600 baud
  eeprom_read_block(&params, (void *)0, sizeof(params)); //Read data block from EEPROM
  Serial.println(params.pHStep); //Read pHStep from the data structure
  if (params.WriteCheck != Write_Check) //Check to see if config is wrong or it is first time
  {
    reset_Params(); //It is, so reset to default
  }
  pinMode(A1 + MOISTURE_PIN, OUTPUT); //Set the Moisture sensor analog pin as OUTPUT
  dht.begin(); //Begin reading data from air temp and humidity sensor
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  for(int i =0; i < numberOfDevices; i++)
  {
	if(sensors.getAddress(tempDeviceAddress, i))
	{
	  sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
	}
  }
  
 }

void loop()
{
    humidity = dht.readHumidity(); //Read Humidity dataa
    temperature = dht.readTemperature();  //Read Tempurture Data
    //Printing Data
    Serial.print("1.");
    Serial.println(humidity);
    Serial.print("2.");
    Serial.println(temperature);
    // Serial.print("C");
    //Read and Print Moisture Data
    moisture = readSensor(MOISTURE_PIN);
    //moisture = analogRead(A0);    
    Serial.print("5.");
    Serial.println(moisture, DEC);

    pH = readpH(ADDRESS);
    Serial.print("4.");
    Serial.println(pH);
	
	for(int i =0; i<numberOfDevices; i++)
	{
	  if(sensors.getAddress(tempDeviceAddress, i))
	  {
		float tempC = sensors.getTempC(tempDeviceAddress);
		Serial.print("3.");
		Serial.println(tempC);

	  }
	}
    delay(1000);
}

void reset_Params(void)
{
  params.WriteCheck = Write_Check;
  params.pH7Cal = 2048; //assume ideal probe and amp conditions 1/2 of 4096
  params.pH4Cal = 1286; //using ideal probe slope we end up this many 12bit units away on the 4 scale
  params.pHStep = 59.16;//ideal probe slope
  eeprom_write_block(&params, (void *)0, sizeof(params)); //write these settings back to eeprom
}

float calcpH(int raw)
{
 float miliVolts = (((float)raw/4096)*vRef)*1000;
 float temp = ((((vRef*(float)params.pH7Cal)/4096)*1000)- miliVolts)/opampGain;
 return (7-(temp/params.pHStep));
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
  while (Wire.available() < 2);
  adc_high = Wire.read();
  adc_low = Wire.read();
  adc_result = (adc_high * 256) + adc_low;
  return calcpH(adc_result);
}
int readSensor( int analogPin ) {
  digitalWrite( A1 + analogPin, HIGH );
  delay( 1 ); // 1 millisecond
  int value = analogRead( analogPin );
  digitalWrite( A1 + analogPin, LOW );
  return value;
}

