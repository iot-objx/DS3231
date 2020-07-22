/*
  DS3231.cpp - Arduino/chipKit library support for the DS3231 I2C Real-Time Clock
  Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
  
  This library has been made to easily interface and use the DS3231 RTC with
  an Arduino or chipKit.

  You can find the latest version of the library at 
  http://www.RinkyDinkElectronics.com/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.

  Commercial use of this library requires you to buy a license that
  will allow commercial use. This includes using the library,
  modified or not, as a tool to sell products.

  The license applies to all part of the library including the 
  examples and tools supplied with the library.
*/
#ifndef DS3231_h
#define DS3231_h

#if defined(__AVR__)
	#include "Arduino.h"
	#include "hardware/avr/HW_AVR_defines.h"
#elif defined(__PIC32MX__)
	#include "WProgram.h"
	#include "hardware/pic32/HW_PIC32_defines.h"
#elif defined(__arm__)
	#include "Arduino.h"
	#include "hardware/arm/HW_ARM_defines.h"
#endif

#define DS3231_ADDR_R	0xD1
#define DS3231_ADDR_W	0xD0
#define DS3231_ADDR		0x68

#define FORMAT_SHORT	1
#define FORMAT_LONG		2

#define FORMAT_LITTLEENDIAN	1
#define FORMAT_BIGENDIAN	2
#define FORMAT_MIDDLEENDIAN	3

#define MONDAY		1
#define TUESDAY		2
#define WEDNESDAY	3
#define THURSDAY	4
#define FRIDAY		5
#define SATURDAY	6
#define SUNDAY		7

// Square-wave output frequency
enum SQWAVE_FREQS_t {
	SQWAVE_1_HZ,
	SQWAVE_1024_HZ,
	SQWAVE_4096_HZ,
	SQWAVE_8192_HZ,
};

// SQW/ALARM Output Selection
enum MODES_t
{
	SQWAVE,
	ALARM1,
	ALARM2,
	ALARMx,
};

// Alarm Masks
enum ALARM_TYPES_t 
{
	ALM1_EVERY_SECOND = 0x0F,	// Alarm once per second
	ALM1_MATCH_SECONDS = 0x0E,	// Alarm when seconds match
	ALM1_MATCH_MINUTES = 0x0C,	// Alarm when minutes and seconds match
	ALM1_MATCH_HOURS = 0x08,	// Alarm when hours, minutes, and seconds match
	ALM1_MATCH_DATE = 0x00,		// Alarm when date, hours, minutes, and seconds match
	ALM1_MATCH_DAY = 0x10,		// Alarm when day, hours, minutes, and seconds match
	ALM2_EVERY_MINUTE = 0x8E,	// Alarm once per minute (00 seconds of every minute)
	ALM2_MATCH_MINUTES = 0x8C,	// Alarm when minutes match
	ALM2_MATCH_HOURS = 0x88,	// Alarm when hours and minutes match
	ALM2_MATCH_DATE = 0x80,		// Alarm when date, hours, and minutes match
	ALM2_MATCH_DAY = 0x90,		// Alarm when day, hours, and minutes match
};

class Time
{
public:
	uint8_t		hour;
	uint8_t		min;
	uint8_t		sec;
	uint8_t		date;
	uint8_t		mon;
	uint16_t	year;
	uint8_t		dow;

	Time();
};

class DS3231
{
	public:
		DS3231(uint8_t data_pin, uint8_t sclk_pin);
		void	begin();
		Time	getTime();
		void	setTime(uint8_t sec, uint8_t min, uint8_t hour);
		void	setDate(uint8_t date, uint8_t mon, uint16_t year, uint16_t epochYear = 1970);
		void	setDateTime(Time t, uint16_t epochYear = 1970);
		void	setDateTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t date, uint8_t mon, uint16_t year, uint16_t epochYear = 1970);
		void	setDOW();
		void	setDOW(uint8_t dow);
		Time	makeDateTime(unsigned long time);
		void	setAlarm(ALARM_TYPES_t alarmType, uint8_t sec, uint8_t min, uint8_t hour, uint8_t daydate);
		uint8_t	checkAlarm(void);

		char	*getTimeStr(uint8_t format=FORMAT_LONG);
		char	*getDateStr(uint8_t slformat=FORMAT_LONG, uint8_t eformat=FORMAT_LITTLEENDIAN, char divider='.');
		char	*getDOWStr(uint8_t format=FORMAT_LONG);
		char	*getMonthStr(uint8_t format=FORMAT_LONG);
		unsigned long getUnixTime();
		unsigned long getUnixTime(Time t);

		void	enable32KHz(bool enable);
		void	setOutput(MODES_t mode);
		void	setSQWRate(SQWAVE_FREQS_t rate);
		float	getTemperature();

	private:
		uint8_t _scl_pin;
		uint8_t _sda_pin;
		uint8_t _burstArray[7];
		boolean	_use_hw;
		uint16_t YEAR0 = 1970; // 1970 or 2000 or user defined

		void	_sendStart(byte addr);
		void	_sendStop();
		void	_sendAck();
		void	_sendNack();
		void	_waitForAck();
		uint8_t	_readByte();
		void	_writeByte(uint8_t value);
		void	_burstRead();
		uint8_t	_readRegister(uint8_t reg);
		void 	_writeRegister(uint8_t reg, uint8_t value);
		uint8_t	_decode(uint8_t value);
		uint8_t	_decodeH(uint8_t value);
		uint8_t	_decodeY(uint8_t value);
		uint8_t	_encode(uint8_t vaule);
#if defined(__arm__)
		Twi		*twi;
#endif
};
#endif
