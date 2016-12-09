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
 * [Prerequisites](https://github.com/nasys/badgerboard/tree/readme_test_branch#prerequisites)
 	* [Mac OS X terminal installation](https://github.com/nasys/badgerboard/tree/readme_test_branch#mac-os-x)
	* [Downloads](https://github.com/nasys/badgerboard/tree/readme_test_branch#downloads)
 	* [Windows cmd installation](https://github.com/nasys/badgerboard/tree/readme_test_branch#windows)
 	* [manual installation](https://github.com/nasys/badgerboard/tree/readme_test_branch#manual)
 * [Badgerboard Setup](https://github.com/nasys/badgerboard/tree/readme_test_branch#badgerboard-setup)
	* [Test out your Badgerboard](https://github.com/nasys/badgerboard/tree/readme_test_branch#test-out-your-badgerboard) 
 	* [LoRa sensor setup](https://github.com/nasys/badgerboard/tree/readme_test_branch#lora-sensor-setup)
 * [Known issues](https://github.com/nasys/badgerboard/tree/readme_test_branch#known-issues)
 * [Used libraries](https://github.com/nasys/badgerboard/tree/readme_test_branch#used-libraries)
 * [Licences](https://github.com/nasys/badgerboard/tree/readme_test_branch#licences)

### Prerequisites

##### Downloads


* [**Arduino**](https://www.arduino.cc/en/Main/Software)
* [**Git**](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)

##### Mac OS X

* Open "**Finder**" (Press &#8984;+ Space ) 
* type in " terminal " and open it
* type "cd Documents/Arduino/libraries"
* type "git clone https://github.com/nasys/badgerboard"
* Now when you open up your Arduino _Badgerboard_ library should appear under **Sketch -> Include library**

NB! with this installation you can use"git pull" to get the latest version of the library

#### Windows

* siia peab midagi veel panema, aga pole olnud võimalust windowsi peal seda proovida


#### manual
* Download this library as **.zip** file
* Open Arduino and include it under **_sketch -> include library -> add .zip library_**
* Open **_files -> examples -> badger -> LoRa_temp_hum_**
### **Badgerboard setup**
#### Test out your Badgerboard

* connect Badgerboard to your PC 
* Open Arduino
* Open Blink example ( **Files > Examples > Basic > Blink** )
* Change the time(number in milliseconds) inside the brackets after "**delay**" to change the speed of blinking
* To upload click on the right pointing arrow on the top-left corner of your Arduino window

In case all of this works then you have successfully uploaded your first program to Badgerboard


#### LoRa sensor setup

* Open "**File -> Examples -> Badger -> LoRa_temp_hum**"
* Replace examples **_devEUI_**[that came with badgerboard(16 hex char)], **_appEUI_**(16 hex char) and **_appKey_**(32 hex char) with the ones that Your LoRaWAN™ solution provider has assigned.  
* Upload Your code to Badgerboard

#### Known issues

* Works only with Arduino version 1.6.10 and later

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

* All of the source files are distributed under the beerware license except where specified otherwise (i.e. external libraries)
 
#### Contacts
In case you found the instructions incomplete or have any questions regarding Badgerboard library
You can :
Open new thread under [issues](https://github.com/nasys/badgerboard/issues)
:e-mail: [contact us](mailto:badgerboard@nasys.no)

