#include "Object3d.h"
#include "Object3dCommon.h" 
#include <fstream>
#include <sstream>
#include <cassert>
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "Model.h"
#include "ModelManager.h"

void Object3d::Initialize(Object3dCommon* object3dCommon)
{
    // 引数で受け取ったポインタをメンバ変数に記録する
    this->object3dCommon = object3dCommon;

    transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    cameraTransform = { {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 4.0f, -10.0f} };

    //// ↓ 追加：モデル読み込み
    //// (※ スライドに合わせて "plane.obj" を読み込む設定にしています)
    //LoadObjFile("resources", "plane.obj");

    //// ↓ 追加：.objが参照しているテクスチャファイルの読み込み
    //TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
    //// ↓ 追加：読み込んだテクスチャの番号を取得して保存しておく
    //modelData.material.textureIndex =
    //    TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
    //CreateVertexData();

    //CreateMaterialData();

    CreateTransformationMatrixData();

    CreateDirectionalLightData();

}

//Object3d::MaterialData Object3d::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
//    // ... (中身は変更なし) ...
//    MaterialData materialData;
//    std::string line;
//    std::ifstream file(directoryPath + "/" + filename);
//    assert(file.is_open());
//    while (std::getline(file, line)) {
//        std::string identifier;
//        std::istringstream s(line);
//        s >> identifier;
//        if (identifier == "map_Kd") {
//            std::string textureFilename;
//            s >> textureFilename;
//            materialData.textureFilePath = directoryPath + "/" + textureFilename;
//        }
//    }
//    return materialData;
//}

// ↓ 変更：戻り値を void にし、ローカル変数の ModelData 宣言を削除
//void Object3d::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
//    // なかで必要な変数の宣言
//    // ModelData modelData; ← 削除（メンバ変数を使うため）
//    std::vector<Math::Vector4> positions;
//    std::vector<Math::Vector3> normals;
//    std::vector<Math::Vector2> texcoords;
//    std::string line;
//
//    //ファイルを開く
//    std::ifstream file(directoryPath + "/" + filename);
//    assert(file.is_open());
//
//    // ↓ 変更：メンバ変数の modelData.vertices を一度空にしておく（安全のため）
//    modelData.vertices.clear();
//
//    //実際にファイルを読み込みModelDataを構築
//    while (std::getline(file, line)) {
//        std::string identifier;
//        std::istringstream s(line);
//        s >> identifier;
//
//        if (identifier == "v") {
//            Math::Vector4 position;
//            s >> position.x >> position.y >> position.z;
//            position.w = 1.0f;
//            positions.push_back(position);
//        } else if (identifier == "vt") {
//            Math::Vector2 texcoord;
//            s >> texcoord.x >> texcoord.y;
//            texcoords.push_back(texcoord);
//        } else if (identifier == "vn") {
//            Math::Vector3 normal;
//            s >> normal.x >> normal.y >> normal.z;
//            normals.push_back(normal);
//        } else if (identifier == "f") {
//            VertexData triangle[3];
//            for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
//                std::string vertexDefiniton;
//                s >> vertexDefiniton;
//                std::istringstream v(vertexDefiniton);
//                uint32_t elementIndices[3];
//                for (int32_t element = 0; element < 3; ++element) {
//                    std::string index;
//                    std::getline(v, index, '/');
//                    elementIndices[element] = std::stoi(index);
//                }
//                Math::Vector4 position = positions[elementIndices[0] - 1];
//                position.x *= -1.0f;
//                Math::Vector2 texcoord = texcoords[elementIndices[1] - 1];
//                texcoord.y = 1.0f - texcoord.y;
//                Math::Vector3 normal = normals[elementIndices[2] - 1];
//
//                triangle[faceVertex] = { position,texcoord };
//            }
//            // ↓ 変更：メンバ変数の modelData.vertices に push_back する
//            modelData.vertices.push_back(triangle[2]);
//            modelData.vertices.push_back(triangle[1]);
//            modelData.vertices.push_back(triangle[0]);
//        } else if (identifier == "mtllib") {
//            std::string materialFilename;
//            s >> materialFilename;
//            // ↓ 変更：メンバ変数の modelData.material に代入する
//            modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
//        }
//    }
//
//    // return modelData; ← 削除（メンバ変数に直接書き込んだため返す必要なし）
//}

//void Object3d::CreateVertexData() {
//    // 頂点リソースを作る
//    // modelData.vertices.size() を使って、読み込んだモデルの頂点数分だけメモリを確保する
//    vertexResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
//
//    // リソースにデータを書き込むためのアドレスを取得して vertexData に割り当てる
//    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
//
//    // 読み込んだモデルデータをリソース上のメモリにコピーする
//    std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
//
//    // 頂点バッファビューを作成する（値を設定するだけ）
//    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
//    vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
//    vertexBufferView.StrideInBytes = sizeof(VertexData);
//}

//void Object3d::CreateMaterialData() {
//    // マテリアル用のリソースを作る
//    materialResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(Material));
//
//    // 書き込むためのアドレスを取得
//    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
//
//    // 初期値の設定（白、ライティング有効、単位行列など）
//    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
//    materialData->enableLighting = false; // 有効
//    materialData->uvTransform = Math::MakeIdentity4x4();
//}

void Object3d::CreateTransformationMatrixData() {
    // 座標変換行列用のリソースを作る
    transformationMatrixResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

    // 書き込むためのアドレスを取得
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));

    // 単位行列で初期化しておく
    transformationMatrixData->WVP = Math::MakeIdentity4x4();
    transformationMatrixData->World = Math::MakeIdentity4x4();
}

void Object3d::CreateDirectionalLightData() {
    // 平行光源用のリソースを作る
    directionalLightResource = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));

    // 書き込むためのアドレスを取得
    directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

    // デフォルト値の設定（白、下向き、輝度1.0など）
    directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
    directionalLightData->intensity = 1.0f;
}


// ... (既存のインクルード) ...

void Object3d::Update() {
    // 1. Transform から WorldMatrix を作る
    Math::Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

    // 2. cameraTransform から cameraMatrix を作る
    Math::Matrix4x4 cameraMatrix = Math::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);

    // 3. cameraMatrix から viewMatrix を作る
    Math::Matrix4x4 viewMatrix = Math::Inverse(cameraMatrix);

    // 4. ProjectionMatrix を作って透視投影行列を書き込む
    // (※ fovやクリップ距離は main.cpp で使っていた値を採用します)
    Math::Matrix4x4 projectionMatrix = Math::MakePerspectiveFovMatrix(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);

    // 5. WVP行列を計算して定数バッファに書き込む
    transformationMatrixData->WVP = Math::Multiply(worldMatrix, Math::Multiply(viewMatrix, projectionMatrix));
    transformationMatrixData->World = worldMatrix;
}

void Object3d::Draw() {
    ID3D12GraphicsCommandList* commandList = object3dCommon->GetDxCommon()->GetCommandList();

    //// 1. VertexBufferViewを設定
    //commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    //// 2. マテリアルCBufferの場所を設定
    //commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

    // 3. 座標変換行列CBufferの場所を設定
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

    //// 4. SRVのDescriptorTableの先頭を設定 (テクスチャ)
    //commandList->SetGraphicsRootDescriptorTable(2, object3dCommon->GetDxCommon()->GetSRVGPUDescriptorHandle(modelData.material.textureIndex));

    // 5. 平行光源CBufferの場所を設定
    // (※ rootParameters の 3番目として追加されている想定です)
    commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

    if (model_) {
        model_->Draw();
    }

    //// 6. 描画！ (DrawCall)
    //commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

}

void Object3d::SetModel(const std::string& filePath) {
    model_ = ModelManager::GetInstance()->FindModel(filePath);
}