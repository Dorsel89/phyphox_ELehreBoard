#include <SD.h>

// +-offset which is allowed
float limitGND = 0.050;//+-50mV
float limitP12 = 0.10;//+-100mV
float limitN12 = 0.10;//+-100mV
float limitSTD = 0.010;//std +- 10mV

float targetGND = 0;
float targetP12 = 12;
float targetN12 = -12;

int16_t threshold = 200;//200mV

File root;
int storedDataFiles = 0;
int LED_Pin = 2;

String Address = "";
char* serialNumberString = "";
char buffer[21];

#include "BLEDevice.h"
// The remote service we wish to connect to.
static BLEUUID serviceUUID("cddf1001-30f7-4671-8b43-5e40ba53514a");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("cddf1002-30f7-4671-8b43-5e40ba53514a");
static BLEUUID    configUUID("cddf1003-30f7-4671-8b43-5e40ba53514a");
static BLEUUID    controleUUID("cddf1004-30f7-4671-8b43-5e40ba53514a");

bool printData = true;

uint8_t configData[20] = {0};

const int nGND = 100;
const int nP12 = 100;
const int nN12 = 100;
const int nSIN = 400;

int volatile dataPointsRequired =0;
const int numberOfSamples = nGND + nP12 + nN12 + nSIN;
float* V1_Pointer = NULL;
float* V2_Pointer = NULL;
float* Timestamps1_Pointer = NULL;
float* Timestamps2_Pointer = NULL;

bool deviceWorks[4] = {false};



int countFiles();
void disableAllRelais();
long volatile modeDataPoint = 0;
long allDataPoint = 0;
volatile bool measuring = false;

bool GROUND_MODE_INITIALIZED = false;
bool P12_MODE_INITIALIZED = false;
bool N12_MODE_INITIALIZED = false;
bool SIN_CHI_MODE_INITIALIZED = false;
bool SIN_CHII_MODE_INITIALIZED = false;

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* configCharacteristic;
static BLERemoteCharacteristic* controleCharacteristic;
static BLEAdvertisedDevice* myDevice;
uint8_t dataReceived[20] = {0};

int volatile currentMode = 0;
#define MODE_GND 1
#define MODE_P12 2
#define MODE_N12 3
#define MODE_SIN_CHI 4
#define MODE_SIN_CHII 5

bool doSomeStatistics(int dataPoints, float* median, float* varianz);
void checkLimits(float* median, float* varianz);
void saveData();

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  float myfloats[4];
  memcpy(&myfloats[0], pData, 16);
  if (measuring && modeDataPoint < dataPointsRequired) {
    if(V1_Pointer != NULL){
      V1_Pointer[modeDataPoint] = myfloats[0];
      if(printData){
        Serial.print("V1: ");
        Serial.print(V1_Pointer[modeDataPoint],5);
        }
      }
    if(V2_Pointer != NULL){
      V2_Pointer[modeDataPoint] = myfloats[1];
      if(printData){
        Serial.print(" V2: ");
        Serial.print(V2_Pointer[modeDataPoint],5);
        }
      }
    if(Timestamps1_Pointer != NULL){
      Timestamps1_Pointer[modeDataPoint] = myfloats[2];
      if(printData){
        Serial.print(" T1: ");
        Serial.print(Timestamps1_Pointer[modeDataPoint],5);
        }
      }
    if(Timestamps2_Pointer != NULL){
      Timestamps2_Pointer[modeDataPoint] = myfloats[3];
      if(printData){
        Serial.print(" T2: ");
        Serial.print(Timestamps2_Pointer[modeDataPoint],5);
        }
      }                  
      if(printData){
        Serial.println("");
      }
      

    modeDataPoint += 1;
    allDataPoint += 1;    
  }
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      currentMode = 0;
      GROUND_MODE_INITIALIZED = false;
      P12_MODE_INITIALIZED = false;
      N12_MODE_INITIALIZED = false;
      SIN_CHI_MODE_INITIALIZED = false;
      SIN_CHII_MODE_INITIALIZED = false;
    }

    void onDisconnect(BLEClient* pclient) {
      connected = false;
      currentMode = 0;
      Serial.println("disconnected");
    }
};


bool connectToServer() {
  Serial.print("Forming a connection to ");
  Address = myDevice->getAddress().toString().c_str();
  Serial.println(Address);
  Address.replace(":", "");

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
  controleCharacteristic = pRemoteService->getCharacteristic(controleUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
}
/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
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
  pinMode(32,  OUTPUT );
  pinMode(33, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);

  pinMode(LED_Pin, OUTPUT);
  digitalWrite(LED_Pin, 1);
  digitalWrite(32, 1);
  digitalWrite(33, 1);
  digitalWrite(25, 1);
  digitalWrite(26, 1);

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
    if(currentMode==0){
      
        // reset offsets
        float zero[2]={0.0};
        configData[0]=1;
        memcpy(&configData[1],&zero[0],8);
        configCharacteristic->writeValue(&configData[0], 20);
        delay(100);
        
        //connected to a new device! go into live mode for first measurements
        configData[0] = 0b00001100; //live mode, CHI + CHII
        configData[1] = 0x60;       //128SPS (per channel -> receiving 64 sps in theory )
        configCharacteristic->writeValue(&configData[0], 20);
        currentMode=1;
        GROUND_MODE_INITIALIZED = false;
        P12_MODE_INITIALIZED = false;
        N12_MODE_INITIALIZED = false;
      
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
      }else if(currentMode ==1){
        if (!GROUND_MODE_INITIALIZED){
          float _V1[nGND]={0};
          float _V2[nGND]={0};
          float _Timestamps1[nGND]={0};
          float _Timestamps2[nGND]={0};
          V1_Pointer = _V1;
          V2_Pointer = _V2;
          Timestamps1_Pointer = _Timestamps1;
          Timestamps2_Pointer = _Timestamps2;
          Serial.println("GROUND SIGNALS ON ALL INPUTS");        
          disableAllRelais();
          digitalWrite(32, 0);
          delay(100);
          measuring = true;
          modeDataPoint = 0;
          allDataPoint = 0;
          GROUND_MODE_INITIALIZED=true;
          dataPointsRequired = nGND;
          
        }else{
          if(modeDataPoint == dataPointsRequired && GROUND_MODE_INITIALIZED){
            measuring=false;
            modeDataPoint=0;
            float myMedian[2]={0};
            float myVarianz[2]={0};
            doSomeStatistics(nGND, myMedian, myVarianz);
            checkLimits(myMedian, myVarianz);
            saveData();
            configData[0]=3;
            memcpy(&configData[1],&myMedian[0],8);
            configCharacteristic->writeValue(&configData[0], 20);
            currentMode=2;
            }
          }
      }else if(currentMode==2){
        if (!P12_MODE_INITIALIZED){
          float _V1[nP12]={0};
          float _V2[nP12]={0};
          float _Timestamps1[nP12]={0};
          float _Timestamps2[nP12]={0};
          V1_Pointer = _V1;
          V2_Pointer = _V2;
          Timestamps1_Pointer = _Timestamps1;
          Timestamps2_Pointer = _Timestamps2;
          Serial.println("+12 SIGNALS ON ALL INPUTS");        
          disableAllRelais();
          digitalWrite(33,0); 
          delay(100);
          measuring = true;
          modeDataPoint = 0;
          P12_MODE_INITIALIZED=true;
          dataPointsRequired = nP12;
          return;
        }else{
          if(modeDataPoint == dataPointsRequired && P12_MODE_INITIALIZED){
            measuring=false;
            float myMedian[2]={0};
            float myVarianz[2]={0};
            doSomeStatistics(nP12,myMedian, myVarianz);
            checkLimits(myMedian, myVarianz);
            saveData();
            currentMode=3;
            
           }
        }
      }else if(currentMode == 3){        
        if (!N12_MODE_INITIALIZED){
          float _V1[nN12]={0};
          float _V2[nN12]={0};
          float _Timestamps1[nN12]={0};
          float _Timestamps2[nN12]={0};
          V1_Pointer = _V1;
          V2_Pointer = _V2;
          Timestamps1_Pointer = _Timestamps1;
          Timestamps2_Pointer = _Timestamps2;
          Serial.println("-12 SIGNALS ON ALL INPUTS");        
          disableAllRelais();
          digitalWrite(25,0); 
          delay(100);
          measuring = true;
          modeDataPoint = 0;
          N12_MODE_INITIALIZED=true;
          dataPointsRequired = nN12;
          return;
        }else{
          if(modeDataPoint == dataPointsRequired && N12_MODE_INITIALIZED){
            measuring=false;
            float myMedian[2]={0};
            float myVarianz[2]={0};
            doSomeStatistics(nN12,myMedian, myVarianz);
            checkLimits(myMedian, myVarianz);
            saveData();
            currentMode=4;
            return;
           }
        }
    }else if(currentMode == 4){
      if(!SIN_CHI_MODE_INITIALIZED){
        uint8_t buf[2]={0};
        int test[1] = {nSIN};
        int16_t intBuf[1]={threshold};
        uint8_t buf2[2]={0};
        memcpy(&buf[0],&test[0],2);
        memcpy(&buf2[0],&intBuf[0],2);
        configData[0] = 0b00010110; //ch1, rising edge
        configData[1] = 0xE0;       //860 SPS
        configData[2] = buf[1];//0x01;       //500mV threshold
        configData[3] = buf[0];//0xF4;
        configData[4] = buf[1];    
        configData[5] = buf[0];
        delay(5);
        configCharacteristic->writeValue(&configData[0], 20);
        float _V1[nSIN]={0};
        //float _V2[nSIN]={0};
        float _Timestamps1[nSIN]={0};
        //float _Timestamps2[nSIN]={0};
        V1_Pointer = _V1;
        V2_Pointer = NULL;
        Timestamps1_Pointer = _Timestamps1;
        Timestamps2_Pointer = NULL;
        Serial.println("SIN CHI SIGNALS ON ALL INPUTS"); 
        disableAllRelais();
        digitalWrite(26,0);
        delay(100); 
        modeDataPoint = 0;
        measuring = true;
        dataPointsRequired = nSIN;  
        uint8_t controleData[20];
        controleData[0]=1;
        controleCharacteristic->writeValue(&controleData[0], 20);
        SIN_CHI_MODE_INITIALIZED=true;
        return;
        }else{
          if(modeDataPoint == dataPointsRequired && SIN_CHI_MODE_INITIALIZED){
            measuring=false;
            saveData();
            currentMode=5;
            return;
           }
        }
    }else if(currentMode == 5){
      if(!SIN_CHII_MODE_INITIALIZED){
        uint8_t buf[2]={0};
        int test[1] = {nSIN};
        memcpy(&buf[0],&test[0],2);        
        configData[0] = 0b00011010; //ch1, rising edge
        configData[1] = 0xE0;       //860 SPS
        configData[2] = 0x01;       //500mV threshold
        configData[3] = 0xF4;
        configData[4] = buf[1];    
        configData[5] = buf[0];
        
        configCharacteristic->writeValue(&configData[0], 20);
        float _V1[nSIN]={0};
        float _V2[nSIN]={0};
        float _Timestamps1[nSIN]={0};
        float _Timestamps2[nSIN]={0};
        V1_Pointer = NULL;
        V2_Pointer = _V1;
        Timestamps1_Pointer = NULL;
        Timestamps2_Pointer = _Timestamps1;
        Serial.println("SIN CHII SIGNALS ON ALL INPUTS"); 

        measuring = true;
        modeDataPoint = 0;
        SIN_CHII_MODE_INITIALIZED=true;
        dataPointsRequired = nSIN;  
        uint8_t controleData[20];
        controleData[0]=1;
        controleCharacteristic->writeValue(&controleData[0], 20); 
        return;     
        }else{
          if(modeDataPoint == dataPointsRequired && SIN_CHII_MODE_INITIALIZED){
            measuring=false;
            float myMedian[2]={0};
            float myVarianz[2]={0};
            saveData();
            currentMode++;
            disableAllRelais();
            Serial.println("");
            Serial.println("test procedure done");
            Serial.print("Ground: ");
            printResult(deviceWorks[0]);
            Serial.print("+12V: ");
            printResult(deviceWorks[1]);
            Serial.print("-12V: ");
            printResult(deviceWorks[2]);
            Serial.print("Sin: ");
            printResult(1);
            currentMode+=1;
            return;
           }
        }
    }        
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(100); // Delay a second between loops.
} // End of loop

void disableAllRelais() {
  digitalWrite(32, 1);
  digitalWrite(33, 1);
  digitalWrite(25, 1);
  digitalWrite(26, 1);
  delay(100);
}
void printResult(bool passed){
        if(passed){
            Serial.println("passed");
        }else{
            Serial.println("ERROR");
        }  
  }

int countFiles() {
  root = SD.open("/elehre");
  root.rewindDirectory();
  int nFiles = 0;
  while (true) {
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

bool doSomeStatistics(int dataPoints, float* median, float* varianz){
  
  for(int i=0;i<dataPoints;i++){
    median[0] += V1_Pointer[i]/dataPoints;
    median[1] += V2_Pointer[i]/dataPoints;
  }          

  for(int i=0;i<dataPoints;i++){
    varianz[0]+=(V1_Pointer[i]-median[0])*(V1_Pointer[i]-median[0])/(dataPoints-1);
    varianz[1]+=(V2_Pointer[i]-median[1])*(V2_Pointer[i]-median[1])/(dataPoints-1);
  }
  
  Serial.println();
  Serial.printf("Median\tCH I\t%f +- %f \n",median[0],sqrt(varianz[0])/sqrt(nGND));
  Serial.printf("standard deviation\tCH I\t%f \n",sqrt(varianz[0]));
  Serial.printf("Median\tCH II\t%f +- %f \n",median[1],sqrt(varianz[1])/sqrt(nGND));
  Serial.printf("standard deviation\tCH II\t%f \n\n",sqrt(varianz[1]));
  return 0;
}
void checkLimits(float* median, float* varianz){
  float limit;
  float target;
  if(currentMode == MODE_GND){
      limit = limitGND;
      target= targetGND;
      }
  if(currentMode == MODE_P12){
      limit = limitP12;
      target= targetP12;
      }
    if(currentMode == MODE_N12){
      limit = limitN12;  
      target= targetN12;          
    }
  
  Serial.print("target was: ");
  Serial.print(target);
  Serial.print(" +- "); 
  Serial.println(limit);
  
  if(sqrt(varianz[0])<limitSTD && (median[0]<target+limit) && (median[0]>target-limit)){
    Serial.print("CHI looks good  -  ");
    if(sqrt(varianz[1])<limitSTD && (median[1]<target+limit) && (median[1]>target-limit)){
      Serial.println("CHII looks good");
      deviceWorks[currentMode-1]=true;
    }else{
      Serial.println("problem on CHII");
      }
  }else{
     Serial.print("problem on CHI  -  ");
     if(sqrt(varianz[1])<limitSTD && (median[1]<target+limit) && (median[1]>target-limit)){
      Serial.println("CHII looks good");
      }
      else{
      Serial.println("problem on CHII");
      }
    }     
    Serial.println("");
}
void saveData(){
  root = SD.open("/elehre/" + Address + ".txt", FILE_APPEND);
  int saveLength =0;
  if(currentMode == 1){
      root.println("GROUND MODE");
      saveLength  = nGND;
  }else if(currentMode == 2){      
      root.println("P12 MODE");
      saveLength  = nP12;
  }else if(currentMode == 3){      
      root.println("N12 MODE");
      saveLength  = nN12;
  }else if(currentMode == 4){       
      root.println("Oszi CHI");
      saveLength  = nSIN;
   }else if(currentMode == 5){
      root.println("Oszi CHII");
      saveLength  = nSIN;    
    }
  for(int i = 0;i<saveLength;i++){
    if(V1_Pointer!=NULL){
      root.print(V1_Pointer[i],7);
      root.print('\t');
      }

    if(V2_Pointer!=NULL){
      root.print(V2_Pointer[i],7);
      root.print('\t');
      }

    if(Timestamps1_Pointer!=NULL){
      root.print(Timestamps1_Pointer[i],7);
      root.print('\t');
      }

    if(Timestamps2_Pointer!=NULL){
      root.print(Timestamps2_Pointer[i],7);
      }
     root.println("");
    
  }
  root.close(); 
}
