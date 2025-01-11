#define DISP_CPP

#include "common.h"
#include "config.h"
#include "functions.h"

char toHuman(byte numProbe)
{
  switch (numProbe)
  {
  case 15:
    return 'B';
    break;
  case 16:
    return 'G';
    break;
  case 17:
    return 'Y';
    break;
  default:
    return ' ';
    break;
  }
}
// We will overload our function for all the devices we have:
byte display(Resistor_Specs DUT, byte dut_flag)
{
  Serial.println("DEVICE: Resistor ");
  Serial.print("R = "); Serial.print(attr::Resistor.R_Value,1); 
  Serial.print(" "); Serial.print(attr::Resistor.Power); Serial.println("Ohms");
}
byte display(Capacitor_Specs DUT, byte dut_flag)
{
  Serial.println("DEVICE: Capacitor "); // Messes up Big Capacitors with inductors?? // Does not detect very small ones
  Serial.print("C = "); Serial.print(attr::Capacitor.C_Value,2); 
  Serial.print(" "); Serial.print(attr::Capacitor.Power); Serial.println("F");
}
byte display(Inductor_Specs DUT, byte dut_flag)
{
  Serial.println("DEVICE: Inductor ");
  Serial.print("L = "); Serial.print(attr::Inductor.L_Value,0); 
  Serial.print(" "); Serial.println("uH");
  Serial.print("R_parasit = "); Serial.print(attr::Inductor.R_parasit);
  Serial.print(" "); Serial.println("Ohms");
}
byte display(Diode_Specs DUT, byte dut_flag)
{
  Serial.println("DEVICE: Diode ");
  Serial.print("High Current forward voltage drop = "); Serial.print(attr::Diode.VdH_Value,0);
  Serial.print(" mV, Test Intensity: "); Serial.print(attr::Diode.HI_Value); Serial.println(" mA");
  Serial.print("Low Current forward voltage drop = "); Serial.print(attr::Diode.VdL_Value,0);
  Serial.print(" mV, Test Intensity: "); Serial.print(attr::Diode.LI_Value); Serial.println(" uA");
  Serial.print("Anode pin: "); Serial.println(attr::Diode.Anode);
  Serial.print("Cathode pin: "); Serial.println(attr::Diode.Cathode);
}
byte display(Semic_Specs DUT, byte dut_flag)
{
  switch (dut_flag)
  {
    case NPN_FLAG: // 4
      Serial.println("DEVICE: BJT NPN Transistor ");
      Serial.print("Collector probe = "); Serial.print(toHuman(attr::Semiconductor.Collector)); Serial.print("    ");
      Serial.print("Base probe = "); Serial.print(toHuman(attr::Semiconductor.Base)); Serial.print("    ");
      Serial.print("Emitter probe = "); Serial.println(toHuman(attr::Semiconductor.Emitter)); 
      Serial.print("Vbe = "); Serial.print(attr::Semiconductor._V1_); Serial.println(" mV ");
      // Serial.print("Vcb = "); Serial.print(attr::Semiconductor._V2_); Serial.println(" mV ");
      Serial.print("Base Current = "); Serial.print(attr::Semiconductor.I_B); Serial.print(" "); Serial.println("uA");
      Serial.print("Amplification factor = "); Serial.println(attr::Semiconductor.Beta);
      break; 

    case PNP_FLAG: // 5
      Serial.println("DEVICE: BJT PNP Transistor ");
      Serial.print("Collector probe = "); Serial.print(toHuman(attr::Semiconductor.Collector)); Serial.print("    ");
      Serial.print("Base probe = "); Serial.print(toHuman(attr::Semiconductor.Base)); Serial.print("    ");
      Serial.print("Emitter probe = "); Serial.println(toHuman(attr::Semiconductor.Emitter)); 
      Serial.print("Vbe = "); Serial.print(attr::Semiconductor._V1_); Serial.println(" mV ");
      // Serial.print("Vcb = "); Serial.print(attr::Semiconductor._V2_); Serial.println(" mV ");
      Serial.print("Base Current = "); Serial.print(attr::Semiconductor.I_B); Serial.print(" "); Serial.println("uA");
      Serial.print("Amplification factor = "); Serial.println(attr::Semiconductor.Beta);
      break; 

    case NMOS_ENH_FLAG: // 6
      Serial.println("DEVICE: Enhancement NMOS Transistor ");
      Serial.print("Gate probe = "); Serial.print(toHuman(attr::Semiconductor.Base)); Serial.print("    ");
      Serial.print("Drain probe = "); Serial.print(toHuman(attr::Semiconductor.Collector)); Serial.print("    ");
      Serial.print("Source probe = "); Serial.println(toHuman(attr::Semiconductor.Emitter)); 
      Serial.print("Threshold Vgs = "); Serial.print(attr::Semiconductor._V1_,0); Serial.println(" mV ");
      //Serial.print("ON State Rds = "); Serial.print(attr::Semiconductor._V2_); Serial.println(" Ohms ");
      break; 

    case NMOS_DEP_FLAG: // 7
      Serial.println("DEVICE: Depletion NMOS Transistor ");
      Serial.print("Gate probe = "); Serial.print(toHuman(attr::Semiconductor.Base)); Serial.print("    ");
      Serial.print("Drain probe = "); Serial.print(toHuman(attr::Semiconductor.Collector)); Serial.print("    ");
      Serial.print("Source probe = "); Serial.println(toHuman(attr::Semiconductor.Emitter)); 
      Serial.print("Threshold Vgs = "); Serial.print(attr::Semiconductor._V1_,0); Serial.println(" mV ");
      //Serial.print("ON State Rds = "); Serial.print(attr::Semiconductor._V2_); Serial.println(" Ohms ");
      break; 

    case PMOS_ENH_FLAG: // 8
      Serial.println("DEVICE: Enhancement PMOS Transistor ");
      Serial.print("Gate probe = "); Serial.print(toHuman(attr::Semiconductor.Base)); Serial.print("    ");
      Serial.print("Drain probe = "); Serial.print(toHuman(attr::Semiconductor.Collector)); Serial.print("    ");
      Serial.print("Source probe = "); Serial.println(toHuman(attr::Semiconductor.Emitter)); 
      Serial.print("Threshold Vgs = "); Serial.print(attr::Semiconductor._V1_,0); Serial.println(" mV ");
      //Serial.print("ON State Rds = "); Serial.print(attr::Semiconductor._V2_); Serial.println(" Ohms ");
      break; 

    case PMOS_DEP_FLAG: // 9
      Serial.println("DEVICE: Depletion PMOS Transistor ");
      Serial.print("Gate probe = "); Serial.print(toHuman(attr::Semiconductor.Base)); Serial.print("    ");
      Serial.print("Drain probe = "); Serial.print(toHuman(attr::Semiconductor.Collector)); Serial.print("    ");
      Serial.print("Source probe = "); Serial.println(toHuman(attr::Semiconductor.Emitter)); 
      Serial.print("Threshold Vgs = "); Serial.print(attr::Semiconductor._V1_,0); Serial.println(" mV ");
      //Serial.print("ON State Rds = "); Serial.print(attr::Semiconductor._V2_); Serial.println(" Ohms ");
      break;
  }
}

#undef DISP_CPP