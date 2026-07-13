#pragma once

// 必要な前方宣言やインクルードを忘れずに
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>          // ComPtrを使うために必要
#include <dxcapi.h>
#include <Windows.h>      // HANDLE用
#include <cstdint>
#include <string>
#include "WinApp.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <chrono>
#include "Math.h"

// ComPtrのために名前空間を省略せず記述するために、ここでは using namespace は使わない

class DirectXCommon
{
public:
	void Initialize(WinApp* winApp);

	static const uint32_t kMaxSRVCount;

	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }

	ID3D12Resource* GetDepthStencilResource() const { return depthStencilResource_.Get(); }

	/// <summary>
	/// バッファリソースの生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	/// <summary>
	/// テクスチャリソースの生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
	
	/// <summary>
	/// テクスチャファイルの読み込み
	/// </summary>
	DirectX::ScratchImage LoadTexture(const std::string& filePath);

	// RenderTexture生成用関数
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format, const Math::Vector4& clearColor);


	// RTVの特定のインデックスのハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle(uint32_t index) const {
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += (index * rtvDescriptorSize_);
		return handle;
	}

	// DSVのハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle() const {
		return dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	}

	// 現在のバックバッファ番号を取得
	UINT GetBackBufferIndex() const {
		return swapChain_->GetCurrentBackBufferIndex();
	}

	// --- リソース転送関数を追加 ---
/// <summary>
/// テクスチャデータの転送
/// </summary>
	void UploadTextureData(
		Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
		const DirectX::ScratchImage& mipImages);

	// --- シェーダーコンパイル関数を追加 ---
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		const std::wstring& filePath,
		const wchar_t* profile);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		UINT numDescriptors,
		bool shaderVisible);

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
		uint32_t descriptorSize,
		uint32_t index);

	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
		uint32_t descriptorSize,
		uint32_t index);

	/// <summary>
	/// SRVの指定番号のCPUディスクリプタハンドルを取得する
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index) const;

	/// <summary>
	/// SRVの指定番号のGPUディスクリプタハンドルを取得する
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index) const;

	ID3D12DescriptorHeap* GetSRVDescriptorHeap() const { return srvDescriptorHeap_.Get(); }
	void PreDraw();
	void PostDraw();


private:

	WinApp* winApp_ = nullptr;



	// --- DirectX 基盤オブジェクト (ComPtr へ変更) ---
	
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;  
	
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;   
	Microsoft::WRL::ComPtr<ID3D12Device> device_;       
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_; 
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_; 
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_; 
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_; 
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2]; 

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_; 
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_; 

	// --- Fence (ComPtr へ変更) ---
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_; 
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	// --- DXCompiler (ComPtr へ変更) ---
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_; 
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_; 
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_; 

	// --- RTV Handles (生ポインタのまま) ---
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];

	// --- depthStencilResource_ ---
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

	// --- RTV/SRV/DSVのディスクリプタサイズ ---
	uint32_t rtvDescriptorSize_ = 0; // 
	uint32_t srvDescriptorSize_ = 0; // 
	uint32_t dsvDescriptorSize_ = 0; // 

	// --- FPS固定用の変数 ---
	std::chrono::steady_clock::time_point reference_;

	D3D12_VIEWPORT viewport_;
	D3D12_RECT scissorRect_;


	// --- 新しいプライベート関数 (Initializeから分割) ---
	
	void InitializeDevice();
	void InitializeCommand();     // コマンド関連 (キュー、アロケータ、リスト) の作成
	void InitializeDescriptorHeaps();
	void InitializeSwapchain();   // スワップチェーン、RTVヒープ、RTVの作成
	void InitializeRTV();         // RTVの設定と作成
	void InitializeDepthBufferResource(); 
	void InitializeDSV();               
	void InitializeFence();       // フェンスの作成
	void InitializeViewport();
	void InitializeScissor(); 
	void InitializeDXC();         // DXCコンパイラの初期化
	void InitializeImGui();       // ImGuiのDirectX部分の初期化


	void InitializeFixFPS();
	// FPS固定更新
	void UpdateFixFPS();

};
