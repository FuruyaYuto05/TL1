#include "Object3dCommon.h"
#include "DirectXCommon.h" // ← 追加: DirectXCommonの機能を使うために必要

// 引数で dxCommon を受け取るように変更
void Object3dCommon::Initialize(DirectXCommon* dxCommon)
{
    // 引数で受け取ってメンバ変数に記録する
    dxCommon_ = dxCommon;

    // グラフィックスパイプラインの生成()を呼び出す
    CreateGraphicsPipeline();
}

void Object3dCommon::CreateRootSignature()
{
    // --- main.cpp からルートシグネチャの作成処理をコピー ---
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // RootParmater作成
    // 平行光源用に rootParameters[3] まで確保
    D3D12_ROOT_PARAMETER rootParameters[4] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    // 平行光源用
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    // Samplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        assert(false);
    }

    hr = dxCommon_->GetDevice()->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_)
    );
    assert(SUCCEEDED(hr));

    signatureBlob->Release();
}

// ... (これまでの関数の実装) ...

void Object3dCommon::CreateGraphicsPipeline()
{
    // スライドの指示：最初にルートシグネチャの作成を呼び出す
    CreateRootSignature();

    // --- main.cpp からパイプラインの生成処理をコピー ---
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[3] = {};
    inputElementDesc[0].SemanticName = "POSITION";
    inputElementDesc[0].SemanticIndex = 0;
    inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDesc[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDesc[1].SemanticName = "TEXCOORD";
    inputElementDesc[1].SemanticIndex = 0;
    inputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDesc[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDesc[2].SemanticName = "NORMAL";
    inputElementDesc[2].SemanticIndex = 0;
    inputElementDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDesc[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayOutDesc{};
    inputLayOutDesc.pInputElementDescs = inputElementDesc;
    inputLayOutDesc.NumElements = _countof(inputElementDesc);

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_RASTERIZER_DESC rastrizeDesc{};
    rastrizeDesc.CullMode = D3D12_CULL_MODE_BACK;
    rastrizeDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // シェーダーのコンパイル
    auto vertexShaderBlob = dxCommon_->CompileShader(L"resources/shaders/Object3d.VS.hlsl", L"vs_6_0");
    auto pixelShaderBlob = dxCommon_->CompileShader(L"resources/shaders/Object3d.PS.hlsl", L"ps_6_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature_;
    graphicsPipelineStateDesc.InputLayout = inputLayOutDesc;
    graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.BlendState = blendDesc;
    graphicsPipelineStateDesc.RasterizerState = rastrizeDesc;

    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
        &graphicsPipelineStateDesc,
        IID_PPV_ARGS(&graphicsPipelineState_)
    );
    assert(SUCCEEDED(hr));
}

// ↓ここから追加
void Object3dCommon::SetCommonDrawSetting()
{
    // コマンドリストの取得
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // ルートシグネチャをセットするコマンド
    commandList->SetGraphicsRootSignature(rootSignature_);
    // グラフィックスパイプラインステートをセットするコマンド
    commandList->SetPipelineState(graphicsPipelineState_);
    // プリミティブトポロジーをセットするコマンド
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
// ↑ここまで追加

