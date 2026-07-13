#include "DirectXCommon.h"
#include "WinApp.h" 
#include <string>
#include <format>
#include <Windows.h>
#include <cassert>
#include <dxgidebug.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Logger.h"
#include "StringUtility.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <thread>
#include "Math.h"

using namespace Microsoft::WRL;

const uint32_t DirectXCommon::kMaxSRVCount = 512;

Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile
)
{
	Logger::Log(StringUtility::ConvertString(std::format(L"Begin CompliteShader,path:{},profile:{}\n", filePath, profile)));

	// dxcUtils_ メンバ変数を使用
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E",L"main",
		L"-T",profile,
		L"-Zi",L"-Qembed_debug",
		L"-Od",
		L"-Zpr",
	};

	// dxcCompiler_ と includeHandler_ メンバ変数を使用
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler_->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler_.Get(), // ComPtrなので .Get()
		IID_PPV_ARGS(shaderResult.GetAddressOf())
	);
	assert(SUCCEEDED(hr));

	// エラーチェック
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(shaderError.GetAddressOf()), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Logger::Log(shaderError->GetStringPointer());
		assert(false);
	}

	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.GetAddressOf()), nullptr);
	assert(SUCCEEDED(hr));
	Logger::Log(StringUtility::ConvertString(std::format(L"Compile Succeded, path:{}, profile:{}\n", filePath, profile)));

	return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes)
{
	//頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	//頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	//バッファリソース
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;
	//バッファの場合はこれらは1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	//バッファの場合はこれにする
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//実際に頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	// device_ メンバ変数を使用
	HRESULT hr = device_->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertexResource.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return vertexResource;
}


Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metadata)
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;

	// device_ メンバ変数を使用
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(resource.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return resource;
}


Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	UINT numDescriptors,
	bool shaderVisible)
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// device_ メンバ変数を使用
	HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(descriptorHeap.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return descriptorHeap;
}

DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string& filePath)
{
	DirectX::ScratchImage image{};
    
    // StringUtility::ConvertString を使用
	std::wstring filePathW = StringUtility::ConvertString(filePath); 
    
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImages{};
    // ミップマップ生成
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	return mipImages;
}

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height)
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

void DirectXCommon::UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages)
{
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
		HRESULT hr = texture->WriteToSubresource(
			UINT(mipLevel),
			nullptr,
			img->pixels,
			UINT(img->rowPitch),
			UINT(img->slicePitch)
		);
		assert(SUCCEEDED(hr));
	}
}

// 静的メンバ関数の実装
D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(
	const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += (descriptorSize * index);
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(
	const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += (descriptorSize * index);
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index) const
{
	// 汎用的な静的関数を呼び出し、SRV用のメンバ変数を渡す
	return GetCPUDescriptorHandle(srvDescriptorHeap_, srvDescriptorSize_, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index) const
{
	// 汎用的な静的関数を呼び出し、SRV用のメンバ変数を渡す
	return GetGPUDescriptorHandle(srvDescriptorHeap_, srvDescriptorSize_, index);
}

// --- DirectXCommon::InitializeDevice の実装 ---

void DirectXCommon::InitializeDevice()
{
	HRESULT hr;

	// 1. デバッグレイヤーをオンに (DXGIファクトリー生成前)
#ifdef _DEBUG
	// デバッグレイヤーを有効化する
	ID3D12Debug* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		debugController->Release();
	}
#endif

	// 2. DXGIファクトリーの生成
	/*hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));*/
	hr = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// 3. アダプターの列挙と選択
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(useAdapter_.GetAddressOf())) !=
		DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter_->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			Logger::Log(StringUtility::ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
			break;
		}
		/*useAdapter_ = nullptr;*/
		useAdapter_.Reset();
	}
	assert(useAdapter_ != nullptr);

	// 4. デバイス生成
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		//hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(device_.GetAddressOf()));
		if (SUCCEEDED(hr)) {
			Logger::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device_ != nullptr);
	Logger::Log("Compleate create D3D12Device!!!\n");

	// 5. エラー時にブレークさせる設定 (デバッグレイヤーがオンの場合のみ)
#ifdef _DEBUG
	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		// メッセージの抑制設定 (main.cppから移動)
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		D3D12_MESSAGE_SEVERITY severities[]{ D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		infoQueue->PushStorageFilter(&filter);

		infoQueue->Release();
	}
#endif // _DEBUG
}



void DirectXCommon::InitializeCommand()
{

	HRESULT hr;

	// コマンドキューの生成
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	// hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)); // main.cppの古いコード
	// ComPtrのメンバ変数を使用
	hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// コマンドアロケータの生成
	// hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)); // main.cppの古いコード
	// ComPtrのメンバ変数を使用
	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// コマンドリストの生成
	// hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList)); // main.cppの古いコード
	// ComPtrのメンバ変数を使用
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(commandList_.GetAddressOf()));
	hr = commandList_->Close();
	assert(SUCCEEDED(hr));
}

void DirectXCommon::InitializeDescriptorHeaps()
{
	// 1. 各ディスクリプタサイズを取得
	rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	srvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	dsvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, false);

	srvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);

	dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

void DirectXCommon::InitializeSwapchain()
{
	HRESULT hr;

	// スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = WinApp::kClientWidth;
	swapChainDesc.Height = WinApp::kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// スワップチェーン生成
	hr = dxgiFactory_->CreateSwapChainForHwnd(
		commandQueue_.Get(),
		winApp_->GetHwnd(), // 👈 メンバ変数 winApp_ を使用してウィンドウハンドルを取得
		&swapChainDesc,
		nullptr,
		nullptr,
		reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf())
	);
	assert(SUCCEEDED(hr));


	
}

void DirectXCommon::InitializeDepthBufferResource()
{
	// 深度バッファリソースの生成
	// ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, WinApp::kClientWidth, WinApp::kClientHeight); // main.cppの古いコード
	depthStencilResource_.Attach(CreateDepthStencilTextureResource(device_.Get(), WinApp::kClientWidth, WinApp::kClientHeight)); // ✅ ComPtr修正 (.Attachを使用)

	
}

void DirectXCommon::InitializeDSV()
{
	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	// DSVの生成 (dsvDescriptorHeap_は既にInitializeDescriptorHeapsで生成済み)
	device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::InitializeRTV()
{
	HRESULT hr;
	const uint32_t bufferCount = _countof(swapChainResources_);

	// 1. スワップチェーンからリソースを引っ張ってくる (バックバッファリソースの取得)
	for (uint32_t i = 0; i < bufferCount; ++i) {
		// ComPtrの配列なので、&swapChainResources_[i] でアドレスを渡す
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
		assert(SUCCEEDED(hr));
	}

	// 2. RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// ディスクリプタの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	// 3. RTVを2つ作成 (バックバッファの数だけ作成)
	for (uint32_t i = 0; i < bufferCount; ++i) {
		// RTVハンドルを設定 (2つ目以降はディスクリプタサイズ分ずらす)
		rtvHandles_[i] = GetCPUDescriptorHandle(rtvDescriptorHeap_, rtvDescriptorSize_, i);

		device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);

		// RTVの生成
		//device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]); // ComPtrなので.Get()を使用
	}
}

void DirectXCommon::InitializeFence()
{
	HRESULT hr;

	// Fenceの生成 (ComPtr使用)
	// fenceValue_はメンバ変数として0で初期化済み
	hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// Fence Eventの作成
	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent_ != nullptr);
}

void DirectXCommon::InitializeViewport()
{
	// ビューポートの設定
	viewport_.Width = WinApp::kClientWidth;
	viewport_.Height = WinApp::kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

void DirectXCommon::InitializeScissor()
{
	// シザリング矩形の設定
	scissorRect_.left = 0;
	scissorRect_.right = WinApp::kClientWidth;
	scissorRect_.top = 0;
	scissorRect_.bottom = WinApp::kClientHeight;
}

void DirectXCommon::InitializeImGui()
{
	// 1. バージョンチェックとコンテキストの生成
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// 2. スタイルの設定
	ImGui::StyleColorsDark();

	// 3. Win32用の初期化
	// WinAppインスタンスのウィンドウハンドルを使用
	ImGui_ImplWin32_Init(winApp_->GetHwnd());

	// 4. DirectX12用の初期化
	// RTVフォーマットは DXGI_FORMAT_R8G8B8A8_UNORM_SRGB を使用
	ImGui_ImplDX12_Init(device_.Get(),
		2, // SwapChainのバッファ数 (固定値 2)
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		srvDescriptorHeap_.Get(),
		srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());
}

void DirectXCommon::InitializeDXC()
{
	HRESULT hr;

	// DXCユーティリティの生成 (ComPtr使用)
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// DXCコンパイラの生成 (ComPtr使用)
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxcCompiler_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// デフォルトインクルードハンドラの生成 (ComPtr使用)
	hr = dxcUtils_->CreateDefaultIncludeHandler(includeHandler_.GetAddressOf());
	assert(SUCCEEDED(hr));
}

void DirectXCommon::InitializeFixFPS()
{
	// 現在時間を記録する
	reference_ = std::chrono::steady_clock::now();
}

// 1/60秒ぴったりの時間
const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
// 1/60秒よりわずかに短い時間を表す定数
const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

void DirectXCommon::UpdateFixFPS()
{
	// 1/60秒ぴったりの時間
	// const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f)); // 👈 グローバルに移動済み

	// 記録時間から現在までの時間を取得
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	// 経過時間をマイクロ秒で取得
	const std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	// 経過時間が短すぎた場合は、その差分だけスリープする
	// kMinTime を使用
	if (elapsed < kMinTime) {
		// 待機が必要な時間を計算
		const std::chrono::microseconds sleepTime = kMinTime - elapsed;

		// 待機 (マイクロ秒単位でのスリープ)
		std::this_thread::sleep_for(sleepTime);
	}

	// 次のフレームのための基準時間を更新
	reference_ = std::chrono::steady_clock::now();


}

void DirectXCommon::Initialize(WinApp* winApp)
{
	assert(winApp);
	this->winApp_ = winApp;

	// FPS固定初期化
	InitializeFixFPS();

	// デバイスの生成、コマンド関連の生成、スワップチェーンの生成を
	// 機能ごとに分割して呼び出す

	InitializeDevice();

	// コマンド関連の生成
	InitializeCommand();

	// スワップチェーンの生成 (ディスクリプタヒープを含む)
	InitializeSwapchain();

	// 深度バッファリソースの生成
	InitializeDepthBufferResource(); //  リソース生成のみ

	InitializeDescriptorHeaps();

	// RTV (レンダーターゲットビュー) の初期化
	InitializeRTV();

	// 深度ステンシルビューの初期化 (リソース生成後)
	InitializeDSV();                 //  ビュー生成のみ

	// 深度バッファとDSV (深度ステンシルビュー) の初期化
	// InitializeDepthBuffer(); // DepthBufferの初期化は、この後のリファクタリングで追加される予定です。

	// フェンスの生成
	InitializeFence();

	InitializeViewport();

	InitializeScissor();

	// DXCコンパイラの生成
	InitializeDXC();

	// ImGuiの初期化
	InitializeImGui();


}

void DirectXCommon::PreDraw(){

	HRESULT hr;

	// コマンドアロケータのリセット
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));

	// コマンドリストのリセット (RootSignatureはnullptrのまま)
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));

	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// TransitionBarrierの設定 (PRESENT -> RENDER_TARGET)
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get(); // ComPtrなので .Get()
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &barrier);

	// 描画先のRTVを設定する (DSVも同時に設定)
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);

	// 画面クリア
	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

	// 深度バッファのクリア
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// ビューポートとシザリング矩形の設定 (メンバ変数を使用)
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);

	// 描画用ディスクリプタヒープの設定 (SRVヒープを指定)
	ID3D12DescriptorHeap* descripterHeaps[] = { srvDescriptorHeap_.Get() };
	commandList_->SetDescriptorHeaps(1, descripterHeaps);

}


void DirectXCommon::PostDraw()
{
	HRESULT hr;

	// 1. TransitionBarrierの設定 (RENDER_TARGET -> PRESENT)
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	// 現在のバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList_->ResourceBarrier(1, &barrier);

	// 2. コマンドリストをクローズ
	hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	// 3. GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);

	// 4. バックバッファとフロントバッファを入れ替え (Present)
	hr = swapChain_->Present(1, 0);
	assert(SUCCEEDED(hr));

	// 5. FenceによるGPU同期処理 (次のフレームの描画を待機)
	fenceValue_++;
	hr = commandQueue_->Signal(fence_.Get(), fenceValue_);
	assert(SUCCEEDED(hr));

	if (fence_->GetCompletedValue() < fenceValue_) {
		hr = fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		assert(SUCCEEDED(hr));
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// FPS固定
	UpdateFixFPS();

	
}


Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format, const Math::Vector4& clearColor)
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	// スライドの指示①：RenderTargetとして利用可能にする
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProperties{};

	// スライドの指示②：当然VRAM上に作る（DEFAULT）
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// クリアカラーの設定（これを設定しないとエラーになります）
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = format;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, // 初期状態はレンダーターゲット
		&clearValue,                        // クリアカラーを渡す
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}
