#pragma once
#define DIRECTINPUT_VERSION   0x0800
#include <dinput.h>
#include <windows.h>
#include <wrl.h>
#include "WinApp.h"

class Input
{
public:

    //namespace省略
    template <class T> using Comptr = Microsoft::WRL::ComPtr<T>;

public: 
    Input();  // コンストラクタの宣言
    ~Input(); // デストラクタの宣言
    
    // メンバ関数
    // 初期化関数にHINSTANCEとHWNDを追加
    void Initialize(WinApp* winApp );
    // 更新
    void Update();

    bool Pushkey(BYTE keyNumber);


private: // メンバ変数（外部から直接アクセスさせない）
    // DirectInputオブジェクト
    IDirectInput8* directInput = nullptr;

    // キーボードデバイス
    IDirectInputDevice8* keyboard;

    // キーボードの現在の状態を保持する配列
    // 256個のキーの状態
    BYTE key[256] = {};

    WinApp* winApp = nullptr;
};