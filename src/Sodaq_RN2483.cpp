/*
* Copyright (c) 2015 SODAQ. All rights reserved.
*
* This file is part of Sodaq_RN2483.
*
* Sodaq_RN2483 is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation, either version 3 of
* the License, or(at your option) any later version.
*
* Sodaq_RN2483 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with Sodaq_RN2483.  If not, see
* <http://www.gnu.org/licenses/>.
*/

#include "Sodaq_RN2483.h"
#include "StringLiterals.h"
#include "Utils.h"
#include "badger.h"

#define ENABLE_SLEEP

/*
#ifdef DEBUG
#define debugPrintLn(...) { if (this->diagStream) this->diagStream->println(__VA_ARGS__); }
#define debugPrint(...) { if (this->diagStream) this->diagStream->print(__VA_ARGS__); }
#warning "Debug mode is ON"
#else
#define debugPrintLn(...)
#define debugPrint(...)
#endif */

// Structure for mapping error response strings and error codes.
typedef struct StringEnumPair
{
    const char* stringValue;
    uint8_t enumValue;
} StringEnumPair_t;

Sodaq_RN2483 LoRaBee;
bool s_sleep_enabled = false;

// Creates a new Sodaq_RN2483 instance.
Sodaq_RN2483::Sodaq_RN2483() :
        loraStream(0),
        diagStream(0),
        inputBufferSize(DEFAULT_INPUT_BUFFER_SIZE),
        receivedPayloadBufferSize(DEFAULT_RECEIVED_PAYLOAD_BUFFER_SIZE),
        packetReceived(false)
{
#ifdef USE_DYNAMIC_BUFFER
    this->isBufferInitialized = false;
#endif
}

// Takes care of the init tasks common to both initOTA() and initABP.
void Sodaq_RN2483::init(SerialType& stream)
{
    //debugPrintLn("[init]");

    this->loraStream = &stream;
	
#ifdef USE_DYNAMIC_BUFFER
    // make sure the buffers are only initialized once
    if (!isBufferInitialized) {
        this->inputBuffer = static_cast<char*>(malloc(this->inputBufferSize));
        this->receivedPayloadBuffer = static_cast<char*>(malloc(this->receivedPayloadBufferSize));

        isBufferInitialized = true;
    }
#endif

    // make sure the module's state is slsynced and woken up
//    sleep();
//	delay(10);
//    sodaq_wdt_safe_delay(15);
    wakeUp();
}

// Initializes the device and connects to the network using Over-The-Air Activation.
// Returns true on successful connection.
bool Sodaq_RN2483::initOTA(SerialType& stream, const uint8_t devEUI[8], const uint8_t appEUI[8], const uint8_t appKey[16], bool adr)
{
    //debugPrintLn("[initOTA]");

    init(stream);

    return resetDevice() &&
        setMacParam(STR_DEV_EUI, devEUI, 8) &&
        setMacParam(STR_APP_EUI, appEUI, 8) &&
        setMacParam(STR_APP_KEY, appKey, 16) &&
        setMacParam(STR_ADR, BOOL_TO_ONOFF(adr));
}

bool Sodaq_RN2483::sendOTA()
{
	wakeUp();
	return joinNetwork(STR_OTAA);
}

// Initializes the device and connects to the network using Activation By Personalization.
// Returns true on successful connection.
bool Sodaq_RN2483::initABP(SerialType& stream, const uint8_t devAddr[4], const uint8_t appSKey[16], const uint8_t nwkSKey[16], bool adr)
{
    //debugPrintLn("[initABP]");

    init(stream);

    return resetDevice() &&
        setMacParam(STR_DEV_ADDR, devAddr, 4) &&
        setMacParam(STR_APP_SESSION_KEY, appSKey, 16) &&
        setMacParam(STR_NETWORK_SESSION_KEY, nwkSKey, 16) &&
        setMacParam(STR_ADR, BOOL_TO_ONOFF(adr)) &&
        joinNetwork(STR_ABP);
}

// Sends the given payload without acknowledgement.
// Returns 0 (NoError) when transmission is successful or one of the MacTransmitErrorCodes otherwise.
uint8_t Sodaq_RN2483::send(uint8_t port, const uint8_t* payload, uint8_t size)
{
    //debugPrintLn("[send]");

    return macTransmit(STR_UNCONFIRMED, port, payload, size);
}

// Sends the given payload with acknowledgement.
// Returns 0 (NoError) when transmission is successful or one of the MacTransmitErrorCodes otherwise.
uint8_t Sodaq_RN2483::sendReqAck(uint8_t port, const uint8_t* payload,
        uint8_t size, uint8_t maxRetries)
{
    //debugPrintLn("[sendReqAck]");
    if (!setMacParam(STR_RETRIES, maxRetries)) {
        // not a fatal error -just show a debug message
        //debugPrintLn("[sendReqAck] Non-fatal error: setting number of retries failed.");
		wakeUp();
//		Serial.println("extrawakeupeakeup");
		setMacParam(STR_RETRIES, maxRetries);
    }
    return macTransmit(STR_CONFIRMED, port, payload, size);
}

// Copies the latest received packet (optionally starting from the "payloadStartPosition"
// position of the payload) into the given "buffer", up to "size" number of bytes.
// Returns the number of bytes written or 0 if no packet is received since last transmission.
uint16_t Sodaq_RN2483::receive(uint8_t* buffer, uint16_t size,
        uint16_t payloadStartPosition)
{
    //debugPrintLn("[receive]");

    if (!this->packetReceived) {
        //debugPrintLn("[receive]: There is no packet received!");
        return 0;
    }

    uint16_t inputIndex = payloadStartPosition * 2; // payloadStartPosition is in bytes, not hex char pairs
    uint16_t outputIndex = 0;

    // check that the asked starting position is within bounds
    if (inputIndex >= this->receivedPayloadBufferSize) {
        //debugPrintLn("[receive]: Out of bounds start position!");
        return 0;
    }

    // stop at the first string termination char, or if output buffer is over, or if payload buffer is over
    while (outputIndex < size
            && inputIndex + 1 < this->receivedPayloadBufferSize
            && this->receivedPayloadBuffer[inputIndex] != 0
            && this->receivedPayloadBuffer[inputIndex + 1] != 0) {
        buffer[outputIndex] = HEX_PAIR_TO_BYTE(
                this->receivedPayloadBuffer[inputIndex],
                this->receivedPayloadBuffer[inputIndex + 1]);

        inputIndex += 2;
        outputIndex++;
    }

    // Note: if the payload has an odd length, the last char is discarded

    buffer[outputIndex] = 0; // terminate the string

    //debugPrintLn("[receive]: Done");
    return outputIndex;
}

// Gets the preprogrammed EUI node address from the module.
// Returns the number of bytes written or 0 in case of error.
uint8_t Sodaq_RN2483::getHWEUI(uint8_t* buffer, uint8_t size)
{
	this->wakeUpIfSleeping();
    //debugPrintLn("[getHWEUI]");

    this->loraStream->print(STR_CMD_GET_HWEUI);
    this->loraStream->print(CRLF);

    // TODO move to general "read hex" method
    uint8_t inputIndex = 0;
    uint8_t outputIndex = 0;

    unsigned long start = millis();
    while (millis() < start + DEFAULT_TIMEOUT) {
        sodaq_wdt_reset();
        //debugPrint(".");

        if (readLn() > 0) {
            //debugPrintLn(this->inputBuffer);
            while (outputIndex < size
                && inputIndex + 1U < this->inputBufferSize
                && this->inputBuffer[inputIndex] != 0
                && this->inputBuffer[inputIndex + 1] != 0) {
                buffer[outputIndex] = HEX_PAIR_TO_BYTE(
                    this->inputBuffer[inputIndex],
                    this->inputBuffer[inputIndex + 1]);
                inputIndex += 2;
                outputIndex++;
            }

            //debugPrint("[getHWEUI] count: "); //debugPrintLn(outputIndex);
            return outputIndex;
        }
    }

    //debugPrint("[getHWEUI] Timed out without a response!");
    return 0;
}
#ifdef ENABLE_SLEEP

void Sodaq_RN2483::wakeUpIfSleeping()
{
	if(this->sleep_enabled)
		this->wakeUp();
}


void Sodaq_RN2483::wakeUp()
{
    //debugPrintLn("[wakeUp]");

    // "emulate" break condition
    this->loraStream->flush();
    this->loraStream->end();
    this->loraStream->begin(300); // *2
    this->loraStream->write((uint8_t)0x00);
    this->loraStream->flush();
    this->loraStream->end();

    // set baudrate
    this->loraStream->begin(getDefaultBaudRate());
    this->loraStream->write((uint8_t)0x55);
    this->loraStream->flush();
	this->sleep_enabled = false;
	
}

void Sodaq_RN2483::sleep()
{
	if(this->sleep_enabled == true)
	{
		//debugPrintLn("[sleeping_already]");
		return;
	}
    //debugPrintLn("[sleep]");

    this->loraStream->print(STR_CMD_SLEEP);
    this->loraStream->print(CRLF);
	this->sleep_enabled = true;
	// Very important to allow serial communication to finish before putting to sleep
	delay(20);	
}
#endif

// Reads a line from the device stream into the "buffer" starting at the "start" position of the buffer.
// Returns the number of bytes read.
uint16_t Sodaq_RN2483::readLn(char* buffer, uint16_t size, uint16_t start)
{
	this->wakeUpIfSleeping();
    int len = this->loraStream->readBytesUntil('\n', buffer + start, size);
    if (len > 0) {
        this->inputBuffer[start + len - 1] = 0; // bytes until \n always end with \r, so get rid of it (-1)
    }

    return len;
}

// Waits for the given string. Returns true if the string is received before a timeout.
// Returns false if a timeout occurs or if another string is received.
bool Sodaq_RN2483::expectString(const char* str, uint16_t timeout)
{
    //debugPrint("[expectString] expecting "); //debugPrint(str);

    unsigned long start = millis();
    while (millis() < start + timeout) {
        sodaq_wdt_reset();
        //debugPrint(".");
//		Serial.println(".");
        if (readLn() > 0) {
            //debugPrint("("); //debugPrint(this->inputBuffer); //debugPrint(")");
//			Serial.println("exStr ln>0");
            if (strstr(this->inputBuffer, str) != NULL) {
                //debugPrintLn(" found a match!");
//				Serial.println("exStr matching");
                return true;
            }
//			Serial.println("exStr not eq");
            return false;
        }
//		Serial.println("_");
    }
//			Serial.print("exStr timeout");
//			Serial.println(timeout);
    return false;
}

bool Sodaq_RN2483::expectOK()
{
    return expectString(STR_RESULT_OK);
}

// Sends a reset command to the module and waits for the success response (or timeout).
// Returns true on success.
bool Sodaq_RN2483::resetDevice()
{
	this->wakeUp();
//	this->wakeUpIfSleeping();
    //debugPrintLn("[resetDevice]");

    this->loraStream->print(STR_CMD_RESET);
    this->loraStream->print(CRLF);
	this->loraStream->flush();
	this->loraStream->end();
	this->loraStream->begin(57600);

    if (expectString(STR_DEVICE_TYPE_RN)) {
		this->wakeUp();
        if (strstr(this->inputBuffer, STR_DEVICE_TYPE_RN2483) != NULL) {
            //debugPrintLn("The device type is RN2483");
			return true;
        }
        else if (strstr(this->inputBuffer, STR_DEVICE_TYPE_RN2903) != NULL) {
            //debugPrintLn("The device type is RN2903");
            // TODO move into init once it is decided how to handle RN2903-specific operations
            setFsbChannels(DEFAULT_FSB);
            return true;
        }
        else {
            //debugPrintLn("Unknown device type!");
            return false;
        }
    }
	this->wakeUp();
    return false;
}

// Enables all the channels that belong to the given Frequency Sub-Band (FSB)
// and disables the rest.
// fsb is [1, 8] or 0 to enable all channels.
// Returns true if all channels were set successfully.
bool Sodaq_RN2483::setFsbChannels(uint8_t fsb)
{
	this->wakeUpIfSleeping();
    //debugPrintLn("[setFsbChannels]");

    uint8_t first125kHzChannel = fsb > 0 ? (fsb - 1) * 8 : 0;
    uint8_t last125kHzChannel = fsb > 0 ? first125kHzChannel + 7 : 71;
    uint8_t fsb500kHzChannel = fsb + 63;
    
    bool allOk = true;
    for (uint8_t i = 0; i < 72; i++) {
        this->loraStream->print(STR_CMD_SET_CHANNEL_STATUS);
        this->loraStream->print(i);
        this->loraStream->print(" ");
        this->loraStream->print(BOOL_TO_ONOFF(((i == fsb500kHzChannel) || (i >= first125kHzChannel && i <= last125kHzChannel))));
        this->loraStream->print(CRLF);

        allOk &= expectOK();
    }

    return allOk;
}

// Sends a join network command to the device and waits for the response (or timeout).
// Returns true on success.
bool Sodaq_RN2483::joinNetwork(const char* type)
{
	this->wakeUpIfSleeping();
    //debugPrintLn("[joinNetwork]");

    this->loraStream->print(STR_CMD_JOIN);
    this->loraStream->print(type);
    this->loraStream->print(CRLF);

    return expectOK() && expectString(STR_ACCEPTED, 10000);
}

// Sends the given mac command together with the given paramValue
// to the device and awaits for the response.
// Returns true on success.
// NOTE: paramName should include a trailing space
bool Sodaq_RN2483::setMacParam(const char* paramName, const uint8_t* paramValue, uint16_t size)
{
	this->wakeUpIfSleeping();
    //debugPrint("[setMacParam] "); //debugPrint(paramName); //debugPrint("= [array]");

    this->loraStream->print(STR_CMD_SET);
    this->loraStream->print(paramName);

    for (uint16_t i = 0; i < size; ++i) {
        this->loraStream->print(static_cast<char>(NIBBLE_TO_HEX_CHAR(HIGH_NIBBLE(paramValue[i]))));
        this->loraStream->print(static_cast<char>(NIBBLE_TO_HEX_CHAR(LOW_NIBBLE(paramValue[i]))));
    }

    this->loraStream->print(CRLF);

    return expectOK();
}

// Sends the given mac command together with the given paramValue
// to the device and awaits for the response.
// Returns true on success.
// NOTE: paramName should include a trailing space
bool Sodaq_RN2483::setMacParam(const char* paramName, uint8_t paramValue)
{
	this->wakeUpIfSleeping();
    //debugPrint("[setMacParam] ");
    //debugPrint(paramName);
    //debugPrint("= ");
    //debugPrintLn(paramValue);

    this->loraStream->print(STR_CMD_SET);
    this->loraStream->print(paramName);
    this->loraStream->print(paramValue);
    this->loraStream->print(CRLF);

    return expectOK();
}

// Sends the given mac command together with the given paramValue
// to the device and awaits for the response.
// Returns true on success.
// NOTE: paramName should include a trailing space
bool Sodaq_RN2483::setMacParam(const char* paramName, const char* paramValue)
{
	this->wakeUpIfSleeping();
    //debugPrint("[setMacParam] ");
    //debugPrint(paramName);
    //debugPrint("= ");
    //debugPrintLn(paramValue);

    this->loraStream->print(STR_CMD_SET);
    this->loraStream->print(paramName);
    this->loraStream->print(paramValue);
    this->loraStream->print(CRLF);

    return expectOK();
}

// Returns the enum that is mapped to the given "error" message.
uint8_t Sodaq_RN2483::lookupMacTransmitError(const char* error)
{
    //debugPrint("[lookupMacTransmitError]: ");
    //debugPrintLn(error);

    if (error[0] == 0) {
        //debugPrintLn("[lookupMacTransmitError]: The string is empty!");
        return NoResponse;
    }

    StringEnumPair_t errorTable[] =
    {
        { STR_RESULT_INVALID_PARAM, InternalError },
        { STR_RESULT_NOT_JOINED, NotConnected },
        { STR_RESULT_NO_FREE_CHANNEL, Busy },
        { STR_RESULT_SILENT, Busy },
        { STR_RESULT_FRAME_COUNTER_ERROR, NetworkFatalError },
        { STR_RESULT_BUSY, Busy },
        { STR_RESULT_MAC_PAUSED, InternalError },
        { STR_RESULT_INVALID_DATA_LEN, PayloadSizeError },
        { STR_RESULT_MAC_ERROR, NoAcknowledgment },
    };

    for (StringEnumPair_t * p = errorTable; p->stringValue != NULL; ++p) {
        if (strcmp(p->stringValue, error) == 0) {
            //debugPrint("[lookupMacTransmitError]: found ");
            //debugPrintLn(p->enumValue);
			
Serial.print("LoRa warning: ");
Serial.println(error);
            return p->enumValue;
        }
    }

    //debugPrintLn("[lookupMacTransmitError]: not found!");
    return NoResponse;
}

uint8_t Sodaq_RN2483::macTransmit(const char* type, uint8_t port, const uint8_t* payload, uint8_t size)
{
	this->wakeUpIfSleeping();
    //debugPrintLn("[macTransmit]");
//	sleep_wdt_approx(15);
    this->loraStream->print(STR_CMD_MAC_TX);
    this->loraStream->print(type);
    this->loraStream->print(port);
    this->loraStream->print(" ");

    for (int i = 0; i < size; ++i) {
        this->loraStream->print(static_cast<char>(NIBBLE_TO_HEX_CHAR(HIGH_NIBBLE(payload[i]))));
        this->loraStream->print(static_cast<char>(NIBBLE_TO_HEX_CHAR(LOW_NIBBLE(payload[i]))));
    }

    this->loraStream->print(CRLF);

    if (!expectOK()) {
//		Serial.println("EEEEEEeeee!exoectOK");
        return lookupMacTransmitError(this->inputBuffer); // inputBuffer still has the last line read
    }
//	Serial.println("noExpOk");
    this->packetReceived = false; // prepare for receiving a new packet

    //debugPrint("Waiting for server response");
    unsigned long timeout = millis() + RECEIVE_TIMEOUT; // hard timeout
    while (millis() < timeout) {
        sodaq_wdt_reset();
		onMacRX();
        //debugPrint(".");
        if (readLn() > 0) {
            if (strstr(this->inputBuffer, " ") != NULL) // to avoid double delimiter search
            {
                // there is a splittable line -only case known is mac_rx
                //debugPrintLn("Splittable response found");
                return onMacRX();
            } else if (strstr(this->inputBuffer, STR_RESULT_MAC_TX_OK)) {
                // done
                //debugPrintLn("Received mac_tx_ok");
//				Serial.println("max tx ok");
                return NoError;
            } else {
                // lookup the error message
                //debugPrintLn("Some other string received (error)");
//				Serial.println("wrong string");
                return lookupMacTransmitError(this->inputBuffer);
            }
        }
    }
//	Serial.println("EEEEEEEEEEE Main timeout");	
    //debugPrintLn("Timed-out waiting for a response!");
    return Timeout;
}

// Parses the input buffer and copies the received payload into the "received payload" buffer
// when a "mac rx" message has been received. It is called internally by macTransmit().
// Returns 0 (NoError) or otherwise one of the MacTransmitErrorCodes.
uint8_t Sodaq_RN2483::onMacRX()
{

    // parse inputbuffer, put payload into packet buffer
    char* token = strtok(this->inputBuffer, " ");
    // sanity check
    if (strcmp(token, STR_RESULT_MAC_RX) != 0) {
        //debugPrintLn("[onMacRX]: mac_rx keyword not found!");
        return InternalError;
    }

    // port
    token = strtok(NULL, " ");

    // payload
    token = strtok(NULL, " "); // until end of string

    uint16_t len = strlen(token) + 1; // include termination char
    memcpy(this->receivedPayloadBuffer, token, len <= this->receivedPayloadBufferSize ? len : this->receivedPayloadBufferSize);

    

    this->packetReceived = true; // enable receive() again
    return NoError;
}

#ifdef DEBUG
int freeRam()
{
    extern int __heap_start;
    extern int *__brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
#endif



bool Sodaq_RN2483::setRadioParam(const char* paramName, const char* paramValue)
{
	this->wakeUpIfSleeping();
//	this->wakeUp();
    //debugPrint("[setRadioParam] ");
    //debugPrint(paramName);
    //debugPrint("= ");
    //debugPrintLn(paramValue);

    this->loraStream->print(STR_CMD_SET);
    this->loraStream->print(paramName);
    this->loraStream->print(paramValue);
    this->loraStream->print(CRLF);

    return expectOK();
}

bool Sodaq_RN2483::setSpreadingFactor(uint8_t sf)
{
	if(sf < 7)
		sf = 7;
	if(sf > 12)
		sf = 12;
	this->loraStream->print(STR_CMD_RADIO_SET);
	
	this->loraStream->print(STR_RADIO_SF);
	this->loraStream->print(sf);
	this->loraStream->print(CRLF);

    bool ok = expectOK();
	if(ok)
	{
		Serial.print("LoRa: Spreading factor set to ");
		Serial.println(sf);
	}
	else
		Serial.println("LoRa: warning. Spreading factor not set!");
	return ok;
}

bool Sodaq_RN2483::setTXPower(int8_t power)
{
	if(power < -3)
		power = -3;
	if(power > 15)
		power = 15;
	this->loraStream->print(STR_CMD_RADIO_SET);
	this->loraStream->print(STR_RADIO_PWR);
	this->loraStream->print(power);
	
	this->loraStream->print(CRLF);

    bool ok = expectOK();
	if(ok)
	{
		Serial.print("LoRa: power set to ");
		Serial.println(power);
	}
	else
		Serial.println("LoRa: power not set!");
	return ok;
}

bool Sodaq_RN2483::setDataRate(uint8_t dr)
{
	if(dr > 7)
		dr = 7;
	this->loraStream->print(STR_CMD_SET);
	this->loraStream->print(STR_MAC_DR_SET);
	this->loraStream->print(dr);
	this->loraStream->print(CRLF);

    bool ok = expectOK();
	if(ok)
	{
		Serial.print("LoRa: datarate set to ");
		Serial.println(dr);
	}
	else
		Serial.println("LoRa: datarate not set!");
	return ok;
}

/*
// Provides a quick test of several methods as a pseudo-unit test.
void Sodaq_RN2483::runTestSequence(SerialType& loraStream, Stream& debugStream)
{
#ifdef DEBUG
    //debugPrint("free ram: ");
    //debugPrintLn(freeRam());

    init(loraStream);

    this->loraStream = &loraStream;
    this->diagStream = &debugStream;

    // expectString
    //debugPrintLn("write \"testString\" and then CRLF");
    if (expectString("testString", 5000)) {
        //debugPrintLn("[expectString] positive case works!");
    }

    //debugPrintLn("");
    //debugPrintLn("write something other than \"testString\" and then CRLF");
    if (!expectString("testString", 5000)) {
        //debugPrintLn("[expectString] negative case works!");
    }

    //debugPrint("free ram: ");
    //debugPrintLn(freeRam());

    // setMacParam(array)
    //debugPrintLn("");
    //debugPrintLn("");
    uint8_t testValue[] = {0x01, 0x02, 0xDE, 0xAD, 0xBE, 0xEF};
    setMacParam("testParam ", testValue, ARRAY_SIZE(testValue));

    // macTransmit
    //debugPrintLn("");
    //debugPrintLn("");
    uint8_t testValue2[] = {0x01, 0x02, 0xDE, 0xAD, 0xBE, 0xEF};
    macTransmit(STR_CONFIRMED, 1, testValue2, ARRAY_SIZE(testValue2));

    //debugPrint("free ram: ");
    //debugPrintLn(freeRam());

    // receive
    //debugPrintLn("");
    //debugPrintLn("==== receive");
    char mockResult[] = "303132333435363738";
    memcpy(this->receivedPayloadBuffer, mockResult, strlen(mockResult) + 1);
    uint8_t payload[64];
    //debugPrintLn("* without having received packet");
    uint8_t length = receive(payload, sizeof(payload));
    //debugPrintLn(reinterpret_cast<char*>(payload));
    //debugPrint("Length: ");
    //debugPrintLn(length);
    //debugPrintLn("* having received packet");
    this->packetReceived = true;
    length = receive(payload, sizeof(payload));
    //debugPrintLn(reinterpret_cast<char*>(payload));
    //debugPrint("Length: ");
    //debugPrintLn(length);

    // onMacRX
    //debugPrintLn("");
    //debugPrintLn("==== onMacRX");
    char mockRx[] = "mac_rx 1 303132333435363738";
    memcpy(this->inputBuffer, mockRx, strlen(mockRx) + 1);
    this->packetReceived = false;// reset
    //debugPrint("Input buffer now is: ");
    //debugPrintLn(this->inputBuffer);
    //debugPrint("onMacRX result code: ");
    //debugPrintLn(onMacRX());
    uint8_t payload2[64];
    if (receive(payload2, sizeof(payload2)) != 9) {
        //debugPrintLn("len is wrong!");
    }
    //debugPrintLn(reinterpret_cast<char*>(payload2));
    if (receive(payload2, sizeof(payload2), 2) != 7) {
        //debugPrintLn("len is wrong!");
    }
    //debugPrintLn(reinterpret_cast<char*>(payload2));
    if (receive(payload2, sizeof(payload2), 3) != 6) {
        //debugPrintLn("len is wrong!");
    }
    //debugPrintLn(reinterpret_cast<char*>(payload2));

    //debugPrint("free ram: ");
    //debugPrintLn(freeRam());

    // lookup error
    //debugPrintLn("");
    //debugPrintLn("");

    //debugPrint("empty string: ");
    //debugPrintLn((lookupMacTransmitError("") == NoResponse) ? "passed" : "wrong");

    //debugPrint("\"random\": ");
    //debugPrintLn((lookupMacTransmitError("random") == NoResponse) ? "passed" : "wrong");

    //debugPrint("\"invalid_param\": ");
    //debugPrintLn((lookupMacTransmitError("invalid_param") == InternalError) ? "passed" : "wrong");

    //debugPrint("\"not_joined\": ");
    //debugPrintLn((lookupMacTransmitError("not_joined") == NotConnected) ? "passed" : "wrong");

    //debugPrint("\"busy\": ");
    //debugPrintLn((lookupMacTransmitError("busy") == Busy) ? "passed" : "wrong");

    //debugPrint("\"invalid_param\": ");
    //debugPrintLn((lookupMacTransmitError("invalid_param") == InternalError) ? "passed" : "wrong");

    //debugPrint("free ram: ");
    //debugPrintLn(freeRam());
#endif
}*/
