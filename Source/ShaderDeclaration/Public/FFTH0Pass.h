#pragma once

#include "RHIResources.h"
#include "RHICommandList.h"
#include "CoreMinimal.h"

struct FSpectrumParameters
{
	// float scale;
	// float angle;
	// float spreadBlend;
	// float swell;
	// float alpha;
	// float peakOmega;
	// float gamma;
	// float shortWavesFade;
};


class FShaderH0Pass
{
public:
	
	FShaderResourceViewRHIRef GaussianNoiseInputSRV;

	FTextureRHIRef OutputH0K;
	FUnorderedAccessViewRHIRef OutputH0KUAV;
	FShaderResourceViewRHIRef OutputH0KSRV;

	float mWaveAmplitude;
	float mWindScale;
	float mWindSpeed;
	float mSize;
	FVector2D mWindDirection;
	FVector2D mOceanSizeLxLz;

	
	FIntPoint GetRenderTargetSize() const
	{
		return CachedRenderTargetSize;
	}

	FShaderH0Pass(){}
	
	
	FShaderH0Pass(
		FShaderResourceViewRHIRef InGaussianNoiseInput,
		FVector2D WindDireciton,
		FVector2D OceanSizeLxLz,
		float WaveAmplitude,
		float WindScale,
		float WindSpeed,
		float TextureSize
	)
	{
		CachedRenderTargetSize = FIntPoint(TextureSize, TextureSize);
		
		mWindDirection = WindDireciton;
		mOceanSizeLxLz = OceanSizeLxLz;
		mWaveAmplitude = WaveAmplitude;
		mWindScale = WindScale;
		mWindSpeed = WindSpeed;
		mSize = TextureSize;
		
		GaussianNoiseInputSRV = InGaussianNoiseInput;

		FRHIResourceCreateInfo CreateInfo;
		OutputH0K = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutputH0KUAV = RHICreateUnorderedAccessView(OutputH0K);
		OutputH0KSRV = RHICreateShaderResourceView(OutputH0K, 0);
	}

private:

	FIntPoint CachedRenderTargetSize;
};

class FH0Pass
{
public:
	static void RunComputeShader_RenderThread(FRHICommandListImmediate& RHICmdList, const FShaderH0Pass& DrawParameters);
};