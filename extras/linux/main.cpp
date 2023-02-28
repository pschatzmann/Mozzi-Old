/**
 * @file main.cpp
 * @author Phil Schatzmann
 * @brief Simulate Arduino Control Loop
 * @version 0.1
 * @date 2021-07-04
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <cstdlib>
#include "Arduino.h"
int main(){
    setup();
    while(true){
        loop();
    }
}
