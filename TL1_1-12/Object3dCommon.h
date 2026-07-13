#pragma once
#include <d3d12.h> // DirectX12の型を使うために追加

class DirectXCommon; // ← 追加: DirectXCommon型の存在をコンパイラに教える

// 3Dオブジェクト共通部
class Object3dCommon
{
public: // メンバ関数
    // 初期化 (引数を追加)
    void Initialize(DirectXCommon* dxCommon);

    // dxCommonのゲッター
    DirectXCommon* GetDxCommon() const { return dxCommon_; }

    // ↓ここに追加
    // 共通描画設定
    void SetCommonDrawSetting();

private: // メンバ関数
    // ルートシグネチャの作成
    void CreateRootSignature();
    // グラフィックスパイプラインの生成
    void CreateGraphicsPipeline();

private: // メンバ変数
    DirectXCommon* dxCommon_ = nullptr; // ← 追加: DirectXCommonのポインタ

    // ルートシグネチャ
    ID3D12RootSignature* rootSignature_ = nullptr;
    // グラフィックスパイプラインステート
    ID3D12PipelineState* graphicsPipelineState_ = nullptr;
};