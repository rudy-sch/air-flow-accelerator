/*********
  Complete project details at http://randomnerdtutorials.com  
*********/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "PMS.h"
#include <stdlib.h>
#include "stdlib.h"
#include <string.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "RBDdimmer.h"//
#define USE_SERIAL  Serial
#define outputPin  25 
#define zerocross  13 // for boards with CHANGEBLE input pins
dimmerLamp dimmer(outputPin, zerocross); //initialase port for dimmer for ESP8266, ESP32, Arduino due boards


int outVal = 0;
int relayPin = 2;

TaskHandle_t Task1;
TaskHandle_t Task2;

Adafruit_BME280 bme; // I2C




HardwareSerial SerialPMS(1);
PMS pms(SerialPMS);
PMS::DATA data;

#define RXD2 17
#define TXD2 16

const int LED = 26; 
int M;
int pmsd=0;
float temperature=0;
float humidity=0;
int low=35;
int med=45;
int high=85;
unsigned long delayTime;

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;



#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }

        Serial.println();

        // Do stuff based on the command received from the app
        if (rxValue.find("A") != -1) { 
          
          M=1;
          
        }
        else if (rxValue.find("B") != -1) {
          
          M=2;
          
        }
        else if(rxValue.find("C") != -1){

          M=3;
          
        }
         else if(rxValue.find("D") != -1){

          M=4;
          
        }
         else if(rxValue.find("E") != -1){

          M=5;
          
        }
         else if(rxValue.find("F") != -1){

          M=6;
          
        }
         else if(rxValue.find("G") != -1){

          M=7;
          
        }

        Serial.println();
        Serial.println("*********");
      }
    }
};






void setup() {
  
  Serial.println(F("BME280 test"));
  
  bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
 status = bme.begin(0x76);  
 if (!status) {
   Serial.println("Could not find a valid BME280 sensor, check wiring!");
   while (1);
  }

// weather monitoring
    Serial.println("-- Weather Station Scenario --");
    Serial.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
    Serial.println("filter off");
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_NONE, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
                      
    // suggested rate is 1/60Hz (1m)
    delayTime = 10000; // in milliseconds
  
  
  dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
  M=0;
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  pinMode(LED, OUTPUT);
  
  Serial.begin(115200);
  SerialPMS.begin(9600, SERIAL_8N1, RXD2, TXD2);

  BLEDevice::init("Exhaust fan - CAVS"); // Give it a name

 // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

//create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
  

}

void Task1code(void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    //Serial.print("yuk");
    //delay(500);
    if (pms.read(data))
    {
    bme.takeForcedMeasurement();
    temperature=bme.readTemperature();
    humidity=bme.readHumidity();

    temperature= temperature-1.2;
    humidity= humidity+10;
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" *C");
  
 
    Serial.print("Humidity = ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.println();

             

  
      Serial.print("PM 2.5 (ug/m3): ");
      Serial.println(data.PM_AE_UG_1_0);
  
      
  
      Serial.println();
      pmsd =data.PM_AE_UG_1_0;
      
      char pms[8];
      itoa(pmsd,pms,10);    

       
     char humidityString [2];
     char temperatureString [2];
     
   
   
     dtostrf (humidity, 1, 0, humidityString);
     dtostrf (temperature, 1, 0, temperatureString);

     if (isnan(temperature)){
        temperature=0;
     }
     else if(isnan(humidity)){
        humidity=0;
     }else if(isnan(pmsd)){
        pmsd=0;
     }

    if (temperature>99){
      temperature= 50;
    }
    else if(temperature<0){
      temperature=50;
    }
    else if(humidity>99){
      humidity=0;
    }
    else if(pmsd>800){
      pmsd=0;
    }
     
      char dhtDataString [10];
     //Penggabungan data sensor menjadi string
     sprintf (dhtDataString, "%.0f,%.0f,%d",humidity,temperature,pmsd)  ;
     

     
//    pCharacteristic->setValue(&txValue, 1); // To send the integer value
//    pCharacteristic->setValue("Hello!"); // Sending a test message
      pCharacteristic->setValue(dhtDataString);
      
       pCharacteristic->notify(); // Send the value to the app!
       Serial.print("*** Sent Value: ");
       Serial.print(dhtDataString);
       Serial.println(" ***");
       delay(15000);
    }
   }
}


  void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

 for(;;){
    if (M==1){
          
          if(temperature>28){
            do{ 
                Serial.print("Mode otomatis!");
                Serial.println();
                Serial.print("Nyala temperatur");
                dimmer.setPower(high);
                digitalWrite(relayPin, LOW);
                delay(2000);
              }while(temperature>27 && M==1);
          }
        
          
           else if(pmsd>90){
            do{
                Serial.print("Mode otomatis!");
                Serial.println();
                Serial.print("Nyala debu");
                dimmer.setPower(high);
                digitalWrite(relayPin, LOW);
                delay(2000);
              }while(pmsd>75 && M==1);
          }
          
          else {
            digitalWrite(relayPin, HIGH);
            Serial.print("Mati");
            delay(2000);
          }
          
        }
        else if (M==2){
          
          
          dimmer.setPower(med);
          digitalWrite(relayPin, LOW);
          
          
        }
      
        else if (M==3){
          
          digitalWrite(relayPin, HIGH);  
                 
          
        }
        else if (M==4){
          
          digitalWrite(relayPin, HIGH);
          
          
        }
        else if (M==5){
          
          dimmer.setPower(low);
          digitalWrite(relayPin, LOW);
         
        }
        else if (M==6){
          
          dimmer.setPower(med);
          digitalWrite(relayPin, LOW);
          
        }
        else if (M==7){
          
          dimmer.setPower(high);
          digitalWrite(relayPin, LOW);
          
        }
    
    }
  
  }
 


void loop() { 
   if(humidity>70){
  Serial.print("Lembab woi");
  Serial.println();
  digitalWrite(LED, HIGH);
  delay(2000);
  digitalWrite(LED, LOW);
  delay(2000);
  }
  else if(temperature >29){
    Serial.print("Panas woi");
  Serial.println();
  digitalWrite(LED, HIGH);
  delay(2000);
  digitalWrite(LED, LOW);
  delay(2000);
  }
   else if(pmsd >90){
    Serial.print("Debu woi");
  Serial.println();
  digitalWrite(LED, HIGH);
  delay(2000);
  digitalWrite(LED, LOW);
  delay(2000);
 
}
}
