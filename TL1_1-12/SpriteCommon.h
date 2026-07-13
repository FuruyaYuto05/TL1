#pragma once

#include <d3d12.h>
#include <wrl.h>
#include "DirectXCommon.h"

//スプライト共通部
class SpriteCommon
{
public:
    //初期化
    void Initialize(DirectXCommon* dxCommon);

    // 描画前処理（DirectXCommon::PreDraw()で呼ばれることを想定）
    void PreDraw(ID3D12GraphicsCommandList* commandList); // [NEW] PreDrawを追加

    //共通描画設定
    void SetCommonDrawSettings(ID3D12GraphicsCommandList* commandList);

    // アクセサ
    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }      
    ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); } 

    // DirectXCommonのポインタを返すGetterを追加
    DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:

    DirectXCommon* dxCommon_;

    // メンバ変数の追加
    // ルートシグネチャ
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    // グラフィックスパイプラインステート
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;



    // ルートシグネチャの作成
    void CreateRootSignature();
    // グラフィックスパイプラインの生成
    void CreateGraphicsPipelineState();

  
};