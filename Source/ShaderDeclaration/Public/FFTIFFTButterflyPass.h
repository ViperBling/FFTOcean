#pragma once

#include "RHIResources.h"
#include "RHICommandList.h"
#include "CoreMinimal.h"

class FShaderIFFTButterflyParameters
{
public:
	// FTextureRHIRef SurfaceTexture;
	// FUnorderedAccessViewRHIRef SurfaceTextureUAV;
	// FShaderResourceViewRHIRef SurfaceTextureSRV;

	float mSize;
	// int mPingPongIdx = 0;
	
	FShaderIFFTButterflyParameters(){}

	FShaderIFFTButterflyParameters(float TextureSize)
	{
		CachedRenderTargetSize = FIntPoint(TextureSize, TextureSize);
		mSize = TextureSize;
		
		// FRHIResourceCreateInfo CreateInfo;
		
		// SurfaceTexture = RHICreateTexture2D(
		// 	TextureSize, TextureSize,
		// 	PF_A32B32G32R32F, 1, 1,
		// 	TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		// SurfaceTextureUAV = RHICreateUnorderedAccessView(SurfaceTexture);
		// SurfaceTextureSRV = RHICreateShaderResourceView(SurfaceTexture, 0);
	}
private:
	
	FIntPoint CachedRenderTargetSize;
};

class FFTIFFTButterflyPass
{
public:
	static void RunComputeShader_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FShaderIFFTButterflyParameters& DrawParameters,
		FShaderResourceViewRHIRef TwiddleSRV,
		FShaderResourceViewRHIRef BufferSRV,
		FUnorderedAccessViewRHIRef BufferUAV,
		FShaderResourceViewRHIRef FrequencySpectrumSRV,
		FUnorderedAccessViewRHIRef FrequencySpectrumUAV);
};
