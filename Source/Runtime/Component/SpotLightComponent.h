#pragma once

#include "PointLightComponent.h"

class FLineBatcher;

class USpotLightComponent : public UPointLightComponent
{
	DECLARE_CLASS(USpotLightComponent, UPointLightComponent)
	REFLECT_START(USpotLightComponent)
		PROPERTY(InnerConeAngle)
		PROPERTY(OuterConeAngle)
		PROPERTY(bCastShadows)
		PROPERTY(ShadowBias)
		PROPERTY(ShadowNormalBias)
	REFLECT_END()

public:
	USpotLightComponent() = default;
	virtual ~USpotLightComponent() override = default;

	float GetInnerConeAngle() const { return InnerConeAngle; }
	float GetOuterConeAngle() const { return OuterConeAngle; }

	FSpotLightInfo GetSpotLightInfo() const;
	virtual FLightSceneProxy* CreateSceneProxy() override;

	virtual void PostEditChangeProperty(const FProperty& Property) override;

	void DrawDebug(FLineBatcher* LineBatcher) const;

	bool GetCastShadows() const { return bCastShadows; }
	void SetCastShadows(bool bInCast) { bCastShadows = bInCast; MarkSceneProxyDirty(); }

	float GetShadowBias() const { return ShadowBias; }
	void SetShadowBias(float InBias) { ShadowBias = InBias; MarkSceneProxyDirty(); }

	float GetShadowNormalBias() const { return ShadowNormalBias; }
	void SetShadowNormalBias(float InBias) { ShadowNormalBias = InBias; MarkSceneProxyDirty(); }

protected:
	float InnerConeAngle = 0.0f;
	float OuterConeAngle = 44.0f;
	bool bCastShadows = true;
	float ShadowBias = 0.001f;
	float ShadowNormalBias = 1.0f;
};
