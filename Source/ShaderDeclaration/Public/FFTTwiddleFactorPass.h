#pragma once

#include "RHIResources.h"
#include "RHICommandList.h"
#include "CoreMinimal.h"

class FShaderTwiddleFactorParameters
{
public:
	FTextureRHIRef OutputTwiddle;
	FUnorderedAccessViewRHIRef OutputTwiddleUAV;
	FShaderResourceViewRHIRef OutputTwiddleSRV;

	float mSize;

	FShaderTwiddleFactorParameters() { }

	FShaderTwiddleFactorParameters(float Size)
	{
		mSize = Size;
		FRHIResourceCreateInfo CreateInfo(TEXT("TwiddleCreateInfo"));

		float logSize = (int)FMath::Log2(Size);

		OutputTwiddle = RHICreateTexture2D(
			logSize, Size, PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutputTwiddleUAV = RHICreateUnorderedAccessView(OutputTwiddle);
		OutputTwiddleSRV = RHICreateShaderResourceView(OutputTwiddle, 0);
	}
};

class FTwiddleFactorPass
{
public:
	static void RunComputeShader_RenderThread(FRHICommandListImmediate& RHICmdList, const FShaderTwiddleFactorParameters& DrawParameters);
};