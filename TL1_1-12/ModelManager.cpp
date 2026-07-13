#include "ModelManager.h"
#include <cassert>

// 実体の定義
ModelManager* ModelManager::instance = nullptr;

ModelManager* ModelManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ModelManager();
    }
    return instance;
}

void ModelManager::Finalize() {
    delete instance;
    instance = nullptr;
}

// 初期化：モデル共通部の生成と初期化
void ModelManager::Initialize(DirectXCommon* dxCommon) {
    modelCommon = new ModelCommon();
    modelCommon->Initialize(dxCommon);
}

// モデルのロード
void ModelManager::LoadModel(const std::string& filePath) {
    // 1. すでに読み込み済みか確認し、あればそのまま終了
    if (models.contains(filePath)) {
        return;
    }

    // 2. 新しくモデルを生成して初期化（ロード）
    // ※スライドの設計に合わせ、ディレクトリパスとファイル名を指定
    std::unique_ptr<Model> model = std::make_unique<Model>();
    model->Initialize(modelCommon, "resources", filePath);

    // 3. コンテナ（map）に格納する
    models.insert(std::make_pair(filePath, std::move(model)));
}

// モデルの検索
Model* ModelManager::FindModel(const std::string& filePath) {
    // コンテナ内に指定したパスのモデルがあればそのポインタを返す
    if (models.contains(filePath)) {
        return models.at(filePath).get();
    }
    return nullptr;
}