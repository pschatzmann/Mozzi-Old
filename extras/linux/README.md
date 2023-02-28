# Mozzi in Linux

You can build and run Mozzi under Linux and OS/X. 
We support the following cmake options:

- BUILD_EXAMPLE (default OFF)
- ARDUINO_SKETCH (default extras/linux/example.cpp)

When the BUILD_EXAMPLE option is ON, we build an executable - if it is OFF we build a library. Please note that the Arduino Sketch must be provided as cpp file and that it must start with an #include "Arduino.h".

To build an Mozzi executable:

```
cd Mozzi
mkdir build
cd build
cmake -D BUILD_EXAMPLE ..
make 
```

This generates the executable in the current (build) directory. 
To install it you can execute:

```
make install
```
