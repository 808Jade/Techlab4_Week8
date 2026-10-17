#pragma once

#include "Component/AmbientLightComponent.h"
#include "Component/DirectionalLightComponent.h"
#include "Component/SpotLightComponent.h"

class FScene;

enum class ELightType : uint8
{
	Ambient, Directional, Point, Spot,
};

class FLightSceneProxy
{
public:
	FLightSceneProxy(ULightComponentBase* InComponent, ELightType InType) : Component(InComponent), Type(InType) {}
	virtual ~FLightSceneProxy() = default;

	virtual void UpdateFromComponent() = 0;

	ULightComponentBase* GetComponent() const { return Component; }
	ELightType GetLightType() const { return Type; }
	FScene* GetScene() const { return Scene; }
	bool IsVisible() const { return bVisible; }

protected:
	ULightComponentBase* Component = nullptr;
	bool bVisible = true;

private:
	friend class FScene;
	FScene* Scene = nullptr;
	ELightType Type;
	bool bQueuedForUpdate = false;
};

class FAmbientLightSceneProxy final : public FLightSceneProxy
{
public:
	explicit FAmbientLightSceneProxy(UAmbientLightComponent* InComponent) : FLightSceneProxy(InComponent, ELightType::Ambient) {}

	void UpdateFromComponent() override
	{
		const UAmbientLightComponent* Light = static_cast<const UAmbientLightComponent*>(Component);
		Info = Light->GetAmbientLightInfo();
		bVisible = Light->IsLightVisible();
	}

	const FAmbientLightInfo& GetInfo() const { return Info; }

private:
	FAmbientLightInfo Info;
};

class FDirectionalLightSceneProxy final : public FLightSceneProxy
{
public:
	explicit FDirectionalLightSceneProxy(UDirectionalLightComponent* InComponent) : FLightSceneProxy(InComponent, ELightType::Directional) {}

	void UpdateFromComponent() override
	{
		const UDirectionalLightComponent* Light = static_cast<const UDirectionalLightComponent*>(Component);
		Info = Light->GetDirectionalLightInfo();
		bVisible = Light->IsLightVisible();

		bCastShadows = Light->GetCastShadows();
		ShadowBias = Light->GetShadowBias();
		ShadowNormalBias = Light->GetShadowNormalBias();
		ShadowOrthoSize = Light->GetShadowOrthoSize();
	}

	const FDirectionalLightInfo& GetInfo() const { return Info; }
	bool GetCastShadows() const { return bCastShadows; }
	float GetShadowBias() const { return ShadowBias; }
	float GetShadowNormalBias() const { return ShadowNormalBias; }
	float GetShadowOrthoSize() const { return ShadowOrthoSize; }

private:
	FDirectionalLightInfo Info;
	bool bCastShadows = true;
	float ShadowBias = 0.0005f;
	float ShadowNormalBias = 1.0f;
	float ShadowOrthoSize = 4000.0f;
};

class FPointLightSceneProxy final : public FLightSceneProxy
{
public:
	explicit FPointLightSceneProxy(UPointLightComponent* InComponent) : FLightSceneProxy(InComponent, ELightType::Point) {}

	void UpdateFromComponent() override
	{
		const UPointLightComponent* Light = static_cast<const UPointLightComponent*>(Component);
		Info = Light->GetPointLightInfo();
		bVisible = Light->IsLightVisible();
	}

	const FPointLightInfo& GetInfo() const { return Info; }

private:
	FPointLightInfo Info;
};

class FSpotLightSceneProxy final : public FLightSceneProxy
{
public:
	explicit FSpotLightSceneProxy(USpotLightComponent* InComponent) : FLightSceneProxy(InComponent, ELightType::Spot) {}

	void UpdateFromComponent() override
	{
		const USpotLightComponent* Light = static_cast<const USpotLightComponent*>(Component);
		Info = Light->GetSpotLightInfo();
		bVisible = Light->IsLightVisible();
		OuterConeAngle = Light->GetOuterConeAngle();
		AttenuationRadius = Light->GetAttenuationRadius();
		bCastShadows = Light->GetCastShadows();
		ShadowBias = Light->GetShadowBias();
		ShadowNormalBias = Light->GetShadowNormalBias();
	}

	const FSpotLightInfo& GetInfo() const { return Info; }
	float GetOuterConeAngle() const { return OuterConeAngle; }
	float GetAttenuationRadius() const { return AttenuationRadius; }
	bool GetCastShadows() const { return bCastShadows; }
	float GetShadowBias() const { return ShadowBias; }
	float GetShadowNormalBias() const { return ShadowNormalBias; }

private:
	FSpotLightInfo Info;
	float OuterConeAngle = 44.0f;
	float AttenuationRadius = 10.0f;
	bool bCastShadows = true;
	float ShadowBias = 0.001f;
	float ShadowNormalBias = 1.0f;
};
