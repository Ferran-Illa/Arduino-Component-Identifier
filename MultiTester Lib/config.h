/*
 * Global Configuration and Settings
 */

#define CONFIG_H

#define __AVR_ATmega328PB__ //The ATMEL microcontroller model (see avr/io.h)

// Defining internal resistances of the Board in Ohms
#define INTERNAL_R_LOW 22
#define INTERNAL_R_HIGH 30


// Constant Global Variables
const bool diode_C1anode[8] = {0,1,0,1,0,1,0,0};
const bool diode_C2anode[8] = {0,0,1,1,0,0,1,0};

const bool C3unused[8] = {0,0,0,0,1,1,1,0}; // Specific for C3
const bool C1C2shortcircuit[8] = {0,1,1,1,0,1,1,0};
