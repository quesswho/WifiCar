#include "ESP8266WiFi.h"

#include "passwords.h" // Includes information such as wifi name, wifi password and authentication password in the protocol.


WiFiServer g_WifiServer(4596);

WiFiClient g_Client;

bool g_Authenticate;


const int ControlGasPin1 = 0;
const int ControlGasPin2 = 4;
const int GasPowerPin = 13;
//const int GasPotPin = A0;
int MotorSpeed = 0;

const int ControlSteerPin1 = 16;
const int ControlSteerPin2 = 14;
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

	Serial.begin(115200);
	while(!Serial) {} // Wait for serial initialization


	WiFi.begin(g_WifikName, g_WifiPassword);
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

			while (g_Client.available() > 0) {
				if (g_Authenticate)
				{
					switch (g_Client.read())
					{
						case 0xF2:
						{
							time = millis();
							break;
						}
						case 0x2F:
						{
							if (g_Client.available() == 2)
							{
								short power = 0;
								g_Client.readBytes((char*)&power, 2);

								if (power < 0)
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
							}
							break;
						}
						case 0x3F:
						{
							if (g_Client.available() == 2)
							{
								short power = 0;
								g_Client.readBytes((char*)&power, 2);

								if (power < 0)
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
							}
							break;
						}
					}
				}
				else
				{
					if (g_Client.read() == 0xF1)
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
			if (time - millis() > 1000) // If 1s passed without status packet then turn of the motors.
			{
				analogWrite(GasPowerPin, 0);
				analogWrite(SteerPowerPin, 0);
			}
		}

		g_Client.stop();
		Serial.println("Client disconnected");
		g_Authenticate = false;
	}
}

void SendStatus()
{
	static char packet[1] = { 0xF2 };
	g_Client.write((char*)packet, 1);
}