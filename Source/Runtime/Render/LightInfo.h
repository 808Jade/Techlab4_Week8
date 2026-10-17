#pragma once

#include "Math/Vector.h"

// Linear
struct FAmbientLightInfo
{
	FVector Color;
	float Padding = 0.0f;
};

struct FDirectionalLightInfo
{
	FVector Color;
	float Padding0 = 0.0f;
	FVector Direction;
	float Padding1 = 0.0f;
};

struct FPointLightInfo
{
	FVector Color;
	float AttenuationRadius = 0.0f;
	FVector Position;
	float Padding = 0.0f;
};

struct FSpotLightInfo
{
	FVector Color;
	float AttenuationRadius = 0.0f;
	FVector Position;
	float CosInnerCone = 1.0f;
	FVector Direction;
	float CosOuterCone = 1.0f;
};

struct FLightingConstants
{
	FAmbientLightInfo Ambient;
	FDirectionalLightInfo Directional;
};

static_assert(sizeof(FAmbientLightInfo) == 16);
static_assert(sizeof(FDirectionalLightInfo) == 32);
static_assert(sizeof(FPointLightInfo) == 32);
static_assert(sizeof(FSpotLightInfo) == 48);
static_assert(sizeof(FLightingConstants) == 48);
