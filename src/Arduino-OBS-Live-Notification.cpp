/*
 Name:		Arduino_OBS_Live_Notification.ino
 Created:	12.11.2021 22:11:42
 Author:	Samuel Nitzsche
*/
#include <string>
#include <WiFi.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <WiFiUdp.h>
#include <uuid.h>
#include <NTPClient.h>
#include <Arduino_JSON.h>
#include <WebSocketsClient.h>
#include <Adafruit_NeoPixel.h>
#include <ESPAsyncWebServer.h>

#define PIN_LED 12
#define NUM_LEDS 158
#define WIFI_SSID "Fingerweg"
#define WIFI_PASSWORD "fff222ccc1234"

#define WEBSOCKET_IP_ADDRESS "192.168.1.38"
#define WEBSOCKET_PORT 4446

#define LOG_SIZE 1024

// Create a WiFiClient
WiFiClient client;
// Create a webSocketClient for OBS
WebSocketsClient webSocket;
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a Json object to save all settings in RAM
JSONVar allSettings;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

Adafruit_NeoPixel pixels(NUM_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);

bool sendInitialRequest = true;
bool ledTimeSwitch = false;
bool currentlyStreaming = false;
bool currentlyRecording = false;
String obsUuidResponse;
String logArray[LOG_SIZE];
int logPointer = 0;

// Function declaration
void checkShutOffTime();
bool isRealtimeBetween(int start, int end);
uint32_t hexToUInt32(String hexColor);
void loadAllSettings();
void ledUpdate();
void ledLoop(const char *color);
void ledSetBrightness();
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void logPrintf(const char *format, ...);
void logPrint(String str);
void logPrintln(String str);
void logStringAndSerialPrint(String str);
String logArrayToString();

// Function definition
void setup()
{
	pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

	// Start Serial Connection
	Serial.begin(115200);
	delay(1000);

	// Initialize SPIFFS
	if (!SPIFFS.begin(true))
	{
		logPrintln("An Error has occurred while mounting SPIFFS");
		return;
	}

	// Start connecting to a WiFi network
	logPrint("[WiFi] Connecting to ");
	logPrint(WIFI_SSID);

	WiFi.setHostname("Twitch-LED-Schild");
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		logPrint(".");
	}

	logPrintln("");
	logPrintln("[WiFi] Connected");
	logPrint("[WiFi] IP address: ");
	logPrintln(WiFi.localIP().toString());

	loadAllSettings();

	timeClient.begin();

	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/index.html", String(), false); });
	server.on("/getSettings", HTTP_GET, [](AsyncWebServerRequest *request)
			  {
		logPrintln("[WebsiteUI] Request");
		AsyncResponseStream* response = request->beginResponseStream("application/json");
		File file = SPIFFS.open("/settings.json", "r");
		String json = file.readString();
		response->print(json);
		request->send(response); });
	server.on(
		"/postSettings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
		[](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
		{
			// The following print statements work + removing them makes no difference
			// This is displayed on monitor "Content type::application/x-www-form-urlencoded"
			String postContent = "";
			for (size_t i = 0; i < len; i++)
			{
				postContent += (char)data[i];
			}
			logPrint("[WebsiteUI] Einstellungen werden gespeichert: ");
			logPrintln(postContent);
			File file = SPIFFS.open("/settings.json", "w+");
			file.print(postContent);
			file.close();
			allSettings = JSON.parse(postContent);
			ledSetBrightness();
			ledUpdate();
			checkShutOffTime();
			request->send(200);
		});

	// Route to load style CSS file
	server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/style.css", "text/css"); });
	// Route to load Twtich-Settings SVG file
	server.on("/Twtich-Settings.svg", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/Twtich-Settings.svg", "image/svg+xml"); });
	// Route to load Twitch-Logo SVG file
	server.on("/Twtich-Logo.svg", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/Twtich-Logo.svg", "image/svg+xml"); });
	// Route to load coloris CSS file
	server.on("/coloris.css", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/coloris.css", "text/css"); });

	// Route to load timepicker javascript
	server.on("/timepicker-ui.js", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/timepicker-ui.js", "text/javascript"); });
	// Route to load coloris javascript
	server.on("/coloris.js", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/coloris.js", "text/javascript"); });
	// Route to load index javascript file
	server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/index.js", "text/javascript"); });

	// Route to load the logs
	server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(200, "text/plain", logArrayToString()); });

	// Start server
	server.begin();

	// Server address, port and URL
	webSocket.begin(WEBSOCKET_IP_ADDRESS, WEBSOCKET_PORT, "/");

	// Event handler
	webSocket.onEvent(webSocketEvent);

	// Try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(10000);

	ledUpdate();
}

unsigned long previousMillisWiFi = 0;
unsigned long previousMillisOffTime = 0;
unsigned long interval = 20000;
void loop()
{
	webSocket.loop();

	unsigned long currentMillis = millis();
	// if WiFi is down, try reconnecting
	if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillisWiFi >= interval))
	{
		logPrint(String(millis()));
		logPrintln("[WiFi] Reconnecting to WiFi...");
		WiFi.disconnect();
		WiFi.reconnect();
		previousMillisWiFi = currentMillis;
	}
	else if (webSocket.isConnected() && sendInitialRequest)
	{
		sendInitialRequest = false;
		logPrintln("[WebSocket] Sending Request");
		String uuid = StringUUIDGen();
		obsUuidResponse = uuid;
		webSocket.sendTXT("{\"message-id\":\"" + uuid + "\",\"request-type\":\"GetStreamingStatus\"}");
	}

	if (currentMillis - previousMillisOffTime >= interval)
	{
		checkShutOffTime();
		previousMillisOffTime = currentMillis;
	}

	delay(100);
}

bool tempLedTimeSwitch = false;
void checkShutOffTime()
{
	int shutOffTime = (int)allSettings["shutOffTime"]["hour"] * 100 + (int)allSettings["shutOffTime"]["minute"];

	if ((bool)allSettings["useShutOffTime"] && isRealtimeBetween(shutOffTime, 600))
	{
		ledTimeSwitch = true;
	}
	else
	{
		ledTimeSwitch = false;
	}

	// To prevent too many updates on the LEDs
	if (ledTimeSwitch != tempLedTimeSwitch)
	{
		logPrintln("[Shutoff Time] Updateing LED from Time");
		tempLedTimeSwitch = ledTimeSwitch;
		ledUpdate();
	}
}

bool isRealtimeBetween(int start, int end)
{
	timeClient.update();
	int realtime = timeClient.getHours() * 100 + timeClient.getMinutes();

	if ((realtime >= start && realtime < end) || (start > end && (realtime >= start || realtime < end)))
	{
		return true;
	}
	else
	{
		return false;
	}
}

uint32_t hexToUInt32(String hexColor)
{
	if (hexColor[0] == '#')
	{
		hexColor = hexColor.substring(1);
	}

	while (hexColor.length() != 6)
	{
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
	logPrintln("[Settings] " + json);
	allSettings = JSON.parse(json);
	ledSetBrightness();
}

void ledUpdate()
{
	if (!(bool)allSettings["manualControl"] && ledTimeSwitch)
	{
		pixels.clear();
		pixels.show();
		return;
	}

	if ((bool)allSettings["manualControl"])
	{
		ledLoop((const char *)allSettings["manualColor"]);
	}
	else if (currentlyStreaming)
	{
		ledLoop((const char *)allSettings["streamingColor"]);
	}
	else if (currentlyRecording)
	{
		ledLoop((const char *)allSettings["recordingColor"]);
	}
	else
	{
		pixels.clear();
		pixels.show();
	}
}

void ledLoop(const char *color)
{
	for (size_t i = 0; i < NUM_LEDS; i++)
	{
		pixels.setPixelColor(i, hexToUInt32(color));
	}
	pixels.show();
}

void ledSetBrightness()
{
	pixels.setBrightness((int)allSettings["brightnessIndex"]);
	pixels.show();
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
	switch (type)
	{
	case WStype_DISCONNECTED:
		logPrintln("[WebSocket] Disconnected!");
		sendInitialRequest = true;
		currentlyStreaming = false;
		currentlyRecording = false;
		ledUpdate();
		break;

	case WStype_CONNECTED:
		logPrintf("[WebSocket] Connected to URL: %s\r\n", payload);
		break;

	case WStype_TEXT:
	{
		// logPrintf("[WebSocket] Got text: %s\r\n", payload);
		JSONVar json = JSON.parse((char *)payload);
		logPrintln((const char *)payload);

		if (json.hasOwnProperty("message-id"))
		{
			if (obsUuidResponse == json["message-id"])
			{
				if (strcmp(json["status"], "ok") != 0)
				{
					logPrint("[WebSocket] Status was revived as ERROR");
					break;
				}
				if (json["streaming"])
				{
					logPrintln("[WebSocket] Status: live!");
					currentlyStreaming = true;
					ledUpdate();
				}
				else if (json["recording"])
				{
					logPrintln("[WebSocket] Status: recording!");
					currentlyRecording = true;
					ledUpdate();
				}
				else
				{
					logPrintln("[WebSocket] Status: not live!");
					currentlyStreaming = false;
					currentlyRecording = false;
					ledUpdate();
				}
			}
			else
			{
				logPrintln("UUIDs not equal!");
			}
		}
		else if (json.hasOwnProperty("update-type"))
		{
			String updateType = (const char *)json["update-type"];

			if (updateType == "StreamStarting" || updateType == "StreamStarted")
			{
				logPrintln("[WebSocket] Event: live!");
				currentlyStreaming = true;
				ledUpdate();
			}
			else if (updateType == "RecordingStarting" || updateType == "RecordingStarted" || updateType == "RecordingResumed")
			{
				logPrintln("[WebSocket] Event: recoding!");
				currentlyRecording = true;
				ledUpdate();
			}
			else if (updateType == "StreamStopping" || updateType == "StreamStopped")
			{
				logPrintln("[WebSocket] Event: not live!");
				currentlyStreaming = false;
				ledUpdate();
			}
			else if (updateType == "RecordingStopping" || updateType == "RecordingStopped" || updateType == "RecordingPaused")
			{
				logPrintln("[WebSocket] Event: not recording!");
				currentlyRecording = false;
				ledUpdate();
			}
			else if (updateType == "StreamStatus")
			{
				if (json.hasOwnProperty("streaming") && json.hasOwnProperty("recording"))
				{
					if (json["streaming"])
					{
						logPrintln("[WebSocket] StreamStatus: live!");
						currentlyStreaming = true;
						ledUpdate();
					}
					else if (json["recording"])
					{
						logPrintln("[WebSocket] StreamStatus: recoding!");
						currentlyRecording = true;
						ledUpdate();
					}
					else
					{
						logPrintln("[WebSocket] StreamStatus: not live!");
						currentlyStreaming = false;
						currentlyRecording = false;
						ledUpdate();
					}
				}
			}
			else if (updateType == "Exiting")
			{
				logPrintln("[WebSocket] OBS is closing... Disconnecting");
				currentlyStreaming = false;
				currentlyRecording = false;
				ledUpdate();
				webSocket.disconnect();
				sendInitialRequest = true;
			}
		}
		else
		{
			logPrintln("[WebSocket] Not known json response");
		}
		break;
	}

	case WStype_ERROR:
		logPrintf("[WebSocket] Error: %s\r\n", payload);
		break;

	default:
		break;
	}
}

void logPrintf(const char *format, ...)
{
	char buff[128];
	va_list arg;
	va_start(arg, format);
	sprintf(buff, format, arg);
	logStringAndSerialPrint(buff);
}

void logPrint(String str)
{
	logStringAndSerialPrint(str);
}

void logPrintln(String str)
{
	logStringAndSerialPrint(str + "\r\n");
}

void logStringAndSerialPrint(String str)
{
	logArray[logPointer] = str;
	logPointer = (logPointer + 1) % 200;
	Serial.print(str);
}

String logArrayToString()
{
	String logStr = "";
	for (int i = 0; i < LOG_SIZE; i++)
	{
		logStr += logArray[(logPointer + i) % LOG_SIZE];
	}
	logStr.trim();
	return logStr;
}