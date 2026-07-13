#pragma once

#include <string>

// 文字列コードユーティリティ
namespace StringUtility {

	/// <summary>
	/// std::string (UTF-8) を std::wstring (UTF-16) に変換する
	/// </summary>
	/// <param name="str">変換元の文字列</param>
	/// <returns>変換されたワイド文字列</returns>
	std::wstring ConvertString(const std::string& str);

	/// <summary>
	/// std::wstring (UTF-16) を std::string (UTF-8) に変換する
	/// </summary>
	/// <param name="str">変換元のワイド文字列</param>
	/// <returns>変換された文字列</returns>
	std::string ConvertString(const std::wstring& str);
}