#include "PostEffect.h"
#include "DirectXCommon.h" 
#include "WinApp.h"       
#include <cassert>
#include "Math.h"

void PostEffect::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // ==========================================
    // 1. RenderTextureとRTVの作成（裏画面の用意）
    // ==========================================

    // クリアカラー設定（スライド通り、テスト用に一旦分かりやすい赤色）
    Math::Vector4 kRenderTargetClearValue = { 1.0f, 0.0f, 0.0f, 1.0f };

    // RenderTextureのリソースを作成
    renderTextureResource_ = dxCommon_->CreateRenderTextureResource(
        WinApp::kClientWidth,  
        WinApp::kClientHeight, 
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        kRenderTargetClearValue);

    // RTV（レンダーターゲットビュー）の作成準備
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // リソースと同じフォーマット
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // RTV用のヒープから、書き込む場所（CPUハンドル）を取得する
    rtvHandleCPU_ = dxCommon_->GetRTVCPUDescriptorHandle(2);

    // デバイスを使ってRTVを実際に作成
    device->CreateRenderTargetView(
        renderTextureResource_.Get(),
        &rtvDesc,
        rtvHandleCPU_);

    // ==========================================
    // 2. SRVの作成（裏画面をテクスチャとして読み込む用意）
    // ==========================================

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; 
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    // SRV用のヒープから、読み込む場所（ハンドル）を取得する
    uint32_t srvIndex = 500;
    srvHandleCPU_ = dxCommon_->GetSRVCPUDescriptorHandle(srvIndex);
    srvHandleGPU_ = dxCommon_->GetSRVGPUDescriptorHandle(srvIndex);

    // デバイスを使ってSRVを実際に作成
    device->CreateShaderResourceView(
        renderTextureResource_.Get(),
        &srvDesc,
        srvHandleCPU_
    );

    // ==========================================
// DepthTexture用SRVの作成
// ==========================================
    D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSrvDesc{};

    // DepthBufferをShaderで読む用のFormat
    depthTextureSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthTextureSrvDesc.Texture2D.MipLevels = 1;

    // srvIndex + 1 に作る
    D3D12_CPU_DESCRIPTOR_HANDLE depthSrvHandleCPU =
        dxCommon_->GetSRVCPUDescriptorHandle(srvIndex + 1);

    device->CreateShaderResourceView(
        dxCommon_->GetDepthStencilResource(),
        &depthTextureSrvDesc,
        depthSrvHandleCPU
    );

    // ==========================================
     // 3. RootSignature の作成
     // ==========================================
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Texture用のディスクリプタレンジ（SRVを1つ使う）
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 2;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // RootParameter の設定
    D3D12_ROOT_PARAMETER rootParameters[2] = {};

    // 0番 : t0, t1 のSRV
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    // 1番 : b0 のConstantBuffer
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].Descriptor.ShaderRegister = 0;

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};

    // s0 : 通常用 Linear Sampler
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // s1 : Depth用 Point Sampler
    staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[1].ShaderRegister = 1;
    staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    // シリアライズして作成
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        assert(false);
    }
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));

    materialResource_ = dxCommon_->CreateBufferResource(sizeof(Material));

    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    Math::Matrix4x4 projectionMatrix = Math::MakePerspectiveFovMatrix(
        0.45f,
        float(WinApp::kClientWidth) / float(WinApp::kClientHeight),
        0.1f,
        100.0f
    );

    materialData_->projectionInverse = Math::Inverse(projectionMatrix);

    // ==========================================
    // 4. PipelineState の作成
    // ==========================================
  
    CreatePipelineState(
        L"resources/shaders/CopyImage.PS.hlsl",
        pipelineStates_[static_cast<size_t>(PostEffectType::CopyImage)]
    );

    CreatePipelineState(
        L"resources/shaders/Grayscale.PS.hlsl",
        pipelineStates_[static_cast<size_t>(PostEffectType::Grayscale)]
    );

    CreatePipelineState(
        L"resources/shaders/Vignette.PS.hlsl",
        pipelineStates_[static_cast<size_t>(PostEffectType::Vignette)]
    );

    CreatePipelineState(
        L"resources/shaders/BoxFilter.PS.hlsl",
        pipelineStates_[static_cast<size_t>(PostEffectType::BoxFilter)]
    );

    CreatePipelineState(
        L"resources/shaders/GaussianFilter.PS.hlsl",
        pipelineStates_[static_cast<size_t>(PostEffectType::GaussianFilter)]
    );

    CreatePipelineState(
        L"resources/shaders/LuminanceBasedOutline.PS.hlsl",
        pipelineStates_[static_cast<size_t>(PostEffectType::LuminanceBasedOutline)]
    );

    CreatePipelineState(
        L"resources/shaders/DepthBasedOutline.PS.hlsl",
        pipelineStates_[static_cast<size_t>(PostEffectType::DepthBasedOutline)]
    );
}

void PostEffect::PreDraw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // 1. 描画先を RenderTexture に設定 (Sceneを描くためDepthも使う)
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVCPUDescriptorHandle();
    commandList->OMSetRenderTargets(1, &rtvHandleCPU_, false, &dsvHandle);

    // 2. 画面クリア（初期化時に設定した ClearValue と同じ赤色）
    float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandleCPU_, clearColor, 0, nullptr);

    // 3. 深度バッファクリア
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // 4. ビューポートとシザー矩形の設定
    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, (FLOAT)WinApp::kClientWidth, (FLOAT)WinApp::kClientHeight, 0.0f, 1.0f };
    commandList->RSSetViewports(1, &viewport);
    D3D12_RECT scissorRect{ 0, 0, WinApp::kClientWidth, WinApp::kClientHeight };
    commandList->RSSetScissorRects(1, &scissorRect);
}

void PostEffect::PostDraw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // 1. 描画先を本来の SwapChain に戻す
    UINT bbIndex = dxCommon_->GetBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE swapChainHandle = dxCommon_->GetRTVCPUDescriptorHandle(bbIndex);

    // SwapChain側はDepthを使わないので nullptr にする！
    commandList->OMSetRenderTargets(1, &swapChainHandle, false, nullptr);

    // 2. SwapChainの画面クリア（RenderTextureの赤色と区別するため青色）
    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    commandList->ClearRenderTargetView(swapChainHandle, clearColor, 0, nullptr);

    // 3. ビューポートとシザー矩形の設定（SwapChain用）
    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, (FLOAT)WinApp::kClientWidth, (FLOAT)WinApp::kClientHeight, 0.0f, 1.0f };
    commandList->RSSetViewports(1, &viewport);
    D3D12_RECT scissorRect{ 0, 0, WinApp::kClientWidth, WinApp::kClientHeight };
    commandList->RSSetScissorRects(1, &scissorRect);
}

void PostEffect::Draw()
{
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // RenderTextureをPixelShaderで読むために状態変更
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = renderTextureResource_.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);

    // DepthをPixelShaderで読むために状態変更
    D3D12_RESOURCE_BARRIER depthBarrier{};
    depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    depthBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    depthBarrier.Transition.pResource = dxCommon_->GetDepthStencilResource();
    depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &depthBarrier);

    commandList->SetGraphicsRootSignature(rootSignature_.Get());

    commandList->SetPipelineState(
        pipelineStates_[static_cast<size_t>(currentEffect_)].Get()
    );

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRootDescriptorTable(0, srvHandleGPU_);

    // projectionInverse用のCBufferは次以降で必要
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());

    commandList->DrawInstanced(3, 1, 0, 0);

    // RenderTextureを次フレームでまた描画先に戻す
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // Depthを次フレームでまた書き込めるように戻す
    depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    commandList->ResourceBarrier(1, &depthBarrier);
}

void PostEffect::CreatePipelineState(
    const wchar_t* psFilePath,
    Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState)
{
    ID3D12Device* device = dxCommon_->GetDevice();

    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(
        L"resources/shaders/Fullscreen.VS.hlsl",
        L"vs_6_0"
    );
    assert(vertexShaderBlob != nullptr);

    Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(
        psFilePath,
        L"ps_6_0"
    );
    assert(pixelShaderBlob != nullptr);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();

    graphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
    graphicsPipelineStateDesc.InputLayout.NumElements = 0;

    graphicsPipelineStateDesc.VS = {
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize()
    };

    graphicsPipelineStateDesc.PS = {
        pixelShaderBlob->GetBufferPointer(),
        pixelShaderBlob->GetBufferSize()
    };

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    graphicsPipelineStateDesc.BlendState = blendDesc;

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = false;
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;

    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    HRESULT hr = device->CreateGraphicsPipelineState(
        &graphicsPipelineStateDesc,
        IID_PPV_ARGS(&pipelineState)
    );
    assert(SUCCEEDED(hr));
}