#include "Model.h"
#include "ModelCommon.h"
#include "DirectXCommon.h"
#include "TextureManager.h"
#include <fstream>
#include <sstream>
#include <cassert>

void Model::Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename) {
    modelCommon_ = modelCommon;

    // モデル読み込み
    LoadObjFile(directoryPath, filename);

    // テクスチャ読み込みと番号取得
    TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
    modelData_.material.textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData_.material.textureFilePath);

    // 頂点とマテリアルのデータ作成
    CreateVertexData();
    CreateMaterialData();

    std::string logPath = "Model Loaded: " + filename + ", TexturePath: [" + modelData_.material.textureFilePath + "]\n";
    OutputDebugStringA(logPath.c_str());

    std::string logIndex = "Texture Index: " + std::to_string(modelData_.material.textureIndex) + "\n";
    OutputDebugStringA(logIndex.c_str());
}

void Model::CreateVertexData() {
    vertexResource_ = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData_.vertices.size());
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    std::memcpy(vertexData_, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
}

void Model::CreateMaterialData() {
    materialResource_ = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = false;
    materialData_->uvTransform = Math::MakeIdentity4x4();
}

void Model::Draw() {
    ID3D12GraphicsCommandList* commandList = modelCommon_->GetDxCommon()->GetCommandList();

    // 1. 頂点バッファの設定
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // 2. マテリアル定数バッファの設定
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // 3. テクスチャ（SRV）の設定
    commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData_.material.textureIndex));

    // 4. 描画コマンド (DrawCall)
    commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

// ※ LoadObjFile と LoadMaterialTemplateFile の中身は Object3d.cpp からそのままコピーしてください。
// ただし、メンバ変数名は modelData_ に書き換える必要があります。

// Model.cpp の末尾に追記

void Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
    std::vector<Math::Vector4> positions;
    std::vector<Math::Vector3> normals;
    std::vector<Math::Vector2> texcoords;
    std::string line;

    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());

    modelData_.vertices.clear(); // メンバ変数名に注意

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "v") {
            Math::Vector4 position;
            s >> position.x >> position.y >> position.z;
            position.w = 1.0f;
            positions.push_back(position);
        } else if (identifier == "vt") {
            Math::Vector2 texcoord;
            s >> texcoord.x >> texcoord.y;
            texcoords.push_back(texcoord);
        } else if (identifier == "vn") {
            Math::Vector3 normal;
            s >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (identifier == "f") {
            VertexData triangle[3];
            for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
                std::string vertexDefiniton;
                s >> vertexDefiniton;
                std::istringstream v(vertexDefiniton);
                uint32_t elementIndices[3];
                for (int32_t element = 0; element < 3; ++element) {
                    std::string index;
                    std::getline(v, index, '/');
                    elementIndices[element] = std::stoi(index);
                }
                Math::Vector4 position = positions[elementIndices[0] - 1];
                position.x *= -1.0f;
                Math::Vector2 texcoord = texcoords[elementIndices[1] - 1];
                texcoord.y = 1.0f - texcoord.y;
                Math::Vector3 normal = normals[elementIndices[2] - 1];

                triangle[faceVertex] = { position, texcoord, normal }; // normalも追加
            }
            modelData_.vertices.push_back(triangle[2]);
            modelData_.vertices.push_back(triangle[1]);
            modelData_.vertices.push_back(triangle[0]);
        } else if (identifier == "mtllib") {
            std::string materialFilename;
            s >> materialFilename;
            modelData_.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
        }
    }
}

Model::MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
    MaterialData materialData;
    std::string line;
    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());
    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;
        if (identifier == "map_Kd") {
            std::string textureFilename;
            s >> textureFilename;
            materialData.textureFilePath = directoryPath + "/" + textureFilename;
        }
    }
    return materialData;
}