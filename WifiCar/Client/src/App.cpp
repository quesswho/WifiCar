#include "App.h"
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN
#include <Xinput.h>

#include <stdio.h>
#include <thread>

#include "../../ESP8266/src/passwords.h" // Includes secret authentication password

App::App(std::string adress, int port)
	: m_Adress(adress), m_Port(port), m_Closed(true), m_Connected(false), m_Authenticated(false), m_ControllerConnected(false), m_ControllerPort(-1), m_RStickDeadZoneX(2048), m_ServerHandle(INVALID_SOCKET)
{}

App::~App()
{}

void App::Run()
{
	m_Closed = false;
	Init();
	m_Connected = Connect();
	if (m_Connected) {
		printf("Connected to esp8266! Authenticating...\n");
		SendPacket(AuthPacket(g_Password));
		m_AuthLatency.Start();
	}
	Timer t;
	t.Start();
	float elapsed = 0.0f;

	while (!m_Closed) // Main loop, check for changes in the controller
	{
		if (m_Connected)
		{
			elapsed = t.End();
			t.Start();
		
			Update(elapsed);
		}
		else
		{
			if (GetIpPort())
			{
				m_Connected = Connect();
			}
			else
			{
				continue;
			}
		}
	}
}

void App::Update(float elapsed)
{

	// Authentication sequence
	if (!m_Authenticated)
	{
		uchar packetBuffer[PACKET_LENGTH];
		ZeroMemory(&packetBuffer, PACKET_LENGTH);
		// Have to put recv on another thread to be able to receive ping packet later.
		int result = recv(m_ServerHandle, (char*)packetBuffer, PACKET_LENGTH, 0);
		if (result > 0)
		{
			if (!m_Authenticated && packetBuffer[0] == 0xF2)
			{
				printf("Authenticated in %ums\n", (uint)roundf((float)m_AuthLatency.End() * 1000.0f));
				printf("Client authenticated!\n");
				m_Authenticated = true;

				// Start thread that recieves packets from esp8266
				std::thread t(&App::Listen, this);
				t.detach();
			}
		}
		else if(result == 0)
		{
			printf("Connection closed.\n");
			m_Authenticated = false;
			m_Connected = false;
		}
		else
		{
			printf("Error: %d\n", WSAGetLastError());
			m_Authenticated = false;
			m_Connected = false;
		}
	}

	static bool flatSteer = false; // When there in no input, don't send empty packets.
	static bool flatGas = false;
	static float tenHZ = 0.0f;

	static float secondCooldown = 1.0f;

	if (m_ControllerConnected)
	{
		XINPUT_STATE newState;
		ZeroMemory(&newState, sizeof(XINPUT_STATE));
		XInputGetState(m_ControllerPort, &newState);
		if (abs(newState.Gamepad.sThumbLX) < m_RStickDeadZoneX) // Wont be handled if right stick is in deadzone
		{
			newState.Gamepad.sThumbLX = 0;
		}
		static short gasPower = 0;
		gasPower = newState.Gamepad.bRightTrigger - newState.Gamepad.bLeftTrigger;

		tenHZ += elapsed;
		if (tenHZ > 1.0f / 10.0f) // 10hz
		{
			tenHZ = 0.0f;
			if (newState.Gamepad.sThumbLX != 0) flatSteer = false;
			if (!flatSteer)
			{
				SendPacket(SteerPacket(newState.Gamepad.sThumbLX / 32));
				if (newState.Gamepad.sThumbLX == 0) flatSteer = true;
			}

			if (gasPower != 0) flatGas = false;
			if (!flatGas)
			{
				SendPacket(GasPacket(gasPower * 4));
				if (gasPower == 0) flatGas = true;
			}
		}

		// Temperature request
		if (secondCooldown > 0.0f) secondCooldown -= elapsed;
		if ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_A) && secondCooldown <= 0.0f)
		{
			secondCooldown = 1.0f;
			SendPacket(TempPacket());
		}
	}

	static float halfSecond = 0.0f;
	halfSecond += elapsed;
	if (halfSecond > 0.25f) // Every 0.25s check for new controllers and send status packet
	{
		halfSecond = 0.0f;
		for (int i = 0; i < XUSER_MAX_COUNT; i++)
		{
			XINPUT_STATE state;
			ZeroMemory(&state, sizeof(XINPUT_STATE));
			if (XInputGetState(i, &state) == ERROR_SUCCESS)
			{
				m_ControllerConnected = true;
				m_ControllerPort = i;
				break;
			}
			else
			{
				m_ControllerPort = -1;
			}
		}
		SendPacket(); // Status Packet
	}
}

void App::Init()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // Initialize winsock 2.2
	{
		printf("Could not initialize Winsock!\n");
		m_Closed = true;
		return;
	}
}

bool App::Connect()
{

	struct addrinfo* addrInfo = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int flags = 1;
	// Setting addrinfo
	getaddrinfo(m_Adress.c_str(), std::to_string(m_Port).c_str(), &hints, &addrInfo);

	int error;
	printf("Connecting...\n");
	for (ptr = addrInfo; ptr != NULL; ptr = ptr->ai_next) {
		// Init socket
		m_ServerHandle = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);

		if (m_ServerHandle == INVALID_SOCKET) {
			printf("Failed to create socket: %d\n", WSAGetLastError());
			WSACleanup();
			return false;
		}

		int opt = 1;
		setsockopt(m_ServerHandle, SOL_SOCKET, TCP_NODELAY, (char*)&opt, sizeof(int));

		// Connect to server
		error = connect(m_ServerHandle, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (error == SOCKET_ERROR) {
			closesocket(m_ServerHandle);
			m_ServerHandle = INVALID_SOCKET;
			continue; //try again
		}
		break;
	}

	freeaddrinfo(addrInfo);

	if (m_ServerHandle == INVALID_SOCKET) {
		printf("Could not connect to server %s:%d\n", m_Adress.c_str(), m_Port);
		WSACleanup();
		return false;
	}
	return true;
}

bool App::GetIpPort()
{
	char* ip = new char[15];
	int port = 0;
	printf("Enter IPv4. E.g 192.168.1.123\n");
	if (scanf("%s", ip) != 1)
	{
		printf("Invalid adress!\n");
		return false;
	}
	printf("Enter port:\n");
	if (scanf("%d", &port) != 1)
	{
		printf("Invalid port!\n");
		return false;
	}
	m_Adress = ip;
	m_Port = port;
	delete[] ip;
	return true;
}

void App::Listen()
{
	uchar packetBuffer[PACKET_LENGTH];
	while (m_Authenticated)
	{
		int result = recv(m_ServerHandle, (char*)packetBuffer, PACKET_LENGTH, 0);
		if (result > 0)
		{
			int i = 0;
			while (result > i)
			{
				if (packetBuffer[i++] == 0x4F)
				{
					if (result > 2) // Atleast 3 bytes
					{
						short int tmpReading;
						memcpy(&tmpReading, &packetBuffer[i], 2);
						float temperature = (tmpReading / 1023.0f) * 165.0f - 40; 
						// (tmpReading / 1023.0f) = turn a range of 0-1023 to 0.0 - 1.0
						// total range of sensor is -40 to 125. 40 + 125 = 165.0
						// Degrees are offset by +40 so go -40

						printf("Temperature: %f C\n", temperature);
						i += 2;
					}
				}
			}
		}
		else if (result == 0)
		{
			printf("Connection closed.\n");
			m_Authenticated = false;
			m_Connected = false;
		}
		else
		{
			printf("Error: %d\n", WSAGetLastError());
			m_Authenticated = false;
			m_Connected = false;
		}
	}
}

void App::SendPacket(AuthPacket auth) const
{
	static uchar packet[9] = { 0xF1 }; // Pre allocated packet and static because of optimization

	memcpy(&packet[1], (char*)&auth.m_Password, 8);
	send(m_ServerHandle, (char*)packet, 9, 0);
}

void App::SendPacket(StatusPacket) const
{
	static uchar packet[1] = { 0xF2 };
	send(m_ServerHandle, (char*)packet, 1, 0);
}

void App::SendPacket(PingPacket ping) const
{
	static uchar packet[5] = { 0x1F }; // Pre allocated packet and static because of optimization

	memcpy(&packet[1], (char*)&ping.m_Payload, 4);
	send(m_ServerHandle, (char*)packet, 5, 0);
}

void App::SendPacket(GasPacket gas) const
{
	static uchar packet[3] = { 0x2F }; // Pre allocated packet and static because of optimization

	memcpy(&packet[1], (char*)&gas.m_Power, 2);
	send(m_ServerHandle, (char*)packet, 3, 0);
}

void App::SendPacket(SteerPacket steer) const
{
	static uchar packet[3] = { 0x3F }; // Pre allocated packet and static because of optimization

	memcpy(&packet[1], (char*)&steer.m_Power, 2);
	send(m_ServerHandle, (char*)packet, 3, 0);
}

void App::SendPacket(TempPacket) const
{
	static uchar packet[1] = { 0x4F };
	send(m_ServerHandle, (char*)packet, 1, 0);
}