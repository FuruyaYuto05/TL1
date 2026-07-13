#pragma once
#include <map>
#include <string>
#include <memory>
#include "Model.h"
#include "ModelCommon.h"

class ModelManager {
public:
    static ModelManager* GetInstance();
    void Finalize();

    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    // モデルのロード（すでに読み込み済みならそれを返す）
    void LoadModel(const std::string& filePath);

    // モデルの検索
    Model* FindModel(const std::string& filePath);

private:
    ModelManager() = default;
    ~ModelManager() = default;
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    static ModelManager* instance;

    // モデルデータをファイルパス（文字列）で管理する
    std::map<std::string, std::unique_ptr<Model>> models;

    // モデル共通部
    ModelCommon* modelCommon = nullptr;
};