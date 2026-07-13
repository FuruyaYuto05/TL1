#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <array>
#include "Math.h"

class DirectXCommon;

enum class PostEffectType {
    CopyImage,
    Grayscale,
    Vignette,
    BoxFilter,
    GaussianFilter,
	LuminanceBasedOutline,
    DepthBasedOutline,
    Count
};

class PostEffect {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(DirectXCommon* dxCommon);

    /// <summary>
    /// 描画前処理（RenderTextureへの描画開始）
    /// </summary>
    void PreDraw();

    /// <summary>
    /// 描画後処理（SwapChainへの描画切り替え）
    /// </summary>
    void PostDraw();

    /// <summary>
    /// 描画（RenderTextureを画面全体に貼り付けて描画）
    /// </summary>
    void Draw();


    void SetEffect(PostEffectType type) { currentEffect_ = type; }

private:
    // DirectXCommonのポインタ
    DirectXCommon* dxCommon_ = nullptr;

    // レンダーターゲットテクスチャのリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResource_;

    // RTVのハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU_{};

    // SRVのハンドル（CPU / GPU）
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};

    // パイプライン生成用
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;


    void CreatePipelineState(const wchar_t* psFilePath, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState);

    PostEffectType currentEffect_ = PostEffectType::CopyImage;

    std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>,static_cast<size_t>(PostEffectType::Count)> pipelineStates_;

    struct Material
    {
        Math::Matrix4x4 projectionInverse;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;
};