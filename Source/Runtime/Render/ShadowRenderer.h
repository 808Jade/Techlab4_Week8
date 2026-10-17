#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "Texture2D.h"
#include "RenderingInfo.h"
#include "RenderPacket.h"
#include "LightInfo.h"
#include "Renderer.h"
#include "TextureCube.h"

class FDirectionalLightSceneProxy;
class FSpotLightSceneProxy;
class FPointLightSceneProxy;

struct FDirectionalShadowConstants
{
	FMatrix DirectionalLightViewProjection;
	float DirectionalShadowBias = 0.0005f;
	uint32 bCastDirectionalShadow = 0;
	float DirectionalShadowTexelSize = 1.953125f;
	float DirectionalNormalBias = 1.0f;
};
static_assert(sizeof(FDirectionalShadowConstants) == 80);

struct FSpotShadowConstants
{
	FMatrix SpotLightViewProjection;
	float SpotShadowBias = 0.001f;
	int32 ShadowSpotLightIndex = -1;
	float SpotShadowTexelSize = 1.0f;
	float SpotNormalBias = 1.0f;
};
static_assert(sizeof(FSpotShadowConstants) == 80);

struct FPointShadowConstants
{
	FVector PointLightPosition;        
	float PointShadowBias = 0.001f;    
	float PointShadowNormalBias = 1.0f;
	float PointAttenuationRadius;      
	int32 ShadowPointLightIndex = -1;
	float Padding;
};
static_assert(sizeof(FPointShadowConstants) == 32);

enum class EShadowProjectionType : uint8
{
	Directional, 
	Spot,        
	Point        
};

struct FProjectedShadowInfo
{
	EShadowProjectionType ProjectionType = EShadowProjectionType::Directional;
	// 1. 빛 시점의 카메라 정보 (메시 드로우 시 VS 상수버퍼 b0에 들어감)
	FMatrix LightViewMatrix;
	FMatrix LightProjMatrix;
	FMatrix LightViewProjection;
	FVector LightPosition;
	// 2. 그림자 파라미터
	float ShadowBias = 0.0005f;
	float ShadowNormalBias = 1.0f;
	float ShadowTexelSize = 1.0f;
	float AttenuationRadius = 0.0f;
	bool bCastShadow = true;
	// 3. 타깃 텍스처 및 패스 정보
	FTexture* TargetShadowMap = nullptr;
	FRenderingInfo ShadowPassInfo;
	// 4. 셰이더 바인딩 슬롯 (Directional: t2/b5, Spot: t3/b6, Point: t4/b7)
	uint32 ShaderResourceSlot = 2;
	uint32 ConstantBufferSlot = 5;
	FConstantBuffer* ConstantBuffer = nullptr;
	int32 LightIndex = -1;
};

class FShadowRenderer
{
public:
	FShadowRenderer() = default;
	~FShadowRenderer() = default;

	FShadowRenderer(const FShadowRenderer&) = delete;
	FShadowRenderer& operator=(const FShadowRenderer&) = delete;

	bool Init(uint32 InResolution = 2048);
	void RenderShadowMap(const FProjectedShadowInfo& ShadowInfo, const FRenderQueue& RenderQueue, const TArray<const FStaticDrawGroup*>& StaticGroups);

	void BindForShading(const FProjectedShadowInfo& ShadowInfo);
	void UnbindForShading(const FProjectedShadowInfo& ShadowInfo);

	void DisableDirectionalShadow();
	void DisableSpotShadow();
	void DisablePointShadow();

	FProjectedShadowInfo SetupDirectionalShadow(const FDirectionalLightSceneProxy& DirProxy, const FVector& FocusPosition);
	FProjectedShadowInfo SetupSpotShadow(const FSpotLightSceneProxy& SpotProxy, int32 InLightIndex = 0);
	FProjectedShadowInfo SetupPointShadow(const FPointLightSceneProxy& PointProxy, int32 InLightIndex = 0);

	FMatrix GetPointLightCubeViewMatrix(uint32 Face, const FVector& LightPosition) const;
	FMatrix GetPointLightCubeProjMatrix(float AttenuationRadius) const;

	FTexture2D* GetDirectionalShadowMap() const { return DirectionalShadowMap.get(); }
	FTexture2D* GetShadowMap() const { return DirectionalShadowMap.get(); }
	FTexture2D* GetSpotShadowMap() const { return SpotShadowMap.get(); }
	FDepthTextureCube* GetPointShadowMap() const { return PointShadowMap.get(); }

	const FMatrix& GetLightViewProjection() const { return LightViewProjection; }
	const FMatrix& GetLightView() const { return LightViewMatrix; }
	const FMatrix& GetLightProj() const { return LightProjMatrix; }
	const FVector& GetLightPosition() const { return LightPosition; }

	float GetShadowBias() const { return ShadowBias; }
	void SetShadowBias(float InBias) { ShadowBias = InBias; }

	float GetShadowNormalBias() const { return ShadowNormalBias; }
	void SetShadowNormalBias(float InBias) { ShadowNormalBias = InBias; }

	float GetOrthoWidth() const { return OrthoWidth; }
	void SetOrthoWidth(float InWidth) { OrthoWidth = InWidth; }

	float GetOrthoHeight() const { return OrthoHeight; }
	void SetOrthoHeight(float InHeight) { OrthoHeight = InHeight; }

	uint32 GetResolution() const { return Resolution; }

	FRenderingInfo GetDirectionalShadowPassInfo() const { return DirectionalShadowPassInfo; }
	FRenderingInfo GetSpotShadowPassInfo() const { return SpotShadowPassInfo; }
	FRenderingInfo GetPointShadowPassInfo() const { return PointShadowPassInfo; }

private:
	void UpdateLightMatrix(const FVector& LightDirection, const FVector& FocusPosition);

	FPipelineState PipelineState;

	uint32 Resolution = 2048;

	float OrthoWidth = 4000.0f;
	float OrthoHeight = 4000.0f;
	float NearClip = 10.0f;
	float FarClip = 10000.0f;

	float ShadowBias = 0.0005f;
	float ShadowNormalBias = 1.0f;

	FVector LightPosition = FVector::ZeroVector;
	FMatrix LightViewMatrix = FMatrix::Identity;
	FMatrix LightProjMatrix = FMatrix::Identity;
	FMatrix LightViewProjection = FMatrix::Identity;

	TUniquePtr<FTexture2D> DirectionalShadowMap;
	FRenderingInfo DirectionalShadowPassInfo;

	TUniquePtr<FTexture2D> SpotShadowMap;
	FRenderingInfo SpotShadowPassInfo;

	TUniquePtr<FDepthTextureCube> PointShadowMap;
	FRenderingInfo PointShadowPassInfo;

	FShaderProgram* ShadowShader = nullptr;
	TUniquePtr<FConstantBuffer> ShadowViewCB;
	TUniquePtr<FConstantBuffer> PerObjectCB;
	TUniquePtr<FConstantBuffer> DirectionalShadowConstantsCB;
	TUniquePtr<FConstantBuffer> SpotShadowConstantsCB;
	TUniquePtr<FConstantBuffer> PointShadowConstantsCB;
};

