#BADGERBOARD

					BADGERBOARD 


 Download this library as .zip 
 Open Arduino and include it under sketch -> include library -> add .zip library
 Open files -> examples -> badger -> LoRa_temp_hum

LoRa sensor setup :
 
 Go to https://developer.noranet.ee and make yourself a user
 Go to nodes on your left sidebar and make an Application and new node
 Name the sensor 
 AppEUI is the same you made under Applications
 devEUI is assigned with the device
 AppKey must be 32 hexadecimal units long and is chosen by you


Replace example devEUI appEUI and appKey with the ones you made in https://developer.noranet.ee. Open nodes and click on your sensor’s EUI, appKey and Application to open new window where are shown char arrays and replace the char array in your code with the one you get from https://developer.noranet.ee.  





