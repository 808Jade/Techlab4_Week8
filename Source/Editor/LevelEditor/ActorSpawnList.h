#pragma once

#include "GameFramework/Actor/ParticleActor.h"
#include "GameFramework/Actor/StaticMeshActor.h"
#include "GameFramework/Actor/TextRenderActor.h"
#include "GameFramework/Actor/ExponentialHeightFogActor.h"
#include "GameFramework/Actor/AmbientLightActor.h"
#include "GameFramework/Actor/DirectionalLightActor.h"
#include "GameFramework/Actor/PointLightActor.h"
#include "GameFramework/Actor/SpotLightActor.h"

// 에디터에서 바로 스폰할 수 있는 액터 종류. 레벨 툴바의 Add 메뉴와 Editor Controls(Benchmark)가 같이 쓴다.
struct FSpawnableActorType
{
	const char* Label;
	UClass* (*GetStaticClass)();
};

inline constexpr FSpawnableActorType SpawnableActorTypes[] =
{
	{ "Actor",                &AActor::StaticClass },
	{ "StaticMesh",           &AStaticMeshActor::StaticClass },
	{ "Particle",             &AParticleActor::StaticClass },
	{ "Text",                 &ATextRenderActor::StaticClass },
	{ "ExponentialHeightFog", &AExponentialHeightFogActor::StaticClass },
	{ "AmbientLight",         &AAmbientLightActor::StaticClass },
	{ "DirectionalLight",     &ADirectionalLightActor::StaticClass },
	{ "PointLight",           &APointLightActor::StaticClass },
	{ "SpotLight",            &ASpotLightActor::StaticClass },
};
