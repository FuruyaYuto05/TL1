#pragma once

#include <string>
#include <Windows.h> // HRESULT, OutputDebugStringAで使用
#include "StringUtility.h"


// ログ出力のための名前空間
namespace Logger {

	/// <summary>
	/// std::stringの内容をデバッグ出力ウィンドウにログとして出力する
	/// </summary>
	/// <param name="message">出力したい文字列</param>
	void Log(const std::string& message);
}