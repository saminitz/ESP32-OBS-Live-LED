/*
 Name:		Arduino_OBS_Live_Notification.ino
 Created:	12.11.2021 22:11:42
 Author:	Samuel Nitzsche
*/

#include <WiFi.h>
#include <WebSocketsClient.h>

#define LED 1
#define WIFI_SSID "Fingerweg"
#define WIFI_PASSWORD "fff222ccc1234"

#define WEBSOCKET_IP_ADDRESS "192.168.1.167"
#define WEBSOCKET_PASSWORD "PlanzenGrün!20"
#define WEBSOCKET_PORT 4446

WiFiClient client;
WebSocketsClient webSocket;

void setup() {
	// Set the LED Pin to be an output
	pinMode(LED, OUTPUT);

	// Start Serial Connection
	Serial.begin(115200);
	delay(1000);

	// We start by connecting to a WiFi network
	Serial.println();
	Serial.println();
	Serial.print("[WIFI] Connecting to ");
	Serial.print(WIFI_SSID);

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


	// server address, port and URL
	webSocket.begin(WEBSOCKET_IP_ADDRESS, WEBSOCKET_PORT, "/");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	//webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
}

void loop() {
	webSocket.loop();
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {

	switch (type) {
	case WStype_DISCONNECTED:
		Serial.println("[WebS] Disconnected!");
		break;
	case WStype_CONNECTED:
		Serial.printf("[WebS] Connected to URL: %s\n", payload);

		// send message to server when Connected
		webSocket.sendTXT("Connected");
		break;
	case WStype_TEXT:
		Serial.printf("[WebS] Got text: %s\n", payload);

		// send message to server
		// webSocket.sendTXT("message here");
		break;

		// send data to server
		// webSocket.sendBIN(payload, length);
		break;
	case WStype_ERROR:
	case WStype_FRAGMENT_TEXT_START:
	case WStype_FRAGMENT_BIN_START:
	case WStype_FRAGMENT:
	case WStype_FRAGMENT_FIN:
		break;
	}

}