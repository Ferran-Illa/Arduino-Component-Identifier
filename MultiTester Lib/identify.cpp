#define IDENTIFY_CPP

#include "common.h"
#include "config.h"
#include "functions.h"

/*
 * We will carry out the combinatory to identify the components and what is connected to each probe.
 *
 * If state 1 is HIGH and state 0 is LOW, selectively pulling up and down the resistors connected to each probe
 * we may well identify the number of terminals of the component.
 * 
 * We will ask to always connect 2 terminal components to probes 1 and 2.Therefore, if any change is detected in
 * the third probe, we will know we have a 3 terminal component. As well, if the third probe impacts the readings of
 * the 1st and 2nd probes, only a 3 terminal component is possible.
 *  
 * Out1 Out2 Out3 Readings
 * 0    0    0    OK Check    
 * 0    0    1   (P1 P2 P3)
 * 0    1    0   (P1 P2 P3)     
 * 0    1    1   (P1 P2 P3)
 * 1    0    0   (P1 P2 P3)
 * 1    0    1   (P1 P2 P3)
 * 1    1    0   (P1 P2 P3)
 * 1    1    1    OK Check
 * 
 */


bool arr_comp(const bool A1[8], const bool A2[8]) // Compares two boolean arrays of size 8 and returns 1 if they are identical, 0 otherwise.
{
    for(byte i = 1; i < 7; i ++)
    {
        if(A1[i] != A2[i]){return 0;}
    }
    return 1;
}

bool wait_discharge(const byte ID1, const byte ID2, const byte ID3)
{
    byte t = 0;
    do{
        delay(100); // Wait for discharge
        t = t+1;
        if (t>100){return 1;} // Raise error. We have taken too long.
    }while( (analogRead(ID1) > 1) | (analogRead(ID2) > 1) | (analogRead(ID3) > 1) ); // If we read some voltage we continue to discharge
    return 0;
}

byte remaining_probe(byte A, byte B)
{
    byte AllProbes[3] = {P1.ID, P2.ID, P3.ID};
    for(byte i=0; i<3; i++)
    {
        if((AllProbes[i] != A) && (AllProbes[i] != B))
        {
            return AllProbes[i];
        }
    }
    return 0;
}

byte isRL(bool Use_Rh, unsigned long time)
{
    float R_val = 0;

    if(Use_Rh)
    {
        R_val = Resistance_Measure(P1.Rm, P2.ID, P1.Rm_val, P1.ID, 0, 1); // Medium Resistances by default (works well under 150kOhms)

        if(R_val > 150){ R_val = Resistance_Measure(P1.Rh, P2.ID, P1.Rh_val, P1.ID, 0, 1);} // Using high value R to make the measure

        attr::Resistor.R_Value = R_val;
        attr::Resistor.Power = 'k';

        return RESISTOR_FLAG; // Cannot possibly be an inductor if we needed high resistance. (We couldn't measure it either)
    }
    
    if (time != 0){ time = 0; } // Sanity check;

    float R_shunt = P1.Rl_val;
    int Tflag = InductorTMeasure(P1, P2, 0, &time); // First testing big inductances

    if( Tflag == 1 | Tflag == 2) // Too low time reading, testing low inductance
    {
        if (time != 0){ time = 0; } // Reset time

        R_shunt = 0; // With Low Inductances we rely purely on internal resistances.
        Tflag = InductorTMeasure(P1, P2, 1, &time);

        if( Tflag == 1 | Tflag == 2 | Tflag == 10) // Still no significant time measures, must be a resistor/short circuit
        {
            R_val = Resistance_Measure(P1.Rl, P2.ID, P1.Rl_val, P1.ID, 0, 0); // Expecting low R by default
            
            if(R_val > 4500) // Medium-value (22k) resistances are better suited to measure the kOhm range
            {
                R_val = Resistance_Measure(P1.Rm, P2.ID, P1.Rm_val, P1.ID, 0, 1);
                attr::Resistor.Power = 'k';
            }
            else
            {
                attr::Resistor.Power = ' ';
            }

            attr::Resistor.R_Value = R_val;

            return RESISTOR_FLAG;
        }
    }

    if( Tflag == 10 )
    {
        return 0; // This should not happen, we detected a shortcircuit and an open circuit simulataneously.
    }

    attr::Inductor.R_parasit = Resistance_Measure(P1.Rl, P2.ID, P1.Rl_val, P1.ID, 0, 0);
    attr::Inductor.L_Value = Inductance_Measure(R_shunt, attr::Inductor.R_parasit, time);
    
    return INDUCTOR_FLAG;
}


byte identify( bool Use_Rh, Probe P1, Probe P2, Probe P3 )
{
       
    byte R1 = P1.Rl;
    byte R2 = P2.Rl;
    byte R3 = P3.Rl;

    if (Use_Rh) // We may need this for big resistances or capacitors
    {
        R1 = P1.Rh;
        R2 = P2.Rh;
        R3 = P3.Rh;
    }

    pinMode(P1.ID, INPUT); // Starting up INPUT pins
    pinMode(P2.ID, INPUT);
    pinMode(P3.ID, INPUT);

    pinMode(P1.Rl, OUTPUT); // Starting up OUTPUT pins
    pinMode(P2.Rl, OUTPUT);
    pinMode(P3.Rl, OUTPUT);

    digitalWrite(P1.Rl, LOW); // Setting everything to GND, in case of charged components.
    digitalWrite(P2.Rl, LOW);
    digitalWrite(P3.Rl, LOW);

    if(wait_discharge(P1.ID, P2.ID, P3.ID)){ return 100; } // Timeout error

    pinMode(P1.Rl, INPUT); // We will measure capacitance now, we need Hi-Z
    pinMode(P2.Rl, INPUT);
    pinMode(P3.Rl, INPUT);

    unsigned long time = 0;
    byte Cap_timetest = 1;
    bool Is_Big = 0;
    
    if(Use_Rh) // This earns us some time
    {
        Cap_timetest = CapacitorTMeasure(P1, P2, 2, &time);
    }
    else
    {
        Cap_timetest = CapacitorTMeasure(P1, P2, 1, &time);
        if(Cap_timetest == 10) // Timeout Flag
        {
            Cap_timetest = CapacitorTMeasure(P1, P2, 0, &time);
            Is_Big = 1;
        }
    }

    float R_tot = 0.0;
    if(!Cap_timetest) // Capacitor detected
    {
        if(Use_Rh)
        { 
            R_tot = P2.Rh_val + (P1.Rl_val) / 1000.0;
            attr::Capacitor.C_Value = Capacitance_Measure(R_tot, time, Is_Big);
        }
        else if(Is_Big)
        {
            R_tot = P2.Rl_val + (INTERNAL_R_LOW + INTERNAL_R_HIGH) / 1000.0;
            attr::Capacitor.C_Value = Capacitance_Measure(R_tot, time, Is_Big);
        }
        else
        {
            R_tot = P2.Rm_val + (P1.Rl_val + INTERNAL_R_LOW + INTERNAL_R_HIGH) / 1000.0;
            attr::Capacitor.C_Value = Capacitance_Measure(R_tot, time, Is_Big);
        }
        return CAPACITOR_FLAG;
    }

    delay(10);
    // Serial.print(analogRead(P1.ID));
    //         Serial.print(" | ");
    // Serial.print(analogRead(P2.ID));
    //         Serial.print(" | ");
    // Serial.println(analogRead(P3.ID));
    // Serial.println("Analog");

    pinMode(R1, OUTPUT); // Starting up OUTPUT pins
    pinMode(R2, OUTPUT);
    pinMode(R3, OUTPUT);

    digitalWrite(R1, LOW); // Setting everything to GND, in case of charged components again
    digitalWrite(R2, LOW);
    digitalWrite(R3, LOW);

    if(wait_discharge(P1.ID, P2.ID, P3.ID)){ return 100; } // Timeout error

    byte combinations = 1;
    unsigned long response = 0;

    bool changed[8];    // Bits that have changed
    byte count = 0;     // We count the number of changes that occurred.
    byte step = 1;      // Steps of the measure process
    bool C1[8] = {0,0,0,0,0,0,0,0}; // Initializing our binary measure buffers
    bool C2[8] = {0,0,0,0,0,0,0,0};
    bool C3[8] = {0,0,0,0,0,0,0,0};
    
    while(step <= 4) // 4 Steps are needed for the measuring process to complete, one for each buffer and a final one for data post-processing
    {
        while (combinations < 0b111){ // moves up to go through all the 6 useful combinations.
            digitalWrite(R1, combinations & 0b1); // Writes HIGH if the flag is set, LOW otherwise.
            digitalWrite(R2, combinations & 0b10);
            digitalWrite(R3, combinations & 0b100);
            delay(10);
            // Translating readings into binary values (1 = HIGH, 0 = LOW)
            if(step == 1) 
            {
                if(Use_Rh){analogRead(P1.ID);} // First measures have been observed to be unreliable with high impedances
                
                C1[combinations] = (analogRead(P1.ID) > 20);
            }
            
            else if(step == 2)
            {
                if(Use_Rh){analogRead(P2.ID);}
                
                C2[combinations] = (analogRead(P2.ID) > 20);
            }
            
            else if(step == 3)
            {
                if(Use_Rh){analogRead(P3.ID);}
                
                C3[combinations] = (analogRead(P3.ID) > 20); 
            }

            else if(step == 4)
            {
                bool CC = ( (C1[combinations] | C2[combinations] << 1 | C3[combinations] << 2) != combinations );
                changed[combinations] = CC; // If there is a change with respect to the input this will flag it.
                count += CC;

                /*
                * Now we have the answer of our device to all possible input combinations. The input is stored in "combinations",
                * the output in "response" in the form of binary values representing state HIGH with 1 and state LOW with 0. 
                * We also compare the input with the output and store it in "comparison". If the input is equal to the output, we get 0, otherwise we get 1.
                * We will go through different cases to identify the components:
                */
            }
            else{ return 0; } // Raise Error
            combinations ++;
            /*
            * NOTE:
            * The repeated measures of analogRead to discard the first measurements are inefficient,
            * but it  is the only method that has been observed to work. Our suspicion is that we are limited by the
            * charging of the Sample and Hold capacitor of the ADC, which is not though to work under High Impedance frameworks.
            * The only possible way to avoid these effects is to allow it to charge properly by dismissing the first readings.
            * Possibly, only the first two readings at the start of each step need to be discarded, but given the time taken is
            * negligible for the user, we have chosen what we think is a better option in terms of reliability, whicle we sacrifice
            * some time and efficiency.
            * 
            * Any new solutions that would make the measurement scheme cleaner will be welcome.
            */
        }
        
        combinations = 1;
        step ++;
    }
    // Shutting down the pins.   
    digitalWrite(R1, LOW); 
    digitalWrite(R2, LOW);
    digitalWrite(R3, LOW);
    
    if(wait_discharge(P1.ID, P2.ID, P3.ID)){ return 101; } // Timeout error 2

    pinMode(R1, INPUT); // Shutting down resistor pins
    pinMode(R2, INPUT);
    pinMode(R3, INPUT);

    // for(int i  = 0; i < 8; i++) // Debug Purposes
    // {
    //     Serial.print(C3[i]); // Works as long as Serial.begin is called in setup()
    //     Serial.print(" | ");
    //     Serial.print(C2[i]);
    //     Serial.print(" | ");
    //     Serial.print(C1[i]);
    //     Serial.println("");
    // }

    /***********************************
    * Analyzing the changes:
    ***********************************/
    if(count == 0)  // Either a Capacitor or open circuit.
    {
        if(Use_Rh){return OPEN_CIRCUIT_FLAG;}
        else
        {
            return identify(1, P1, P2, P3); // This will call the function again and tell it to use a high resistance value
        } 
    }
    // TWO TERMINAL Devices, assuming always connected to probes 1 and 2. These Values can be found in the config.h file.
    /*
    const bool diode_C1anode[8] = {0,1,0,1,0,1,0,0};
    const bool diode_C2anode[8] = {0,0,1,1,0,0,1,0};

    const bool C3unused[8] = {0,0,0,0,1,1,1,0}; // Specific for C3
    const bool C1C2shortcircuit[8] = {0,1,1,1,0,1,1,0};
    */

    if(arr_comp(C3, C3unused)){

        if( arr_comp(C1, C1C2shortcircuit) && arr_comp(C2, C1C2shortcircuit)) // What the output looks for a short-circuit (low enough R / Inductor)
        {
            return isRL(Use_Rh, time); // Measuring Resistances and Inductances
        }

        else if( arr_comp(C1, diode_C1anode) && arr_comp(C2, C1C2shortcircuit) )
        {
            attr::Diode.Anode   = P1.ID;
            attr::Diode.Cathode = P2.ID;
            attr::Diode.VdH_Value = Diode_Measure(0, P1.ID, P2.ID); // High  Intensity measure
            attr::Diode.VdL_Value = Diode_Measure(1, P1.ID, P2.ID); // Low Intensity measure
            return DIODE_AC_FLAG;
        }

        else if( arr_comp(C2, diode_C2anode) && arr_comp(C1, C1C2shortcircuit) )
        {
            attr::Diode.Anode   = P2.ID;
            attr::Diode.Cathode = P1.ID;
            attr::Diode.VdH_Value = Diode_Measure(0, P2.ID, P1.ID); // High  Intensity measure
            attr::Diode.VdL_Value = Diode_Measure(1, P2.ID, P1.ID); // Low Intensity measure
            return DIODE_CA_FLAG;   
        }
    }

    // THREE TERMINAL devices (semiconductors)
    
    // General Variables
    bool is_111 = 0;
    byte count_111 = 0;

    // For BJTs
    byte bjt_count_111 = 0;
    byte bjt_pins[3] = {0,0,0};

    // For MOS
    byte Drain_Rl = 0;
    byte Source_Rl = 0;
    byte Gate_Rh = 0;
    byte Gate_Rl = 0;

    if(count == 4) // NMOS - depletion (Could be PMOS - depletion, but these devices are not manufactured)
    {
        bool Gate_Slow =0;
        for(byte i = 1; i<7; i++) // Useful combinations
        {
            is_111 = (C1[i] & C2[i] & C3[i]);
            if(changed[i] && !is_111) 
            {
                if      (C2[i] & C3[i]){ attr::Semiconductor.Base = P1.ID; Gate_Slow = P1.Rl;} // The probe with a non-powered reading is the Gate
                else if (C1[i] & C3[i]){ attr::Semiconductor.Base = P2.ID; Gate_Slow = P2.Rl;}
                else if (C1[i] & C2[i]){ attr::Semiconductor.Base = P3.ID; Gate_Slow = P3.Rl;}

                // TODO: Diode measure
            }
        }
        if(Get_DS(Gate_Slow)) // If we can identify Source and Drain we may carry out other measures
        {
            MOS_Measure(NMOS_DEP_FLAG);
        }
        return NMOS_DEP_FLAG;
    }

    // BJT loop:
    for(int i = 1; i<7; i++)
    {
        is_111 = C1[i] & C2[i] & C3[i]; // is 1 if we have read 111.
        count_111 += is_111;


        if (changed[i] && is_111) // Changed and all pins had power (BJT)
        { 
            switch (i)
            {
                case 0b001:
                    bjt_pins[bjt_count_111] = P1.ID;
                    bjt_count_111 ++;
                    break;
                case 0b010:
                    bjt_pins[bjt_count_111] = P2.ID;
                    bjt_count_111 ++;
                    break;
                case 0b100:
                    bjt_pins[bjt_count_111] = P3.ID;
                    bjt_count_111 ++;
                    break;
            }
        }
    }

    if(count <= 2)
    {
        //Bjt_Measure(BJT_FLAG, bjt_pins);
        return BJT_FLAG; // Only BJT could portray this behaviour, we powered the base (NPN) or the emitter/collector (PNP), yet we do not know which.
    }
    else if(bjt_count_111 == 1) // 111 was read once, we powered the base of a NPN
    {
        NPN_Measure(bjt_pins);
        return NPN_FLAG;
    }
    else if(bjt_count_111 == 2) // 111 was read twice, we powered the emitter & collector of a PNP
    {
        PNP_Measure(bjt_pins);
        return PNP_FLAG;
    }
    else if(count_111 == 1 && count == 3) // PMOS
    {

        // Identifying drain, gate and source
        for(byte i = 1; i<7; i++) // Useful combinations
        {
            is_111 = (C1[i] & C2[i] & C3[i]);
            if(changed[i] && is_111) // The non-powered probe is the source
            {
                switch (i)
                {
                    case 0b110:
                        attr::Semiconductor.Emitter = P1.ID; // This name is not the best
                        break;
                    case 0b101:
                        attr::Semiconductor.Emitter = P2.ID;
                        break;
                    case 0b011:
                        attr::Semiconductor.Emitter = P3.ID;
                        break;
                }
            }
            else if(changed[i] && !is_111) 
            {
                if      (C2[i] & C3[i]){ attr::Semiconductor.Base = P1.ID; } // The probe with a non-powered reading is the Gate
                else if (C1[i] & C3[i]){ attr::Semiconductor.Base = P2.ID; }
                else if (C1[i] & C2[i]){ attr::Semiconductor.Base = P3.ID; }
            }
        }
        attr::Semiconductor.Collector = remaining_probe(attr::Semiconductor.Emitter, attr::Semiconductor.Base);

        MOS_Measure(PMOS_ENH_FLAG);

        return PMOS_ENH_FLAG;
    }
    else if(count_111 == 2 && count == 3) // NMOS - enhancement
    {
        //nmos_enh_measure();
        for(byte i = 1; i<7; i++) // Useful combinations
        {
            is_111 = (C1[i] & C2[i] & C3[i]);
            if(changed[i] && !is_111) 
            {
                if      (C2[i] & C3[i]){ attr::Semiconductor.Base = P1.ID; } // The probe with a non-powered reading is the Gate
                else if (C1[i] & C3[i]){ attr::Semiconductor.Base = P2.ID; }
                else if (C1[i] & C2[i]){ attr::Semiconductor.Base = P3.ID; }

                switch (i)// The only powered probe is the source
                {
                    case 0b001:
                        attr::Semiconductor.Emitter = P1.ID; // This name is not the best
                        break;
                    case 0b010:
                        attr::Semiconductor.Emitter = P2.ID;
                        break;
                    case 0b100:
                        attr::Semiconductor.Emitter = P3.ID;
                        break;
                }
            }
        }
        attr::Semiconductor.Collector = remaining_probe(attr::Semiconductor.Emitter, attr::Semiconductor.Base);

        MOS_Measure(NMOS_ENH_FLAG);
        return NMOS_ENH_FLAG;
    }

    return 0; // NOT IDENTIFIED
}

#undef IDENTIFY_CPP