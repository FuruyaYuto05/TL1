#pragma once

#include "Math.h" 
#include <d3d12.h>
#include <wrl.h>
#include <string>


// スプライト共通部（SpriteCommon）のポインタ型を使用するため、前方宣言を行う
class SpriteCommon;

//スプライト
class Sprite
{
public:

    uint32_t textureIndex = 0;

    // 初期化
    // SpriteCommon* を引数として受け取る
    void Initialize(SpriteCommon* spriteCommon, const std::string& textureFilePath);

    void Update();

    // 描画処理メソッド
    void Draw(ID3D12GraphicsCommandList* commandList);


    // 座標の Getter と Setter を追加

    /// <summary>
    /// 座標のGetter
    /// </summary>
    const Math::Vector2& GetPosition() const { return position_; }

    /// <summary>
    /// 座標のSetter
    /// </summary>
    void SetPosition(const Math::Vector2& position) { this->position_ = position; }

    // --- スケール (Scale) の Setter/Getter ---
/// <summary>
/// スケールのGetter
/// </summary>
    const Math::Vector3& GetScale() const { return transform_.scale; }

    /// <summary>
    /// スケールのSetter
    /// </summary>
    void SetScale(const Math::Vector3& scale) { this->transform_.scale = scale; }


    // --- 回転角 (Rotate) の Setter/Getter ---
    /// <summary>
    /// 回転角のGetter
    /// </summary>
    const Math::Vector3& GetRotation() const { return transform_.rotate; }

    /// <summary>
    /// 回転角のSetter
    /// </summary>
    void SetRotation(const Math::Vector3& rotate) { this->transform_.rotate = rotate; }

    // [NEW] 色 (Color) の Setter/Getter を追加
/// <summary>
/// 色のGetter
/// </summary>
    const Math::Vector4& GetColor() const { return materialData_->color; }

    /// <summary>
    /// 色のSetter
    /// </summary>
    void SetColor(const Math::Vector4& color) { this->materialData_->color = color; }

    // サイズ (Size) の Getter と Setter を追加
/// <summary>
/// サイズのGetter
/// </summary>
    const Math::Vector2& GetSize() const { return size_; }

    /// <summary>
    /// サイズのSetter
    /// </summary>
    void SetSize(const Math::Vector2& size) { this->size_ = size; }

    // アンカーポイントのGetter
    const Math::Vector2& GetAnchorPoint() const { return anchorPoint_; }

    // アンカーポイントのSetter
    void SetAnchorPoint(const Math::Vector2& anchorPoint) { this->anchorPoint_ = anchorPoint; }

    // 左右フリップのGetter/Setter
    bool GetFlipX() const { return isFlipX_; }
    void SetFlipX(bool isFlipX) { isFlipX_ = isFlipX; }

    // 上下フリップのGetter/Setter
    bool GetFlipY() const { return isFlipY_; }
    void SetFlipY(bool isFlipY) { isFlipY_ = isFlipY; }


    // テクスチャ左上座標のGetter/Setter
    const Math::Vector2& GetTextureLeftTop() const { return textureLeftTop_; }
    void SetTextureLeftTop(const Math::Vector2& textureLeftTop) { textureLeftTop_ = textureLeftTop; }

    // テクスチャ切り出しサイズのGetter/Setter
    const Math::Vector2& GetTextureSize() const { return textureSize_; }
    void SetTextureSize(const Math::Vector2& textureSize) { textureSize_ = textureSize; }

private:
    //SpriteCommon のポインタをメンバ変数として保持
    SpriteCommon* spriteCommon_ = nullptr; // nullptrlで初期化

    // 頂点データ
    struct VertexData {
        Math::Vector4 position;
        Math::Vector2 texcoord;
        Math::Vector3 normal; // スライドの指示通り、一旦normalも含めておく
    };

    // 頂点データ (メンバ変数)

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;   // VertexBuffer
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;    // IndexBuffer

    // バッファリソース内のデータを指すポインタ (マップ先)
    VertexData* vertexData_ = nullptr;
    uint32_t* indexData_ = nullptr;

    // バッファリソースの使い道を補足するバッファビュー
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;

    void CreateVertexData();


    // マテリアルデータ
    struct Material {
        Math::Vector4 color;             // 色情報 (Vector4)
        int32_t enableLighting;          // ライティング有効/無効フラグ (int32_t)
        float padding[3];                // パディング (float[3])
        Math::Matrix4x4 uvTransform;     // UV変換行列 (Matrix4x4)
    };

    // マテリアル (メンバ変数)

    // バッファリソース (ConstantBuffer)
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;

    // バッファリソース内のデータを指すポインタ
    Material* materialData_ = nullptr;


    // マテリアルデータとバッファの生成を担うプライベートメソッド
    void CreateMaterial();



    // 座標変換行列データ
    struct TransformationMatrix {
        Math::Matrix4x4 WVP;    // World View Projection Matrix
        Math::Matrix4x4 World;  // World Matrix
    };

    // [確認] 座標変換行列 (メンバ変数)

    // バッファリソース (ConstantBuffer)
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;

    // バッファリソース内のデータを指すポインタ
    TransformationMatrix* transformationMatrixData_ = nullptr;

    // トランスフォーム情報
    Math::Transform transform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

    // 座標変換行列とバッファの生成を担うプライベートメソッド
    void CreateTransformationMatrix();

    // 行列更新処理をカプセル化するためのプライベートメソッド
    void UpdateTransformationMatrix();


    // 座標をメンバ変数として保持 (Vector2)
    Math::Vector2 position_ = { 500.0f, 400.0f }; 

    float rotation = 0.0f;

    // サイズをメンバ変数として保持 (Vector2)
    Math::Vector2 size_ = { 100.0f, 100.0f };

    // アンカーポイントをメンバ変数として保持 (初期値は左上の 0.0f, 0.0f)
    Math::Vector2 anchorPoint_ = { 0.5f, 0.5f };

    // 左右フリップ
    bool isFlipX_ = false;
    // 上下フリップ
    bool isFlipY_ = false;


    // テクスチャ左上座標
    Math::Vector2 textureLeftTop_ = { 0.0f, 0.0f };

    // テクスチャ切り出しサイズ
    Math::Vector2 textureSize_ = { 512.0f, 512.0f };

    //  テクスチャサイズをイメージに合わせる
    void AdjustTextureSize();

};