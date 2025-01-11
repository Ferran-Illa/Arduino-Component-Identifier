#define TIME_CPP

#include <config.h>
#include <common.h>
#include <functions.h>

/*
  * Measures the time needed for the current flowing through an inductor/capacitor to cause a voltage
  * difference equal to the Internal BandGap Reference (1.02V Calibrated, typical 1.1V). 
  * This is one step in the procedure of measuring an inductance or capacitance.
  */
/*
  * Pin Mapping (D1->D13 = 1->13, A0->A5 = 14->20)
  * Low resistances at pins 5 / 8 / 11
  * The way we connected the pins ( A1 = 15 | 5,6,7 )( A2 = 16 | 8,9,10 )( A3 = 17 | 11,12,13 )
  */
byte InductorTMeasure(Probe probeA, Probe probeB, _Bool I_Mode, unsigned long *time){

  // ShuntPin has our Resistance connected to probe B which is the one that makes the measurement
  // Initializing variables:
      
  int OverflowTicks = 0;             // Overflow counter
  int Offset = 68*62.5;               // Tick Offset due to processing
  long Count = 0;                    // Time Counter
  _Bool TOUT = 0;                // Timeout Flag
  byte ShuntPin = probeB.Rl;

  if (I_Mode){                  // 1 for High Current (Low Resistance), 0 for Low Current (High Resistance).
    pinMode(ShuntPin, INPUT);   // No Shunt Resistor
    pinMode(probeA.ID, OUTPUT);
    pinMode(probeB.ID, OUTPUT); // Pull down probeB directly (we have a ~25 Ohm internal resistance)

    digitalWrite(probeA.ID, LOW); 
    digitalWrite(probeB.ID, LOW);
  }
  else{
    pinMode(ShuntPin, OUTPUT); // Shunt Resistor Enabled (680 Ohms)
    pinMode(probeA.ID, OUTPUT);
    pinMode(probeB.ID, INPUT); // Probe B will monitor the values but no current will flow through it.
    
    digitalWrite(ShuntPin, LOW); 
    digitalWrite(probeA.ID, LOW);
  }

  /* NOTE:
  * High current will benefit from the circuit's internal resistances and their values
  * should be precisely determined to provide accurate enough measurements. Ri_H is the
  * value of the internal resistor when the pin is set to HIGH and Ri_L when set to LOW.
  *
  * Connections when current flows will be as follows:
  * High Current: GND - Ri_H - pin Ax // pin Ay - Ri_L - GND
  * Low Current: GND - Ri_H - pin probeA // pin probeB - R_shunt - Ri_L - GND
  *
  * Any doubts about the registers read and written here can be consulted in the
  * ATMEL MEGA328PB datasheet and manual.
  */


  ADCSRA = (0<<ADEN); // Switch off the ADC, needed to start the comparator
  ADCSRB = (1<<ACME); // Use Analog Multiplexed Input (To compare through pinB)

  // Setting up the analog comparator: enabling it | Internal bandgap reference | Clearing Interrupts | Disabling interrupts | Enabling Input Capture
  ACSR = (0 << ACD) | (1 << ACBG) | (1 << ACI) | (0 << ACIE) | (1 << ACIC);
  // Set ADCx (probeB) as negative input to the comparator, ADCx corresponds to the analog pin Ax, where the value of the ADMUX register is x. (x in [0,7])
  ADMUX = probeB.ID - 14; //Ax has a value of 14 + x, as A0 = 14 = 0xe (given there are 13 digital pins)

  delay(10); // Allow bandgap reference to settle

  // Timer
  OverflowTicks = 0;                    // reset overflow counter
  TCCR1A = 0;                           // set default mode
  TCCR1B = 0;                           // setting adequate timer modes
  TCCR1B |= (1<<ICNC1);                 // Input Capture Noise Canceler enabled
  TCNT1 = 0;                            // Reset counter
  ICR1 = 0;

  // Clearing all flags (Input Capture , Output Compare B , Output Compare A , Overflow Flag)
  TIFR1 = (1 << ICF1) | (1 << OCF1B) | (1 << OCF1A) | (1 << TOV1);
  TCCR1B |= (1 << CS10); // Start Timer on 1:1 clk divider

  digitalWrite(probeA.ID, HIGH);
                
  // Time Loop:
  while(1){
    //int Tflags = TIFR1;              // Get Timer 1 flags
    if(TIFR1 & (1 << ICF1)){break;}    // We stop the timer when the value rises above the bandgap reference.

    if (TIFR1 & (1 << TOV1))
    {
      // Overflow at 4.096 ms for 16 MHz
      TIFR1 = (1 << TOV1);              // Reset Overflow Flag
      wdt_reset();                      // Reset Watchdog to avoid timeout
      OverflowTicks ++;                 // Increase Overflow count
    }

    if (OverflowTicks == 160) // Max Waiting time (655ms)
      {
        TOUT = 1;
        break; //Stop the loop
      }
  }

  TCCR1B = 0;                           // stop Timer
  TIFR1 = (1 << ICF1);                  // reset Input Capture flag
  Count = ICR1*62.5;                    // Timer value
 
  digitalWrite(probeA.ID, LOW); // Stop Current Flow as soon as possible, could be an issue with inductors
  delay(10);
  // Reset all used pins
  pinMode(probeA.ID, INPUT);
  pinMode(ShuntPin,  INPUT); 
  pinMode(probeB.ID, INPUT);

  Count += OverflowTicks * 4096000; // Adding the overflow ticks

  // Serial.println(time); // Debug and Calibration Purposes, uncomment to print the ticks

  /* 
   * We introduce an offset to account for clock cycles used by delay and the loop
   * This is a calibration made when shorting the pins together, as a wire must return 0ns.
   * This offset is in fact the time elapsed during the call to digitalWrite. Could be improved
   * to be much less, but digitalWrite fits our purposes perfectly.
   */              

  if (Count < Offset) {Count = 0;} // If we measure a time shorter than Offset, we set the minimum time possible (0)
  else {Count -= Offset;}

  *time = Count;                 // Storing the time value

  // Reset Everything
  ADCSRA= 135;
  ADCSRB= 0;
  TCCR1A= 1;
  TCCR1B= 3;
  TIFR1= 39;
  
  if (TOUT)                 {return 10;}  // Timeout Flag
  if (Count == 0)           {return 1;}   // No Time Flag
  if (Count < 200)          {return 2;}   // Low Time Flag
  else if (Count > 100000)  {return 3;}   // High Time Flag

  return 0; //Successful
}

byte CapacitorTMeasure(Probe probeA, Probe probeB, byte R_Mode, unsigned long *time)
{
   /*
   * Measures the time needed for a capacitor to charge to VCC from a voltage
   * difference equal to the Internal BandGap Reference (1.02V Calibrated, typical 1.1V). 
   * This is one step in the procedure of measuring a capacitance.
   */
  /*
   * Pin Mapping (D1->D13 = 1->13, A0->A5 = 14->20)
   * Low resistances at pins 5 / 8 / 11
   * The way we connected the pins ( A1 = 15 | 5,6,7 )( A2 = 16 | 8,9,10 )( A3 = 17 | 11,12,13 )
   */
  // ShuntPin has our Resistance connected to probe B
  // Initializing variables:
      
  int OverflowTicks;             // Overflow counter
  int Offset = 4375;             // Nanosecond Offset due to processing
  long Count = 0;                // Time Counter
  _Bool TOUT = 0;                // Timeout Flag
  byte ShuntPin = probeB.Rl;     // The main discharge Resistor
  byte Pullup = probeA.ID;
  pinMode(probeA.ID, INPUT);
  pinMode(probeB.ID, INPUT); // Probe B will monitor the voltage.

  // 0 for Low Resistance
  if (R_Mode == 1) // 1 for Medium Resistance
  {
    Pullup = probeA.Rl;              
    ShuntPin = probeB.Rm;
  }
  else if (R_Mode == 2) // 2 for High Resistance
  {
    Pullup = probeA.Rl;              
    ShuntPin = probeB.Rh;
  }

  pinMode(Pullup, OUTPUT);
  pinMode(ShuntPin, OUTPUT);
  digitalWrite(ShuntPin, LOW);
  digitalWrite(Pullup, LOW); // Will act as the pullup and pulldown resistor.

  
  ADCSRA = (0<<ADEN); // Switch off the ADC, needed to start the comparator
  ADCSRB = (1<<ACME); // Use Analog Multiplexed Input (To compare through pinB)

  // Setting up the analog comparator: enabling it | Internal bandgap reference | Clearing Interrupts | Disabling interrupts | Enabling Input Capture 
  ACSR = (0 << ACD) | (1 << ACBG) | (1 << ACI) | (0 << ACIE) | (1 << ACIC);
  // Set ADCx (probeB) as negative input to the comparator, ADCx corresponds to the analog pin Ax, where the value of the ADMUX register is x. (x in [0,7])
  ADMUX = probeB.ID - 14; //Ax has a value of 14 + x, as A0 = 14 = 0xe (given there are 13 digital pins)

  delay(10); // Allow bandgap reference to settle

  // Timer
  OverflowTicks = 0;                    // reset overflow counter
  TCCR1A = 0;                           // set default mode
  TCCR1B = 0;                           // setting adequate timer modes
  //TCCR1B |= (1<<ICNC1);                 // Input Capture Noise Canceler enabled
  TCNT1 = 0;                            // Reset counter
  ICR1 = 0;

  // Clearing all flags (Input Capture , Output Compare B , Output Compare A , Overflow Flag)
  TIFR1 = (1 << ICF1) | (1 << OCF1B) | (1 << OCF1A) | (1 << TOV1);
  TCCR1B |= (1 << CS10); // Start Timer on 1:1 clk divider

  digitalWrite(Pullup, HIGH);
                
  // Time Loop:
  while(1){
    //int Tflags = TIFR1;              // Get Timer 1 flags
    if(ACSR & (1 << ACO)){break;}    // We stop the timer when the value rises above the bandgap reference.

    if (TIFR1 & (1 << TOV1))
    {
      // Overflow at 4.096 ms for 16 MHz
      TIFR1 = (1 << TOV1);                // Reset Overflow Flag
      wdt_reset();                      // Reset Watchdog to avoid timeout
      OverflowTicks ++;                 // Increase Overflow count
    }

    if (OverflowTicks == 500) // Max Waiting time 2s
      {
        TOUT = 1;
        //Serial.print("Timeout");
        break; //Stop the loop
      }
  }
  
  TCCR1B = 0;                           // stop Timer
  TIFR1 = (1 << ICF1);                  // reset Input Capture flag
  Count = ICR1*62.5;                    // Timer value
  // Reset all used pins
  digitalWrite(Pullup, LOW);
  pinMode(Pullup, INPUT);
  pinMode(ShuntPin, INPUT);
  Count += OverflowTicks * 4096000; // Adding the overflow ticks
  // Serial.println(time); // Debug and Calibration Purposes, uncomment to print the ticks
  /* 
   * We introduce an offset to account for clock cycles used by delay and the loop
   * This is a calibration made when shorting the pins together, as a wire must return 0ns.
   * This offset is in fact the time elapsed during the call to digitalWrite. Could be improved
   * to be much less, but digitalWrite fits our purposes perfectly.
   */              
  if (Count < Offset) {Count = 0;} // If we measure a time shorter than Offset, we set the minimum time possible (0)
  else {Count -= Offset;}

  *time = Count;                 // Storing the time value //TODO: *??

  // Reset Everything
  ADCSRA= 135;
  ADCSRB= 0;
  TCCR1A= 1;
  TCCR1B= 3;
  TIFR1= 39;
  
  if (TOUT)                 {return 10;}  // Timeout Flag
  if (Count == 0)           {return 1;}   // No Time Flag
  if (Count < 1000)         {return 2;}   // Low Time Flag

  return 0; //Successful
}

#undef TIME_CPP