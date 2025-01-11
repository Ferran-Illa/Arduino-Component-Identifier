/*
 * Common Header File
 */

/*
 * Necessary Header includes:
 */


// Basic Includes
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <Arduino.h>
#include <HardwareSerial.h>

// AVR (Maybe we don't need all of them)
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#define COMMON_H

// Probes
class Probe
{
  public:
    const byte ID;        // ANALOG PIN
    const byte Rl;        // DIGITAL PIN // Low value shunt resistor
    const byte Rm;        // DIGITAL PIN // middle-value shunt resistor (geometric mean of Rl and Rh)
    const byte Rh;        // DIGITAL PIN // High value shunt resistor
    const float Rl_val;   //In  Ohms
    const float Rm_val;   //In  kOhms
    const float Rh_val;   //In  kOhms

};

// Probe Setup, with callibrated R values.
/*
const Probe P1 = 
{
  .ID = 15, //A1
  .Rl = 5, .Rm = 6, .Rh = 7,
  .Rl_val = 663.4,
  .Rm_val = 21.80,
  .Rh_val = 677.5,
};

const Probe P2 = 
{
  .ID = 16, //A2
  .Rl = 8, .Rm = 9, .Rh = 10,
  .Rl_val = 662.3,
  .Rm_val = 21.50,
  .Rh_val = 677.7,
};

const Probe P3= 
{
  .ID = 17, //A3
  .Rl = 11, .Rm = 12, .Rh = 13,
  .Rl_val = 659.7,
  .Rm_val = 21.67,
  .Rh_val = 687.0,
};
*/


// Resistor
class Resistor_Specs
{
  public:
    float R_Value;
    char Power;    // For k Ohm scale
    byte ProbeA;   // Connected probes' IDs
    byte ProbeB;
};

// Inductor
class Inductor_Specs
{
  public:
    float L_Value;    // In uH
    float R_parasit;  // In Ohms
    byte ProbeA;      // Connected probes' IDs
    byte ProbeB;
};

// Capacitors
class Capacitor_Specs
{
  public:
    float C_Value;  // Capacity
    char Power;     // "u", "n", "p", probably pico ("p") will hardly ever be used
    byte ProbeA;    // Connected probes' IDs
    byte ProbeB;
};

// Diodes, we may want to include the exponential model
class Diode_Specs
{
  public:
    float VdH_Value;  // High Current forward voltage drop (mV)
    float VdL_Value;  // Low  Current forward voltage drop (mV)
    float HI_Value;   // High test current value (uA)
    float LI_Value;   // Low test current value (uA)
    byte Anode;       // Connected probes' IDs
    byte Cathode;
};

//  Semiconductors
class Semic_Specs
{
  public:
    byte Base;              // Connected probes' IDs
    byte Collector;
    byte Emitter;
    float _V1_;             // Voltage Drop #1 (mV)
    float _V2_;             // Voltage Drop #2 (mV)
    float I_B;              // Base Current for BJT (uA)
    unsigned int Beta;      // Amplification factor (BJT)
};

// Flags:
#define BJT_FLAG        0b00000010 // 2
#define MOS_FLAG        0b00000011 // 3
#define NPN_FLAG        0b00000100 // 4
#define PNP_FLAG        0b00000101 // 5
#define NMOS_ENH_FLAG   0b00000110 // 6
#define NMOS_DEP_FLAG   0b00000111 // 7
#define PMOS_ENH_FLAG   0b00001000 // 8
#define PMOS_DEP_FLAG   0b00001001 // 9
#define DIODE_AC_FLAG   0b00010000 // 1 << 4 (16)
#define DIODE_CA_FLAG   0b00010001 // 1 << 4 + 1 (17)
#define CAPACITOR_FLAG  0b00100000 // 1 << 5 (32)
#define INDUCTOR_FLAG   0b01000000 // 1 << 6 (64)
#define RESISTOR_FLAG   0b10000000 // 1 << 7 (128)
#define SHORT_CIRCUIT_FLAG 0b00001111  // 15
#define OPEN_CIRCUIT_FLAG  0b11110000  // 240        

// Attributes, Global Variables to be modified within functions
namespace attr
{
  extern Resistor_Specs    Resistor;
  extern Capacitor_Specs   Capacitor;
  extern Inductor_Specs    Inductor;
  extern Diode_Specs       Diode;
  extern Semic_Specs       Semiconductor;
  //extern bool Use_Rh;
}

extern const Probe P1;
extern const Probe P2;
extern const Probe P3;