#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

class App
{
public:
	App(std::string adress, int port);
	~App();

	void Run();

private:
	const int m_RStickDeadZoneX;

	int m_Port;
	std::string m_Adress;

	bool m_Closed;
	bool m_Connected;

	SOCKET m_ServerHandle;
	void Update(float elapsed);

	void Init();
	bool Connect();

	bool GetIpPort();

	bool m_ControllerConnected;
	int m_ControllerPort;
};