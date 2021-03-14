#include "ESP8266WiFi.h"

#include "passwords.h" // Includes information such as wifi name, wifi password and authentication password in the protocol.

WiFiServer g_WifiServer(4596);

WiFiClient g_Client;

bool g_Authenticate;

const int ControlGasPin1 = 0;
const int ControlGasPin2 = 4;
const int GasPowerPin = 13;
int MotorSpeed = 0;

const int ControlSteerPin1 = 16;
const int ControlSteerPin2 = 14;
const int SteerPowerPin = 12;
int SteeringValue = 0;

const int PowerSensor = 2;

void setup() {
	pinMode(ControlGasPin1, OUTPUT);
	pinMode(ControlGasPin2, OUTPUT);
	pinMode(GasPowerPin, OUTPUT);
	digitalWrite(GasPowerPin, LOW);

	pinMode(ControlSteerPin1, OUTPUT);
	pinMode(ControlSteerPin2, OUTPUT);
	pinMode(SteerPowerPin, OUTPUT);
	digitalWrite(SteerPowerPin, LOW);

	pinMode(PowerSensor, OUTPUT);
	digitalWrite(PowerSensor, LOW);

	Serial.begin(115200);
	while(!Serial) {} // Wait for serial initialization

	Serial.println("Hello");

	WiFi.begin(g_WifiName, g_WifiPassword);
	while (WiFi.status() != WL_CONNECTED) { // Connect to wifi
		delay(500);
		Serial.println("Connecting...");
	}

	Serial.print("Connected to WiFi. Using ip: ");
	Serial.println(WiFi.localIP());

	g_WifiServer.begin();
}

void loop()
{
	static long time = millis();

	g_Client = g_WifiServer.available();
	g_Client.setNoDelay(true);
	if (g_Client) {
		Serial.print("Client Connected with ip: ");
		Serial.println(g_Client.remoteIP());
		while (g_Client.connected()) {

			while (g_Client.available() > 0) { // while receiving bytes
				if (g_Authenticate)
				{
					switch (g_Client.read())
					{
						case 0xF2: // Status packet
						{
							time = millis();
							break;
						}
						case 0x2F: // Gas packet
						{
							short power = 0;
							g_Client.readBytes((char*)&power, 2);

							if (power < 0) // Switch poles to go backwards or forwards
							{
								digitalWrite(ControlGasPin1, LOW);
								digitalWrite(ControlGasPin2, HIGH);
							}
							else
							{
								digitalWrite(ControlGasPin1, HIGH);
								digitalWrite(ControlGasPin2, LOW);
							}

							analogWrite(GasPowerPin, abs(power));
							break;
						}
						case 0x3F: // Steer packet
						{
							short power = 0;
							g_Client.readBytes((char*)&power, 2);

							if (power < 0) // Switch poles to steer right or left
							{
								digitalWrite(ControlSteerPin1, LOW);
								digitalWrite(ControlSteerPin2, HIGH);
							}
							else
							{
								digitalWrite(ControlSteerPin1, HIGH);
								digitalWrite(ControlSteerPin2, LOW);
							}

							analogWrite(SteerPowerPin, abs(power));
							break;
						}
						case 0x4F: // Temperature request
						{
							digitalWrite(PowerSensor, HIGH);
							delay(1); // takes 2-4 microseconds to activate. 1ms just to be safe
							SendTemperature(analogRead(0));
							digitalWrite(PowerSensor, LOW);
							break;
						}
						default:
						{

							Serial.println("Something is wrong");
							Serial.println(g_Client.read());
							break;
						}
					}
				}
				else
				{
					if (g_Client.read() == 0xF1) // Check for auth packet, if not then disconnect
					{
						if (g_Client.available() == 8)
						{
							long long unsigned int password = 0;
							g_Client.readBytes((char*)&password, 8);
							if (password == g_Password)
							{
								g_Authenticate = true;
								Serial.println("Client authenticated!");
								SendStatus();
								time = millis();
								continue;
							}
						}
					}
					g_Client.stop();
				}
			}

			delay(20); // Drain less current
			if (time - millis() > 500) // If 0.5 seconds has passed without status packet then turn of the motors to prevent possible disaster.
			{
				//digitalWrite(GasPowerPin, LOW);
				//digitalWrite(SteerPowerPin, LOW);
			}
		}

		g_Client.stop();
		Serial.println("Client disconnected");
		digitalWrite(GasPowerPin, LOW);
		digitalWrite(SteerPowerPin, LOW);
		g_Authenticate = false;
	}
}

void SendStatus()
{
	static char packet[1] = { 0xF2 };
	g_Client.write((char*)packet, 1);
}

void SendTemperature(short int temp)
{
	Serial.println(temp);

	static char packet[3] = { 0x4F };
	memcpy(&packet[1], &temp, 2);
	g_Client.write((char*)packet, 3);
}