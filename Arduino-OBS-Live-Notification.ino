/*
 Name:		Arduino_OBS_Live_Notification.ino
 Created:	12.11.2021 22:11:42
 Author:	Samuel Nitzsche
*/

#include <WiFi.h>
#include <JSON.h>
#include <SPIFFS.h>
#include <ESP-UUID.h>
#include <WebSocketsClient.h>
#include <ESPAsyncWebServer.h>

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

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup() {
	// Set the LED Pin to be an output
	pinMode(LED, OUTPUT);
	digitalWrite(LED, HIGH);

	// Start Serial Connection
	//Serial.begin(115200);
	//delay(1000);

	// Initialize SPIFFS
	if (!SPIFFS.begin(true)) {
		Serial.println("An Error has occurred while mounting SPIFFS");
		return;
	}

	// Start connecting to a WiFi network
	Serial.println();
	Serial.println();
	//Serial.print("[WIFI] Connecting to ");
	//Serial.print(WIFI_SSID);

	WiFi.setHostname("ESP32-Twitch-LED-Board");
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

	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/index.html", String(), false); });
	// Route for root / web page
	server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/style.css", "text/css"); });
	// Route to load style.css file
	server.on("/Twtich-Settings.svg", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/Twtich-Settings.svg", "image/svg+xml"); });
	// Route to load style.css file
	server.on("/Twtich-Logo.svg", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/Twtich-Logo.svg", "image/svg+xml"); });
	// Route to load style.css file
	server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/bootstrap.min.css", "text/css"); });
	// Route to load jquery.js file
	server.on("/coloris.css", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/coloris.css", "text/css"); });
	// Route to load jquery.js file
	server.on("/jquery-1.10.2.min.js", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/jquery-1.10.2.min.js", "text/javascript"); });
	// Route to load jquery.js file
	server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/bootstrap.bundle.min.js", "text/javascript"); });
	// Route to load jquery.js file
	server.on("/coloris.js", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/coloris.js", "text/javascript"); });

	// Start server
	server.begin();

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