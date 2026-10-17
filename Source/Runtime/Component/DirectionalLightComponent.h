#pragma once

#include "LightComponent.h"

class FLineBatcher;

class UDirectionalLightComponent : public ULightComponent
{
	DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)
	REFLECT_START(UDirectionalLightComponent)
		PROPERTY(ShadowDistance)
		REFLECT_END()

public:
	UDirectionalLightComponent() = default;
	virtual ~UDirectionalLightComponent() override = default;

	FDirectionalLightInfo GetDirectionalLightInfo() const;
	virtual FLightSceneProxy* CreateSceneProxy() override;

	void DrawDebug(FLineBatcher* LineBatcher) const;

private:

	float ShadowDistance = 7.0f;	// 그림자를 그릴 반경
};
