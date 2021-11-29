#pragma once

#include "Engine/TextureRenderTarget2D.h"

class FShaderCopyRT
{
public:
	UTextureRenderTarget2D* RenderTarget = nullptr;
	FShaderCopyRT(){}
	
	FShaderCopyRT(UTextureRenderTarget2D* InRenderTarget)
	{
		RenderTarget = InRenderTarget;
	}
};

class FCopyRT
{
public:
	static void DrawToRenderTarget_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FShaderCopyRT& CopyRTParams,
		FShaderResourceViewRHIRef InputTextureRT);
};

class FShaderNormalParameters
{
public:
	UTextureRenderTarget2D* RenderTarget = nullptr;
	FVector2D mOceanSizeLxLz;
	float mSize;
	float mChoppyness;
	float mNormalStrength;
	
	FShaderNormalParameters(){}
	
	FShaderNormalParameters(
		UTextureRenderTarget2D* InRenderTarget,
		FVector2D OceanSizeLxLz,
		float Size,
		float Choppynes,
		float NormalStrength)
	{
		RenderTarget = InRenderTarget;

		mOceanSizeLxLz = OceanSizeLxLz;
		mSize = Size;
		mChoppyness = Choppynes;
		mNormalStrength = NormalStrength;
	}
};

class FFTNormalPass
{
public:
	static void RunPixelShader_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FShaderNormalParameters& NormalParams,
		FShaderResourceViewRHIRef InputTextureX,
		FShaderResourceViewRHIRef InputTextureY,
		FShaderResourceViewRHIRef InputTextureZ,
		FShaderResourceViewRHIRef InputDHxKt,
		FShaderResourceViewRHIRef InputDHzKt,
		FShaderResourceViewRHIRef InputDxzKt,
		FShaderResourceViewRHIRef InputDxxKt,
		FShaderResourceViewRHIRef InputDzzKt);
};