#include "ADS1X15.h"
#include "mbed.h"
#include "phyphoxBle.h"
#include "power_save.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include "flashUtility.h"

#define TESTMODUS_GND 1
#define TESTMODUS_P12 2
#define TESTMODUS_N12 3
#define TESTMODUS_SIN 4

#define MODE_DUAL_ADS1115 0
#define MODE_FAST01_ADS1115 1
#define MODE_FAST23_ADS1115 2
#define MODE_FAST_ONBOARD 3

bool zeroRuns=true;

volatile bool testMode = false;
volatile bool CHI =false;
volatile bool CHII =false;
volatile bool fastMode = false;
volatile bool risingEdge = false;
volatile bool internalADC = false;

volatile float GROUNDOFFSET[2] = {0};// = 0.025187502;


volatile float threshold =0;


DigitalOut myLED(P0_4); //rote led
//AnalogIn onBoardADC1(P0_4); //rote led

PinName scl = p13; //P0_13;    
PinName sda = p14; 
PinName rdy = p15;

//float R1 =68;
//float R2 =480;

float R1 =10;
float R2 =130;

DigitalIn adcReady(rdy);

uint8_t configData[20];
uint8_t osziControlgData[20];
volatile bool activeTrigger = false;
volatile uint16_t currentDataRate = RATE_ADS1115_64SPS;

I2C myI2C(sda, scl);
Adafruit_ADS1115 ads(&myI2C);
Timer t;                        // Used for mode2
float prevValue = 0;            // Used for writePrev()
float prevTime = 0;             // Used for writePrev()

// adjustable variables:

volatile uint16_t multipleValuesAmount = 100; // how many values are saved in multiple read functions

int ledCounter=0;
LowPowerTicker  powerOnBlink;

uint32_t test = 0x0FE000;
FLASH myCONFIG(test);

void blinkNtimes(int times){
    for(int i=0;i<times;i++){
        myLED=!myLED;
        ThisThread::sleep_for(200ms);
        myLED=!myLED;
        ThisThread::sleep_for(200ms);
    }
 }
void powerOn() {
    if(ledCounter==19){
        myLED = 0;
        ledCounter=0;
    }else {
        myLED = 1;
        ledCounter+=1;
    }
    
}



bool getNBit(uint8_t byte, int position){
    return byte & (1<<position);
}
void receiveOsziControleData(){
    PhyphoxBLE::readOszi(&osziControlgData[0],20);
    activeTrigger = osziControlgData[0];
}

/**
    Gets data from PhyPhox app and uses it to switch between modes and inputs
*/
void receivedData() {           // get data from phyphox app
  PhyphoxBLE::read(&configData[0],20);
  t.reset();
  testMode = getNBit(configData[0],0);  //false
  if(testMode){
    CHI=1;
    CHII=1;
  }else {
    fastMode = getNBit(configData[0],1);
    CHI = getNBit(configData[0],2);
    CHII = getNBit(configData[0],3);
    risingEdge = getNBit(configData[0],4);
    internalADC = getNBit(configData[0],5);
    uint16_t targetRate = configData[1];
    ads.setDataRate(targetRate);
    threshold = 0.001* (configData[3] | (configData[2] << 8));
    multipleValuesAmount = (configData[5] | (configData[4] << 8));
  }

}

/**
    Used to be able to get inputs while being in a loop when waiting for a change in mode2.
*/


float calibrateVoltage(float raw, int Channel){
    if(Channel == 1 ){
        return (-1*raw*R2/R1 + GROUNDOFFSET[0]);
    }else{
        return (-1*raw*R2/R1 + GROUNDOFFSET[1]);
    }
    
}
/*
    Red pins to get voltage value.

    @param readValue The array in which the value should be stored in at position 0.
*/

void measureVoltageADS1115(float *readValue) {
  
  if(CHI){
    readValue[0] = calibrateVoltage(ads.computeVolts(ads.readADC_Differential_0_1()),1);
    readValue[2] = 0.001*duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();
   
  }else {
    readValue[0]=0;
  }
  if(CHII){
    readValue[1] = calibrateVoltage(ads.computeVolts(ads.readADC_Differential_2_3()),2);                                  // store value.
    readValue[3]= 0.001*duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();
  }else {
    readValue[1]=0;
  }
}

/*
    The main code fore mode 2. It waits until the voltage changed. After that, it reads a given amount of
    values. After reading the values, they will be sent to PhyPhox. 

    @param value the array where the values will be stored in.
    @param prevAmount the max amount of values which should be saved before the voltage change triggers.
*/

void waitAndReadMult(float* _voltage, float* _timestamp) {
   
  //float multVal[multipleValuesAmount];
  //float multValTime[multipleValuesAmount];
  float result[2];
  int channeloffset =1;
  if(CHI){//CHannel 1-2
      ads.con_diffEnded(1);
      channeloffset =1;
  }else {//channel 2-3
      ads.con_diffEnded(0);     
      channeloffset=2;
  }
  

int counter = 0;
bool triggerFired = false;
float voltagebuffer[2];
float timeBuffer[2];
bool firstValue = false;

t.reset();
t.start();

 while (counter<multipleValuesAmount) {


    if(adcReady==0 && !triggerFired && counter < 2){
        float voltage = calibrateVoltage(ads.computeVolts(ads.getLastConversionResults()),channeloffset);
        if(!firstValue){
            voltagebuffer[0]=voltage;
            timeBuffer[0]=duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();        
            firstValue=true;
            
            continue;
        }else if(firstValue && !triggerFired){
            voltagebuffer[1] = voltagebuffer[0];
            timeBuffer[1]=timeBuffer[0];

            voltagebuffer[0] = voltage;
            timeBuffer[0]=duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();
        }
        if(risingEdge && voltagebuffer[0]>threshold && voltagebuffer[1]<threshold){
            triggerFired = true;
            counter=2;
            _voltage[0]=voltagebuffer[1];
            _voltage[1]=voltagebuffer[0];
            _timestamp[1] = timeBuffer[0];
            _timestamp[0] = timeBuffer[1];
            
        }else if (!risingEdge && voltagebuffer[0]<threshold && voltagebuffer[1]>threshold) {
            triggerFired = true;
            counter=2;
            _voltage[0]=voltagebuffer[1];
            _voltage[1]=voltagebuffer[0];
            _timestamp[1] = timeBuffer[0];
            _timestamp[0] = timeBuffer[1];
        }
    }

    if(adcReady==0 && triggerFired){
        
        _voltage[counter]=calibrateVoltage(ads.computeVolts(ads.getLastConversionResults()),channeloffset);
        _timestamp[counter]=duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();    
        counter++;
    }
}
t.stop();
    float offset = _timestamp[0];
    for (int i=0; i<multipleValuesAmount; i++) {
        _timestamp[i]  =0.001*(_timestamp[i]-offset);
    }
    
    

}


//###################################################
void runTESTMODE(){

    int nGND;
    int nP12;
    int nN12;
    int nSIN;
    
    float buf[3];
    int delayMS = 0;
    // GROUND TEST
    bool GND=0;

    
    float groundOffsetBuffer[2]={0};
    GROUNDOFFSET[0]=0;
    GROUNDOFFSET[1]=0;
    while (testMode && configData[1]==0) {
        ThisThread::sleep_for(100ms);
    }
    
    while(testMode && configData[1]==1){
        nGND = configData[2];
        if(!GND){
            ThisThread::sleep_for(1s);
            for(int i=0;i<nGND;i++){
                measureVoltageADS1115(buf);
                PhyphoxBLE::write(buf[0],buf[1],buf[2]);
                groundOffsetBuffer[0]+= buf[0]*-1.0/nGND;
                groundOffsetBuffer[1]+= buf[1]*-1.0/nGND;         
                ThisThread::sleep_for(delayMS*1ms);
            }
            GND=true;
        }
        //STORE ON FLASH
        
        ThisThread::sleep_for(100ms);
    }
    
    GROUNDOFFSET[0]=groundOffsetBuffer[0];
    GROUNDOFFSET[1]=groundOffsetBuffer[1];
    
    myCONFIG.writeELehreConfig(groundOffsetBuffer);
    // +12V TEST    ###################################################
    
    bool P12 = false;
    while(testMode && configData[1]==2){
        nP12 = configData[2];
        if(!P12){
            ThisThread::sleep_for(1s);
            for(int i=0;i<nP12;i++){
                measureVoltageADS1115(buf);
                PhyphoxBLE::write(buf[0],buf[1],buf[2]);          
                ThisThread::sleep_for(delayMS*1ms);
            }
            P12=true;
            
        }  
        ThisThread::sleep_for(100ms);
    }
    

    // -12V TEST    ###################################################
    bool N12 = false;
    while(testMode && configData[1]==3){
        nN12 = configData[2];
        if(!N12){
            ThisThread::sleep_for(1s);
            for(int i=0;i<nN12;i++){
                measureVoltageADS1115(buf);
                PhyphoxBLE::write(buf[0],buf[1],buf[2]);          
                ThisThread::sleep_for(delayMS*1ms);            
            }
            N12=true;
        }
        ThisThread::sleep_for(100ms);
    }
    

    // SIN TEST ###################################################
    bool sinus = false;
    
    while(testMode && configData[1]==4){
        nSIN = configData[2];
        //risingEdge = true;
        //multipleValuesAmount = nSin;
        //threshold = 0.5; //0.5V
        if(!sinus){
            ThisThread::sleep_for(1s);
            for(int i=0;i<nSIN;i++){
                measureVoltageADS1115(buf);
                PhyphoxBLE::write(buf[0],buf[1],buf[2]);          
                ThisThread::sleep_for(delayMS*1ms);
            }
            sinus=true;
        }
        ThisThread::sleep_for(100ms);
    }
    ThisThread::sleep_for(100ms);
  

    //measureVoltageADS1115(buf);
    //write(buf);

}
void initOffset(){
    float arrayBufF[2] = {0};
    int8_t error=0;
    error = myCONFIG.readELehreConfig(arrayBufF);
    if(arrayBufF[0]<0.5 && arrayBufF[0]>-0.5){
        GROUNDOFFSET[0] = arrayBufF[0];
    }
    if(arrayBufF[1]<0.5 && arrayBufF[1]>-0.5){
        GROUNDOFFSET[1] = arrayBufF[1];
    }
}
/**
    Initialization and loop which uses a switch case to either behave for mode 1
    or mode 2.
*/
int main() {

    myLED=1; //turn led off
    initOffset();
  //power_save();
  
  float value[4];                           // the array where the values should be stored in.
  PhyphoxBLE::start("elehre");              // start BLE
  
  ads.setGain(GAIN_TWO);
  ads.setDataRate(currentDataRate);
  ThisThread::sleep_for(500ms);

    
    t.reset();
    t.start();
  
  PhyphoxBLE::configHandler = &receivedData;   // used to receive data from PhyPhox.
  PhyphoxBLE::OsziHandler = &receiveOsziControleData;   // used to receive data from PhyPhox.
  
  //powerOnBlink.attach(&powerOn,100ms);

while (true) {                            // start loop.
    //ThisThread::sleep_for(100ms);

    //myLED = !activeTrigger;


    if(PhyphoxBLE::activeConnections>0){
        
        if(testMode){
            //TESTMODE
            //myLED=1;
            if(zeroRuns==true){
                zeroRuns=false;
                CHI=1;
                CHII=1;
                runTESTMODE();

                
            }
        }else if(!testMode) {
            //MEASUREMENT MODE
            
            if(fastMode && activeTrigger){
                //FASTMODE
                myLED = 0;
                activeTrigger = false;
                float voltage[multipleValuesAmount];
                float timestamp[multipleValuesAmount];
                
                waitAndReadMult(voltage,timestamp);
                                
                for(int i=0;i<multipleValuesAmount;i++){
                    PhyphoxBLE::write(voltage[i],timestamp[i]);
                    ThisThread::sleep_for(20ms);
                }
        
                myLED=1;
            }else if(!fastMode) {
                //LIVE MODE
                if(!internalADC){
                    measureVoltageADS1115(value);
                }
                PhyphoxBLE::write(value[0],value[1],value[2],value[3]);                 
            }
            
        }
    }
    else {
        ThisThread::sleep_for(100ms);
    }
    
  }
  
}

