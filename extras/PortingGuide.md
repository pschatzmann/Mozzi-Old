# Porting Guide

This is a quick checklist of what needs to be done to support a new processor architecture:

1. Add the logic into __hardware_defines.h__ which identifies your new implementation
2. Implement a new AudioConfig\<PLATFORM\>.h
3. Implement a new implementation class in Mozzi\<PLATFORM\>.h (and optionally cpp) 
4. Add the class as forward declaration to mozi_config_defines.h
5. Implement or activate the following methods in mozzi_pgmspace.h for your environement
    - FLASH_OR_RAM_READ
    - CONSTTABLE_STORAGE
6. Include your AudioConfig\<PLATFORM.h\> to AduioConfigAll.h



Before you start the excercise I recommend to get a proper feeling for the timings

| Function             | Nano 33 Sense  | ESP32     | Pico MBed     | Nano AVR |
|----------------------|----------------|-----------|---------------|----------|
| aSin(SIN2048_DATA)   |  27'000        | 780'500   |   27'000      | 115'000  |              
| Output of 1 Pin      |  53'000        | NA        |   53'500      |          |
| Output of 2 Pins     |  38'000        | NA        |   44'500      | 153'000  |

- The values are in samples per second - rounded by 500) 