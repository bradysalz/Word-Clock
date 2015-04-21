# Word-Clock
MSP430 based Word Clock for my GE423 HW Project. Decided to make it more of a "word search" than a simple light up clock. Tells the time in five minute intervals. 

## Description
First thing to do is program the init script for the RTC. A backup coin cell battery will make sure it shouldn't need to be re-calibrated for another 15 years (approximate). In normal mode, the MSP430 wakes from sleep and pings the RTC approximately every minute. This occurs by doing several I2C register reads from the device. Each of the "words" (listed below) is an LED load with one LED per letter, and the entire word is turned on/off by a transistor (n-channel FET). All of the 21 words' transistor gates are driven by three 8-bit daisy-chained serial-parallel shift registers. Once the RTC data is parsed, it is converted and shifted out to light up the appropriate phrasing. 

We also have a button available on the MSP430 that will open and close shades on the front of the clock via two servo motors. I eventually plan to automate this based on a custom alarm or light sensor. 

### Words
- Minutes
    - Five
    - Ten
    - Quarter
    - Twenty
    - Half
        
+ Phrases
    + Past
    + To
    + It Is
    + O Clock

As well as the hours One through Twelve. 

### To Do
- [x] Outline letters
- [x] Design Schematic
- [x] Design PCB
- [x] Code MSP430 RTC reads
- [x] Code MSP430 shift register writes
- [x] Code MSP430 servo drivers
- [x] Design physical enclosure
- [ ] Build physical enclosure
- [ ] Wire LEDs/resistors
- [ ] Add curtains
