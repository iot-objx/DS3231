# DS3231
The DS3231 is a low-cost, extremely accurate (+/- 2ppm) I2C
real-time clock (RTC) with an integrated temperature-compensated crystal oscillator (TCXO) and crystal. This library is an extension of [this](http://rinkydinkelectronics.com/library.php?id=73) well-written library. The library lacked Alarm functionalities and some easy-to-use wrappers. Therefore, this is another flavor of that existent library. Refer to the datasheet for detailed explanation of the [DS3231](https://datasheets.maximintegrated.com/en/ds/DS3231.pdf) chip. 

![DS3231 Module](.\Documentation\DS3231.png)

***
## Functions
Firstly, create a `DS3231` object. The instance has the following format `DS3231(SDA, SCL)` where `SDA` and `SCL` connects to the respective pins of the RTC module. 

### Time and Date 
In order to set/get the date and time info, you have access to the following functions/methods. 

**Set Functions/Methods:**
* **`setTime(sec, min, hour)`**: is used to set the time using the seconds, minutes and hours input parameters. 

* **`setDate(date, mon, year, epochYear)`**: is used to set the date using date, month and year arguments. The fourth input parameter is the epoch year. If not supplied, the library assumes it to be 1970. 

* **`setDateTime(tm, epochYear)`**: is the combo to set both date and time using the `Time` structure. The epoch year is assumed to be 1970 if not supplied. 

* **`setDateTime(sec, min, hour, date, mon, year, epochYear)`**: sets the date and time with individual paramters explicitly provided. Again, epoch year argument is optional.

* **`setDOW()`**: sets day of the week intelligently. No input paramter is required. The function calculates the day from the internally available `Time` structure. 

* **`setDOW(dow)`**: sets day of the week manually where Monday is the first day of the week. Therefore, dow can be any numbers 1 to 7 inclusive. You can also use the library defines: `MONDAY`, `TUESDAY`, `WEDNESDAY`, `THURSDAY`, `FRIDAY`, `SATURDAY`, and `SUNDAY`. If `dow` is not provided, the method sets day of the week intelligently i.e. it calculates the day from the internally available current `Time` structure. 

* **`makeDateTime(epochSec)`**: Returns a `Time` structure generated from the epoch seconds or Unix time. This is particularly useful when the source of time synchronization is GPS. 

**Get Functions/Methods:**
* **`getTime()`**: returns a `Time` structure that has `hour`, `min`, `sec`, `date`, `mon`, `year`, and `dow` fields to hold the corresponding time and date data. 

* **`getTimeStr(format)`**: returns a string containing the current time in the specified format; `FORMAT_SHORT`: no seconds field or `FORMAT_LONG` (default): with seconds field.

* **`getDateStr(formatYear, formatEndian, divider)`**: returns a string containing the current date in the specified format. The `formatYear` argument can be `FORMAT_SHORT` for years in two digits *yy* or `FORMAT_LONG` (default) for years in four digits *yyyy*. For the date format or endianness, use `FORMAT_LITTLEENDIAN` (default) for *dd.mm.yyyy*, `FORMAT_BIGENDIAN` for *yyyy.mm.dd* and `FORMAT_MIDDLEENDIAN`	for *mm.dd.yyyy*. Lastly, the single character divider of the date components are set using the `divider` paramter. The default is dot. 

* **`getDOWStr(format)`**: returns a string containing the current day of the week (in English) in the specified format; `FORMAT_SHORT`: abbreviated 3 letter day-of-the-week or `FORMAT_LONG` (default): full form.

* **`getMonthStr(format)`**: returns a string containing the current month of the year (in English) in the specified format; `FORMAT_SHORT`: abbreviated 3 letter month or `FORMAT_LONG` (default): full form.

* **`getUnixTime(Time t);`**: returns the Unix equivalent of the supplied `Time` structure. If the time structure is not provided, it retuns the Unix equivalent of the current time fetched from DS3231. 


***
### Alarms
By default, the DS3231 chip has two hardware alarms. These alarms can also be used as interrupt sources. Nonetheless, one can always implement infinite number of alarms (in theory) by polling. 

* **`setAlarm(alarmType, sec, min, hour, daydate)`**: set an alarm time by setting the alarm registers only. To cause the
**INT/SQW** pin to be asserted on alarm match, use `setOutput()` (see the Output section below). This method can set either Alarm 1 or Alarm 2, depending on the alarmType. The available alarm types are: 

    * `ALM1_EVERY_SECOND`:	Alarm once per second
	* `ALM1_MATCH_SECONDS`:	Alarm when seconds match
	* `ALM1_MATCH_MINUTES`: Alarm when minutes and seconds match
	* `ALM1_MATCH_HOURS`: Alarm when hours, minutes, and seconds match
	* `ALM1_MATCH_DATE`: Alarm when date, hours, minutes, and seconds match
	* `ALM1_MATCH_DAY`: Alarm when day, hours, minutes, and seconds match
	* `ALM2_EVERY_MINUTE`: Alarm once per minute (00 seconds of every minute)
	* `ALM2_MATCH_MINUTES`: Alarm when minutes match
	* `ALM2_MATCH_HOURS`: Alarm when hours and minutes match
	* `ALM2_MATCH_DATE`: Alarm when date, hours, and minutes match
	* `ALM2_MATCH_DAY`: Alarm when day, hours, and minutes match

    When setting Alarm 2, the seconds value must be supplied but is ignored, recommend using zero. This is because Alarm 2 has no seconds register.

* **`checkAlarm()`**: returns the alarm number (if any) and resets the alarm flag bit.

***
### Output
The following functions set the behavior of the **INT/SQW** pin of the DS3231 module. The output type can wither be alarm or square wave but not both. 

* **`enable32KHz(enable)`**: enables/disables the Square Wave output on 32kHz pin with `true/false` arguments. 
* **`setOutput(mode)`**: sets the output behavior/signal of the **INT/SQW** pin. The possible selections modes are:
    * `SQWAVE`: Square wave output
	* `ALARM1`: Alarm 1 Interrupt
	* `ALARM2`: Alarm 2 Interrupt
	* `ALARMx`: Alarm 1 & 2 Interrupt
* **`setSQWRate(rate)`**: sets the Square Wave frequency or output rate. The accepted rates are:
    * `SQWAVE_1_HZ`: 1 Hertz
	* `SQWAVE_1024_HZ`: 1.024 kilo Hertz
	* `SQWAVE_4096_HZ`: 4.096 kilo Hertz
	* `SQWAVE_8192_HZ`: 8.192 kilo Hertz

    The set rate only takes effect if the Square Wave output was enabled using `setOutput` method. 

***
### Temperature
* **`getTemperature()`**: returns the temperature in the vicinity of the DS3231 chip with a resolution of 0.25 °C. The temperature gets updated once in every 64 seconds. This is a hardawre/chipset limitation. 

***
### Note:
The PDF documentation is outdated and will be removed in the future updates. Refer to the description above.