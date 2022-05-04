#pragma once

#include "RHIResources.h"
#include "RHICommandList.h"
#include "CoreMinimal.h"

class FShaderIFFTButterflyParameters
{
public:

	float mSize;
	
	FShaderIFFTButterflyParameters(){}

	FShaderIFFTButterflyParameters(float TextureSize)
	{
		CachedRenderTargetSize = FIntPoint(TextureSize, TextureSize);
		mSize = TextureSize;
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
