#pragma once
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include "Math.h"

// 前方宣言
class ModelCommon;

// 3Dモデル
class Model
{
public: // 構造体 (Object3dから引っ越し)
    struct VertexData {
        Math::Vector4 position;
        Math::Vector2 texcoord;
        Math::Vector3 normal;
    };

    struct Material {
        Math::Vector4 color;
        int32_t enableLighting;
        float padding[3];
        Math::Matrix4x4 uvTransform;
    };

    struct MaterialData {
        std::string textureFilePath;
        uint32_t textureIndex = 0;
    };

    struct ModelData {
        std::vector<VertexData> vertices;
        MaterialData material;
    };

private: // メンバ変数
    // モデル共通部のポインタ (新規追加)
    ModelCommon* modelCommon_ = nullptr;

    // Objファイルのデータ (Object3dから引っ越し)
    ModelData modelData_;

    // 頂点リソース関連 (Object3dから引っ越し)
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    VertexData* vertexData_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // マテリアルリソース関連 (Object3dから引っ越し)
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;


public: // メンバ関数
    // 初期化
    // Model.h
    void Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename);

    void Draw();

private: // メンバ関数 (Object3dから引っ越し)
    void CreateVertexData();
    void CreateMaterialData();
    void LoadObjFile(const std::string& directoryPath, const std::string& filename);
    static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
};