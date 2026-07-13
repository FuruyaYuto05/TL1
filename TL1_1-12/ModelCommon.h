#pragma once

// 前方宣言
class DirectXCommon;

// 3Dモデル共通部
class ModelCommon
{
public: // メンバ関数
    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    // ゲッター
    DirectXCommon* GetDxCommon() const { return dxCommon_; }

private: // メンバ変数
    DirectXCommon* dxCommon_ = nullptr;
};