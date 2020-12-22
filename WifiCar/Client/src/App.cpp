#include "App.h"
#include "Timer.h"
#include <string>

#include <iostream>



App::App()
	: m_Closed(true), m_ControllerConnected(false), m_ControllerPort(-1), m_RStickDeadZoneX(2048)
{

}

App::~App()
{}

void App::Run()
{
	m_Closed = false;
	Timer t;
	t.Start();
	float elapsed = 0.0f;
	
	while (!m_Closed) // Main loop, check for changes in the controller
	{
		elapsed = t.End();
		t.Start();
		
		Update(elapsed);
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
			std::cout << std::to_string(newState.Gamepad.bRightTrigger) << ", " << std::to_string(newState.Gamepad.sThumbRX) << std::endl;
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