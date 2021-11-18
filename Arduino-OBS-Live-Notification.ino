/*
 Name:		Arduino_OBS_Live_Notification.ino
 Created:	12.11.2021 22:11:42
 Author:	Samuel Nitzsche
*/

#include <WiFi.h>
#include <JSON.h>
#include <WebSocketsClient.h>

#define LED 1
#define WIFI_SSID "Fingerweg"
#define WIFI_PASSWORD "fff222ccc1234"

#define WEBSOCKET_IP_ADDRESS "192.168.1.167"
#define WEBSOCKET_PASSWORD "PlanzenGrün!20"
#define WEBSOCKET_PORT 4446

WiFiClient client;
WebSocketsClient webSocket;

bool sendRequest = true;
JSONVar uuidJSONCombination;

void setup() {
	// Set the LED Pin to be an output
	pinMode(LED, OUTPUT);
	digitalWrite(LED, HIGH);

	// Start Serial Connection
	//Serial.begin(115200);
	//delay(1000);

	// Start connecting to a WiFi network
	Serial.println();
	Serial.println();
	//Serial.print("[WIFI] Connecting to ");
	//Serial.print(WIFI_SSID);

	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		//Serial.print(".");
	}

	Serial.println("");
	Serial.println("[WIFI] Connected");
	//Serial.print("[WIFI] IP address: ");
	Serial.println(WiFi.localIP());
	Serial.println("");

	// Server address, port and URL
	webSocket.begin(WEBSOCKET_IP_ADDRESS, WEBSOCKET_PORT, "/");

	// Event handler
	webSocket.onEvent(webSocketEvent);

	// Try ever 5000 again if connection has failed
	//webSocket.setReconnectInterval(5000);
}

void loop() {
	webSocket.loop();

	if (webSocket.isConnected() && sendRequest)
	{
		sendRequest = false;
		Serial.println("Sending Request");
		String uuid = StringUUIDGen();
		uuidJSONCombination[0]["uuid"] = uuid;
		webSocket.sendTXT("{\"message-id\":\"" + uuid + "\",\"request-type\":\"GetStreamingStatus\"}");
	}

	delay(100);
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
	switch (type) {
	case WStype_DISCONNECTED:
		Serial.println("[WebS] Disconnected!");
		sendRequest = true;
		digitalWrite(LED, HIGH);
		break;

	case WStype_CONNECTED:
		Serial.printf("[WebS] Connected to URL: %s\r\n", payload);
		break;

	case WStype_TEXT:
	{
		//Serial.printf("[WebS] Got text: %s\r\n", payload);
		JSONVar json = JSON.parse((char*)payload);
		Serial.println((const char*)payload);


		if (json.hasOwnProperty("message-id"))
		{
			if (strcmp(uuidJSONCombination[0]["uuid"], json["message-id"]) == 0) {
				if (strcmp(json["status"], "ok") != 0)
				{
					Serial.print("[WebS] Status was revived as ERROR");
					break;
				}
				if (json["streaming"] || json["recording"])
				{
					Serial.println("[WebS] Status: live!");
					digitalWrite(LED, LOW);
				}
				else
				{
					Serial.println("[WebS] Status: not live!");
					digitalWrite(LED, HIGH);
				}
			}
			else
			{
				Serial.println("UUIDs not equal!");
			}
		}
		else if (json.hasOwnProperty("update-type"))
		{
			String updateType = (const char*)json["update-type"];

			if (updateType == "StreamStarting" || updateType == "StreamStarted" || updateType == "RecordingStarting" || updateType == "RecordingStarted" || updateType == "RecordingResumed")
			{
				Serial.println("[WebS] Event: live!");
				digitalWrite(LED, LOW);
			}
			else if (updateType == "StreamStopping" || updateType == "StreamStopped" || updateType == "RecordingStopping" || updateType == "RecordingStopped" || updateType == "RecordingPaused")
			{
				Serial.println("[WebS] Event: not live!");
				digitalWrite(LED, HIGH);
			}
			else if (updateType == "StreamStatus")
			{
				if (json.hasOwnProperty("streaming") && json.hasOwnProperty("recording"))
				{
					if (json["streaming"] || json["recording"])
					{
						Serial.println("[WebS] StreamStatus: live!");
						digitalWrite(LED, LOW);
					}
					else
					{
						Serial.println("[WebS] StreamStatus: not live!");
						digitalWrite(LED, HIGH);
					}
				}
			}
			else if (updateType == "Exiting")
			{
				Serial.println("[WebS] OBS is closing... Disconnecting");
				digitalWrite(LED, HIGH);
				webSocket.disconnect();
				sendRequest = true;
			}
		}
		else
		{
			Serial.println("Not known json response");
		}
		break;
	}

	case WStype_ERROR:
		Serial.printf("[WebS] Error: %s\r\n", payload);
	case WStype_FRAGMENT_TEXT_START:
	case WStype_FRAGMENT_BIN_START:
	case WStype_FRAGMENT:
	case WStype_FRAGMENT_FIN:
		break;
	}
}


// I'm having difficulties including external files so I will copy the file directly
// Repository: https://github.com/furrysalamander/esp-uuid
// Library for generating a UUID4 on ESP32
// Mike Abbott - 2019
// MIT License

#ifndef UUID_GEN
#define UUID_GEN
// For a 32 bit int, returnVar must be of length 8 or greater.
void IntToHex(const unsigned int inInt, char* returnVar)
{
	const char* HEXMAP = "0123456789abcdef";
	for (int i = 0; i < 8; i++)
	{
		// Shift each hex digit to the right, and then map it to its corresponding value
		returnVar[7 - i] = HEXMAP[(inInt >> (i * 4)) & 0b1111];
	}
}

// returnUUID must be of length 37 or greater
// (For the null terminator)
void UUIDGen(char* returnUUID)
{
	for (int i = 0; i < 4; i++)
	{
		unsigned int chunk = esp_random();
		// UUID4 requires a few bits to be specific values.
		if (i == 1)
		{
			chunk &= 0xFFFF0FFF;
			chunk |= 0x00004000;
		}
		if (i == 2)
		{
			chunk &= 0b00111111111111111111111111111111;
			chunk |= 0b10000000000000000000000000000000;
		}
		char chunkChars[8];
		IntToHex(chunk, chunkChars);
		for (int p = 0; p < 8; p++)
		{
			returnUUID[p + 8 * i] = chunkChars[p];
		}
	}
	int dashOffset = 4;
	const int UUID_NUM_DIGITS = 32;
	for (int i = UUID_NUM_DIGITS - 1; i >= 0; i--)
	{
		if (i == 7 || i == 11 || i == 15 || i == 19) // location of dashes
		{
			returnUUID[i + dashOffset--] = '-';
		}
		returnUUID[i + dashOffset] = returnUUID[i];
	}
	returnUUID[36] = 0;
}

String StringUUIDGen()
{
	char returnUUID[37];
	UUIDGen(returnUUID);
	return String(returnUUID);
}
#endif