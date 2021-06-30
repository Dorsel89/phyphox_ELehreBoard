
# phyphox ELehreBoard
1. [phyphox Experiments](#qr)
2. [Configuration](#config)
   * [Measure-Mode](#Measure-Mode)
   * [Oscilloscope-Mode](#Oscilloscope)
   * [Config-Mode](#Config-Mode)
3. [Specifications](#Specification)
4. [Test-Results](#Test-Results)

## phyphox Experiments <a name="qr"></a>
**Livemode + Amperemeter feature**

![Livemode](XML/elehrelivemode.png?raw=true "Livemode")
<br>
**Oscilloscope-Mode** <br>
HEX Config: 16E00064012C <br>
byte 0: 0x16, binary 00010110 <br>
Byte 1: 0xE0, 860 SPS <br>
Byte 2-3:  0x0064, uint16 100mV threshold <br>
Byte 4-5:  0x012C, int16 300 number of samples


https://cryptii.com/pipes/integer-encoder <br>
https://www.rapidtables.com/convert/number/binary-to-hex.html

![Oscilloscope-Mode](XML/oscilloscope-mode.png?raw=true "Oscilloscope")



## Config <a name="config"></a>

Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0
------|-------|-------|-------|-------|-------|-------|-------
| | | | 0: Threshold falling edge | 0: Disable CH II| 0: Disable CH I|  0: Live-Mode | 0: Measure-Mode
| | | | 1: Threshold rising edge | 1: Enable CH II| 1: Enable CH I | 1: Oscilloscope-Mode| 1: ConfigMode


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

#### Threshold *(Oscilloscope-Mode)*
Byte 2 - Byte 3 <br>
*int16_t* in mV

#### Number of datapoints in Oscilloscope-Mode <a name="Oscilloscope"></a>
Byte 4 - Byte 5
*uint16_t*

### Config-Mode *(Byte 0, Bit 0 = 1)* <a name="Config-Mode"></a>
Byte 0: <br>
if bit 1 is 1, the config values will be stored on flash <br>
Byte 1: <br>
*float* ground offset of ChI in Volt <br>
Byte 2: <br>
*float* ground offset of ChII in Volt <br>

## Specifications <a name="Specification"></a>
* Sensors
* * ADS1115 16bit 
* * NRF52840 internal ADC 8-12bit (up to 200kHz sampling rate)
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

