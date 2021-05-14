
# phyphox ELehreBoard
1. [phyphox Experiments](#qr)
2. [Configuration](#config)
   * [Measure-Mode](#Measure-Mode)
   * [Oscilloscope-Mode](#Oscilloscope)
   * [Test-Mode](#Test-Mode) 
3. [Specifications](#Specification)
4. [Test-Results](#Test-Results)

## phyphox Experiments <a name="qr"></a>
Mode 1

![Mode 1](XML/ELehreBoardMode1_dominik.png?raw=true "Mode 1")

Mode 2

![Mode 2](XML/ELehreBoardMode2_dominik.png?raw=true "Mode 2")

## Config <a name="config"></a>
Byte 0

Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0
------|-------|-------|-------|-------|-------|-------|-------
| | | | 0: Treshold falling edge | 0: Disable CH II| 0: Disable CH I|  0: Live-Mode | 0: Measure-Mode
| | | | 1: Treshold rising edge | 1: Enable CH II| 1: Enable CH I | 1: Oscilloscope-Mode| 1: Test-Mode


### Measure-Mode *(Byte 0, Bit 0 = 0)* <a name="Measure-Mode"></a>
#### Data-Rate
Byte 1
Value| SPS ADS1115  | SPS Internal ADC
-----|----------|-------------
0x00  | 8 | 
0x20 | 16 |
0x40 | 32 |
0x60 | 64 |
0x80  | 128 |
0xA0  | 250 |
0xC0  | 475 |
0xE0  | 860 |

#### Treshold *(Oscilloscope-Mode)*
Byte 2 - Byte 3 <br>
*int16_t* in mV

#### Number of datapoints in Oscilloscope-Mode <a name="Oscilloscope"></a>
Byte 3 - Byte 4
*uint16_t*

### Test-Mode *(Byte 0, Bit 0 = 1)* <a name="Test-Mode"></a>
Byte 1: <br>
*uint8_t* number of active test cycle
Byte 2: <br>
*uint8_t* number of required data points

## Specifications <a name="Specification"></a>
* Voltage range: ±12V
* Least significant bit: 0.37mV
* Input resistance 1MΩ
### Charging electronics
* Undervoltage protection: 3.3V
* Overvoltage protection: 4.1V
* Charge current: 0.5A
### Current consumption
* active Advertising: 0.8mA
* at measureing with both channels: 5.4mA




## Test-Results <a name="Test-Results"></a>

