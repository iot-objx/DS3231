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
#include "DS3231.h"

// Include hardware-specific functions for the correct MCU
#if defined(__AVR__)
	#include "hardware/avr/HW_AVR.h"
#elif defined(__PIC32MX__)
  #include "hardware/pic32/HW_PIC32.h"
#elif defined(__arm__)
	#include "hardware/arm/HW_ARM.h"
#endif

// DS3231 Register Addresses
#define REG_SEC			0x00
#define REG_MIN			0x01
#define REG_HOUR		0x02
#define REG_DOW			0x03
#define REG_DATE		0x04
#define REG_MON			0x05
#define REG_YEAR		0x06
#define ALM1_SECONDS	0x07
#define ALM1_MINUTES	0x08
#define ALM1_HOURS		0x09
#define ALM1_DAYDATE	0x0A
#define ALM2_MINUTES	0x0B
#define ALM2_HOURS		0x0C
#define ALM2_DAYDATE	0x0D
#define REG_CON			0x0E
#define REG_STATUS		0x0F
#define REG_AGING		0x10
#define REG_TEMPM		0x11
#define REG_TEMPL		0x12

// Alarm mask bits
#define A1M1 7
#define A1M2 7
#define A1M3 7
#define A1M4 7
#define A2M2 7
#define A2M3 7
#define A2M4 7
#define DYDT 6 // Day/Date flag bit in alarm Day/Date registers

// Control register bits
#define EOSC	7
#define BBSQW	6
#define CONV	5
#define RS2		4
#define RS1		3
#define INTCN	2
#define A2IE	1
#define A1IE	0

// Status register bits
#define OSF		7
#define BB32KHZ 6
#define CRATE1	5
#define CRATE0	4
#define EN32KHZ 3
#define BSY		2
#define A2F		1
#define A1F		0

#define SECS_DAY                (86400L)
#define LEAPYEAR(year)          (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)          (LEAPYEAR(year) ? 366L : 365L)
static const uint8_t calender[] = { 31,28,31,30,31,30,31,31,30,31,30,31 }; // Calender Days In Month

/* Public */

Time::Time()
{
	this->year = 2014;
	this->mon  = 1;
	this->date = 1;
	this->hour = 0;
	this->min  = 0;
	this->sec  = 0;
	this->dow  = 3;
}

DS3231::DS3231(uint8_t data_pin, uint8_t sclk_pin)
{
	_sda_pin = data_pin;
	_scl_pin = sclk_pin;
}

Time DS3231::getTime()
{
	Time t;
	_burstRead();
	t.sec	= _decode(_burstArray[0]);
	t.min	= _decode(_burstArray[1]);
	t.hour	= _decodeH(_burstArray[2]);
	t.dow	= _burstArray[3];
	t.date	= _decode(_burstArray[4]);
	t.mon	= _decode(_burstArray[5]);
	t.year	= _decodeY(_burstArray[6]) + YEAR0;
	return t;
}

void DS3231::setTime(uint8_t sec, uint8_t min, uint8_t hour)
{
	if (((hour>=0) && (hour<24)) && ((min>=0) && (min<60)) && ((sec>=0) && (sec<60)))
	{
		_writeRegister(REG_HOUR, _encode(hour));
		_writeRegister(REG_MIN, _encode(min));
		_writeRegister(REG_SEC, _encode(sec));
	}
}

void DS3231::setDate(uint8_t date, uint8_t mon, uint16_t year, uint16_t epochYear)
{
	YEAR0 = epochYear;
	year -= epochYear;
	if (((date>0) && (date<=31)) && ((mon>0) && (mon<=12)) && ((year>=0) && (year<=99)))
	{
		//year -= 2000;
		_writeRegister(REG_YEAR, _encode(year));
		_writeRegister(REG_MON, _encode(mon));
		_writeRegister(REG_DATE, _encode(date));
	}
}

void DS3231::setDateTime(Time t, uint16_t epochYear) {
	setTime(t.sec, t.min, t.hour);
	setDate(t.date, t.mon, t.year, epochYear);
	setDOW(t.dow);
}

void DS3231::setDateTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t date, uint8_t mon, uint16_t year, uint16_t epochYear = 1970) {
	setTime(sec, min, hour);
	setDate(date, mon, year, epochYear);
	setDOW();
}

void DS3231::setDOW()
{
	int dow;
	byte mArr[12] = {6,2,2,5,0,3,5,1,4,6,2,4};
	Time _t = getTime();
  
	dow = (_t.year % 100);
	dow = dow*1.25;
	dow += _t.date;
	dow += mArr[_t.mon-1];
	if (((_t.year % 4)==0) && (_t.mon<3))
		dow -= 1;
	while (dow>7)
		dow -= 7;
	_writeRegister(REG_DOW, dow);
}

void DS3231::setDOW(uint8_t dow)
{
	if ((dow>0) && (dow<8))
		_writeRegister(REG_DOW, dow);
}

Time DS3231::makeDateTime(unsigned long time) {
	Time t;
	unsigned long dayclock, dayno;
	uint16_t year = YEAR0;
	//int yday = 0;

	dayno = time / SECS_DAY;
	dayclock = time % SECS_DAY;
	
	t.sec = dayclock % 60; // Seconds 0 - 59
	t.min = (dayclock % 3600) / 60; // Minutes 0 - 59
	t.hour = dayclock / 3600; // Hours 0 - 23
	
	t.dow = (dayno + 3) % 7 + 1; // Day0 was a thursday(3) | day of the week, range 1 to 7 => Start of Week Mon

	while (dayno >= YEARSIZE(year)) {
		dayno -= YEARSIZE(year);
		year++;
	}
	
	//t.year = year - YEAR0; // The number of years since YEAR0
	t.year = year;
	//yday = dayno; // day in the year, range 0 to 365

	t.mon = 0; 
	/* METHOD 1*/
	unsigned int calDay = calender[t.mon];
	while (dayno >= calDay) {
		dayno -= calDay;
		t.mon++;
		calDay = (LEAPYEAR(year) && (t.mon == 1)) ? (calender[t.mon] + 1) : calender[t.mon];
	}
	
	/* METHOD 2*/
	/*while (dayno >= ((LEAPYEAR(year) && (t.mon == 1)) ? (calender[t.mon] + 1) : calender[t.mon])) {
		dayno -= (LEAPYEAR(year) && (t.mon == 1)) ? (calender[t.mon] + 1) : calender[t.mon];
		t.mon++;
	}*/

	t.mon += 1; // month, range 1 to 12
	t.date = dayno + 1; // day of the month, range 1 to 31

	return t;
}

// Set an alarm time. Sets the alarm registers only.  To cause the
// INT pin to be asserted on alarm match, use setOutput().
// This method can set either Alarm 1 or Alarm 2, depending on the
// value of alarmType (use a value from the ALARM_TYPES_t enumeration).
// When setting Alarm 2, the seconds value must be supplied but is
// ignored, recommend using zero. (Alarm 2 has no seconds register.)
void DS3231::setAlarm(ALARM_TYPES_t alarmType, uint8_t sec, uint8_t min, uint8_t hour, uint8_t daydate)
{
	uint8_t addr;

	sec = _encode(sec); 
	min = _encode(min);
	hour = _encode(hour);
	daydate = _encode(daydate);	// Date: 1-31 | Day: 1-7
	
	if (alarmType & 0x01) sec |= (1 << A1M1);
	if (alarmType & 0x02) min |= (1 << A1M2);
	if (alarmType & 0x04) hour |= (1 << A1M3);
	if (alarmType & 0x10) daydate |= (1 << DYDT);
	if (alarmType & 0x08) daydate |= (1 << A1M4);

	if (!(alarmType & 0x80)) // Alarm 1
	{
		addr = ALM1_SECONDS;
		_writeRegister(addr++, sec); 
	}
	else					 // Alarm 2
	{
		addr = ALM2_MINUTES;
	}
	_writeRegister(addr++, min);
	_writeRegister(addr++, hour);
	_writeRegister(addr++, daydate);
}

// Returns the alarm number (if any) and resets the alarm flag bit.
uint8_t DS3231::checkAlarm(void) {
	uint8_t _reg = _readRegister(REG_STATUS); 
	uint8_t _creg = _readRegister(REG_CON) & _reg; 
	
	if (_reg & (1 << A1F))
	{
		_reg &= ~(1 << A1F); 
	}
	if (_reg & (1 << A2F))
	{
		_reg &= ~(1 << A2F);
	}
	_writeRegister(REG_STATUS, _reg); 

	return _creg & 0x03;
}

char *DS3231::getTimeStr(uint8_t format)
{
	static char output[] = "xxxxxxxx";
	Time t;
	t=getTime();
	if (t.hour<10)
		output[0]=48;
	else
		output[0]=char((t.hour / 10)+48);
	output[1]=char((t.hour % 10)+48);
	output[2]=58;
	if (t.min<10)
		output[3]=48;
	else
		output[3]=char((t.min / 10)+48);
	output[4]=char((t.min % 10)+48);
	output[5]=58;
	if (format==FORMAT_SHORT)
		output[5]=0;
	else
	{
	if (t.sec<10)
		output[6]=48;
	else
		output[6]=char((t.sec / 10)+48);
	output[7]=char((t.sec % 10)+48);
	output[8]=0;
	}
	return (char*)&output;
}

char *DS3231::getDateStr(uint8_t slformat, uint8_t eformat, char divider)
{
	static char output[] = "xxxxxxxxxx";
	int yr, offset;
	Time t;
	t=getTime();
	switch (eformat)
	{
		case FORMAT_LITTLEENDIAN:
			if (t.date<10)
				output[0]=48;
			else
				output[0]=char((t.date / 10)+48);
			output[1]=char((t.date % 10)+48);
			output[2]=divider;
			if (t.mon<10)
				output[3]=48;
			else
				output[3]=char((t.mon / 10)+48);
			output[4]=char((t.mon % 10)+48);
			output[5]=divider;
			if (slformat==FORMAT_SHORT)
			{
				yr=t.year-YEAR0;
				if (yr<10)
					output[6]=48;
				else
					output[6]=char((yr / 10)+48);
				output[7]=char((yr % 10)+48);
				output[8]=0;
			}
			else
			{
				yr=t.year;
				output[6]=char((yr / 1000)+48);
				output[7]=char(((yr % 1000) / 100)+48);
				output[8]=char(((yr % 100) / 10)+48);
				output[9]=char((yr % 10)+48);
				output[10]=0;
			}
			break;
		case FORMAT_BIGENDIAN:
			if (slformat==FORMAT_SHORT)
				offset=0;
			else
				offset=2;
			if (slformat==FORMAT_SHORT)
			{
				yr=t.year-YEAR0;
				if (yr<10)
					output[0]=48;
				else
					output[0]=char((yr / 10)+48);
				output[1]=char((yr % 10)+48);
				output[2]=divider;
			}
			else
			{
				yr=t.year;
				output[0]=char((yr / 1000)+48);
				output[1]=char(((yr % 1000) / 100)+48);
				output[2]=char(((yr % 100) / 10)+48);
				output[3]=char((yr % 10)+48);
				output[4]=divider;
			}
			if (t.mon<10)
				output[3+offset]=48;
			else
				output[3+offset]=char((t.mon / 10)+48);
			output[4+offset]=char((t.mon % 10)+48);
			output[5+offset]=divider;
			if (t.date<10)
				output[6+offset]=48;
			else
				output[6+offset]=char((t.date / 10)+48);
			output[7+offset]=char((t.date % 10)+48);
			output[8+offset]=0;
			break;
		case FORMAT_MIDDLEENDIAN:
			if (t.mon<10)
				output[0]=48;
			else
				output[0]=char((t.mon / 10)+48);
			output[1]=char((t.mon % 10)+48);
			output[2]=divider;
			if (t.date<10)
				output[3]=48;
			else
				output[3]=char((t.date / 10)+48);
			output[4]=char((t.date % 10)+48);
			output[5]=divider;
			if (slformat==FORMAT_SHORT)
			{
				yr=t.year-YEAR0;
				if (yr<10)
					output[6]=48;
				else
					output[6]=char((yr / 10)+48);
				output[7]=char((yr % 10)+48);
				output[8]=0;
			}
			else
			{
				yr=t.year;
				output[6]=char((yr / 1000)+48);
				output[7]=char(((yr % 1000) / 100)+48);
				output[8]=char(((yr % 100) / 10)+48);
				output[9]=char((yr % 10)+48);
				output[10]=0;
			}
			break;
	}
	return (char*)&output;
}

char *DS3231::getDOWStr(uint8_t format)
{
	char *output = "xxxxxxxxxx";
	char *daysLong[]  = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
	char *daysShort[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
	Time t;
	t=getTime();
	if (format == FORMAT_SHORT)
		output = daysShort[t.dow-1];
	else
		output = daysLong[t.dow-1];
	return output;
}

char *DS3231::getMonthStr(uint8_t format)
{
	char *output= "xxxxxxxxx";
	char *monthLong[]  = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	char *monthShort[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	Time t;
	t=getTime();
	if (format == FORMAT_SHORT)
		output = monthShort[t.mon-1];
	else
		output = monthLong[t.mon-1];
	return output;
}

unsigned long DS3231::getUnixTime() {
	return getUnixTime(getTime());
}

unsigned long DS3231::getUnixTime(Time t)
{
	//unsigned long s = 0; // Seconds elapsed since epoch year midnight
	unsigned int days = 0;

	// if the current year is a leap one -> add one day (86400 sec)
	if (!(t.year % 4) && (t.mon > 2)) 
	{
		//s += 86400;
		//days++;
		days = 1;
	}

	// dec the current month (find how many months have passed from the current year)
	t.mon--;

	// sum the days from January to the current month of current year
	while (t.mon)
	{
		t.mon--; // dec the month
		//s += calender[t.mon] * 86400;  // add the number of days from a month * 86400 sec
		days += calender[t.mon];
	}

	// Next, add to s variable: (the number of days from each year (even leap years)) * 86400 sec,
	// the number of days from the current month
	// the each hour & minute & second from the current day
	/*s += ((((t.year - YEAR0) * 365) + ((t.year - YEAR0) / 4))*(unsigned long)86400) + (t.date - 1)*(unsigned long)86400 +
		(t.hour*(unsigned long)3600) + (t.min*(unsigned long)60) + (unsigned long)t.sec;*/
	days += ((t.year - YEAR0) * 365) + ((t.year - YEAR0) / 4) + (t.date - 1); // excluding today i.e. -1

	//return s;
	return ( (((days * 24L) + t.hour) * 60L + t.min) * 60L + t.sec ); // Days --> Hours --> Minutes --> Seconds
}

void DS3231::enable32KHz(bool enable)
{
  uint8_t _reg = _readRegister(REG_STATUS);
  _reg &= ~(1 << 3);
  _reg |= (enable << 3);
  _writeRegister(REG_STATUS, _reg);
}

void DS3231::setOutput(MODES_t mode)
{
  uint8_t _reg = _readRegister(REG_CON); 
  //_reg &= ~((1 << A2IE) | (1 << A1IE)); 
  //_reg &= ~(0x1F);
  
  if (mode == SQWAVE)
  {
	  /*_reg |= (1 << INTCN);
	  _reg &= ~((1 << A2IE) | (1 << A1IE));*/

	  //_reg = (_reg & 0xE3); // ?_reg & 11100011?
	  //_reg &= ~(1 << INTCN);
	  _reg &= ~((1 << INTCN) | (1 << A2IE) | (1 << A1IE));
  }
  else
  {
	  //_reg &= ~(1 << INTCN); 
	  _reg |= _BV(INTCN);
	  //_reg &= ~((1 << A2IE) | (1 << A1IE));

	  if (mode == ALARM1)
	  {
		  _reg |= (1 << A1IE); 
		  _reg &= ~(1 << A2IE);
	  }
	  else if (mode == ALARM2)
	  {
		  _reg &= ~(1 << A1IE);
		  _reg |= (1 << A2IE); 
	  }
	  else
	  {
		  _reg |= (1 << A1IE);
		  _reg |= (1 << A2IE); 
	  }

	  //if (mode & ALARM1)
	  //{
		 // _reg |= (1 << A1IE); Serial.println(F("Alarm 1 Interrupt Enable"));
		 // //_reg &= ~(1 << A2IE);
	  //}
	  //if (mode & ALARM2)
	  //{
		 // //_reg &= ~(1 << A1IE);
		 // _reg |= (1 << A2IE); Serial.println(F("Alarm 2 Interrupt Enable"));
	  //}
  }
  _writeRegister(REG_CON, _reg); 
}

void DS3231::setSQWRate(SQWAVE_FREQS_t rate)
{
  uint8_t _reg = _readRegister(REG_CON);
  _reg &= ~((1 << RS2) | (1 << RS1)); 
  
  _reg |= (rate << 3);
  _writeRegister(REG_CON, _reg); 
}

float DS3231::getTemperature()
{
	uint8_t _msb = _readRegister(REG_TEMPM);
	uint8_t _lsb = _readRegister(REG_TEMPL);
	return (float)_msb + ((_lsb >> 6) * 0.25f);
}

/* Private */

void	DS3231::_sendStart(byte addr)
{
	pinMode(_sda_pin, OUTPUT);
	digitalWrite(_sda_pin, HIGH);
	digitalWrite(_scl_pin, HIGH);
	digitalWrite(_sda_pin, LOW);
	digitalWrite(_scl_pin, LOW);
	shiftOut(_sda_pin, _scl_pin, MSBFIRST, addr);
}

void	DS3231::_sendStop()
{
	pinMode(_sda_pin, OUTPUT);
	digitalWrite(_sda_pin, LOW);
	digitalWrite(_scl_pin, HIGH);
	digitalWrite(_sda_pin, HIGH);
	pinMode(_sda_pin, INPUT);
}

void	DS3231::_sendNack()
{
	pinMode(_sda_pin, OUTPUT);
	digitalWrite(_scl_pin, LOW);
	digitalWrite(_sda_pin, HIGH);
	digitalWrite(_scl_pin, HIGH);
	digitalWrite(_scl_pin, LOW);
	pinMode(_sda_pin, INPUT);
}

void	DS3231::_sendAck()
{
	pinMode(_sda_pin, OUTPUT);
	digitalWrite(_scl_pin, LOW);
	digitalWrite(_sda_pin, LOW);
	digitalWrite(_scl_pin, HIGH);
	digitalWrite(_scl_pin, LOW);
	pinMode(_sda_pin, INPUT);
}

void	DS3231::_waitForAck()
{
	pinMode(_sda_pin, INPUT);
	digitalWrite(_scl_pin, HIGH);
	while (digitalRead(_sda_pin)==HIGH) {}
	digitalWrite(_scl_pin, LOW);
}

uint8_t DS3231::_readByte()
{
	pinMode(_sda_pin, INPUT);

	uint8_t value = 0;
	uint8_t currentBit = 0;

	for (int i = 0; i < 8; ++i)
	{
		digitalWrite(_scl_pin, HIGH);
		currentBit = digitalRead(_sda_pin);
		value |= (currentBit << 7-i);
		delayMicroseconds(1);
		digitalWrite(_scl_pin, LOW);
	}
	return value;
}

void DS3231::_writeByte(uint8_t value)
{
	pinMode(_sda_pin, OUTPUT);
	shiftOut(_sda_pin, _scl_pin, MSBFIRST, value);
}

uint8_t	DS3231::_decode(uint8_t value)
{
	uint8_t decoded = value & 127;
	decoded = (decoded & 15) + 10 * ((decoded & (15 << 4)) >> 4);
	return decoded;
}

uint8_t DS3231::_decodeH(uint8_t value)
{
  if (value & 128)
    value = (value & 15) + (12 * ((value & 32) >> 5));
  else
    value = (value & 15) + (10 * ((value & 48) >> 4));
  return value;
}

uint8_t	DS3231::_decodeY(uint8_t value)
{
	uint8_t decoded = (value & 15) + 10 * ((value & (15 << 4)) >> 4);
	return decoded;
}

uint8_t DS3231::_encode(uint8_t value)
{
	uint8_t encoded = ((value / 10) << 4) + (value % 10);
	return encoded;
}

