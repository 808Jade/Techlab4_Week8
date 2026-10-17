#include "EnginePCH.h"

#include "LightDepthRenderer.h"
#include "RenderCommand.h"
#include "RenderResourceManager.h"
#include "Render/Buffer.h"


bool FLightDepthRenderer::Init()
{
	Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/LightDepthView.hlsl");
	if (!Shader)
	{
		HTR_LOG(Error, "[Light Depth View] shader not found");
		return false;
	}


	LightCB = RenderCommand::CreateConstantBuffer(sizeof(FLightDepthConstants));
	WorldCB = RenderCommand::CreateConstantBuffer(sizeof(FWorld));

	PipelineState.Shader = Shader;
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	return true;
}

void FLightDepthRenderer::OnRender(const FMatrix& LightViewProj, const FRenderQueue& RenderTargets)
{
	FLightDepthConstants LightConstants;
	LightConstants.LightViewProj = LightViewProj;
	FWorld WorldConstants;


	RenderCommand::BindPipelineState(PipelineState);

	RenderCommand::UpdateBufferData(LightCB.get(), &LightConstants);
	RenderCommand::BindConstantBuffer(0, LightCB.get(), EShaderBindFlagBits::Vertex);

	
	for (auto &RenderTarget : RenderTargets) {
		if (RenderTarget.Model == nullptr || RenderTarget.Mesh == nullptr) continue;
		RenderCommand::BindMesh(RenderTarget.Mesh, RenderTarget.LODIndex);
		WorldConstants.World = * RenderTarget.Model;
		RenderCommand::UpdateBufferData(WorldCB.get(), &WorldConstants);
		RenderCommand::BindConstantBuffer(2, WorldCB.get(), EShaderBindFlagBits::Vertex);

		RenderCommand::DrawIndexed(
			RenderTarget.IndexCount ? RenderTarget.IndexCount : RenderTarget.Mesh->GetIndexBuffer(RenderTarget.LODIndex)->GetIndexCount(),
			RenderTarget.StartIndex
		);
	}
}
