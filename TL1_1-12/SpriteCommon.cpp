#include "SpriteCommon.h"
#include "DirectXCommon.h" // dxCommon_ や GetDevice() のために必要
#include "WinApp.h"        // WinAppの定数（画面サイズなど）が必要な場合を想定
#include "dxcapi.h"        // Shader Compileのために必要
#include <cassert>         // assertのために必要

using namespace Microsoft::WRL;




// =============================================================
// ルートシグネチャの作成 (CreateRootSignature)
// =============================================================
void SpriteCommon::CreateRootSignature() {
    // 依存ポインタからデバイスを取得
    ID3D12Device* device = dxCommon_->GetDevice();
    HRESULT hr = S_FALSE;

    // RootSignatureの作成ロジックは main.cpp から移動済み
    // ... (D3D12_ROOT_SIGNATURE_DESC, D3D12_DESCRIPTOR_RANGE, D3D12_ROOT_PARAMETER, D3D12_STATIC_SAMPLER_DESC の設定) ...

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0; descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[3] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange; rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0; staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    ComPtr<ID3DBlob> signatureBlob = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) { assert(false); }

    hr = device->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_)
    );
    assert(SUCCEEDED(hr));
}


// =============================================================
// グラフィックスパイプラインの生成 (CreateGraphicsPipelineState)
// =============================================================
void SpriteCommon::CreateGraphicsPipelineState() {
    ID3D12Device* device = dxCommon_->GetDevice();
    HRESULT hr = S_FALSE;

    // スライドの指示：最初にルートシグネチャを作成する関数を呼び出す
    CreateRootSignature();

    // GraphicsPipelineStateの作成ロジックは main.cpp から移動済み

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[2] = {};
    inputElementDesc[0].SemanticName = "POSITION"; inputElementDesc[0].SemanticIndex = 0; inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; inputElementDesc[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDesc[1].SemanticName = "TEXCOORD"; inputElementDesc[1].SemanticIndex = 0; inputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT; inputElementDesc[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayOutDesc{};
    inputLayOutDesc.pInputElementDescs = inputElementDesc;
    inputLayOutDesc.NumElements = _countof(inputElementDesc);

    // BlendState
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // RasterizerState
    D3D12_RASTERIZER_DESC rastrizeDesc{};
    rastrizeDesc.CullMode = D3D12_CULL_MODE_NONE;
    rastrizeDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // Shader Compile (dxCommon_を利用してシェーダーコンパイルを呼び出す)
    ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(L"resources/shaders/Object3d.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);
    ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"resources/shaders/Object3d.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    // DepthStencilState 
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // PipelineStateの作成
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
    graphicsPipelineStateDesc.InputLayout = inputLayOutDesc;
    graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.BlendState = blendDesc;
    graphicsPipelineStateDesc.RasterizerState = rastrizeDesc;

    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    hr = device->CreateGraphicsPipelineState(
        &graphicsPipelineStateDesc,
        IID_PPV_ARGS(&graphicsPipelineState_)
    );
    assert(SUCCEEDED(hr));
}

// =============================================================
// 共通描画設定 (SetCommonDrawSettings)
// =============================================================
void SpriteCommon::SetCommonDrawSettings(ID3D12GraphicsCommandList* commandList) {

    // 1. ルートシグネチャをセットするコマンド
    commandList->SetGraphicsRootSignature(rootSignature_.Get());

    // 2. グラフィックスパイプラインステートをセットするコマンド
    commandList->SetPipelineState(graphicsPipelineState_.Get());

    // 3. プリミティブトポロジーをセットするコマンド
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

// =============================================================
// 初期化 (Initialize)
// =============================================================
void SpriteCommon::Initialize(DirectXCommon* dxCommon)
{
    // 引数で受け取ってメンバ変数に記録する
    dxCommon_ = dxCommon;
    assert(dxCommon_ != nullptr);

    // グラフィックスパイプラインの生成を呼び出す
    CreateGraphicsPipelineState();
}


// =============================================================
// 描画前処理 (PreDraw)
// =============================================================
void SpriteCommon::PreDraw(ID3D12GraphicsCommandList* commandList) {
    SetCommonDrawSettings(commandList);
}