/*
 Name:		Arduino_OBS_Live_Notification.ino
 Created:	12.11.2021 22:11:42
 Author:	Samuel Nitzsche
*/

#include <string>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESP-UUID.h>
#include <Arduino_JSON.h>
#include <Adafruit_NeoPixel.h>
#include <WebSocketsClient.h>
#include <ESPAsyncWebServer.h>

#define PIN_LED 4
#define NUM_LEDS 287
#define WIFI_SSID "Fingerweg"
#define WIFI_PASSWORD "fff222ccc1234"

#define WEBSOCKET_IP_ADDRESS "192.168.1.167"
#define WEBSOCKET_PASSWORD "PlanzenGrün!20"
#define WEBSOCKET_PORT 4446

// Create 
WiFiClient client;
// Create a webSocketClient for OBS
WebSocketsClient webSocket;
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
JSONVar allSettings;

Adafruit_NeoPixel pixels(NUM_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);

bool sendRequest = true;
String obsUuidResponse;

void setup() {
	pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
	ledOff();

	// Start Serial Connection
	Serial.begin(115200);
	delay(1000);

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

	WiFi.setHostname("Twitch-LED-Schild");
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("[WIFI] Connected");
	Serial.print("[WIFI] IP address: ");
	Serial.println(WiFi.localIP());
	Serial.println("");

	loadAllSettings();


	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/index.html", String(), false);
		});
	server.on("/getSettings", HTTP_GET, [](AsyncWebServerRequest* request) {
		Serial.println("[WebsiteUI] Request");
		AsyncResponseStream* response = request->beginResponseStream("application/json");
		File file = SPIFFS.open("/settings.json", "r");
		String json = file.readString();
		response->print(json);
		request->send(response);
		});
	server.on("/postSettings", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
		[](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
			// The following print statements work + removing them makes no difference
			// This is displayed on monitor "Content type::application/x-www-form-urlencoded"
			String postContent = "";
			for (size_t i = 0; i < len; i++)
			{
				postContent += (char)data[i];
			}
			Serial.print("[WebsiteUI] Einstellungen werden gespeichert: ");
			Serial.println(postContent);
			File file = SPIFFS.open("/settings.json", "w+");
			file.print(postContent);
			file.close();
			allSettings = JSON.parse(postContent);
			request->send(200);
		});

	// Route to load style CSS file
	server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/style.css", "text/css"); });
	// Route to load Twtich-Settings SVG file
	server.on("/Twtich-Settings.svg", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/Twtich-Settings.svg", "image/svg+xml"); });
	// Route to load Twitch-Logo SVG file
	server.on("/Twtich-Logo.svg", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/Twtich-Logo.svg", "image/svg+xml"); });
	// Route to load coloris CSS file
	server.on("/coloris.css", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/coloris.css", "text/css"); });

	// Route to load timepicker javascript
	server.on("/timepicker-ui.js", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/timepicker-ui.js", "text/javascript"); });
	// Route to load coloris javascript
	server.on("/coloris.js", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/coloris.js", "text/javascript"); });
	// Route to load index javascript file
	server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(SPIFFS, "/index.js", "text/javascript"); });

	// Start server
	server.begin();

	// Server address, port and URL
	webSocket.begin(WEBSOCKET_IP_ADDRESS, WEBSOCKET_PORT, "/");

	// Event handler
	webSocket.onEvent(webSocketEvent);

	// Try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(10000);
}

void loop() {
	webSocket.loop();

	if (webSocket.isConnected() && sendRequest)
	{
		sendRequest = false;
		Serial.println("Sending Request");
		String uuid = StringUUIDGen();
		obsUuidResponse = uuid;
		webSocket.sendTXT("{\"message-id\":\"" + uuid + "\",\"request-type\":\"GetStreamingStatus\"}");
	}

	delay(100);
}

uint32_t hexToUInt32(String hexColor)
{
	if (hexColor[0] == '#') {
		hexColor = hexColor.substring(1);
	}

	while (hexColor.length() != 6) {
		hexColor += "0";
	}

	int r = strtol(hexColor.substring(0, 2).c_str(), NULL, 16);
	int g = strtol(hexColor.substring(2, 4).c_str(), NULL, 16);
	int b = strtol(hexColor.substring(4, 6).c_str(), NULL, 16);
	return pixels.Color(r, g, b);
}

void loadAllSettings()
{
	File file = SPIFFS.open("/settings.json", "r");
	String json = file.readString();
	allSettings = JSON.parse(json);
}

void ledOn()
{
	for (size_t i = 0; i < NUM_LEDS; i++)
	{
		pixels.setPixelColor(i, hexToUInt32((const char*)allSettings["streamingColor"]));
	}
	pixels.show();
}

void ledOff()
{
	pixels.clear();
	pixels.show();
}

void ledSetBrightness(int value)
{
	pixels.setBrightness(value % 100);
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
	switch (type) {
	case WStype_DISCONNECTED:
		Serial.println("[WebSocket] Disconnected!");
		sendRequest = true;
		ledOff();
		break;

	case WStype_CONNECTED:
		Serial.printf("[WebSocket] Connected to URL: %s\r\n", payload);
		break;

	case WStype_TEXT:
	{
		//Serial.printf("[WebSocket] Got text: %s\r\n", payload);
		JSONVar json = JSON.parse((char*)payload);
		Serial.println((const char*)payload);


		if (json.hasOwnProperty("message-id"))
		{
			if (obsUuidResponse == json["message-id"]) {
				if (strcmp(json["status"], "ok") != 0)
				{
					Serial.print("[WebSocket] Status was revived as ERROR");
					break;
				}
				if (json["streaming"] || json["recording"])
				{
					Serial.println("[WebSocket] Status: live!");
					ledOn();
				}
				else
				{
					Serial.println("[WebSocket] Status: not live!");
					ledOff();
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
				Serial.println("[WebSocket] Event: live!");
				ledOn();
			}
			else if (updateType == "StreamStopping" || updateType == "StreamStopped" || updateType == "RecordingStopping" || updateType == "RecordingStopped" || updateType == "RecordingPaused")
			{
				Serial.println("[WebSocket] Event: not live!");
				ledOff();
			}
			else if (updateType == "StreamStatus")
			{
				if (json.hasOwnProperty("streaming") && json.hasOwnProperty("recording"))
				{
					if (json["streaming"] || json["recording"])
					{
						Serial.println("[WebSocket] StreamStatus: live!");
						ledOn();
					}
					else
					{
						Serial.println("[WebSocket] StreamStatus: not live!");
						ledOff();
					}
				}
			}
			else if (updateType == "Exiting")
			{
				Serial.println("[WebSocket] OBS is closing... Disconnecting");
				ledOff();
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
		Serial.printf("[WebSocket] Error: %s\r\n", payload);
	case WStype_FRAGMENT_TEXT_START:
	case WStype_FRAGMENT_BIN_START:
	case WStype_FRAGMENT:
	case WStype_FRAGMENT_FIN:
		break;
	}
}