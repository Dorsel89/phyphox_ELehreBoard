#include "Adafruit_ADS1015.h"
#include "mbed.h"
#include "phyphoxBle.h"
#include "power_save.h"
#include <chrono>
#include <cstdint>

I2C myI2C(p15, p16);
Adafruit_ADS1115 ads(&myI2C);
int countMultValue = 0;
Timer t;

// adjustable variables:
bool spikeElimination = false;
int mode = 1;
float changeRate = 1.5; // decides when the V-value change is high enough
int multipleValuesAmount =
    300;              // how many values are saved in multiple read functions
int saveForMult = 25; // how many values are saved before the change of V
int switchInput = 1;  // 0 = red, 1 = blue

void receivedData() {
  float readInput;

  /*
     uint8_t readArray[2];
     PhyphoxBLE::read(&readArray[0],1);
     mode = readArray[0];
     if(readArray[1] > 0) {
         spikeElimination = true;
     } else {
         spikeElimination = false;
     }
  */

  //   PhyphoxBLE::read(readInput);
  //   if (readInput > 0) {
  //     spikeElimination = true;
  //     mode = readInput;
  //   } else if (readInput < 0) {
  //     spikeElimination = false;
  //     mode = -readInput;
  //   } else {
  //     mode = 0;
  //   }

  PhyphoxBLE::read(readInput);

  if (readInput != switchInput) {
    if (readInput == 0) {
      ads.startComparator_SingleEnded(1, 4096);
    } else {
      ads.startComparator_SingleEnded(3, 4096);
    }
  }

  switchInput = readInput;
}

void write(float *value) {
  //   float spikeEliminationF = spikeElimination;
  //   float modeF = mode;
  //   PhyphoxBLE::write(value[0], value[1], spikeEliminationF, modeF);

  float input = switchInput;
  PhyphoxBLE::write(value[0], value[1], input);
}

// void sendVoltageADS1115(float *readValue) {
//   float m, b;
//   float dummy = 0.125 * ads.readADC_SingleEnded(
//                             0); // Ohne funktioniert der erste readADC nicht
//   //_______ROT____________________
//   float Input1 = 0.125 * ads.readADC_SingleEnded(0);
//   m = (2 * 1.79) / (2651 - 97.5);
//   b = 1.79 - m * 2651.0;
//   float Input1C = m * Input1 + b;

//   float Input1L = 0.125 * ads.readADC_SingleEnded(1);
//   m = (2 * 6.03) / (2198.0 - 1076.0);
//   b = 6.03 - m * 2198.0;
//   float Input1LC = m * Input1L + b;

//   //_______BLAU____________________
//   m = (2 * 6.03) / (2198.0 - 1076.0);
//   b = 6.03 - m * 2198.0;
//   float Input2 = 0.125 * ads.readADC_SingleEnded(3);

//   // return Voltage in V
//   float Input2C = m * Input2 + b;

//   if (Input1LC >= 1.8) {
//     if (spikeElimination) {
//       if (Input1LC <= 15) {
//         readValue[0] = Input1LC;
//       }
//     } else {
//       readValue[0] = Input1LC;
//     }
//   } else {
//     if (spikeElimination) {
//       if (Input1C < 2.0) {
//         readValue[0] = Input1C;
//       }
//     } else {
//       readValue[0] = Input1C;
//     }
//   }

//   if ((spikeElimination && Input2C <= 15 && Input2C >= -15) ||
//       !spikeElimination) {
//     readValue[1] = Input2C;
//   }
// }

void sendVoltageADS1115(float *readValue) {
  float m, b;
  float Input1L = 0.125 * ads.getLastConversionResults();
  //m = 0.8746;
  //b = -161.5;
  m = 0.0155642023;
  b = -17.1060817;
  b = b - 10.8045;
   m = (2 * 1.79) / (2651 - 97.5);
   b = 1.79 - m * 2651.0;
  float Input1LC = m * Input1L + b;
  if (spikeElimination) {
    if (Input1LC <= 15) {
      readValue[0] = Input1LC;
    }
  } else {
    readValue[0] = Input1LC;
  }
}

// void sendVoltageADS1115(float *readValue, int port) {
//   float m, b;
//   float dummy = 0.125 * ads.readADC_SingleEnded(
//                             0); // Ohne funktioniert der erste readADC nicht

//   if (port == 0) {
//     //_______ROT____________________
//     float Input1 = 0.125 * ads.readADC_SingleEnded(0);
//     m = (2 * 1.79) / (2651 - 97.5);
//     b = 1.79 - m * 2651.0;
//     float Input1C = m * Input1 + b;

//     float Input1L = 0.125 * ads.readADC_SingleEnded(1);
//     m = (2 * 6.03) / (2198.0 - 1076.0);
//     b = 6.03 - m * 2198.0;
//     float Input1LC = m * Input1L + b;

//     if (Input1LC >= 1.8) {
//       if (spikeElimination) {
//         if (Input1LC <= 15) {
//           readValue[0] = Input1LC;
//         }
//       } else {
//         readValue[0] = Input1LC;
//       }
//     } else {
//       if (spikeElimination) {
//         if (Input1C < 2.0) {
//           readValue[0] = Input1C;
//         }
//       } else {
//         readValue[0] = Input1C;
//       }
//     }

//   } else {
//     //_______BLAU____________________
//     m = (2 * 6.03) / (2198.0 - 1076.0);
//     b = 6.03 - m * 2198.0;
//     float Input2 = 0.125 * ads.readADC_SingleEnded(3);

//     // return Voltage in V
//     float Input2C = m * Input2 + b;

//     if ((spikeElimination && Input2C <= 15 && Input2C >= -15) ||
//         !spikeElimination) {
//       readValue[1] = Input2C;
//     }
//   }
// }

bool eliminateSpikes(float *value) {
  if (value[0] > 15.0 || value[1] > 15) {
    return false;
  }
  return true;
}

// void readMulitplevalues(float *oldValue) {
//   float values[multipleValuesAmount];
//   float result[2];
//   for (int i = 0; i < multipleValuesAmount; i++) {
//     sendVoltageADS1115(oldValue);
//     values[i] = oldValue[switchInput];
//     // ThisThread::sleep_for(1ms);
//   }
//   for (int i = 0; i < multipleValuesAmount; i++) {
//     result[0] = values[i];
//     result[1] = i + countMultValue * multipleValuesAmount;
//     write(result);
//     ThisThread::sleep_for(10ms);
//   }
//   countMultValue++;
// }

bool checkChange(float *oldValue, float changeRate, bool spikes) {
  float newValue[2];
  //   sendVoltageADS1115(oldValue, switchInput);
  sendVoltageADS1115(oldValue);

  if (changeRate == 0) {

  } else if (oldValue[switchInput] >= newValue[switchInput] * changeRate ||
             oldValue[switchInput] <=
                 newValue[switchInput] * (1 / changeRate)) {
    oldValue[switchInput] = newValue[switchInput];
    return true;
  } else {
    oldValue[switchInput] = newValue[switchInput];
    return false;
  }
  return false;
}

void waitAndReadMult(float *oldValue, int prevAmount) {
  float prevValues[prevAmount];
  float prevValuesTime[prevAmount];
  float multVal[multipleValuesAmount];
  float multValTime[multipleValuesAmount];
  float result[2];
  int count = 0;
  int saved = 0;
  //   sendVoltageADS1115(oldValue, switchInput);
  sendVoltageADS1115(oldValue);

  t.start();

  while (!checkChange(oldValue, changeRate, spikeElimination)) {
    prevValues[count] = oldValue[switchInput];
    prevValuesTime[count] =
        duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();
    saved++;
    if (saved >= prevAmount) {
      saved = prevAmount - 1;
    }
    count = (count + 1) % prevAmount;
    // ThisThread::sleep_for(1ms);
  }

  for (int i = saved; i < multipleValuesAmount; i++) {
    multVal[i] = oldValue[switchInput];
    multValTime[i] =
        duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count();
    // ThisThread::sleep_for(1ms);
    //   sendVoltageADS1115(oldValue, switchInput);
    sendVoltageADS1115(oldValue);
  }

  t.stop();

  for (int i = 0; i < saved; i++) {
    multVal[i] = prevValues[(count + i - saved) % prevAmount];
    multValTime[i] = prevValuesTime[(count + i - saved) % prevAmount];
  }

  for (int i = 0; i < multipleValuesAmount; i++) {
    result[0] = multVal[i];
    result[1] = multValTime[i];

    if (result[1] >= 1) {
      write(result);
    }
    ThisThread::sleep_for(5ms);
  }
}

int main() {
  float value[2];
  PhyphoxBLE::start("test");
  ads.setGain(GAIN_ONE);
  ThisThread::sleep_for(30ms);
  sendVoltageADS1115(value); // ititialize value.
  if(switchInput == 0) {
      ads.startComparator_SingleEnded(1, 4096);
  } else {
      ads.startComparator_SingleEnded(3, 4096);
  } 

  PhyphoxBLE::configHandler = &receivedData;
  
  sendVoltageADS1115(value); // ititialize value.

  while (true) {
    PhyphoxBLE::poll();
    switch (mode) {
    case (1): // continuous read + write with respect to spikes
      sendVoltageADS1115(value);
      write(value);
      // waitAndReadMult(value);
      break;
    case (
        2): // read multiple values if there is a change with respect to spikes
      waitAndReadMult(value, saveForMult);
      break;
    default: // continuous write 1.0
      float defaultArray[] = {1.0, 1.0};
      write(defaultArray);
      break;
    }
    // ThisThread::sleep_for(30ms);
  }
}