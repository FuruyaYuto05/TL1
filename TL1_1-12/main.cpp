#define DIRECTINPUT_VERSION     0x0800
#include <dinput.h>

#include "Input.h"

#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <fstream>
#include <sstream>
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include <wrl.h>
#include "SpriteCommon.h"
#include "Sprite.h"
#include "Math.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "ModelManager.h"
#include "PostEffect.h"
#include "LevelLoader.h"


using namespace Microsoft::WRL;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

//ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}

	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0
	);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConverString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}


	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

IDxcBlob* CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler
)

{
	Log(ConverString(std::format(L"Begin CompliteShader,path:{},profile:{}\n", filePath, profile)));
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
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
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);
	assert(SUCCEEDED(hr));





	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		assert(false);
	}

	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	Log(ConverString(std::format(L"Compile Succeded, path:{}, profile:{}\n", filePath, profile)));
	shaderSource->Release();
	shaderResult->Release();
	return shaderBlob;

}


DirectX::ScratchImage LoadTexture(const std::string& filePath) {
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);

	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	return mipImages;
}

ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
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

	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {

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




// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	WinApp* winApp = nullptr;

	winApp = new WinApp();
	winApp->Initialize();

	DirectXCommon* dxCommon = nullptr;

	// DirectXの初期化
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

	TextureManager::GetInstance()->SetDirectXCommon(dxCommon);

	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("monsterBall.png");

	TextureManager::GetInstance()->Initialize();

	SpriteCommon* spriteCommon = nullptr;
	//スプライト共通部の初期化
	spriteCommon = new SpriteCommon;
	spriteCommon->Initialize(dxCommon);


	Object3dCommon* object3dCommon = nullptr;
	// 3Dオブジェクト共通部の初期化
	object3dCommon = new Object3dCommon();
	object3dCommon->Initialize(dxCommon);


	////DXGIファクトリーの作成
	IDXGIFactory7* dxgiFactory = nullptr;
	////HRESULTはWindows刑のエラーコードであり、
	////関数が成功したかどうかSUCCEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	////初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、同化にもできない場合が多いのでassertにしておく
	//assert(SUCCEEDED(hr));


	ID3D12Device* device = nullptr;

	//ポインタ
	Input* input = nullptr;
	//入力の初期化
	input = new Input();
	input->Initialize(winApp);


	//ポストエフェクト
	PostEffect* postEffect = nullptr;
	postEffect = new PostEffect();
	postEffect->Initialize(dxCommon);


	//HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//assert(fenceEvent != nullptr);

	////dxCompilerを初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	//hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	//assert(SUCCEEDED(hr));
	//hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	//assert(SUCCEEDED(hr));

	IDxcIncludeHandler* includeHandler = nullptr;
	//hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	//assert(SUCCEEDED(hr));


	//RootSignatureの作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//RootParmater作成
	D3D12_ROOT_PARAMETER rootParameters[3] = {};
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
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	//Samplerの設定
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
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	ID3D12RootSignature* rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature) // rootSignatureはまだローカル変数なので、&を維持
	);
	assert(SUCCEEDED(hr));

	D3D12_INPUT_ELEMENT_DESC inputElementDesc[2] = {};
	inputElementDesc[0].SemanticName = "POSITION";
	inputElementDesc[0].SemanticIndex = 0;
	inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDesc[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDesc[1].SemanticName = "TEXCOORD";
	inputElementDesc[1].SemanticIndex = 0;
	inputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDesc[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayOutDesc{};
	inputLayOutDesc.pInputElementDescs = inputElementDesc;
	inputLayOutDesc.NumElements = _countof(inputElementDesc);

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


	D3D12_RASTERIZER_DESC rastrizeDesc{};
	rastrizeDesc.CullMode = D3D12_CULL_MODE_BACK;
	rastrizeDesc.FillMode = D3D12_FILL_MODE_SOLID;


	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	/*graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;*/

	/*ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(
		&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState)
	);
	assert(SUCCEEDED(hr));*/

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	//D3D12_RESOURCE_DESC vertexResourceDesc{};




	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	//DirectX::ScratchImage mipImages = LoadTexture(modelData.material.textureFilePath);
	//const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	//ID3D12Resource* textureResource = CreateTextureResource(device, metadata);
	ComPtr<ID3D12Resource> textureResource = dxCommon->CreateTextureResource(mipImages.GetMetadata());

	//UploadTextureData(textureResource, mipImages);
	dxCommon->UploadTextureData(textureResource, mipImages);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	//srvDesc.Format = metadata.format;
	srvDesc.Format = mipImages.GetMetadata().format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);
	srvDesc.Texture2D.MipLevels = UINT(mipImages.GetMetadata().mipLevels);

	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	//textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//device->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandleCPU);
	dxCommon->GetDevice()->CreateShaderResourceView(
		textureResource.Get(), // ComPtrなので .Get()
		&srvDesc,
		dxCommon->GetSRVCPUDescriptorHandle(0) // 0番目のSRVヒープスロットを使用すると仮定
	);

	/*ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, WinApp::kClientWidth, WinApp::kClientHeight);
	ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());*/

	/*ComPtr<ID3D12Resource> vertexResourceSprite = dxCommon->CreateBufferResource(sizeof(VertexData) * 6);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};*/

	//vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();

	//vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;

	//vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	//VertexData* vertexDataSprite = nullptr;
	//vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	//1枚目の三角形
	//vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
	//vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	//vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	//vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	//vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };
	//vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	//vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f };
	//vertexDataSprite[3].texcoord = { 1.0f,0.0f };


	//ID3D12Resource* transformationMatrixResourceSprite = CreateBufferResource(device, sizeof(Matrix4x4));
	//ComPtr<ID3D12Resource> transformationMatrixResourceSprite = dxCommon->CreateBufferResource(sizeof(Matrix4x4));


	//Matrix4x4* transformationMatrixDataSprite = nullptr;

	//transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));

	//*transformationMatrixDataSprite = Math::MakeIdentity4x4();

	//Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };


	//ID3D12Resource* indexResourceSprite = CreateBufferResource(device, sizeof(uint32_t) * 6);
	ComPtr<ID3D12Resource> indexResourceSprite = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);

	//D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	//indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	//indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	//indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
	indexDataSprite[3] = 1; indexDataSprite[4] = 3; indexDataSprite[5] = 2;


	// 1. まず、使いたい画像のファイル名をリストにします
	std::vector<std::string> texFiles = {
		"resources/uvChecker.png",
		"monsterBall.png", // ← スライドで使っている別の画像のパスに変えてください
		"resources/uvChecker.png",
		"monsterBall.png",
		"resources/uvChecker.png"
	};

	// ※注意: ここで使う画像は、事前に TextureManager::LoadTexture で読み込んでおく必要があります！
	// もし読み込み済みでなければ、TextureManager::LoadTexture("resources/monsterBall.png"); などを追加してください。

	std::vector<Sprite*> sprites;

	for (uint32_t i = 0; i < 5; ++i) {
		Sprite* sprite = new Sprite();

		// 2. ループの番号 (i) に応じて、違うファイル名を取り出して渡します
		// texFiles[i] を使うのがポイントです！
		sprite->Initialize(spriteCommon, texFiles[i]);

		float x_position = 100.0f + 150.0f * i;
		sprite->SetPosition({ x_position, 100.0f });
		sprite->SetSize({ 128.0f, 128.0f }); // 見やすいサイズに設定

		sprites.push_back(sprite);
	}


	//std::vector<Sprite*> sprites;
	//for (uint32_t i = 0; i < 5; ++i) {
	//	Sprite* sprite = new Sprite();
	//	sprite->Initialize(spriteCommon, "resources/uvChecker.png");
	//	float x_position = 100.0f + 150.0f * i;
	//	sprite->SetPosition({ x_position, 100.0f });
	//	sprites.push_back(sprite);
	//}


	Sprite* sprite = new Sprite();
	// Initialize()に SpriteCommon などの必要な情報を渡す想定
	sprite->Initialize(spriteCommon, "resources/uvChecker.png");

	// 1. DirectXCommonの初期化のあと、TextureManagerなどの後が良いです
	ModelManager::GetInstance()->Initialize(dxCommon);

	// 2. ここでモデルをロードします（ファイルパスを指定）
	// これにより、ModelManager内の map にモデルデータが保管されます
	ModelManager::GetInstance()->LoadModel("plane.obj");
	// 他のモデルを使う場合はここに追加します
	ModelManager::GetInstance()->LoadModel("axis.obj");

	//フィールド
	ModelManager::GetInstance()->LoadModel("terrain.obj");

	//Object3d* object3d = new Object3d();
	//object3d->Initialize(object3dCommon);

	// 1体目のオブジェクト（左側）
	Object3d* object3d_1 = new Object3d();
	object3d_1->Initialize(object3dCommon);
	object3d_1->SetModel("plane.obj"); // 新しく作った文字列版の SetModel を使用
	object3d_1->SetTranslate({ -2.0f, 0.0f, 0.0f }); // 少し左にずらす
	object3d_1->SetRotate({ 0.0f, 3.14f, 0.0f });

	// 2体目のオブジェクト（右側）
	Object3d* object3d_2 = new Object3d();
	object3d_2->Initialize(object3dCommon);
	object3d_2->SetModel("axis.obj"); // 同じモデルデータを使い回す！
	object3d_2->SetTranslate({ 2.0f, 0.0f, 0.0f }); // 少し右にずらす
	object3d_2->SetRotate({ 0.0f, 0.0f, 0.0f });

	//フィールド用オブジェクト
	Object3d* terrainObj = new Object3d();
	terrainObj->Initialize(object3dCommon);
	terrainObj->SetModel("terrain.obj");
	terrainObj->SetTranslate({ 0.0f, -1.0f, 0.0f });
	terrainObj->SetRotate({ 0.0f, 0.0f, 0.0f });

	std::vector<Object3d*> levelObjects;

	// レベルデータ読み込み
	LevelData* levelData = LoadLevelData("blenderTest");

	// JSONのデータを元にObject3dを作る
	for (const LevelData::ObjectData& objectData : levelData->objects) {

		// file_name が空ならスキップ
		if (objectData.fileName.empty()) {
			continue;
		}

		// 必要なモデルを読み込む
		ModelManager::GetInstance()->LoadModel(objectData.fileName);

		Object3d* newObject = new Object3d();
		newObject->Initialize(object3dCommon);
		newObject->SetModel(objectData.fileName);

		newObject->SetTranslate({
			objectData.translation.x,
			objectData.translation.y,
			objectData.translation.z
			});

		// Blender側の回転は度数法なので、ゲーム側用にラジアンへ変換
		const float degToRad = 3.14159265f / 180.0f;

		newObject->SetRotate({
			objectData.rotation.x * degToRad,
			objectData.rotation.y * degToRad,
			objectData.rotation.z * degToRad
			});

		newObject->SetScale({
	        objectData.scaling.x,
	        objectData.scaling.y,
	        objectData.scaling.z
			});

		levelObjects.push_back(newObject);
	}

	//ウィンドウの×ボタンが押されるまでループ
	while (true) {

		// 変更: WinApp::ProcessMessage() にメッセージ処理を任せる
		// ProcessMessage() が true を返したら WM_QUIT が来たということ
		if (winApp->ProcessMessage()) {
			break; // ゲームループを抜ける
		}
		{
			//入力の更新  <= ❌ 初期化時に一度だけ呼ばれている
			input->Update();

			//postEffect切り替え
			if (input->Pushkey(DIK_1)) {
				postEffect->SetEffect(PostEffectType::CopyImage);
			}

			if (input->Pushkey(DIK_2)) {
				postEffect->SetEffect(PostEffectType::Grayscale);
			}

			if (input->Pushkey(DIK_3)) {
				postEffect->SetEffect(PostEffectType::Vignette);
			}

			if (input->Pushkey(DIK_4)) {
				postEffect->SetEffect(PostEffectType::BoxFilter);
			}

			if (input->Pushkey(DIK_5)) {
				postEffect->SetEffect(PostEffectType::GaussianFilter);
			}

			if (input->Pushkey(DIK_6)) {
				postEffect->SetEffect(PostEffectType::LuminanceBasedOutline);
			}

			if (input->Pushkey(DIK_7)) {
				postEffect->SetEffect(PostEffectType::DepthBasedOutline);
			}

			//object3d->Update();

			// 更新
			//object3d_1->Update();
			//object3d_2->Update();
			//terrainObj->Update();

			for (Object3d* object : levelObjects) {
				object->Update();
			}

			if (input->Pushkey(DIK_0)) {
				OutputDebugStringA("Hit 0\n");
			}

			// [NEW] Sprite::Update() を呼び出す
			sprite->Update();

			// スプライトの更新
			// vector の要素を一つずつ取り出す
			for (Sprite* sprite : sprites) {

				// 例: 特定のスプライトのみを動かす、または共通のロジック
				// sprite->SetRotation(sprite->GetRotation() + 0.01f);

				// 座標変換行列の再計算
				sprite->Update();
			}


			/*Matrix4x4 worldMatrixSprite = Math::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = Math::MakeIdentity4x4();
			Matrix4x4 projectionMatrixSprite = MakeOrthorgraphicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixSprite = Math::Multiply(worldMatrixSprite, Math::Multiply(viewMatrixSprite, projectionMatrixSprite));
			*transformationMatrixDataSprite = worldViewProjectionMatrixSprite;*/

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			//transform.rotate.y += 0.03f;
		/*	Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 cameraMatrix = Math::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = Math::Multiply(worldMatrix, Math::Multiply(viewMatrix, projectionMatrix));
			*wvpData = worldViewProjectionMatrix;*/

			//*wvpData = worldMatrix;



			//開発用UIの処理
			ImGui::ShowDemoWindow();

			ImGui::Begin("Settings");
			//ImGui::ColorEdit4("material", &materialData->x, ImGuiColorEditFlags_AlphaPreview);

			//ImGui::DragFloat3("Object rotate.y", &transform.rotate.x, 0.1f);
			//ImGui::DragFloat3("Object transform", &transform.translate.x, 0.1f);
			//ImGui::DragFloat3("Object scale", &transform.scale.x, 0.1f);

			//ImGui::DragFloat3("Sprite rotate.y", &transformSprite.rotate.x, 0.1f);
			//ImGui::DragFloat3("Sprite transform", &transformSprite.translate.x, 1.1f);
			//ImGui::DragFloat3("Sprite scale", &transformSprite.scale.x, 0.1f);

			// 1. 現在の位置を取得
			Math::Vector2 currentPos = sprite->GetPosition();
			Math::Vector3 currentRot = sprite->GetRotation();

			Math::Vector2 currentSize = sprite->GetSize();

			// 2. DragFloat2 で編集（ここでは Vector3ではなく Vector2 が適切）
			// ImGui::DragFloat2 を使って位置を編集します。
			if (ImGui::DragFloat2("Position", &currentPos.x, 1.0f))
			{
				// 3. 編集された値を Setter で設定
				sprite->SetPosition(currentPos);
			}


			// --- Rotation (Vector3) ---
			if (ImGui::DragFloat3("Rotation", &currentRot.x, 0.01f)) {
				// [FIX] Vector3 の変数を Setter に渡す
				sprite->SetRotation(currentRot);
			}

			// [NEW] Color の Getter/Setter を使った ImGui ウィジェットを追加
			Math::Vector4 currentColor = sprite->GetColor();

			// DragFloat4 ではなく ColorEdit4 を使うことで、カラーピッカーが表示される
			if (ImGui::ColorEdit4("Color", &currentColor.x, ImGuiColorEditFlags_AlphaPreview)) {
				// 編集された値を Setter で設定
				sprite->SetColor(currentColor);
			}

			if (ImGui::DragFloat2("Size", &currentSize.x, 0.1f, 0.1f, 1000.0f)) // 1.0f 単位で、最小 1.0f までの制限
			{
				sprite->SetSize(currentSize);
			}

			ImGui::Text("--- 3D Object ---");

			// 現在の3Dオブジェクトの位置を取得
			Math::Vector3 planeobjPos = object3d_1->GetTranslate();

			Math::Vector3 axisobjPos = object3d_2->GetTranslate();

			// DragFloat3 を使って、X, Y, Z の3つの値を操作できるようにする
			if (ImGui::DragFloat3("Planeobj Position", &planeobjPos.x, 0.1f)) {
				// 操作されたら、新しい位置をセットし直す
				object3d_1->SetTranslate(planeobjPos);
			}
			// 回転の操作
			Math::Vector3 planeobjRot = object3d_1->GetRotate();
			if (ImGui::DragFloat3("Planeobj Rotation", &planeobjRot.x, 0.01f)) {
				object3d_1->SetRotate(planeobjRot);
			}

			if (ImGui::DragFloat3("axisobj Position", &axisobjPos.x, 0.1f)) {
				// 操作されたら、新しい位置をセットし直す
				object3d_2->SetTranslate(axisobjPos);
			}
			Math::Vector3 axisobjRot = object3d_1->GetRotate();
			if (ImGui::DragFloat3("axisobj Rotation", &axisobjRot.x, 0.01f)) {
				object3d_2->SetRotate(axisobjRot);
			}

			ImGui::End();

			//ImGuiの内部コマンド生成
			ImGui::Render();

			// --- 描画前処理 ---
			dxCommon->PreDraw();

			postEffect->PreDraw();

			// 3Dオブジェクトの描画準備。3Dオブジェクトの描画に共通のグラフィックスコマンドを積む
			object3dCommon->SetCommonDrawSetting();

			//object3d->Draw();

			// 描画
			//object3d_1->Draw();
			//object3d_2->Draw();
			//terrainObj->Draw();

			for (Object3d* object : levelObjects) {
				object->Draw();
			}

			// Todo: 全てのObject3d個々の描画


			////これから書き込むバックバッファのインデックスを取得
			//UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();



			////TransitionBarrierの設定
			//D3D12_RESOURCE_BARRIER barrier{};

			//barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			//barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			//barrier.Transition.pResource = swapChainResources[backBufferIndex];
			//barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			//barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			//commandList->ResourceBarrier(1, &barrier);


			//commandList->IASetIndexBuffer(&indexBufferViewSprite);



			////描画先のRTVを設定する
			//commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
			////指定した色で画面全体をクリアする
			//float clearColor[] = { 0.1f,0.25,0.5f,1.0f };
			//commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

			//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			//commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

			////描画用ディスクリプタヒープの設定
			//ID3D12DescriptorHeap* descripterHeaps[] = { srvDescriptorHeap };
			//commandList->SetDescriptorHeaps(1, descripterHeaps);

			//commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			//commandList->RSSetViewports(1, &viewport);
			//commandList->RSSetScissorRects(1, &scissorRect);
			//dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature);
			//commandList->SetPipelineState(graphicsPipelineState);
			//dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState);
			//dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
			//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			////マテリアルCBufferの場所を設定
			//commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

			////wvp用のCBufferの場所を設定
			//commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
			////srv用のCBufferの場所を設定
			//commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress()); // Material CBV
			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress()); // WVP CBV
			//dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, dxCommon->GetSRVGPUDescriptorHandle(0)); // SRV (Texture) Binding

			//commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
			//commandList->DrawInstanced(6, 1, 0, 0);

			//dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

			// --------------------------------------------------------------------------------------
			// 【スプライト描画の準備】
			// --------------------------------------------------------------------------------------
			// スプライト描画の共通設定を積む
			spriteCommon->SetCommonDrawSettings(dxCommon->GetCommandList());

			// [NEW] Sprite::Draw() を呼び出し、個別の描画コマンドを積む
			//sprite->Draw(dxCommon->GetCommandList());

			// 2. 個別スプライトの描画
   //        vector の要素を一つずつ取り出し、Drawを呼び出す
			//for (Sprite* sprite : sprites) {
			//	sprite->Draw(dxCommon->GetCommandList());
			//}


			postEffect->PostDraw();

			postEffect->Draw();


			//commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);

			//commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());

			//commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);



			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

			//barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			//barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			//commandList->ResourceBarrier(1, &barrier);

			//hr = commandList->Close();
			//assert(SUCCEEDED(hr));


			////GPUにコマンドリストの実行を行わせる
			//ID3D12CommandList* commandLists[] = { commandList };
			//commandQueue->ExecuteCommandLists(1, commandLists);
			//swapChain->Present(1, 0);

			////Fenceの値を更新
			//fenceValue++;
			//commandQueue->Signal(fence, fenceValue);

			//if (fence->GetCompletedValue() < fenceValue) {
			//	fence->SetEventOnCompletion(fenceValue, fenceEvent);
			//	WaitForSingleObject(fenceEvent, INFINITE);
			//}

			//hr = commandAllocator->Reset();
			//assert(SUCCEEDED(hr));
			//hr = commandList->Reset(commandAllocator, nullptr);
			//assert(SUCCEEDED(hr));


			dxCommon->PostDraw();
		}

	}

	//変数から型を推測する

	Log(ConverString(std::format(L"WSTRING{}\n", L"abc")));



	//出力ウィンドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	//delete object3d;

	delete object3d_1;
	delete object3d_2;
	delete terrainObj;

	for (Object3d* object : levelObjects) {
		delete object;
	}

	delete levelData;

	delete sprite;

	delete input;

	delete postEffect;


	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//indexResourceSprite->Release();

	//transformationMatrixResourceSprite->Release();
	//vertexResourceSprite->Release();

	/*dsvDescriptorHeap->Release();
	depthStencilResource->Release();*/

	//srvDescriptorHeap->Release();

	//wvpResource->Release();
	//materialResourceの開放処理
	//materialResource->Release();

	//vertexResource->Release();
	//graphicsPipelineState->Release();
	signatureBlob->Release();
	if (errorBlob) {
		errorBlob->Release();
	}
	//rootSignature->Release();
	//pixelShaderBlob->Release();
	//vertexShaderBlob->Release();




	//fence->Release();
	//rtvDescriptorHeap->Release();
	//swapChainResources[0]->Release();
	//swapChainResources[1]->Release();
	//swapChain->Release();
	//commandList->Release();
	//commandAllocator->Release();
	//commandQueue->Release();
	//device->Release();
	//useAdapter->Release();
	//dxgiFactory->Release();

	//delete spriteCommon;

	// スプライトの解放
	for (Sprite* sprite : sprites) {
		if (sprite) {
			delete sprite; // newしたオブジェクトを解放
		}
	}

	delete object3dCommon;

	ModelManager::GetInstance()->Finalize();

	//windowsAPIの終了処理
	winApp->Finalize();

	TextureManager::GetInstance()->Finalize();

	delete dxCommon;

	delete winApp;
	winApp = nullptr;

	/*IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}*/


	//CloseHandle(fenceEvent);
	return 0;
}
