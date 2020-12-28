#include "App.h"
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN
#include <Xinput.h>


#include "Timer.h"

#include <stdio.h>

App::App(std::string adress, int port)
	: m_Adress(adress), m_Port(port), m_Closed(true), m_Connected(false), m_ControllerConnected(false), m_ControllerPort(-1), m_RStickDeadZoneX(2048), m_ServerHandle(INVALID_SOCKET)
{}

App::~App()
{}

void App::Run()
{
	m_Closed = false;
	
	Init();
	m_Connected = Connect();
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
	static float sixtyHZ = 0.0f;
	if (m_ControllerConnected)
	{
		XINPUT_STATE newState;
		ZeroMemory(&newState, sizeof(XINPUT_STATE));
		XInputGetState(m_ControllerPort, &newState);
		if (abs(newState.Gamepad.sThumbRX) < m_RStickDeadZoneX) // Wont be handled if right stick is in deadzone
		{
			newState.Gamepad.sThumbRX = 0;
		}

		sixtyHZ += elapsed;
		if (sixtyHZ > 1.0f / 60.0f) // 60hz
		{
			sixtyHZ = 0.0f;
			//std::cout << std::to_string(newState.Gamepad.bRightTrigger) << ", " << std::to_string(newState.Gamepad.sThumbRX) << std::endl;
		}
	}

	static float halfSecond = 0.0f;
	halfSecond += elapsed;
	if (halfSecond > 0.5f) // Every 0.5s check for new controllers or disconnections
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