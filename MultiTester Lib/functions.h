/* 
 * Global Functions
*/
/*
 * Each Source File has a local ID. If the ID is not set, we import functions of that specific file.
 */

#ifndef TIME_CPP
    extern byte InductorTMeasure(Probe probeA, Probe probeB, _Bool I_Mode, unsigned long *time);
    extern byte CapacitorTMeasure(Probe probeA, Probe probeB, byte R_Mode, unsigned long *time);
#endif

#ifndef IDENTIFY_CPP
    extern byte identify( bool Use_Rh, Probe P1, Probe P2, Probe P3 );
#endif

#ifndef MEASURE_CPP
    extern float Resistance_Measure (int RshuntID, int Vcc_ID ,float Rshunt, const int analogPin, bool inverted, bool ignore_internal);
    extern float Capacitance_Measure(float Rshunt, unsigned long t, bool Is_Big);
    extern float Inductance_Measure (float Rshunt, float R_inductor, unsigned long t);
    extern float Diode_Measure(bool Hi_I, byte Anode, byte Cathode);
    extern void  NPN_Measure(byte bjt_pins[3]);
    extern void  PNP_Measure(byte bjt_pins[3]);
    extern void  MOS_Measure(byte MOSType);
    extern bool  Get_DS(byte Gate2GND);
#endif

#ifndef DISP_CPP // The display function is overloaded. Flags allow for versatility within the function, some may not be used yet.
    extern byte display( Resistor_Specs     DUT, byte dut_flag);
    extern byte display( Capacitor_Specs    DUT, byte dut_flag);
    extern byte display( Inductor_Specs     DUT, byte dut_flag);
    extern byte display( Diode_Specs        DUT, byte dut_flag);
    extern byte display( Semic_Specs        DUT, byte dut_flag);
#endif
