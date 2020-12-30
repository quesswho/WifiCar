#include "ESP8266WiFi.h"

// globals I know...
const char* g_NetworkName = "Loading...";
const char* g_Password = "p_ssw0rd";

WiFiServer wifiServer(4596);

void setup() {
	Serial.begin(115200);
	while(!Serial) {} // Wait for serial initialization

	WiFi.begin(g_NetworkName, g_Password);
	while (WiFi.status() != WL_CONNECTED) { // Connect to wifi
		delay(500);
		Serial.println("Connecting...");
	}
	Serial.print("Connected to WiFi. Using ip:");
	Serial.println(WiFi.localIP());


	wifiServer.begin();
}

void loop()
{
	WiFiClient client = wifiServer.available();

	if (client) {
		Serial.print("Client Connected with ip: ");
		Serial.println(client.remoteIP());
		while (client.connected()) {

			while (client.available() > 0) {
				char c = client.read();
				Serial.write(c);
			}

			delay(10);
		}

		client.stop();
		Serial.println("Client disconnected");

	}
}

/*

const int ControlGasPin1 = 0;
const int ControlGasPin2 = 4;
const int GasPowerPin = 13;
//const int GasPotPin = A0;
int MotorSpeed = 0;

const int ControlSteerPin1 = 2;
const int ControlSteerPin2 = 5;
const int SteerPowerPin = 12;
//const int SteerPotPin = A1;
int SteeringValue = 0;

void setup() {
	pinMode(ControlGasPin1, OUTPUT);
	pinMode(ControlGasPin2, OUTPUT);
	pinMode(GasPowerPin, OUTPUT);
	pinMode(GasPowerPin, LOW);

	pinMode(ControlSteerPin1, OUTPUT);
	pinMode(ControlSteerPin2, OUTPUT);
	pinMode(SteerPowerPin, OUTPUT);
	pinMode(SteerPowerPin, LOW);
	Serial.begin(9600);
}

void loop() {
	delay(1);
	MotorSpeed = (analogRead(GasPotPin)) / 2;
	//Serial.println(analogRead(GasPotPin));
	if (MotorSpeed <= 255)
	{
		digitalWrite(ControlGasPin1, HIGH);
		digitalWrite(ControlGasPin2, LOW);
	}
	else
	{
		MotorSpeed -= 255;
		digitalWrite(ControlGasPin1, LOW);
		digitalWrite(ControlGasPin2, HIGH);
	}

	analogWrite(GasPowerPin, MotorSpeed);


	// Steering

	SteeringValue = (analogRead(SteerPotPin)) / 2;
	if (SteeringValue <= 255)
	{
		digitalWrite(ControlSteerPin1, HIGH);
		digitalWrite(ControlSteerPin2, LOW);
	}
	else
	{
		SteeringValue -= 255;
		digitalWrite(ControlSteerPin1, LOW);
		digitalWrite(ControlSteerPin2, HIGH);
	}

	analogWrite(SteerPowerPin, abs(SteeringValue));
}*/