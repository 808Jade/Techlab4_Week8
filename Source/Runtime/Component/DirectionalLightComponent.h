#pragma once

#include "LightComponent.h"

class FLineBatcher;

class UDirectionalLightComponent : public ULightComponent
{
	DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)
	REFLECT_START(UDirectionalLightComponent)
		PROPERTY(bCastShadows)
		PROPERTY(ShadowBias)
		PROPERTY(ShadowNormalBias)
		PROPERTY(ShadowOrthoSize)
	REFLECT_END()

public:
	UDirectionalLightComponent() = default;
	virtual ~UDirectionalLightComponent() override = default;

	FDirectionalLightInfo GetDirectionalLightInfo() const;
	virtual FLightSceneProxy* CreateSceneProxy() override;

	void DrawDebug(FLineBatcher* LineBatcher) const;

	bool GetCastShadows() const { return bCastShadows; }
	void SetCastShadows(bool bInCast) { bCastShadows = bInCast; MarkSceneProxyDirty(); }

	float GetShadowBias() const { return ShadowBias; }
	void SetShadowBias(float InBias) { ShadowBias = InBias; MarkSceneProxyDirty(); }

	float GetShadowNormalBias() const { return ShadowNormalBias; }
	void SetShadowNormalBias(float InBias) { ShadowNormalBias = InBias; MarkSceneProxyDirty(); }

	float GetShadowOrthoSize() const { return ShadowOrthoSize; }
	void SetShadowOrthoSize(float InSize) { ShadowOrthoSize = InSize; MarkSceneProxyDirty(); }

private:
	bool bCastShadows = true;
	float ShadowBias = 0.0005f;
	float ShadowNormalBias = 1.0f;
	float ShadowOrthoSize = 4000.0f;
};
