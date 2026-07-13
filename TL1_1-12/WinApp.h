#pragma once
#include <Windows.h>
#include <cstdint>
//WindowsAPI
class WinApp
{

private:

	HWND hwnd = nullptr;

	WNDCLASS wc{};

public:

	//getter
	HWND GetHwnd() const { return hwnd; }

	HINSTANCE GetInstance() const { return wc.hInstance; }

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

	bool ProcessMessage();

	void Initialize();

	void Update();

	void Finalize();
};

