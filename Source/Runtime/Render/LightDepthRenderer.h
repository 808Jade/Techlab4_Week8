#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "Texture2D.h"
#include "RenderPacket.h"

struct FLightDepthConstants
{
	FMatrix LightViewProj;
};

struct FWorld {
	FMatrix World;
};

class FLightDepthRenderer
{
public:
	FLightDepthRenderer() = default;
	~FLightDepthRenderer() = default;

	FLightDepthRenderer(const FLightDepthRenderer&) = delete;
	FLightDepthRenderer operator=(const FLightDepthRenderer&) = delete;

	bool Init();
	void OnRender(const FMatrix& LightViewProj, const FRenderQueue& RenderTargets);
private:
	FPipelineState PipelineState;
	FShaderProgram* Shader = nullptr;
	TUniquePtr<FConstantBuffer> LightCB;
	TUniquePtr<FConstantBuffer> WorldCB;

};