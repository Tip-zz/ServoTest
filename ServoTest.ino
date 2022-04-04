/*
File:         ServoTest.ino
Origin:       13-Mar-2021
Author:       Tip Partridge
Target:       Arduino Nano
Description:
  * Servo thingie test.
  * Servo gets PWM signal, 800 - 2200us.
  * Frequency = 50Hz.
  * Speed controlled by analog input, 0..5V.
  * Using Timer/Counter in Fast PWM mode. Counts to PWMTOP, jumps to 0, and sets OC1A output.
    Output compare at OCR1A resets OC1A output.

Revision History: 
  13-Mar-2021 TEP v0.1 First version that works and has comments.
  26-Feb-2022 TEP v0.2 Display PWMICR only when it changes.
*/
///#include <Arduino.h>
//
////////////////////////////
// Global constant and variable declarations
////////////////////////////
//
#define Version "ServoTest v0.2"

#define Baud 115200

// Pin for Servo PWM
#define PWMPin 9      // D9 (OC1A)

// Pin for Analog input
#define AnaPin 0      // A0 (ADC0)

// Timer Stuff
#define system_clock 16e6
#define timer_prescale 8
#define PWMin 800e-6
#define PWMax 2200e-6
#define ADCMax 1023  // 10 bit ADC
float PWMClock = system_clock / timer_prescale;
#define PWMFreq_default 50 // Default update rate (Hz)
////#define PWMFreq_default 200 // Try some different frequencies
float PWMFreq;
float PWMm;     // Slope for ADC to OCR1A
float PWMb;     // Offset for ADC to OCR1A
unsigned long PWMTOP;   // Count to create PWMFreq, goes to ICR1 register.
unsigned long PWMICR, PWMICR0;   // Count for desired pulse width, goes to OCR1A register.

// TCCR1A Timer/Counter Control Register A = 0x82
//   7 COM1A1 = 1 Compare Output Mode for Channel A:1
//   6 COM1A0 = 0 Compare Output Mode for Channel A:0
//   5 COM1B1 = 0 Compare Output Mode for Channel B:1
//   4 COM1B0 = 0 Compare Output Mode for Channel B:0
//   3 x = 0
//   2 x = 0
//   1 WGM11 = 1 Waveform Generation Mode:1
//   0 WGM10 = 0 Waveform Generation Mode:0 Fast mode TOP: 5=255,6=511,7=1023,14=ICR1,15=OCR1A
// TCCR1B Timer/Counter Control Register B = 0x1a
//   7 ICNC1 = 0 Input Capture Noise Canceler
//   6 ICES1 = 0 Input Capture Edge Select
//   5 Reserved = 0 Must be 0
//   4 WGM13 = 1 Waveform Generation Mode:3
//   3 WGM12 = 1 Waveform Generation Mode:2
//   2 CS12 = 0 Clock Select:2
//   1 CS11 = 1 Clock Select:1
//   0 CS10 = 0 Clock Select:0 000=none, 001=/1, 010=/8, 011=/64,
//                             100=/256, 101=/1024, 110=Ext neg, 111=Ext pos
// TCCR1C Timer/Counter Control Register C = 0
//   7 FOC1A = 0 Force Output Compare for Channel A
//   6 FOC1B = 0 Force Output Compare for Channel B
//   0..5 x = 0
// TCNT1 Counter
// OCR1A Output Compare Register A
// OCR1A Output Compare Register B
// ICR1 Input Capture Regigter
// TIMSK1 Timer/Counter Interrupt Mask Register
//   7 x = 0
//   6 x = 0
//   5 ICIE1 = 0 Timer/Counter1, Input Capture Interrupt Enable
//   4 x = 0
//   3 x = 0
//   2 OCIE1B = 0 Timer/Counter1, Output Compare B Match Interrupt Enable
//   1 OCIE1A = 0 Timer/Counter1, Output Compare A Match Interrupt Enable
//   0 TOIE1 = 0 Timer/Counter1, Overflow Interrupt Enable
// TIFR1 Timer/Counter Interrupt Flag Register
//   7 x = 0
//   6 x = 0
//   5 ICF1 = 0 Timer/Counter1, Input Capture Flag
//   4 x = 0
//   3 x = 0
//   2 OCF1B = 0 Timer/Counter1, Output Compare B Match Flag
//   1 OCF1A = 0 Timer/Counter1, Output Compare A Match Flag
//   0 TOV1 = 0 Timer/Counter1, Overflow Flag

//
////////////////////////////
// Setup
////////////////////////////
//
void setup()
  {
// Hardware init
  Serial.begin(Baud);            // USB virtual serial port
  Serial.println(Version);
  pinMode(PWMPin, OUTPUT);
  digitalWrite(PWMPin, LOW);
// Calculate runtine parameters (might want to edit these at runtime)
  PWMClock = system_clock / timer_prescale;
  PWMFreq = PWMFreq_default;
  PWMTOP = PWMClock / PWMFreq;
  PWMm = (PWMax - PWMin) / ADCMax;
  PWMb = PWMin;
Serial.print("PWMin = ");
Serial.println(PWMin*1e6);
Serial.print("PWMax = ");
Serial.println(PWMax*1e6);
Serial.print("PWMm = ");
Serial.println(PWMm*1e6);
Serial.print("PWMb = ");
Serial.println(PWMb*1e6);
Serial.print("PWMPin = ");
Serial.println(PWMPin);
// Initial OCR1A value from ADC reading
  PWMICR0 = PWMClock * (analogRead(AnaPin) * PWMm + PWMb);
  PWMICR = PWMICR0;
Serial.print("PWMICR = ");
Serial.println(PWMICR);
//
  TIMSK1 = 0x00;                // Interrupts off for now...
  TCCR1A = 0x00;                // Normal mode
  TCCR1B = 0x00;                // Stop clock
  TCCR1C = 0x00;                // Nothing forced
  TCNT1 = 0x00;                 // Clear counter
  ICR1 = PWMTOP;                // PWM TOP count (constant)
  OCR1A = PWMICR;               // PWM output compare (adjusted per ADC each pulse)
  TIFR1 = 0xff;                 // Clear flag registers
  TCCR1A = 0x82;                // Set output mode and PWM mode
  TCCR1B = 0x1a;                // Set clock prescale and PWM mode
  TIMSK1 |= _BV(TOIE1);         // Timer 1 overflow interrupt enable
  sei();                        // turn interrupts on
  }

//
////////////////////////////
// Main Loop
////////////////////////////
//
void loop()
  {
  if (PWMICR != PWMICR0)      // Changed?
    {
    Serial.println(PWMICR);   // Display pulse width
    PWMICR0 = PWMICR;         // Save new value
    delay(1000);
    }
  }

////////////////////////////
// Timer 1 ISR Controls pulse width
////////////////////////////
ISR(TIMER1_OVF_vect) {    // Timer/Counter1 Overflow
// Read ADC, Calculate Output Compare value, Write new value to register
  PWMICR = PWMClock * (analogRead(AnaPin) * PWMm + PWMb);
  OCR1A = PWMICR;
}
