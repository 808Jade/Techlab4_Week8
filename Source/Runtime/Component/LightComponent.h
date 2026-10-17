#pragma once

#include "LightComponentBase.h"
#include "Camera/CameraComponent.h"
#include <Render/Texture2D.h>

class ULightComponent : public ULightComponentBase
{
	DECLARE_CLASS(ULightComponent, ULightComponentBase)

public:
	ULightComponent() {
		SetCamera(CreateDefaultSubobject<UCameraComponent>("ULightComponent"));
		Camera->PrimaryComponentTick.bCanEverTick = false;
	};
	virtual ~ULightComponent() override = default;

	FVector GetLightDirection() const
	{
		const FMatrix World = GetWorldMatrix();
		return FVector(World[0][0], World[0][1], World[0][2]).GetSafeNormal();
	}

	void EnsureShadowMap();
	void UpdateLightCamera();

	UCameraComponent* GetCamera() { return Camera; }
	void SetCamera(UCameraComponent* inCamera) { Camera = inCamera; }

	void SetShadowResolution(uint32 inShadowResolution) { ShadowResolution = inShadowResolution; }
	uint32 GetShadowResolution() { return ShadowResolution; }

	FTexture2D* GetLightResource() const { return lightResource.get(); }

private:
	UCameraComponent* Camera = nullptr;
	TUniquePtr<FTexture2D> lightResource = nullptr;
	uint32 ShadowResolution = 2048;
};
