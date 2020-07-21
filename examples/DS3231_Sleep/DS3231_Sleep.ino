/*
    Name:       SleepRTC.ino
    Created:  11-Nov-19 9:55:28 AM
    Author:     Md Abdullah AL IMRAN
*/
// DS3231_Sleep
// Copyright (C)2020 Md Abdullah Al Imran. All right reserved
//
// A quick demo of how to use my DS3231-library to demonstrate 
// the Alram and Square Wave features in a Sleep application. 
//
// To use the hardware I2C (TWI) interface of the Arduino you must connect
// the pins as follows:
//
// Arduino Uno/2009:
// ----------------------
// DS3231:  SDA pin   -> Arduino Analog 4 or the dedicated SDA pin
//          SCL pin   -> Arduino Analog 5 or the dedicated SCL pin
//
// Arduino Leonardo:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 2 or the dedicated SDA pin
//          SCL pin   -> Arduino Digital 3 or the dedicated SCL pin
//
// Arduino Mega:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 20 (SDA) or the dedicated SDA pin
//          SCL pin   -> Arduino Digital 21 (SCL) or the dedicated SCL pin
//
// Arduino Due:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 20 (SDA) or the dedicated SDA1 (Digital 70) pin
//          SCL pin   -> Arduino Digital 21 (SCL) or the dedicated SCL1 (Digital 71) pin
//
// The internal pull-up resistors will be activated when using the 
// hardware I2C interfaces.


#include<DS3231.h>
#include<TimerOne.h>
#include<avr/sleep.h>
#include<avr/power.h>
#include<util/atomic.h>

#define RTC_SQW_PIN     0x03
DS3231 RTC(SDA, SCL);

volatile unsigned long milsecElapsed = 0L; // Mimics millisecond counter

unsigned long tmMillis[2], tmTimer1[2];
String tmRTC[2];

void setup()
{
  Serial.begin(9600);
  Serial.println(F("--------------------------------------------------"));

  RTC.begin();
  RTC.setDateTime(0, 0, 0, 1, 1, 1970);
  milsecElapsed = 0L;
  Timer1.initialize(1000); // 1 ms = 1000us. <Min 1us Max 8.3s>
  Timer1.attachInterrupt(millisecISR); // Attach the timer ISR
  Serial.println("\tTimer1 Config");

  pinMode(RTC_SQW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN), rtcPulseISR, FALLING);
  RTC.setOutput(SQWAVE);
  RTC.setSQWRate(SQWAVE_1_HZ); // Do NOT use other rates. Refer to Application Engineer's mail.
  Serial.println("\tRTC Config");

  Serial.println("ALL Ok");
  delay(5000);
}

void loop()
{
  // Prepare for sleep
  Time curTime = RTC.getTime();
  curTime.sec += 30;
  if (curTime.sec > 59)
  {
    curTime.min += 1;
    curTime.sec = (curTime.sec % 60);
  }
  //curTime.min += 30;
  if (curTime.min > 59)
  {
    curTime.hour += 1;
    curTime.min = (curTime.min % 60);
  }
  //curTime.hour += 30;
  if (curTime.hour > 24)
  {
    curTime.date += 1;
    curTime.hour = (curTime.hour % 24);
  }

  //Serial.print(F("ALARM: "));
  //Serial.print(curTime.hour); Serial.print(F(":")); Serial.print(curTime.min); Serial.print(F(":")); Serial.println(curTime.sec);

  RTC.checkAlarm(); // Clear RTC Alarm flags
  RTC.setAlarm(ALARM_TYPES_t::ALM1_MATCH_MINUTES, curTime.sec, curTime.min, curTime.hour, curTime.date);
  RTC.setOutput(MODES_t::ALARM1);

  //Serial.println(F("SLEEP")); 
  //Serial.print(F("\tMILLIS: ")); Serial.println(millis());
  //Serial.print(F("\tTIMER1: ")); Serial.println(milsecElapsed);
  //Serial.print(F("\tRTC   : ")); Serial.println(RTC.getTimeStr());
  //delay(100);

  tmMillis[0] = millis();
  tmTimer1[0] = milsecElapsed;
  tmRTC[0] = RTC.getTimeStr();

  // MCU Put to sleep
  sleepNow();
  //sleepNowPRR();
  /*
        zZ
      zZ
    zZ
  */
  // MCU Woke up

  //Serial.println(F("AWAKE"));
  //Serial.print(F("\tMILLIS: ")); Serial.println(millis());
  //Serial.print(F("\tTIMER1: ")); Serial.println(milsecElapsed);
  //Serial.print(F("\tRTC   : ")); Serial.println(RTC.getTimeStr());
  //delay(100);

  tmMillis[1] = millis();
  tmTimer1[1] = milsecElapsed;
  tmRTC[1] = RTC.getTimeStr();

  Serial.println(F("Before Sleep"));
  Serial.print(F("\tMILLIS: ")); Serial.println(tmMillis[0]);
  Serial.print(F("\tRTC CLK: ")); Serial.print(tmRTC[0] + "."); Serial.println(tmTimer1[0]);

  Serial.println(F("After Sleep"));
  Serial.print(F("\tMILLIS: ")); Serial.println(tmMillis[1]);
  Serial.print(F("\tRTC CLK: ")); Serial.print(tmRTC[1] + "."); Serial.println(tmTimer1[1]);

  // Stuck here
  Serial.println(F("DONE")); Serial.flush();
  RTC.checkAlarm(); // Clear RTC Alarm flags
  while (true)
  {
    Serial.print(RTC.getTimeStr()); Serial.print(F(".")); Serial.println(milsecElapsed);
    delay(1000);
  }
}

// Timer1 Interrupt Service Routine - mimics millisecond clock
void millisecISR() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    milsecElapsed++;
  }
}

// RTC 1 second pulse
void rtcPulseISR(void) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    milsecElapsed = 0L;
  }
}

// The arduino is put to sleep here
void sleepNow()
{
  /* Now is the time to set the sleep mode. In the Atmega8 datasheet
   * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
   * there is a list of sleep modes which explains which clocks and
   * wake up sources are available in which sleep mode.
   *
   * In the avr/sleep.h file, the call names of these sleep modes are to be found:
   *
   * The 5 different modes are:
   *     SLEEP_MODE_IDLE         -the least power savings
   *     SLEEP_MODE_ADC
   *     SLEEP_MODE_PWR_SAVE
   *     SLEEP_MODE_STANDBY
   *     SLEEP_MODE_PWR_DOWN     -the most power savings
   *
   * For now, we want as much power savings as possible, so we
   * choose the according
   * sleep mode: SLEEP_MODE_PWR_DOWN
   *
   */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here

  sleep_enable();          // enables the sleep bit in the mcucr register
               // so sleep is possible. just a safety pin 

  /* Now it is time to enable an interrupt. We do it here so an
   * accidentally pushed interrupt button doesn't interrupt
   * our running program. if you want to be able to run
   * interrupt code besides the sleep function, place it in
   * setup() for example.
   *
   * In the function call attachInterrupt(A, B, C)
   * A   can be either 0 or 1 for interrupts on pin 2 or 3.
   *
   * B   Name of a function you want to execute at interrupt for A.
   *
   * C   Trigger mode of the interrupt pin. can be:
   *             LOW        a low level triggers
   *             CHANGE     a change in level triggers
   *             RISING     a rising edge of a level triggers
   *             FALLING    a falling edge of a level triggers
   *
   * In all but the IDLE sleep modes only LOW can be used.
   */

  attachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN), wakeUpNow, LOW); // use interrupt 0 (pin 2) and run function
                     // wakeUpNow when pin 2 gets LOW 

  sleep_mode();            // here the device is actually put to sleep!!
               // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP

  sleep_disable();         // first thing after waking from sleep:
               // disable sleep...
  //detachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN));
  // disables interrupt 0 on pin 2 so the 
  // wakeUpNow code will not be executed 
  // during normal running time.
}

// The arduino is put to sleep (using PRR registers)
void sleepNowPRR() {
  // disable ADC
  ADCSRA = 0;

  // turn off various modules
  power_all_disable();
  //power_timer1_enable(); // ??????????????????

  // Note: You must use the PRR after setting ADCSRA to zero, otherwise the ADC is "frozen" in an active state.

  set_sleep_mode(SLEEP_MODE_IDLE);
  noInterrupts();           // timed sequence follows
  sleep_enable();

  attachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN), wakeUpNow, RISING);
  interrupts();             // guarantees next instruction executed
  sleep_cpu();              // sleep within 3 clock cycles of above    



  //sleep_mode();            // here the device is actually put to sleep!!
               // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP

  sleep_disable();         // first thing after waking from sleep:
               // disable sleep...
  power_all_enable(); // restore all
  detachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN));
  // disables interrupt 0 on pin 2 so the 
  // wakeUpNow code will not be executed 
  // during normal running time.
}

// The interrupt is handled after wakeup here
void wakeUpNow()
{
  // execute code here after wake-up before returning to the loop() function
  // timers and code using timers (serial.print and more...) will not work here.
  // we don't really need to execute any special functions here, since we
  // just want the thing to wake up

  // Do after waking up...
  attachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN), rtcPulseISR, FALLING);
  RTC.setOutput(SQWAVE);
  RTC.setSQWRate(SQWAVE_1_HZ); // Do NOT use other rates. Refer to Application Engineer's mail.
}
