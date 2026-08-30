#include "blur.h"

#include <memory>
#include <array>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

static constexpr const char* BLUR_X_SHADER = R"(
Texture2D tex : register(t0);
Texture2D noiseTex : register(t1);
SamplerState samp : register(s0);

cbuffer Constants : register(b0) {
    float pixelSize;
    float2 noiseScale;   // e.g. float2(128.0, 128.0)
    float2 noiseOffset;  // Can animate or keep static
    float noiseOpacity;  // e.g. 0.05
}

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    float4 color = 0;
    float total_weight = 0;

    for (float x = -10; x <= 10; x++) {
        float weight = exp(-(x * x) / 2056);
        color += tex.Sample(samp, uv + float2(x * pixelSize, 0)) * weight;
        total_weight += weight;
    }

    float4 blurred = color / total_weight;

    // Sample noise
    float noise = noiseTex.Sample(samp, uv * noiseScale + noiseOffset).r;

    // Add noise as grain (grayscale applied to RGB)
    blurred.rgb += noise * noiseOpacity;

    return blurred;
}
)";


static constexpr const char* BLUR_Y_SHADER = R"(
Texture2D tex : register(t0);
Texture2D noiseTex : register(t1);
SamplerState samp : register(s0);

cbuffer Constants : register(b0) {
    float pixelSize;
    float2 noiseScale;   // e.g. float2(128.0, 128.0)
    float2 noiseOffset;  // Can animate or keep static
    float noiseOpacity;  // e.g. 0.05
}

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    float4 color = 0;
    float total_weight = 0;

    for (float y = -10; y <= 10; y++) {
        float weight = exp(-(y * y) / 2056);
        color += tex.Sample(samp, uv + float2(0, y * pixelSize)) * weight;
        total_weight += weight;
    }


    float4 blurred = color / total_weight;

    // Sample noise
    float noise = noiseTex.Sample(samp, uv * noiseScale + noiseOffset).r;

    // Add noise as grain (grayscale applied to RGB)
    blurred.rgb += noise * noiseOpacity;

    return blurred;
}
)";



bool DX11BlurEffect::CreateShaders() { // (Same as before)
	HRESULT hr = S_OK;
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	// Compile X shader
	hr = D3DCompile(
		BLUR_X_SHADER,
		strlen(BLUR_X_SHADER),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION |
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&shaderBlob,
		&errorBlob
	);

	if (FAILED(hr)) {
		if (errorBlob) {
			std::cout << "Failed to compile X shader: "
				<< (char*)errorBlob->GetBufferPointer();
			errorBlob->Release();
		}
		return false;
	}

	// Create X pixel shader
	hr = device->CreatePixelShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&blurShaderX
	);

	if (shaderBlob) shaderBlob->Release();

	if (FAILED(hr)) {
		std::cout << "Failed to create blur X shader: HR: " << hr;
		return false;
	}

	// Compile Y shader
	hr = D3DCompile(
		BLUR_Y_SHADER,
		strlen(BLUR_Y_SHADER),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION |
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&shaderBlob,
		&errorBlob
	);

	if (FAILED(hr)) {
		if (errorBlob) {
			std::cout << "Failed to compile Y shader: "
				<< (char*)errorBlob->GetBufferPointer();
			errorBlob->Release();
		}
		return false;
	}

	// Create Y pixel shader
	hr = device->CreatePixelShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&blurShaderY
	);

	if (shaderBlob) shaderBlob->Release();
	if (errorBlob) errorBlob->Release();

	if (FAILED(hr)) {
		std::cout << "Failed to create blur Y shader: HR: " << hr;
		return false;
	}

	return true;
}

bool DX11BlurEffect::CreateSamplerState() { // (Same as before)
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = device->CreateSamplerState(&samplerDesc, &samplerState);
	if (FAILED(hr)) {
		std::cout << "Failed to create sampler state: HR: " << hr;
		return false;
	}

	return true;
}


bool DX11BlurEffect::CreateBlurTextures(int width, int height) {
	if (blurTextureX && blurTextureY && backbufferWidth == width && backbufferHeight == height)
		return true;

	auto create_texture_resources = [&](ID3D11Texture2D** texture, ID3D11ShaderResourceView** srv, ID3D11RenderTargetView** rtv, std::string debug_name) -> bool {
		if (*texture) (*texture)->Release(); *texture = nullptr;
		if (*srv) (*srv)->Release(); *srv = nullptr;
		if (*rtv) (*rtv)->Release(); *rtv = nullptr;

		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, texture);
		if (FAILED(hr)) { std::cout << ("Failed to create blur texture (" + debug_name + "): HR: ").c_str() << hr; return false; }
		hr = device->CreateShaderResourceView(*texture, nullptr, srv);
		if (FAILED(hr)) { std::cout << ("Failed to create blur SRV (" + debug_name + "): HR: ").c_str() << hr; return false; }
		hr = device->CreateRenderTargetView(*texture, nullptr, rtv);
		if (FAILED(hr)) { std::cout << ("Failed to create blur RTV (" + debug_name + "): HR: ").c_str() << hr; return false; }
		return true;
		};

	if (!create_texture_resources(&blurTextureX, &blurSRVX, &blurRTVX, "X")) return false;
	if (!create_texture_resources(&blurTextureY, &blurSRVY, &blurRTVY, "Y")) return false;

	// Create the original blurTexture as well (though might not be directly used in updated apply_blur)
	if (!create_texture_resources(&blurTexture, &blurSRV, &blurRTV, "Original")) return false;


	return true;
}

// Set up constants for the shaders
struct BlurConstants {
	float pixelSize[4];  // Using float4 for constant buffer alignment
};

bool DX11BlurEffect::initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx) { // (Updated to call CreateBlurTextures and CreateFullscreenQuadResources)
	if (!dev || !ctx) {
		std::cout << "Invalid DirectX device or context provided";
		return false;
	}

	device = dev;
	context = ctx;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(BlurConstants); // Define BlurConstants struct (see below)
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = device->CreateBuffer(&desc, nullptr, &constantBuffer);
	if (FAILED(hr)) {
		std::cout << "Failed to create constant buffer: HR: " << hr << std::endl;
		return false; // Or handle the error as you see fit
	}


	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE; // Or whatever your normal depth setting is
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // Your normal setting
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // Or your normal setting
	// ... set other stencil settings as needed

	hr = device->CreateDepthStencilState(&depthStencilDesc, &defaultDepthStencilState); // Store this

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // No depth writes for blur
	hr = device->CreateDepthStencilState(&depthStencilDesc, &noDepthWriteBlurState); // Store this
	if (FAILED(hr)) {
		std::cout << "Failed to create depth stencil states: " << hr << std::endl;
		return false;
	}

	if (!CreateShaders() || !CreateSamplerState() || !CreateBlurTextures(1, 1) || !CreateFullscreenQuadResources()) {
		std::cout << "Failed to initialize blur effect";
		return false;
	}

	isInitialized = true;
	return true;
}

void DX11BlurEffect::begin_blur() {
	if (!isInitialized || !device || !context) {
		std::cout << "Blur effect not initialized";
		return;
	}

	ID3D11RenderTargetView* currentRTV = nullptr;
	context->OMGetRenderTargets(1, &currentRTV, nullptr);
	if (!currentRTV) {
		std::cout << "No active render target";
		return;
	}

	ID3D11Texture2D* currentRT = nullptr;
	currentRTV->GetResource(reinterpret_cast<ID3D11Resource**>(&currentRT));
	if (!currentRT) {
		currentRTV->Release();
		std::cout << "Failed to get render target texture";
		return;
	}

	D3D11_TEXTURE2D_DESC desc;
	currentRT->GetDesc(&desc);

	if (backbufferWidth != desc.Width || backbufferHeight != desc.Height) {
		if (!CreateBlurTextures(desc.Width, desc.Height)) {
			currentRT->Release();
			currentRTV->Release();
			return;
		}
		backbufferWidth = desc.Width;
		backbufferHeight = desc.Height;
	}

	context->OMGetRenderTargets(1, &rtBackup, nullptr);

	if (blurTexture && currentRT) {
		context->CopyResource(blurTexture, currentRT);
	}

	currentRT->Release();
	currentRTV->Release();

	if (!isInitialized) {
		std::cout << "Blur effect not initialized";
		return;
	}

	// Clear blur textures
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(blurRTVX, clearColor);
	context->ClearRenderTargetView(blurRTVY, clearColor);

	// Disable depth writes
	context->OMSetDepthStencilState(noDepthWriteBlurState, 0);

	// Update constant buffer contents
	BlurConstants constants;
	D3D11_MAPPED_SUBRESOURCE mapped;

	// **1. Horizontal Blur Pass to blurTextureX**
	ID3D11RenderTargetView* rtvX[] = { blurRTVX };
	context->OMSetRenderTargets(1, rtvX, nullptr);
	context->PSSetShader(blurShaderX, nullptr, 0);
	context->PSSetConstantBuffers(0, 1, &constantBuffer);
	context->PSSetSamplers(0, 1, &samplerState);
	context->PSSetShaderResources(0, 1, &blurSRV);

	{
		BlurConstants constants = { {1.0f / backbufferWidth, 0.0f, 0.0f, 0.0f} };
		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr)) { /* Handle error */ }
		memcpy(mapped.pData, &constants, sizeof(constants));
		context->Unmap(constantBuffer, 0);
	}

	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(backbufferWidth), static_cast<float>(backbufferHeight), 0.0f, 1.0f };
	context->RSSetViewports(1, &viewport);
	DrawFullscreenQuad();

	ID3D11ShaderResourceView* nullSRV[] = { nullptr };
	context->PSSetShaderResources(0, 1, nullSRV);
	ID3D11RenderTargetView* nullRTV[] = { nullptr };
	context->OMSetRenderTargets(1, nullRTV, nullptr);


	// **2. Vertical Blur Pass to blurTextureY**
	ID3D11RenderTargetView* rtvY[] = { blurRTVY };
	context->OMSetRenderTargets(1, rtvY, nullptr);
	context->PSSetShader(blurShaderY, nullptr, 0);
	context->PSSetConstantBuffers(0, 1, &constantBuffer);
	context->PSSetSamplers(0, 1, &samplerState);
	context->PSSetShaderResources(0, 1, &blurSRVX);

	{
		BlurConstants constants = { {1.0f / backbufferHeight, 0.0f, 0.0f, 0.0f} };
		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr)) { /* Handle error */ }
		memcpy(mapped.pData, &constants, sizeof(constants));
		context->Unmap(constantBuffer, 0);
	}

	context->RSSetViewports(1, &viewport);
	DrawFullscreenQuad();

	context->OMSetRenderTargets(1, nullRTV, nullptr);


	// **3. Render the *blurred texture (blurTextureY)* to ImGui**
	context->OMSetRenderTargets(1, &rtBackup, nullptr);
	viewport = { 0.0f, 0.0f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f };
	context->RSSetViewports(1, &viewport);

	// Restore depth state
	context->OMSetDepthStencilState(defaultDepthStencilState, 0);
}

bool isTextureWhite(ID3D11Device* dev, ID3D11DeviceContext* context, ID3D11ShaderResourceView* srv) {
	ID3D11Resource* resource = nullptr;
	srv->GetResource(&resource);
	if (!resource) return false;

	ID3D11Texture2D* texture = nullptr;
	bool isWhite = true;
	if (SUCCEEDED(resource->QueryInterface(&texture))) {
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);

		// Create a staging texture to read from
		D3D11_TEXTURE2D_DESC stagingDesc = desc;
		stagingDesc.Usage = D3D11_USAGE_STAGING;
		stagingDesc.BindFlags = 0;
		stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		stagingDesc.MiscFlags = 0;

		ID3D11Texture2D* stagingTex = nullptr;
		HRESULT hr = dev->CreateTexture2D(&stagingDesc, nullptr, &stagingTex);
		if (SUCCEEDED(hr)) {
			context->CopyResource(stagingTex, texture);

			D3D11_MAPPED_SUBRESOURCE mapped;
			if (SUCCEEDED(context->Map(stagingTex, 0, D3D11_MAP_READ, 0, &mapped))) {
				unsigned char* data = reinterpret_cast<unsigned char*>(mapped.pData);
				for (UINT y = 0; y < desc.Height; ++y) {
					unsigned char* row = data + y * mapped.RowPitch;
					for (UINT x = 0; x < desc.Width * 4; x += 4) {
						// Check if any pixel is not pure white
						if (row[x] != 255 || row[x + 1] != 255 || row[x + 2] != 255) {
							isWhite = false;
							break;
						}
					}
					if (!isWhite) break;
				}
				context->Unmap(stagingTex, 0);
			}
			stagingTex->Release();
		}
		texture->Release();
	}
	resource->Release();
	return isWhite;
}

void DX11BlurEffect::apply_blur(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size, float radius, float rounding, ImDrawFlags flags) {
	if (!isInitialized) {
		std::cout << "Blur effect not initialized";
		return;
	}

	auto textureID = reinterpret_cast<ImTextureID>(blurSRVY);
	if (textureID != NULL) {
		ImVec2 screenSize = ImGui::GetIO().DisplaySize;

		// Compute UVs based on the window's position and size
		ImVec2 uv0(pos.x / screenSize.x, pos.y / screenSize.y);
		ImVec2 uv1((pos.x + size.x) / screenSize.x, (pos.y + size.y) / screenSize.y);

		drawList->AddImageRounded(
			textureID, // Use blurSRVY (Y texture SRV) for final blurred image!
			pos,
			ImVec2(pos.x + size.x, pos.y + size.y),
			uv0,
			uv1,
			ImColor(ImVec4(1.f, 1.f, 1.f, 1.f)),
			rounding,
			flags
		);
	}
	else {
		//L_PRINT(LOG_ERROR) << "Invalid Texture!";
	}


	//if (constantBuffer) constantBuffer->Release(); // Consider reusing constant buffer
}

void DX11BlurEffect::end_blur() { // (Same as before)
	if (!isInitialized) {
		std::cout << "Blur effect not initialized";
		return;
	}

	if (rtBackup) {
		context->OMSetRenderTargets(1, &rtBackup, nullptr);
		rtBackup->Release();
		rtBackup = nullptr;
	}

	// Reset shader state
	context->PSSetShader(nullptr, nullptr, 0);
	ID3D11SamplerState* nullSampler = nullptr;
	context->PSSetSamplers(0, 1, &nullSampler);
}


// Definition of FullscreenQuadVertex structure (inside cpp file, no need to be in header if only used here)
struct DX11BlurEffect::FullscreenQuadVertex {
	float position[2]; // In clip space, so only 2D needed
};


bool DX11BlurEffect::CreateFullscreenQuadResources() { // (Same as provided before)
	// 1. Vertex Buffer
	FullscreenQuadVertex vertices[] = {
		{-1.0f,  1.0f}, // Top-left
		{ 1.0f,  1.0f}, // Top-right
		{-1.0f, -1.0f}, // Bottom-left
		{ 1.0f, -1.0f}  // Bottom-right
	};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = sizeof(vertices);
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vbData = { vertices, 0, 0 };

	HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &fullscreenQuadVertexBuffer);
	if (FAILED(hr)) {
		std::cout << "Failed to create fullscreen quad vertex buffer: HR: " << hr;
		return false;
	}

	// 2. Input Layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Vertex Shader for Fullscreen Quad
	static constexpr const char* FULLSCREEN_QUAD_VS = R"(
        struct VS_OUTPUT
        {
            float4 position : SV_POSITION;
            float2 uv : TEXCOORD0;
        };

        VS_OUTPUT main(float2 position : POSITION)
        {
            VS_OUTPUT output;
            output.position = float4(position, 0.0f, 1.0f);
            output.uv = float2((position.x + 1.0f) * 0.5f, 1.0f - (position.y + 1.0f) * 0.5f);
            return output;
        }
    )";


	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* errorBlobVS = nullptr;

	hr = D3DCompile(
		FULLSCREEN_QUAD_VS,
		strlen(FULLSCREEN_QUAD_VS),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&vsBlob,
		&errorBlobVS
	);

	if (FAILED(hr)) {
		if (errorBlobVS) {
			std::cout << "Failed to compile fullscreen quad VS: " << (char*)errorBlobVS->GetBufferPointer();
			errorBlobVS->Release();
		}
		return false;
	}


	hr = device->CreateVertexShader(
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		nullptr,
		&fullscreenQuadVertexShader
	);
	if (FAILED(hr)) {
		std::cout << "Failed to create fullscreen quad vertex shader: HR: " << hr;
		return false;
	}


	hr = device->CreateInputLayout(
		layout,
		numElements,
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		&fullscreenQuadInputLayout
	);

	if (vsBlob) vsBlob->Release();
	if (errorBlobVS) errorBlobVS->Release();


	if (FAILED(hr)) {
		std::cout << "Failed to create fullscreen quad input layout: HR: " << hr;
		return false;
	}
	return true;
}


void DX11BlurEffect::DrawFullscreenQuad() { // (Same as provided before)
	if (!fullscreenQuadVertexBuffer || !fullscreenQuadVertexShader) {
		std::cout << "Fullscreen quad resources not initialized. Call CreateFullscreenQuadResources first.";
		return;
	}

	UINT stride = sizeof(FullscreenQuadVertex);
	UINT offset = 0;

	context->IASetVertexBuffers(0, 1, &fullscreenQuadVertexBuffer, &stride, &offset);
	context->IASetInputLayout(fullscreenQuadInputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->VSSetShader(fullscreenQuadVertexShader, nullptr, 0);


	context->Draw(4, 0);
}


DX11BlurEffect::~DX11BlurEffect() { // (Updated destructor to release new resources)
	if (blurShaderX) blurShaderX->Release();
	if (blurShaderY) blurShaderY->Release();
	if (samplerState) samplerState->Release();
	if (blurTexture) blurTexture->Release();
	if (blurSRV) blurSRV->Release();
	if (blurRTV) blurRTV->Release();
	if (blurTextureX) blurTextureX->Release();
	if (blurSRVX) blurSRVX->Release();
	if (blurRTVX) blurRTVX->Release();
	if (blurTextureY) blurTextureY->Release();
	if (blurSRVY) blurSRVY->Release();
	if (blurRTVY) blurRTVY->Release();
	if (fullscreenQuadVertexBuffer) fullscreenQuadVertexBuffer->Release();
	if (fullscreenQuadVertexShader) fullscreenQuadVertexShader->Release();
	if (fullscreenQuadInputLayout) fullscreenQuadInputLayout->Release();

	if (constantBuffer) {
		constantBuffer->Release();
		constantBuffer = nullptr; // Good practice to set to nullptr after releasing
	}
}