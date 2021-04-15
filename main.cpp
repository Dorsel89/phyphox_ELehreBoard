#include "Adafruit_ADS1X15.h"
#include "mbed.h"
#include "phyphoxBle.h"
#include "power_save.h"
#include <chrono>
#include <cstdint>


DigitalOut myLED(P0_4); //rote led

PinName scl = p13; //P0_13;    
PinName sda = p14; 
PinName rdy = p15;

I2C myI2C(scl, sda);
Adafruit_ADS1115 ads(&myI2C);
Timer t;                        // Used for mode2
float prevValue = 0;            // Used for writePrev()
float prevTime = 0;             // Used for writePrev()

// adjustable variables:
int mode = 1;                   // defines the mode
float changeRate = 1.15;        // decides when the V-value change is high enough
int multipleValuesAmount = 300; // how many values are saved in multiple read functions
int saveForMult = 25;           // how many values are saved before the voltage-change
int switchInput = 0;            // 0 = red, 1 = blue


/**
    Gets data from PhyPhox app and uses it to switch between modes and inputs
*/


void receivedData() {           // get data from phyphox app
  float readInput;
  PhyphoxBLE::read(readInput);

  if (readInput > 1) {          // config 3 = mode1, 4 = mode2
    mode = readInput - 2;
    t.stop();                   // stop reset timer.
    t.reset();
    ThisThread::sleep_for(20ms);
    return;
  }

  if (readInput != switchInput) {   // used to switch input
    if (readInput == 0) {
      ads.startComparator_SingleEnded(1, 4096);
    } else {
      ads.startComparator_SingleEnded(3, 4096);
    }
  }
  switchInput = readInput;
}


/*
    Send data to PhyPhox App.

    @param value an array where the value and (if in mode2) the time is stored.
*/

void write(float *value) {
  float input = switchInput;
  float modeSelected = mode;
  PhyphoxBLE::write(value[0], value[1], input, modeSelected);
}


/**
    Used to be able to get inputs while being in a loop when waiting for a change in mode2.
*/

void writePrev() {
  float input = switchInput;
  float modeSelected = mode;
  PhyphoxBLE::write(prevValue, prevTime, input, modeSelected);
}


/*
    Red pins to get voltage value.

    @param readValue The array in which the value should be stored in at position 0.
*/

void measureVoltageADS1115(float *readValue) {
  readValue[0] = ads.computeVolts(ads.readADC_Differential_0_1());                                  // store value.
  readValue[1] = ads.computeVolts(ads.readADC_Differential_2_3());                                  // store value.
}


/*
    Checks if the voltage changed over a given factor.

    @param oldValue the array where the value, which should be compared, is stored in.
    @param changeRate the factor which defines how much the value has to change to trigger.
    @return if the value changed (enough).
*/

bool checkChange(float *oldValue, float changeRate) {
  float newValue[2];
  measureVoltageADS1115(newValue);

  if (changeRate == 0) {                            // error handling.
    return false;
  } else if (oldValue[0] >= newValue[0] * changeRate ||
             oldValue[0] <= newValue[0] * (1 / changeRate)) {
    oldValue[0] = newValue[0];
    return true;                                    // value has changed (enough).
  } else {
    oldValue[0] = newValue[0];
    return false;                                   // value has not changed (enough).
  }
  return false;
}



/*
    The main code fore mode 2. It waits until the voltage changed. After that, it reads a given amount of
    values. After reading the values, they will be sent to PhyPhox. 

    @param value the array where the values will be stored in.
    @param prevAmount the max amount of values which should be saved before the voltage change triggers.
*/

void waitAndReadMult(float *value, int prevAmount) {
  float prevValues[prevAmount];
  float prevValuesTime[prevAmount];
  float multVal[multipleValuesAmount];
  float multValTime[multipleValuesAmount];
  float result[2];
  int count = 0;
  int saved = 0;
  measureVoltageADS1115(value);

  t.start();                                        // start timer

  while (!checkChange(value, changeRate)) {         // wait until voltage-change is high enough.
    prevValues[count] = value[0];                   // save up to prevAmount values and times.
    prevValuesTime[count] =
        duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();
    saved++;
    if (saved >= prevAmount) {                      // do not exceed prevAmount
      saved = prevAmount - 1;
    }
    count = (count + 1) % prevAmount;       // more than prevAmount -> start from beginning
                                            //          and save prevAmount-1 values before.
    writePrev();                            // used to still read data from PhyPhox while waiting in the loop.
  }

  for (int i = saved; i < multipleValuesAmount; i++) {      // read alltogether multipleValuesAmount values.
    multVal[i] = value[0];
    multValTime[i] =
        duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();
    measureVoltageADS1115(value);
  }

  t.stop();                                                 // stop timer.

  for (int i = 0; i < saved; i++) {             // store the previous saved prevAmount values 
                                                //          in the same array with the rest.
    multVal[i] = prevValues[(count + i - saved) % prevAmount];
    multValTime[i] = prevValuesTime[(count + i - saved) % prevAmount];
  }

  for (int i = 0; i < multipleValuesAmount; i++) {          // send all values to PhyPhox.
    result[0] = multVal[i];
    result[1] = multValTime[i];

    if (result[1] >= 1) {
      write(result);                    // used to send value together with further information.
    }
    ThisThread::sleep_for(5ms);
    prevValue = result[0];
    prevTime = result[1];
  }
}


/**
    Initialization and loop which uses a switch case to either behave for mode 1
    or mode 2.
*/
int main() {
  float value[2];                           // the array where the values should be stored in.
  PhyphoxBLE::start("elehre");              // start BLE
  ads.setGain(GAIN_TWO);
  ThisThread::sleep_for(30ms);
  //sendVoltageADS1115(value);                // initialize value.
  /*
  if (switchInput == 0) {                   // prepare config for the selected input.
    ads.startComparator_SingleEnded(1, 4096);
  } else {
    ads.startComparator_SingleEnded(3, 4096);
  }
  */

  PhyphoxBLE::configHandler = &receivedData;   // used to receive data from PhyPhox.

  while (true) {                            // start loop.

    switch (mode) {
    case (1):                               // continuous read + write with respect to spikes
      measureVoltageADS1115(value);
      write(value);
      break;
    case (2):                               // read multiple values if there is a change with respect to spikes
      waitAndReadMult(value, saveForMult);
      break;
    default:                                // continuous write 1.0
      float defaultArray[] = {1.0, 1.0};
      write(defaultArray);
      break;
    }

  }
}