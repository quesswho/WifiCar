#pragma once

#include <Windows.h>
#define WIN32_LEAN_AND_MEAN
#include <Xinput.h>

class App
{
public:
	App();
	~App();

	void Run();

private:
	const int m_RStickDeadZoneX;

	bool m_Closed;
	void Update(float elapsed);

	bool m_ControllerConnected;
	int m_ControllerPort;
};