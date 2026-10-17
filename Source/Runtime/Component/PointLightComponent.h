#pragma once

#include "LightComponent.h"

class UPointLightComponent : public ULightComponent
{
	DECLARE_CLASS(UPointLightComponent, ULightComponent)
	REFLECT_START(UPointLightComponent)
		PROPERTY(AttenuationRadius)
		PROPERTY(bCastShadows)
		PROPERTY(ShadowBias)
		PROPERTY(ShadowNormalBias)
	REFLECT_END()

public:
	UPointLightComponent() = default;
	virtual ~UPointLightComponent() override = default;

	float GetAttenuationRadius() const { return AttenuationRadius; }

	bool GetCastShadows() const { return bCastShadows; }
	void SetCastShadows(bool bInCast) { bCastShadows = bInCast; MarkSceneProxyDirty(); }

	float GetShadowBias() const { return ShadowBias; }
	void SetShadowBias(float InBias) { ShadowBias = InBias; MarkSceneProxyDirty(); }

	float GetShadowNormalBias() const { return ShadowNormalBias; }
	void SetShadowNormalBias(float InBias) { ShadowNormalBias = InBias; MarkSceneProxyDirty(); }

	FPointLightInfo GetPointLightInfo() const;
	virtual FLightSceneProxy* CreateSceneProxy() override;

	virtual void PostEditChangeProperty(const FProperty& Property) override;

protected:
	float AttenuationRadius = 10.0f;
	bool bCastShadows = true;
	float ShadowBias = 0.001f;
	float ShadowNormalBias = 1.0f;
};
