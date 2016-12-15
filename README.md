				 ___               _                      _                             _
				(  _`\            ( )                    ( )                           ( )
				| (_) )   _ _    _| |   __     __   _ __ | |_      _      _ _  _ __   _| |
				|  _ <' /'_` ) /'_` | /'_ `\ /'__`\( '__)| '_`\  /'_`\  /'_` )( '__)/'_` |
				| (_) )( (_| |( (_| |( (_) |(  ___/| |   | |_) )( (_) )( (_| || |  ( (_| |
			    (____/'`\__,_)`\__,_)`\__  |`\____)(_)   (_,__/'`\___/'`\__,_)(_)  `\__,_)
									 ( )_) |
								 	  \___/'
============================

### **Table of Contents**
 * [Prerequisites](#prerequisites)
 	* [Mac OS X terminal installation](#mac-os-x)
	* [Downloads](#downloads)
 	* [Windows cmd installation](#windows-cmd-installation)
 	* [manual installation](#manual-installation)
 * [Badgerboard Setup](#badgerboard-setup)
	* [Test out your Badgerboard](#test-out-your-badgerboard) 
 	* [LoRa sensor setup](#lora-sensor-setup)
 * [Debugging](#debugging)
 * [Known issues](#known-issues)
 * [Version log](#version-log)
 * [Used libraries](#used-libraries)
 * [Licences](#licences)

### Prerequisites

##### Downloads


* [**Arduino**](https://www.arduino.cc/en/Main/Software)
* [**Git**](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)

##### Mac OS X

* Open "**Finder**" (Press &#8984;+ Space ) 
* type in "terminal" and open it
* type "cd Documents/Arduino/libraries"
* type "git clone https://github.com/nasys/badgerboard"
* Now when you open up your Arduino and _Badgerboard_ library should appear under **Sketch -> Include library**

##### Windows

Coming soon

#### manual
* Download this library as **.zip** file
* Open Arduino and include it under **Sketch -> Include library -> add .zip library**
* Open **Files -> Examples -> badger -> LoRa_temp_hum**

### **Badgerboard setup**
#### Test out your Badgerboard
```diff
- DO NOT POWER ON WITHOUT ANTENNA! 

```


* connect Badgerboard to your PC/Mac 
* Open Arduino
* Open Blink example **Files -> Examples -> Basic -> Blink**
* Make sure you have chosen correct board  **Tools -> Board -> LilyPad Arduino USB** and correct port **Tools -> Port** 
* Change the time(number in milliseconds) inside the brackets after "**delay**" to change the speed of blinking
* To upload click on the right pointing arrow on the top-left corner of your Arduino window

In case all of this works then you have successfully uploaded your first program to Badgerboard


#### LoRa sensor setup

* Open **File -> Examples -> Badgerboard -> LoRa_temp_hum**
* Replace examples **devEUI**(16 hex char), **appEUI**(16 hex char) and **appKey**(32 hex char) with the ones that Your LoRaWAN™ solution provider has assigned.([example 1](#example-1)) 
* Upload Your code to Badgerboard

##### example 1

	
	In Arduino code your devEUI, appEUI and appKey are represented by char array and should look similar to 
	this example
	
	const uint8_t devEUI[8] = {
	0x11, 0x11, 0x22, 0x22,
	0x33, 0x33, 0x44, 0x44
	};

	const uint8_t appKey[16] = {
	0x11, 0x11, 0x11, 0x11,
	0x11, 0x11, 0x22, 0x22,
	0x22, 0x22, 0x22, 0x22,
	0x33, 0x33, 0x33, 0x33
	};

	const uint8_t appEUI[8] = {
	0x11, 0x22, 0x33, 0x44,
	0x55, 0x66, 0x77, 0x88
	};

### Debugging

To see Debugging messages open Serial Monitor **Tools -> Serial Monitor**
#### Known issues

* Take a look under [troubleshooting](https://github.com/nasys/badgerboard/wiki/Troubleshooting) page
* Works only with Arduino version 1.6.10 and later


### Version log

#### Version 0.0.2
:calendar:06.12.2016

	* Changed spreading factor changing to automatic
	* Added liquid detection example 

#### Version 0.0.1
:calendar:01.11.2016 

	Initial version of Badgerboard library

 

	
#### Used libraries
* [**FaBoHumidity_HTS221**](https://github.com/FaBoPlatform/FaBoHumidity-HTS221-Library)
* [**LowPower**](https://github.com/rocketscream/Low-Power)
* [**Sodaq_RN2483**](https://github.com/SodaqMoja/Sodaq_RN2483)



### **Licences**

All of the source files are distributed under the beerware license except where specified otherwise (i.e. external libraries)
 
#### Contacts
In case you found the instructions incomplete or have any questions regarding Badgerboard library
You can :
* Open new thread under [issues](https://github.com/nasys/badgerboard/issues)
* :e-mail: [contact us](mailto:badgerboard@nasys.no)

