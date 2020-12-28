#pragma once
#include "App.h"

int main()
{
	App* app = new App(std::string("192.168.1.219"), 4596);
	app->Run();
	delete app;
	return 0;
}
