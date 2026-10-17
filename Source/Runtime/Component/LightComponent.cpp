#include "EnginePCH.h"
#include "LightComponent.h"
#include <Render/RenderCommand.h>

void ULightComponent::EnsureShadowMap()
{
	if (lightResource != nullptr) return;

	D3D11_TEXTURE2D_DESC Desc{};
	Desc.Width = ShadowResolution;
	Desc.Height = ShadowResolution;
	Desc.ArraySize = 1;
	Desc.Format = DXGI_FORMAT_R24G8_TYPELESS;              
	Desc.SampleDesc.Count = 1;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	Desc.MipLevels = 1;

	lightResource = RenderCommand::CreateTexture2D(Desc);
}

void ULightComponent::UpdateLightCamera()
{
	Camera->SetRelativeLocation(GetWorldLocation());
	Camera->SetRelativeRotation(GetWorldRotation());
}
