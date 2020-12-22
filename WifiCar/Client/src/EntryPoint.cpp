#pragma once
#include "App.h"

/*int WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
*/
int main()
{
	App* app = new App();
	app->Run();
	delete app;
	return 0;
}
