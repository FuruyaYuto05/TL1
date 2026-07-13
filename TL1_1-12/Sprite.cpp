#include "Sprite.h"
#include "SpriteCommon.h" 
#include <cassert> // (ポインタチェックのために追加)
#include "Math.h"
#include "WinApp.h" // [FIX] WinApp::kClientWidth/Height を参照するために追加
#include <algorithm>
#include "TextureManager.h"


// 初期化
// [FIX] Initialize の引数に SpriteCommon* を追加
void Sprite::Initialize(SpriteCommon* spriteCommon, const std::string& textureFilePath)
{
    // スライドの指示通り、引数で受け取ったポインタをメンバ変数に記録する
    this->spriteCommon_ = spriteCommon;

    textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

    // ポインタが有効か確認
    assert(this->spriteCommon_ != nullptr);

    CreateVertexData();

    // マテリアルデータ作成関数を呼び出す
    CreateMaterial();

    // 座標変換行列作成関数を呼び出す
    CreateTransformationMatrix();

    AdjustTextureSize();

    // TODO: 今後、ここにリソース生成のロジックを実装していく
}

// =============================================================
// 頂点データとバッファの生成
// =============================================================
void Sprite::CreateVertexData() {
    ID3D12Device* device = spriteCommon_->GetDxCommon()->GetDevice();
    HRESULT hr = S_FALSE;

    // --- 1. VertexResource / IndexResource を作る ---
    const size_t kVertexCount = 4; // 矩形は通常4頂点
    const size_t kIndexCount = 6;  // 矩形は通常6インデックス

    // VertexResource (4頂点)
    vertexResource_ = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * kVertexCount);
    // IndexResource (6インデックス)
    indexResource_ = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * kIndexCount);

    // --- 2. VertexResource / IndexResource にデータを書き込むためのアドレスを取得 ---
    // VertexData* vertexData_ = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    // uint32_t* indexData_ = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

    // --- 3. VertexData の初期値設定 (main.cpp から移植) ---
    // ※ ここではスプライトのサイズを仮で 128x128 と設定するロジックを移植します
    // main.cppの座標 {0.0f,360.0f}, {0.0f,0.0f}, {640.0f,360.0f}, {640.0f,0.0f}
    float left = 0.0f;
    float top = 0.0f;
    float right = 1.0f;  
    float bottom = 1.0f; 

    vertexData_[0].position = { left, bottom, 0.0f, 1.0f };    // 左下
    vertexData_[0].texcoord = { 0.0f, 1.0f };
    vertexData_[1].position = { left, top, 0.0f, 1.0f };       // 左上
    vertexData_[1].texcoord = { 0.0f, 0.0f };
    vertexData_[2].position = { right, bottom, 0.0f, 1.0f };   // 右下
    vertexData_[2].texcoord = { 1.0f, 1.0f };
    vertexData_[3].position = { right, top, 0.0f, 1.0f };      // 右上
    vertexData_[3].texcoord = { 1.0f, 0.0f };
    // Normalは一旦0で埋める
    vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };
    vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };
    vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };
    vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };

    // IndexData の設定 (main.cpp から移植)
    indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2; // 1枚目の三角形 (左下、左上、右下)
    indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2; // 2枚目の三角形 (左上、右上、右下)

    // --- 4. VertexBufferView / IndexBufferView を作成する ---
    // VertexBufferView
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // IndexBufferView
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * kIndexCount;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

// =============================================================
// マテリアルデータとバッファの生成
// =============================================================
void Sprite::CreateMaterial() {
    // ID3D12Device* device = spriteCommon_->GetDxCommon()->GetDevice(); // デバイスはリソース作成関数内で使用されるため省略

    // --- 1. マテリアルリソースを作る (ConstantBuffer) ---
    // Sprite::Material構造体のサイズでリソースを作成
    materialResource_ = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
    assert(materialResource_);
    // --- 2. マテリアルリソースにデータを書き込むためのアドレスを取得 ---
    HRESULT hr = materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    assert(SUCCEEDED(hr));
    // --- 3. マテリアルデータの初期値を書き込む (スライドの指示) ---
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = false; // false = 0
    // paddingはDxCommonで適切なサイズになるよう調整されるため、ここでは省略
    materialData_->uvTransform = Math::MakeIdentity4x4();
}

// =============================================================
// 座標変換行列とバッファの生成
// =============================================================
void Sprite::CreateTransformationMatrix() {

    // --- 1. 座標変換行列リソースを作る (ConstantBuffer) ---
    // Sprite::TransformationMatrix構造体のサイズでリソースを作成
    transformationMatrixResource_ = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

    // --- 2. リソースにデータを書き込むためのアドレスを取得 ---
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

    // --- 3. 単位行列を書き込む (スライドの指示) ---
    transformationMatrixData_->WVP = Math::MakeIdentity4x4();
    transformationMatrixData_->World = Math::MakeIdentity4x4();
}


// =============================================================
//  座標変換行列の更新
// =============================================================
void Sprite::UpdateTransformationMatrix() {
    // TransformからWorldMatrixを作る
    // transform_ は Sprite::h で {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} で初期化済み
    Math::Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    // ViewMatrixを作って単位行列を代入 (2Dカメラは通常単位行列)
    Math::Matrix4x4 viewMatrix = Math::MakeIdentity4x4();

    // ProjectionMatrixを作って平行投影行列を書き込む
    // WinApp::kClientWidth/Height は、メインループからアクセス可能な定数である前提
    // WinApp.hをSprite.cppでインクルードして、定数を参照する必要があります。
  Math::Matrix4x4 projectionMatrix = Math::MakeOrthorgraphicMatrix(
        0.0f,
        0.0f,                          // [FIX] ここを 0.0f に変更 (Top = 0)
        (float)WinApp::kClientWidth,
        (float)WinApp::kClientHeight,  // [FIX] ここを Height に変更 (Bottom = Height)
        0.0f,
        100.0f
    );

    // WVP = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
    Math::Matrix4x4 worldViewProjectionMatrix = Math::Multiply(worldMatrix, Math::Multiply(viewMatrix, projectionMatrix));

    // transformationMatrixData_->WVP に書き込む
    transformationMatrixData_->WVP = worldViewProjectionMatrix;

    // transformationMatrixData_->World に書き込む
    transformationMatrixData_->World = worldMatrix;

    // TODO: 頂点データ更新 (UpdateVertexData) を行う場合は、この関数内で transform_ を使用して頂点データを変更します。
}

void Sprite::Update() {
    // 頂点リソースにデータを書き込む (4点分) / インデックスリソースにデータを書き込む (6個分)
    // -> 現状、頂点/インデックスデータは固定のため、Update()では省略。
    //    位置やサイズが変わる場合に、UpdateVertexData()を呼び出します。


    // アンカーポイントを使って、頂点の座標（0.0f～1.0fの範囲）をずらす
    float left = 0.0f - anchorPoint_.x;
    float right = 1.0f - anchorPoint_.x;
    float top = 0.0f - anchorPoint_.y;
    float bottom = 1.0f - anchorPoint_.y;

    // 左右反転：アンカーポイントを基準に左右を入れ替える（符号を反転）
    if (isFlipX_) {
        left = -left;
        right = -right;
    }

    // 上下反転：アンカーポイントを基準に上下を入れ替える（符号を反転）
    if (isFlipY_) {
        top = -top;
        bottom = -bottom;
    }


    // 左右上下の座標を頂点データに書き込む
    // ※ vertexData_ は CreateVertexData() で Map してあるので、ここで書き込めばGPUに反映されます

    // 左下 (Index 0)
    vertexData_[0].position = { left, bottom, 0.0f, 1.0f };

    // 左上 (Index 1)
    vertexData_[1].position = { left, top, 0.0f, 1.0f };

    // 右下 (Index 2)
    vertexData_[2].position = { right, bottom, 0.0f, 1.0f };

    // 右上 (Index 3)
    vertexData_[3].position = { right, top, 0.0f, 1.0f };

    // --- 2.テクスチャ範囲指定（UV座標）の計算 ---

    // 指定されているテクスチャの情報を取得（幅や高さを知るため）
    const DirectX::TexMetadata& metadata =
        TextureManager::GetInstance()->GetMetaData(textureIndex);

    // ピクセル単位の指定を、UV座標（0.0～1.0）に変換する
    float tex_left = textureLeftTop_.x / metadata.width;
    float tex_right = (textureLeftTop_.x + textureSize_.x) / metadata.width;
    float tex_top = textureLeftTop_.y / metadata.height;
    float tex_bottom = (textureLeftTop_.y + textureSize_.y) / metadata.height;

    // 頂点データ（TexCoord）に書き込む
    vertexData_[0].texcoord = { tex_left, tex_bottom }; // 左下
    vertexData_[1].texcoord = { tex_left, tex_top };    // 左上
    vertexData_[2].texcoord = { tex_right, tex_bottom };// 右下
    vertexData_[3].texcoord = { tex_right, tex_top };   // 右上


    // 行列更新処理を呼び出す
    transform_.translate = { position_.x,position_.y,0.0f };

    transform_.scale = { size_.x, size_.y, 1.0f };

    UpdateTransformationMatrix();

    // TODO: 必要に応じて、マテリアルの色やUV情報などを更新するロジックを追加
}

// =============================================================
// 描画処理 (Draw)
// =============================================================
void Sprite::Draw(ID3D12GraphicsCommandList* commandList) {

    // 1. VertexBufferView を設定
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // 2. IndexBufferView を設定
    commandList->IASetIndexBuffer(&indexBufferView_);

    // 3. マテリアル CBuffer の場所を設定 (RootParameter[0])
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // 4. 座標変換行列 CBuffer の場所を設定 (RootParameter[1])
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    // 5. SRV の Descriptor Table の先頭を設定 (RootParameter[2])
    // ※ SpriteCommon が持つ Descriptor Heap の先頭から、このスプライトのテクスチャハンドルを取得する必要があります。
    //    ここでは、仮に0番目のスロット（テクスチャ）を使用していると仮定し、SpriteCommonからヒープハンドルを取得します。
    //    (実装によっては、この SRV 設定は SpriteCommon::PreDraw に含まれることもありますが、ここでは Draw で個別に行います)
    //    ⇒ 正確には、SRVのヒープ自体は SetCommonDrawSettings でセット済みなので、ここでは個別の SRV のハンドルを設定します。
    // commandList->SetGraphicsRootDescriptorTable(2, spriteCommon_->GetSRVGPUDescriptorHandle(0)); // GetSRVGPUDescriptorHandle は仮のメソッド

    commandList->SetGraphicsRootDescriptorTable(2,TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));
    // ToDo: SRVハンドルの取得ロジックが必要だが、今回は main.cpp の描画コマンドをそのまま移植し、
    //      テクスチャアトラスを考慮しない暫定的なコードとする。
    //      main.cpp で使われていた D3D12_GPU_DESCRIPTOR_HANDLE を直接参照できないため、一旦この行は保留し、DrawCallに進みます。

    // 6. 描画! (DrawCall)
    // インデックス数 6, インスタンス数 1 で描画
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

// テクスチャサイズをイメージに合わせる
void Sprite::AdjustTextureSize() {
    // テクスチャメタデータを取得
    const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureIndex);

    // テクスチャサイズをスプライトのサイズに設定する
    textureSize_.x = static_cast<float>(metadata.width);
    textureSize_.y = static_cast<float>(metadata.height);

    // 画像サイズをテクスチャサイズに合わせる
    size_ = textureSize_;
}