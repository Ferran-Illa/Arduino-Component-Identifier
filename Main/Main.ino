#include <config.h>
#include <common.h>
#include <functions.h>
// Our Probes, with callibrated resistance values.
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

const byte BRB_pin = 4;
volatile bool buttonPressed = true; // A first measure is carried when booting.

namespace attr
{
  Resistor_Specs    Resistor;
  Capacitor_Specs   Capacitor;
  Inductor_Specs    Inductor;
  Diode_Specs       Diode;
  Semic_Specs       Semiconductor;
  //extern bool Use_Rh;
}

void setup()
{
  Serial.begin(9600);
  while(!Serial){};

  pinMode(P1.ID, INPUT); // Starting up INPUT pins 
  pinMode(P2.ID, INPUT);
  pinMode(P3.ID, INPUT);

  pinMode(BRB_pin, INPUT_PULLUP); // The Waiting Button
}

void loop()
{
  if(buttonPressed)
  {
    Serial.println(""); // Newline
    Serial.println(""); // Newline
    Serial.println("NEW MEASURE:");
    byte dut_flag = identify(0, P1, P2, P3);

    switch (dut_flag)
    {
      case BJT_FLAG: // 2
          Serial.println(" DEVICE: Unidentified BJT ");
          break; 

      case MOS_FLAG: // 3
          Serial.println(" DEVICE: Unidentified MOS ");
          break; 

      case NPN_FLAG: // 4
      case PNP_FLAG: // 5
      case NMOS_ENH_FLAG: // 6
      case NMOS_DEP_FLAG: // 7
      case PMOS_ENH_FLAG: // 8
      case PMOS_DEP_FLAG: // 9
          display(attr::Semiconductor, dut_flag);
          break;

      case DIODE_AC_FLAG: // 1 << 4 (16)
        display(attr::Diode, dut_flag);
        break; 

      case DIODE_CA_FLAG: // 1 << 4 + 1 (17)
        display(attr::Diode, dut_flag);
        break; 

      case CAPACITOR_FLAG: // 1 << 5 (32)
        display(attr::Capacitor, dut_flag);
        break; 

      case INDUCTOR_FLAG: // 1 << 6 (64)
        display(attr::Inductor, dut_flag);
        break; 

      case RESISTOR_FLAG: // 1 << 7 (128)
        display(attr::Resistor, dut_flag);
        break; 

      case SHORT_CIRCUIT_FLAG: // 15
        Serial.println(" SHORTED PROBES ");
        break;  

      case OPEN_CIRCUIT_FLAG: // 240
        Serial.println("DEVICE: Open Circuit");
        break;          
      
      default:
        Serial.println("MEASUREMENT ERROR");
        break;
    }
    buttonPressed = false;
  }

  buttonPressed = !digitalRead(BRB_pin); // Set flag if button is pressed (reading is LOW)
  waitmsg(buttonPressed);
}

void waitmsg( bool buttonPressed )
{
  static int repeats = 0;
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();

  if(!repeats)
  {
    Serial.println(""); // Newline
    Serial.println("Waiting Button Press");
    repeats = 10;
  }

  if(buttonPressed){ repeats = 0; delay(100);} // Resetting the message

  if(currentTime - lastTime >= 5000)
  {
    Serial.print(" . ");
    repeats --;
    lastTime = currentTime;
  }
}


bool waitdischarge(const byte ID1, const byte ID2, const byte ID3)
{
    byte t = 0;
    do{
        delay(100); // Wait for discharge
        t = t+1;
        if (t>100){return 1;} // Raise error. We have taken too long.
    }while( (analogRead(ID1) > 1) | (analogRead(ID2) > 1) | (analogRead(ID3) > 1) ); // If we read some voltage we continue to discharge
    return 0;
}
