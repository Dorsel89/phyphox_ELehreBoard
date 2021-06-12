#include <SD.h>

/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */
File root;
int storedDataFiles =0;
bool ifnotdone = true;
bool configModeDONE =false;
int LED = 2;
bool statusLED=false; 
String Address="";
char* serialNumberString ="";
char buffer[21];
#include "BLEDevice.h"
bool LEDstatus = 0;
// The remote service we wish to connect to.
static BLEUUID serviceUUID("cddf1001-30f7-4671-8b43-5e40ba53514a");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("cddf1002-30f7-4671-8b43-5e40ba53514a");
static BLEUUID    configUUID("cddf1003-30f7-4671-8b43-5e40ba53514a");

bool printData = false;
bool printDataPointNumber = false;
/*
configData[0]:
Byte 0 :: 0 = Messmodus | 1 = Testmodus
Byte 1 :: 0 = Live-Mode | 1 = Fast-Mode   
Byte 2 :: ChI Enable 
Byte 3 :: ChII Enable
configData[1]: Datarate in Live-Mode  | Treshold in Fast-Mode | Testmodus ID in Testmode
configData[2]: Datarate in Live-Mode  | Treshold in Fast-Mode | Anzahl Messpunkte in Testmode
configData[3]

*/
uint8_t configData[3] = {1,0,0};


const uint8_t nGND = 100;
const uint8_t nP12= 100;
const uint8_t nN12= 100;
const uint8_t nSIN= 100;
 
const int numberOfSamples = nGND+nP12+nN12+nSIN;
float V1[numberOfSamples] = {0};
float V2[numberOfSamples] = {0};
float Timestamps[numberOfSamples] = {0};

bool deviceWorks[4]={false};
// +-offset which is allowed
float limitGND = 0.050;//+-50mV
float limitP12 = 0.10;//+-100mV
float limitN12 = 0.10;//+-100mV
float limitSTD = 0.010;//std +- 10mV

int dataPoint = 0;


static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* configCharacteristic;
static BLEAdvertisedDevice* myDevice;
uint8_t dataReceived[20] = {0};

int currentMode=0;
#define MODE_GND 1
#define MODE_P12 2
#define MODE_N12 3
#define MODE_SIN 4

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    digitalWrite(LED ,statusLED);
    statusLED=!statusLED;
    float myfloats[3];
    memcpy(&myfloats[0],pData,12); 
    V1[dataPoint] = myfloats[0];
    V2[dataPoint] = myfloats[1];
    Timestamps[dataPoint] = myfloats[2];
    dataPoint+=1;
        
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    currentMode=0;
  } 

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    currentMode=0;
    Serial.println("disconnected");
  }
};


bool connectToServer() {
    Serial.print("Forming a connection to ");
    Address = myDevice->getAddress().toString().c_str();
    Serial.println(Address);
    Address.replace(":","");
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    configCharacteristic = pRemoteService->getCharacteristic(configUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
/*
    if(pRemoteCharacteristic->canRead()) {
      //std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }
*/
    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    //Serial.print("BLE Advertised Device found: ");
    //Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("");
  Serial.println("");
  Serial.println("STARTING E-LEHRE TESTCENTER");
  BLEDevice::init("");
  //pinMode(LED_BUILTIN,OUTPUT);
  pinMode(32,OUTPUT);
  pinMode(33,OUTPUT);
  pinMode(25,OUTPUT);
  pinMode(26,OUTPUT);

  pinMode(LED,OUTPUT);
  digitalWrite(LED,1);
  digitalWrite(32,1);
  digitalWrite(33,1);
  digitalWrite(25,1);
  digitalWrite(26,1);

  if (!SD.begin(4)) {
    Serial.println("sd initialization failed!");
    return;
  }

  
  storedDataFiles = countFiles();
 
  Serial.print(storedDataFiles);
  Serial.println(" txt files found");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    Serial.println("");
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {

    
    switch(currentMode){
      case 0:
          configData[1]=currentMode;
          configData[2]=0;
          configCharacteristic->writeValue(&configData[0],3);
          currentMode++;
          

          if (SD.exists("/elehre/"+Address+".txt")){ 
            Serial.println(Address+".txt exists.");
            root = SD.open("/elehre/" + Address + ".txt");
            char character;
            char Buffer[50];
            int bufferposition =0; 
            char trennzeichen = ';';                       
            while (root.available() > 0){
              character = root.read();
              if(character==trennzeichen){
                buffer[bufferposition]='\0';
                serialNumberString=buffer;
                break;
                }
              buffer[bufferposition]=character;
              bufferposition+=1;
              }
            root.close();
         
          }else {
            Serial.println(Address+".txt doesn't exist... creating it..");
            root = SD.open("/elehre/" + Address + ".txt", FILE_WRITE);
            root.close();
            Serial.print("New SN: ");
            sprintf(buffer, "E%04d", storedDataFiles+1);
            serialNumberString = buffer;
          }
          Serial.print("GIVEN SN: ");
          Serial.println(serialNumberString);
          root = SD.open("/elehre/" + Address + ".txt", FILE_WRITE);
          root.print(serialNumberString); 
          root.print(";");
          root.println(Address+";");
          root.close();
          //configData[3]=nGND;
          //configData[1]=1;
          //configData[0]=3;
          //configCharacteristic->writeValue(&configData[0],3);
          //nGND

      case MODE_GND:
        if(!configModeDONE){
          configData[1]=currentMode;
          configData[2]=nGND;
          configCharacteristic->writeValue(&configData[0],3);
          Serial.println("GROUND SIGNALS ON ALL INPUTS");        
          disableAllRelais();
          digitalWrite(32,0);    
          configModeDONE=true;
          dataPoint=0;      
          }
        if(dataPoint == nGND){
          //####### STATISTICS START #######
          float median[2] = {0};
          for(int i=0;i<nGND;i++){
            median[0] += V1[i]/nGND;
            median[1] += V2[i]/nGND;
          }          
          float varianz[2] ={0};
          for(int i=0;i<nGND;i++){
            varianz[0]+=(V1[i]-median[0])*(V1[i]-median[0])/(nGND-1);
            varianz[1]+=(V2[i]-median[1])*(V2[i]-median[1])/(nGND-1);
          }
          
          Serial.println();
          Serial.printf("Median\tCH I\t%f +- %f \n",median[0],sqrt(varianz[0])/sqrt(nGND));
          Serial.printf("standard deviation\tCH I\t%f \n",sqrt(varianz[0]));
          Serial.printf("Median\tCH II\t%f +- %f \n",median[1],sqrt(varianz[1])/sqrt(nGND));
          Serial.printf("standard deviation\tCH II\t%f \n\n",sqrt(varianz[1]));
          root = SD.open("/elehre/" + Address + ".txt", FILE_APPEND);

          float limit = limitGND;
          if(sqrt(varianz[0])<limitSTD && (median[0]<limit) && (median[0]>-limit)){
            if(sqrt(varianz[1])<limitSTD && (median[1]<limit) && (median[1]>-limit)){
              deviceWorks[currentMode-1]=true;
            }
          }
          
          
          //####### STATISTICS END #######
          
          
          Serial.println("Kanal I\tKanal II");
          for(int i=0;i<nGND;i++){
            Serial.print(V1[i],6);
            Serial.print("\t"); 
            Serial.print(V2[i],6);
            Serial.print("\t"); 
            Serial.println(Timestamps[i],6);
            root.print(V1[i],7);
            root.print(";");
            root.print(V2[i],7);
            root.print(";");
            root.print(Timestamps[i]);
            root.println(";");            
          }
          currentMode++;
          configModeDONE=false;
          root.close();
        }
   
      
      case MODE_P12:
        if(!configModeDONE){
          configData[1]=currentMode;
          configData[2]=nP12;
          configCharacteristic->writeValue(&configData[0],3); 
          Serial.println("");
          Serial.println("+12V ON CHANNELS");
          disableAllRelais();
          digitalWrite(33,0);    
          configModeDONE=true;      
        }

        if(dataPoint == nGND+nP12){
          //####### STATISTICS START #######
          float median[2] = {0};
          for(int i=nGND;i<nGND+nP12;i++){
            median[0] += V1[i]/nP12;
            median[1] += V2[i]/nP12;
          }          
          float varianz[2] ={0};
          for(int i=nGND;i<nGND+nP12;i++){
            varianz[0]+=(V1[i]-median[0])*(V1[i]-median[0])/(nP12-1);
            varianz[1]+=(V2[i]-median[1])*(V2[i]-median[1])/(nP12-1);
          }
          Serial.println();
          Serial.printf("Median\tCH I\t%f +- %f \n",median[0],sqrt(varianz[0])/sqrt(nP12));
          Serial.printf("standard deviation\tCH I\t%f \n",sqrt(varianz[0]));
          Serial.printf("Median\tCH II\t%f +- %f \n",median[1],sqrt(varianz[1])/sqrt(nP12));
          Serial.printf("standard deviation\tCH II\t%f \n\n",sqrt(varianz[1]));

          float target = 12.0;
          float limit = limitP12;
          if(sqrt(varianz[0])<limitSTD && (median[0]<target+limit) && (median[0]>target-limit)){
            if(sqrt(varianz[1])<limitSTD && (median[1]<target+limit) && (median[1]>target-limit)){
              deviceWorks[currentMode-1]=true;
            }
          }
          
          //####### STATISTICS END #######
                    
          Serial.print("Kanal I");
          Serial.print("\t");
          Serial.println("Kanal II");
          root = SD.open("/elehre/" + Address + ".txt", FILE_APPEND);
          root.print("\n");                   
          for(int i=0;i<nP12;i++){
            Serial.print(V1[i+nGND],6);
            Serial.print("\t");
            Serial.print(V2[i+nGND],6);
            Serial.print("\t");
            Serial.println(Timestamps[i+nGND],6);
            root.print(V1[i+nGND],7);
            root.print(";");
            root.print(V2[i+nGND],7);
            root.print(";");
            root.print(Timestamps[i+nGND]);
            root.println(";");                        
          }
          root.close();          
          currentMode++;
          configModeDONE=false;
        }
               
      case MODE_N12:
        if(!configModeDONE){
          configData[1]=currentMode;
          configData[2]=nN12;
          configCharacteristic->writeValue(&configData[0],3); 
          Serial.println("");
          Serial.println("-12V ON CHANNELS");
          disableAllRelais();
          digitalWrite(25,0);    
          configModeDONE=true;      
        }
        
        if(dataPoint == nGND+nP12+nN12){
          //####### STATISTICS START #######
          float median[2] = {0};
          for(int i=nGND+nP12;i<nGND+nP12+nN12;i++){
            median[0] += V1[i]/nN12;
            median[1] += V2[i]/nN12;
          }          
          float varianz[2] ={0};
          for(int i=nGND+nP12;i<nGND+nP12+nN12;i++){
            varianz[0]+=(V1[i]-median[0])*(V1[i]-median[0])/(nN12-1);
            varianz[1]+=(V2[i]-median[1])*(V2[i]-median[1])/(nN12-1);
          }
          
          Serial.println();
          Serial.printf("Median\tCH I\t%f +- %f \n",median[0],sqrt(varianz[0])/sqrt(nN12));
          Serial.printf("standard deviation\tCH I\t%f \n",sqrt(varianz[0]));
          Serial.printf("Median\tCH II\t%f +- %f \n",median[1],sqrt(varianz[1])/sqrt(nN12));
          Serial.printf("standard deviation\tCH II\t%f \n\n",sqrt(varianz[1]));

          root = SD.open("/elehre/" + Address + ".txt", FILE_APPEND);

          float target = -12;
          float limit = limitN12;
          if(sqrt(varianz[0])<limitSTD && (median[0]<target+limit) && (median[0]>target-limit)){
            if(sqrt(varianz[1])<limitSTD && (median[1]<target-limit) && (median[1]>target-limit)){
              deviceWorks[currentMode-1]=true;
            }
          }          
          //####### STATISTICS END #######          
          Serial.print("Kanal I");
          Serial.print("\t");
          Serial.println("Kanal II");
          root = SD.open("/elehre/" + Address + ".txt", FILE_APPEND);
          root.print("\n");                   
          for(int i=0;i<nN12;i++){
            Serial.print(V1[i+nGND+nP12],6);
            Serial.print("\t");
            Serial.print(V2[i+nGND+nP12],6);
            Serial.print("\t");
            Serial.println(Timestamps[i+nGND+nP12],6);
            root.print(V1[i+nGND+nP12],7);
            root.print(";");
            root.print(V2[i+nGND+nP12],7);
            root.print(";");
            root.print(Timestamps[i+nGND+nP12]);
            root.println(";");                        
          }
          root.close();          
          currentMode++;
          configModeDONE=false;
        }

           
         
      case MODE_SIN:
        if(!configModeDONE){
          configData[1]=currentMode;
          configData[2]=nSIN;
          configCharacteristic->writeValue(&configData[0],3); 
          Serial.println("");
          Serial.println("SIN ON CHANNELS");
          disableAllRelais();
          digitalWrite(26,0);    
          configModeDONE=true;      
        }

        if(dataPoint == nGND+nP12+nN12+nSIN){
          Serial.print("Kanal I");
          Serial.print("\t");
          Serial.println("Kanal II");
          root = SD.open("/elehre/" + Address + ".txt", FILE_APPEND);          
          root.print("\n");                             
          for(int i=0;i<nSIN;i++){
            Serial.print(V1[i+nGND+nP12+nN12],6);
            Serial.print("\t");
            Serial.print(V2[i+nGND+nP12+nN12],6);
            Serial.print("\t");
            Serial.println(Timestamps[i+nGND+nP12+nN12],6);
            root.print(V1[i+nGND+nP12+nN12],7);
            root.print(";");
            root.print(V2[i+nGND+nP12+nN12],7);
            root.print(";");
            root.print(Timestamps[i+nGND+nP12+nN12]);              
            root.println(";");
          }
          currentMode++;
          configModeDONE=false;
          disableAllRelais();
          configData[1]=0;
          configData[2]=0;
          root.close();
          deviceWorks[currentMode-1]=true;          
          configCharacteristic->writeValue(&configData[0],3);
          delay(500);
          if(deviceWorks[0] && deviceWorks[1] && deviceWorks[2] && deviceWorks[3]){
            Serial.println(" ~ DEVICE WORKS ~ ") ;  
            }else{
            Serial.println(" ~ DEVICE DOES NOT WORK ~ ") ;    
            Serial.println(" RESULTS: ") ;
            Serial.print("GND: ");
            Serial.println(deviceWorks[0]);
            Serial.print("P12: ");
            Serial.println(deviceWorks[1]);
            Serial.print("N12: ");
            Serial.println(deviceWorks[2]);
            Serial.print("SIN: ");
            Serial.println(deviceWorks[3]);
              }
          
        }      

      default:
        //TODO      
        break;

        
      }
    
    
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  if(printDataPointNumber){
    Serial.println(dataPoint);
    }
  
  delay(1000); // Delay a second between loops.
} // End of loop

void disableAllRelais(){
        digitalWrite(32,1);
        digitalWrite(33,1);
        digitalWrite(25,1);
        digitalWrite(26,1);
        delay(100);
  }

int countFiles(){
  root = SD.open("/elehre");
  root.rewindDirectory();
  int nFiles=0;
  while(true) {
    File entry =  root.openNextFile();
    if (! entry) {
     // no more files
     //Serial.println("**nomorefiles**");
     break;
    }
    //Serial.println(entry.name());
    nFiles++;
    entry.close();
  }
  root.close();
  return nFiles;  
  }
