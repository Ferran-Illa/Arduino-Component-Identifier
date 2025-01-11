#define MEASURE_CPP

#include "common.h"
#include "config.h"
#include "functions.h"



/*
 * "n" is the number of measurements, analogPin is the pin where we are measuring on.
 * This function measures n times a resistor in a voltage divider configuration and computes
 * its value. The scheme is 5V -- R -- analogPin -- R_shunt -- 0V.
 * 
 * For low resistance values we need to take into consideration the internal resistances of
 * the board. They are stored in the config.h file.
 */ 
float Resistance_Measure(int RshuntID, int Vcc_ID ,float Rshunt, const int analogPin, bool inverted, bool ignore_internal)
{
    pinMode(analogPin, INPUT);
    pinMode(RshuntID, OUTPUT);
    pinMode(Vcc_ID, OUTPUT);
    delay(1);
    digitalWrite(RshuntID, LOW);
    digitalWrite(Vcc_ID, HIGH);
    
    float Rih = INTERNAL_R_HIGH;
    float Ril = INTERNAL_R_LOW;     // See config.h
    
    // We will work over the following variable, overwriting it with our calculations
    float Value = 0;

    for(int j=0; j<100; j++)
    {
        Value += analogRead(analogPin);
        delay(1);
        if(ignore_internal){delay(10);}
    }
    Value /= 1023; // 10 bit ADC
    Value /= 100;    // Averaging

    if(inverted){Value = 5-Value;} // In case the resistor configuration is inverted ( 5V - Rshunt - R - 0 )

    if( ignore_internal )
    { 
        Value = (Rshunt)/Value - Rshunt;
    }
    else
    {
        Value = (Rshunt+Ril)/Value - Rshunt - Ril - Rih;
    }

    // Tidying up the used pins
    digitalWrite(RshuntID, LOW);
    digitalWrite(Vcc_ID, LOW);
    delay(10);
    pinMode(analogPin, INPUT);
    pinMode(RshuntID, INPUT);
    pinMode(Vcc_ID, INPUT);

    if (Value < 0){ Value = 0;} // Sanity check
    
    return Value;
}

/* 
 * Computing Capacitance, we have precalculated the logarithm, as everything should be known.
 * This is the result from the equation of a capacitor discharging through a resistance:
 * 
 * V(t) = V0 exp(-t/RC)
 * 
 * And we have measured the time it takes for the system to discharge to 1.1V (the bandgap reference).
 * 
 */
float Capacitance_Measure(float Rshunt, unsigned long t, bool Is_Big)
{
    static const float factor = 0.6604462612; // 1/(log(Vref/V0)), with V_ref = 1.1V and V0 = 5V
    float C = 0;
    
    // C = (t/R) * (1/[log(Vref/V0)]) From the equation of a capacitor discharge.
    if(Is_Big)
    {
        C = 1000 * factor * t / Rshunt; // Rshunt in Ohms
    }
    else
    {
        C = factor * t / Rshunt; 
    }

    // Prefix Assignment and Rescaling, given that t is in ns and R_shunt in kOhms
    if (C > 1000000)
    { 
        attr::Capacitor.Power = 'u';
        C /= 1000000;
    }
    else if ( C > 1000)
    { 
        attr::Capacitor.Power = 'n';
        C /= 1000;  
    }
    else
    { 
        attr::Capacitor.Power = 'p'; 
    }

    if(C>400){ C = round(C/10)*10;}
    else if(C>50){C = round(C); }

    return C; 
}


/*
  * Inductance measurements will be made using the equation (ideal):
  *
  *           I(t) = I0*(1 - exp(-t*R/L))  =>   L = - t*R / ln( 1 - I(t)/I0 )  
  *
  * Where R is the TOTAL Resistance of the circuit, I0 the maximum current flowing through
  * the circuit, and I(t) the current flowing at time t. Taking into account the pin's
  * internal resistances, we will measure the time instant t_meas when:
  * 
  *  I(t_meas) = V_ref/(R_shunt + Ri_L);   I0 = 5V / R;    R = RL + Ri_H + Ri_L + R_shunt  
  * 
  * Hence:
  *
  *                  I(t_meas)/I0 = 1.1 * R / (5 * R_shunt + Ri_L)
  *
  * Only a simple Resistance measurement of RL is left.
  */

float Inductance_Measure(float Rshunt, float R_inductor, unsigned long t)
{
    // "t" is the inductor discharge time 
    const float Rih = INTERNAL_R_HIGH;
    const float Ril = INTERNAL_R_LOW; // See config.h

    float L = 0;
    float R = Rshunt + Rih + Ril + R_inductor;
    float I_ratio = 0.22 * R / (Ril + Rshunt); // I(t)/I0 = [Vref/(Ril + Rshunt)] / [5/R], Vref = 1.1V, Vcc = 5V

    float num = t*R/1000;

    L = -num / (log(1- I_ratio)); // The /1000 scales to uH, as time (t) is in ns

    return L;
}

float Diode_Measure(bool Low_I, byte Anode, byte Cathode)
{
    byte R_pullup = 0;
    byte R_val = 0;

    if(Anode == P1.ID)
    {
        R_pullup = P1.Rl;
        R_val = P1.Rl_val + INTERNAL_R_LOW + INTERNAL_R_HIGH;

        if(Low_I)
        {
            R_pullup = P1.Rh;
            R_val = P1.Rh_val;
        }
    }
    else if(Anode == P2.ID)
    {
        R_pullup = P2.Rl;
        R_val = P2.Rl_val + INTERNAL_R_LOW + INTERNAL_R_HIGH;

        if(Low_I)
        {
            R_pullup = P2.Rh;
            R_val = P2.Rh_val;
        }
    }
    else if(Anode == P3.ID)
    {
        R_pullup = P3.Rl;
        R_val = P3.Rl_val + INTERNAL_R_LOW + INTERNAL_R_HIGH;

        if(Low_I)
        {
            R_pullup = P3.Rh;
            R_val = P3.Rh_val;
        }
    }
    else{ return -1; } // Error

    // Setting up the probes for measurement
    pinMode(Anode, INPUT);
    pinMode(Cathode, OUTPUT);
    pinMode(R_pullup, OUTPUT);
    
    digitalWrite(Cathode, LOW);
    digitalWrite(R_pullup, HIGH);
    delay(10);

    float Voltage = 0;
    unsigned long ADC_Reading = 0;

    for(int i = 0; i < 100; i++)
    {
        ADC_Reading += analogRead(Anode);
    }

    digitalWrite(R_pullup, LOW);
    pinMode(Cathode, INPUT);
    pinMode(R_pullup, INPUT);

    Voltage = 5*ADC_Reading/102.30; // 102.30 = 1.023*100 (ADC conversion to mV and average)
    float Current = 0;
    // Storing Intensity values
    if(Low_I)
    {
        Current = (5000-Voltage)/R_val;
        attr::Diode.HI_Value = Current;
    }
    else
    {
        Current = (5000-Voltage)/(R_val); 
        Voltage -= Current*INTERNAL_R_LOW; // Accountign for Internal Resistances
        attr::Diode.LI_Value = Current;
    }

    return round(Voltage/10)*10;
}

// Bridge between BJT measures and the Semic class
void Assign_BJT(float VDrop[2], unsigned int Beta[2], float Ibase[2], byte Test1, byte Test2)
{
    if(Beta[1] >= Beta[2])
    {
        attr::Semiconductor.Beta = Beta[1];
        attr::Semiconductor._V1_ = VDrop[1];    // Base - Emitter Voltage Drop
        attr::Semiconductor._V2_ = VDrop[2];    // Collector - Base Voltage Drop
        attr::Semiconductor.I_B = Ibase[1];     // Base Current

        attr::Semiconductor.Collector = Test1;
        attr::Semiconductor.Emitter   = Test2;

        if(Beta[1] == Beta[2]){ Serial.println("Symmetrical BJT"); }
        else if(Beta[1]/Beta[2] < 2){ Serial.println("Possibly Symmetrical BJT"); }
    }
    else if(Beta[1] < Beta[2])
    {
        attr::Semiconductor.Beta = Beta[2];
        attr::Semiconductor._V1_ = VDrop[2];    // Base - Emitter Voltage Drop
        attr::Semiconductor._V2_ = VDrop[1];    // Collector - Base Voltage Drop
        attr::Semiconductor.I_B = Ibase[2];     // Base Current

        attr::Semiconductor.Collector = Test2;
        attr::Semiconductor.Emitter   = Test1;

        if(Beta[2]/Beta[1] < 2){ Serial.println("Possibly Symmetrical BJT");}
    }
}

// Measures the Amplification Factor and the Characteristic Voltage Drops.
unsigned int PNP_Beta_Measure(byte Base, byte Collector, byte Emitter, byte Rb, float Rb_val, byte Re, float Re_val)
{
    // Setting up the measurement scheme
    
    pinMode(Base, INPUT);
    pinMode(Collector, OUTPUT);
    pinMode(Emitter, INPUT);
    pinMode(Rb, OUTPUT);
    pinMode(Re, OUTPUT);

    
    digitalWrite(Collector, LOW);
    digitalWrite(Rb, LOW);
    digitalWrite(Re, HIGH);
    delay(1); // Voltage Stabilization

    float R_factor = (Rb_val*1000 + INTERNAL_R_LOW) / (INTERNAL_R_HIGH + Re_val); // Rb_val will be in kOhms
    unsigned int ADC_B = 0;
    unsigned int ADC_E = 0;

    for(byte i = 0; i<50; i++)
    {
        ADC_B += analogRead(Base);
    }
    delay(1);
    analogRead(Emitter); // Discarding the first measure, after switching ports it may be unreliable

    for(byte i = 0; i<50; i++)
    {
        ADC_E += analogRead(Emitter);
    }

    // Resetting used pins
    digitalWrite(Re, LOW);

    pinMode(Re, INPUT);
    pinMode(Rb, INPUT);
    pinMode(Collector, INPUT);

    // Averaging and conversion to V is implicitly done through the fraction. Notice how 1023*50 = 51150
    float Beta = (51150 - ADC_E) * R_factor; 
    Beta /= ADC_B;
    Beta -= 1;

    attr::Semiconductor._V1_ = round( (ADC_E - ADC_B)*5000.0/51150.0 ); // A convenient place to temporarily store Vbe
    // Current flow:
    float I_b = 5.0*ADC_B/1023; // To V
    I_b /= 50; // Average
    I_b = I_b/(Rb_val + INTERNAL_R_LOW/1000) * 1e3; // Conversion of V to uA for the Base Resistor.
    attr::Semiconductor.I_B = I_b;

    if(Beta < 0){ Beta = 0; } // Sanity Check
    return static_cast<unsigned int>(Beta);
}

// Measures the Amplification Factor and the Characteristic Voltage Drops.
unsigned int NPN_Beta_Measure(byte Base, byte Collector, byte Emitter, byte Rb, float Rb_val, byte Re, float Re_val)

{
    // Setting up the measurement scheme
    pinMode(Base, INPUT);
    pinMode(Collector, OUTPUT);
    pinMode(Emitter, INPUT);
    pinMode(Rb, OUTPUT);
    pinMode(Re, OUTPUT);

    digitalWrite(Re, LOW);
    digitalWrite(Collector, HIGH);
    digitalWrite(Rb, HIGH);
    delay(1); // Voltage Stabilization

    float R_factor = (Rb_val*1000 + INTERNAL_R_HIGH) / (INTERNAL_R_LOW + Re_val); // Rb_val will be in kOhms
    unsigned int ADC_B = 0;
    unsigned int ADC_E = 0;

    for(int i = 0; i<50; i++)
    {
        ADC_B += analogRead(Base);
    }
    delay(1);
    analogRead(Emitter); // Discarding the first measure, after switching ports it may be unreliable

    for(int i = 0; i<50; i++)
    {
        ADC_E += analogRead(Emitter);
    }
    
    // Resetting used pins
    digitalWrite(Collector, LOW);
    digitalWrite(Rb, LOW);

    pinMode(Re, INPUT);
    pinMode(Rb, INPUT);
    pinMode(Collector, INPUT);

    // Averaging and conversion to V is implicitly done through the fraction. Notice how 1023*50 = 51150
    float Beta = ADC_E*R_factor;
    Beta /= 51150-ADC_B;
    Beta -= 1;

    attr::Semiconductor._V1_ = round( (ADC_B - ADC_E)*5000.0/51150.0 ); // A convenient place to temporarily store Vbe

    // Current Flow:

    float I_b = 5.0*ADC_B/1023.0; // To V
    I_b /= 50.0; // Average

    I_b = (5 - I_b)/(Rb_val + INTERNAL_R_HIGH/1000) * 1e3; // Conversion of V to uA for the Base Resistor.

    attr::Semiconductor.I_B = I_b;

    if(Beta < 0){ Beta = 0; } // Sanity Check
    return static_cast<unsigned int>(Beta);
}

// NPN can be seen as two diodes with common anode (the base)
void NPN_Measure(byte bjt_pins[3])
{
    byte NPNBase = 0;
    float VDrop[2] = {0.0,0.0};
    float Ib[2] = {0.0,0.0};
    unsigned int Beta[2] = {0,0};
    const byte ProbeIDs[3] = {P1.ID, P2.ID, P3.ID};

    // bjt_pins[i] must be 0 or Px.ID, x=1,2,3
    for(byte i = 0; i<3; i++) 
    {
        if(bjt_pins[i]) // Any non-zero value will trigger the if statement
        {
            NPNBase = bjt_pins[i];
        }
    }

    attr::Semiconductor.Base = NPNBase;

    // Measuring Voltage drops and beta, for all 3 possibilities
    if(NPNBase == P1.ID)
    {
        Beta[1] = NPN_Beta_Measure(NPNBase, P2.ID, P3.ID, P1.Rm, P1.Rm_val, P3.Rl, P3.Rl_val);
        VDrop[1] = attr::Semiconductor._V1_;    // Recovering the value of Vbe
        Ib[1] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Beta[2] = NPN_Beta_Measure(NPNBase, P3.ID, P2.ID, P1.Rm, P1.Rm_val, P2.Rl, P2.Rl_val);
        VDrop[2] = attr::Semiconductor._V1_;    // Recovering the value of Vbe
        Ib[2] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Assign_BJT(VDrop, Beta, Ib, P2.ID, P3.ID);
    }
    else if(NPNBase == P2.ID)
    {
        Beta[1] = NPN_Beta_Measure(NPNBase, P1.ID, P3.ID, P2.Rm, P2.Rm_val, P3.Rl, P3.Rl_val);
        VDrop[1] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[1] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Beta[2] = NPN_Beta_Measure(NPNBase, P3.ID, P1.ID, P2.Rm, P2.Rm_val, P1.Rl, P1.Rl_val);
        VDrop[2] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[2] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Assign_BJT(VDrop, Beta, Ib, P1.ID, P3.ID);
    }
    else if(NPNBase == P3.ID)
    {
        Beta[1] = NPN_Beta_Measure(NPNBase, P1.ID, P2.ID, P3.Rm, P3.Rm_val, P2.Rl, P2.Rl_val);
        VDrop[1] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[1] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Beta[2] = NPN_Beta_Measure(NPNBase, P2.ID, P1.ID, P3.Rm, P3.Rm_val, P1.Rl, P1.Rl_val);
        VDrop[2] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[2] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Assign_BJT(VDrop, Beta, Ib, P1.ID, P2.ID);
    }

    return;

}

// PNP can be seen as two diodes with common cathode (the base)
void PNP_Measure(byte bjt_pins[3]) 
{
    byte PNPBase = 0;
    float VDrop[2] = {0.0,0.0};
    float Ib[2] = {0.0,0.0};
    unsigned int Beta[2] = {0,0};

    const byte ProbeIDs[3] = {P1.ID, P2.ID, P3.ID};
    
    // Identifying the PNPBase, we know the two anodes. This simply checks which probe is not an anode.
    for(byte i = 0; i<3; i++) 
    {
        if(bjt_pins[0] != ProbeIDs[i] && bjt_pins[1] != ProbeIDs[i] && bjt_pins[2] != ProbeIDs[i])
        {
            PNPBase = ProbeIDs[i];
        }
    }

    attr::Semiconductor.Base = PNPBase;

    // Measuring Voltage drops and beta, for all 3 possibilities
    if(PNPBase == P1.ID)
    {
        Beta[1] = PNP_Beta_Measure(PNPBase, P2.ID, P3.ID, P1.Rm, P1.Rm_val, P3.Rl, P3.Rl_val);
        VDrop[1] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[1] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Beta[2] = PNP_Beta_Measure(PNPBase, P3.ID, P2.ID, P1.Rm, P1.Rm_val, P2.Rl, P2.Rl_val);
        VDrop[2] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[2] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Assign_BJT(VDrop, Beta, Ib, P2.ID, P3.ID);
    }
    else if(PNPBase == P2.ID)
    {
        Beta[1] = PNP_Beta_Measure(PNPBase, P1.ID, P3.ID, P2.Rm, P2.Rm_val, P3.Rl, P3.Rl_val);
        VDrop[1] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[1] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Beta[2] = PNP_Beta_Measure(PNPBase, P3.ID, P1.ID, P2.Rm, P2.Rm_val, P1.Rl, P1.Rl_val);
        VDrop[2] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[2] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Assign_BJT(VDrop, Beta, Ib, P1.ID, P3.ID);
    }
    else if(PNPBase == P3.ID)
    {
        Beta[1] = PNP_Beta_Measure(PNPBase, P1.ID, P2.ID, P3.Rm, P3.Rm_val, P2.Rl, P2.Rl_val);
        VDrop[1] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[1] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Beta[2] = PNP_Beta_Measure(PNPBase, P2.ID, P1.ID, P3.Rm, P3.Rm_val, P1.Rl, P1.Rl_val);
        VDrop[2] = attr::Semiconductor._V1_; // Recovering the value of Vbe
        Ib[2] = attr::Semiconductor.I_B;        // Recovering the value of Ib

        Assign_BJT(VDrop, Beta, Ib, P1.ID, P2.ID);
    }

    return;
}

void MOS_Measure(byte MOSType)
{
    // Initialization and variable selection
    byte Drain  = attr::Semiconductor.Collector;
    byte Gate   = attr::Semiconductor.Base;
    byte Source = attr::Semiconductor.Emitter;

    byte Drain_Rl; byte Source_Rl; byte Gate_Rh; byte Gate_Rl;
    float Rl_val = 0;

    if      (Drain == P1.ID){ Drain_Rl = P1.Rl; Rl_val = P1.Rl_val;}
    else if (Drain == P2.ID){ Drain_Rl = P2.Rl; Rl_val = P2.Rl_val;}
    else if (Drain == P3.ID){ Drain_Rl = P3.Rl; Rl_val = P3.Rl_val;}
    else{return;}

    if      (Gate == P1.ID){ Gate_Rl = P1.Rl; Gate_Rh = P1.Rh; }
    else if (Gate == P2.ID){ Gate_Rl = P2.Rl; Gate_Rh = P2.Rh; }
    else if (Gate == P3.ID){ Gate_Rl = P3.Rl; Gate_Rh = P3.Rh; }
    else{return;}

    if      (Source == P1.ID){ Source_Rl = P1.Rl; }
    else if (Source == P2.ID){ Source_Rl = P2.Rl; }
    else if (Source == P3.ID){ Source_Rl = P3.Rl; }
    else{return;}

    pinMode(Gate, INPUT);
    pinMode(Drain, INPUT);
    pinMode(Drain_Rl, INPUT);
    pinMode(Source_Rl, INPUT);
    pinMode(Source, INPUT);
    bool Gate_Pullup = 1;

    bool Is_PMOS = MOSType == PMOS_ENH_FLAG;
    float Vgs = 0;

    // Measurement of Threshold Vgs
    if(Is_PMOS) // PMOS
    {
        pinMode(Drain_Rl, OUTPUT);
        pinMode(Source, OUTPUT);

        digitalWrite(Source, HIGH);
        digitalWrite(Drain_Rl, LOW);
    }
    else // NMOS
    {
        pinMode(Drain_Rl, OUTPUT);
        pinMode(Source, OUTPUT);
        
        digitalWrite(Source, LOW);
        digitalWrite(Drain_Rl, HIGH);
        Gate_Pullup = 0;
    }

    // We repeat the measurement 10 times
    for(byte i = 0; i<10; i++)
    {
        // We charge/discharge the gate
        pinMode(Gate_Rl, OUTPUT);
        pinMode(Gate_Rh, OUTPUT);

        digitalWrite(Gate_Rl, Gate_Pullup);
        digitalWrite(Gate_Rh, Gate_Pullup);
        delay(10); // Wait 10ms to charge/discharge gate

        digitalWrite(Gate_Rl, LOW);
        pinMode(Gate_Rl, INPUT);
        delay(10);  // Additional time to compensate the setting change on Gate_Rl

        digitalWrite(Gate_Rh, !Gate_Pullup);

        if (Is_PMOS)          // p-channel
        {
        // FET conducts when the voltage at drain reaches high level
        while (!digitalRead(Drain)){};
        }
        else                  // n-channel
        {
        // FET conducts when the voltage at drain reaches low level
        while (digitalRead(Drain)){};             
        }

        int ADC_Reading = analogRead(Gate);

        if(Is_PMOS)
        {
            Vgs -= (1023 - ADC_Reading);
        }
        else
        {
            Vgs += ADC_Reading;
        }
    }
    // Reseting used pins

    digitalWrite(Gate_Rh, LOW);
    digitalWrite(Source, LOW);
    digitalWrite(Drain_Rl, LOW);

    //pinMode(Gate_Rl, INPUT); // Already set
    pinMode(Gate_Rh, INPUT);
    //pinMode(Gate, INPUT); // Already set
    pinMode(Drain, INPUT);
    pinMode(Drain_Rl, INPUT);
    pinMode(Source_Rl, INPUT);
    pinMode(Source, INPUT);

    Vgs *= 5000/1023; // To mV
    Vgs /= 10; // Average
    
    attr::Semiconductor._V1_ = round(Vgs/10)*10; // Rounding to 10 mV
    return;
}

bool Get_DS(byte Gate2GND)
{
    pinMode(Gate2GND, OUTPUT);
    digitalWrite(Gate2GND, LOW);

    // The suspects
    byte ID_A = 0; byte R_A = 0; 
    byte ID_B = 0; byte R_B = 0;

    if(Gate2GND = P1.Rl)
    { 
        ID_A = P2.ID; R_A = P2.Rl;
        ID_B = P3.ID; R_B = P3.Rl;
    }
    else if(Gate2GND = P2.Rl)
    {
        ID_A = P1.ID; R_A = P1.Rl;
        ID_B = P3.ID; R_B = P3.Rl;
    }
    else if(Gate2GND = P3.Rl)
    {
        ID_A = P2.ID; R_A = P2.Rl;
        ID_B = P1.ID; R_B = P1.Rl;
    }
    else{return 0;} // Error

    // The Proof Variables
    unsigned int ADC_A = 0;
    unsigned int ADC_B = 0;

    // Testing the Crime Scene
    pinMode(ID_A, INPUT);
    pinMode(ID_B, OUTPUT);
    pinMode(R_A, OUTPUT);
    pinMode(R_B, INPUT);

    digitalWrite(ID_B, HIGH);
    digitalWrite(R_A, LOW);

    for(byte i=0; i<50; i++){ ADC_B += analogRead(ID_B);}

    digitalWrite(ID_B, LOW);

    pinMode(ID_A, OUTPUT);
    pinMode(ID_B, INPUT);
    pinMode(R_A, INPUT);
    pinMode(R_B, OUTPUT);

    digitalWrite(ID_A, HIGH);
    digitalWrite(R_B, LOW);

    for(byte i=0; i<50; i++){ ADC_A += analogRead(ID_A);}

    digitalWrite(ID_A, LOW);
    pinMode(ID_A, INPUT);
    pinMode(ID_B, INPUT);
    pinMode(R_A, INPUT);
    pinMode(R_B, INPUT);
    pinMode(Gate2GND, INPUT);

    /* We are basing out theory on the diode forward voltage drop (Vd) being small enough, such that
     * the two voltages measured are: V1 = 5 - Vd; V2 ~ Vgs_threshold, and we verify V1 > V2.
     * No other method to differentiate the Drain from the Source has been found.
     */

    if(ADC_A > ADC_B)
    {
        attr::Semiconductor.Collector = ID_B;
        attr::Semiconductor.Emitter = ID_A;
    }
    else if (ADC_B > ADC_A)
    {
        attr::Semiconductor.Collector = ID_A;
        attr::Semiconductor.Emitter = ID_B;
    }
    else{ return 0; } // Failure, proof is inconclusive
    return 1; // Success, bad guys apprehended
}
#undef MEASURE_CPP