#include "Input.h"
#include <cassert>


#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")


// コンストラクタの定義
Input::Input()
{
	// 初期化は Initialize で行うため、ここでは何もしなくてOK
}

// デストラクタの定義
Input::~Input()
{
	// DirectInputリソースの解放 (Release) 処理があればここで行う
	if (keyboard) {
		keyboard->Unacquire(); // デバイスの解放
		keyboard->Release();
		keyboard = nullptr;
	}
	if (directInput) {
		directInput->Release();
		directInput = nullptr;
	}
}

void Input::Initialize(WinApp* winApp)
{

	// DirectInputオブジェクトの生成
	
	HRESULT result = DirectInput8Create(
		GetModuleHandle(nullptr), // ② WinMainのhInstaceでもOKですが、ここではモジュールハンドルを取得しています
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&directInput,
		nullptr
	);
	assert(SUCCEEDED(result));

	//キーボードデバイスの生成
	
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
	
	this->winApp = winApp;
}

void Input::Update()
{

	//キーボード情報の取得開始
	keyboard->Acquire();

	//全キーの入力状態を取得する
	keyboard->GetDeviceState(sizeof(key), key);

}

bool Input::Pushkey(BYTE keyNumber)
{
	//指定キーをお押していればtrueを返す
	if (key[keyNumber]) {
		return true;
	}
	//そうでなければfalse
	return false;
}
