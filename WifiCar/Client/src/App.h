#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

using uchar = unsigned char;
using uint = unsigned int;
using uint64 = unsigned long long int;

#define PACKET_LENGTH 512

struct StatusPacket
{};

struct AuthPacket
{
	AuthPacket(uint64 password)
		: m_Password(password)
	{}
	uint64 m_Password;
};

struct PingPacket
{
	int m_Payload;
};

struct GasPacket
{
	GasPacket(short power)
		: m_Power(power)
	{}
	short m_Power;
};

struct SteerPacket
{
	SteerPacket(short power)
		: m_Power(power)
	{}
	short m_Power;
};

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
	bool m_Authenticated;

	SOCKET m_ServerHandle;
	void Update(float elapsed);

	void Init();
	bool Connect();

	bool GetIpPort();

	bool m_ControllerConnected;
	int m_ControllerPort;

	// Sending Packets
	void SendPacket(AuthPacket auth) const;
	void SendPacket(StatusPacket status = {}) const;
	void SendPacket(PingPacket auth) const;
	void SendPacket(GasPacket auth) const;
	void SendPacket(SteerPacket auth) const;
};