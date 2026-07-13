#pragma once
#include <string>
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include "externals/DirectXTex/DirectXTex.h"

class DirectXCommon; // 前方宣言

class TextureManager
{
private:
    static TextureManager* instance;

    TextureManager() = default;
    ~TextureManager() = default;

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // ★ SRVインデックスの開始番号（ImGuiが0番を使うので1から）
    static uint32_t kSRVIndexTop;


    DirectXCommon* dxCommon_ = nullptr;

    struct TextureData {
        std::string filePath;
        DirectX::TexMetadata metadata;
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU{};
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};
    };

    std::vector<TextureData> textureDatas;

public:
    static TextureManager* GetInstance();
    void SetDirectXCommon(DirectXCommon* dxCommon);

    void Initialize();
    void Finalize();

    void LoadTexture(const std::string& filePath);

    uint32_t GetTextureIndexByFilePath(const std::string& filePath);

    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

    // メタデータを取得
    const DirectX::TexMetadata& GetMetaData(uint32_t textureIndex);
};
