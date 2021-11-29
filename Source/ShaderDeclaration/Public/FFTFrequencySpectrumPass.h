#pragma once
#include "RHIResources.h"
#include "RHICommandList.h"
#include "CoreMinimal.h"

class FShaderFrequencyParameters
{
public:
	float mSize;
	float mChoppyness;
	float mWorldTimeSeconds;
	FVector2D mOceanSizeLxLz;

	FTextureRHIRef OutSurfaceX;
	FUnorderedAccessViewRHIRef OutSurfaceXUAV;
	FShaderResourceViewRHIRef OutSurfaceXSRV;

	FTextureRHIRef OutSurfaceY;
	FUnorderedAccessViewRHIRef OutSurfaceYUAV;
	FShaderResourceViewRHIRef OutSurfaceYSRV;

	FTextureRHIRef OutSurfaceZ;
	FUnorderedAccessViewRHIRef OutSurfaceZUAV;
	FShaderResourceViewRHIRef OutSurfaceZSRV;

	FTextureRHIRef OutDHxKt;
	FUnorderedAccessViewRHIRef OutDHxKtUAV;
	FShaderResourceViewRHIRef OutDHxKtSRV;

	FTextureRHIRef OutDHzKt;
	FUnorderedAccessViewRHIRef OutDHzKtUAV;
	FShaderResourceViewRHIRef OutDHzKtSRV;

	FTextureRHIRef OutDxz;
	FUnorderedAccessViewRHIRef OutDxzUAV;
	FShaderResourceViewRHIRef OutDxzSRV;

	FTextureRHIRef OutDxx;
	FUnorderedAccessViewRHIRef OutDxxUAV;
	FShaderResourceViewRHIRef OutDxxSRV;

	FTextureRHIRef OutDzz;
	FUnorderedAccessViewRHIRef OutDzzUAV;
	FShaderResourceViewRHIRef OutDzzSRV;

	FShaderFrequencyParameters(){}

	FShaderFrequencyParameters(
		float Time,
		float Choppyness,
		FVector2D OceanSizeLxLz,
		float TextureSize
		)
	{
		CachedRenderTargetSize = FIntPoint(TextureSize, TextureSize);
		
		mSize = TextureSize;
		mChoppyness = Choppyness;
		mOceanSizeLxLz = OceanSizeLxLz;
		mWorldTimeSeconds = Time;

		FRHIResourceCreateInfo CreateInfo;
		
		OutSurfaceX = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutSurfaceXUAV = RHICreateUnorderedAccessView(OutSurfaceX);
		OutSurfaceXSRV = RHICreateShaderResourceView(OutSurfaceX, 0);

		OutSurfaceY = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutSurfaceYUAV = RHICreateUnorderedAccessView(OutSurfaceY);
		OutSurfaceYSRV = RHICreateShaderResourceView(OutSurfaceY, 0);

		OutSurfaceZ = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutSurfaceZUAV = RHICreateUnorderedAccessView(OutSurfaceZ);
		OutSurfaceZSRV = RHICreateShaderResourceView(OutSurfaceZ, 0);

		OutDHxKt = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutDHxKtUAV = RHICreateUnorderedAccessView(OutDHxKt);
		OutDHxKtSRV = RHICreateShaderResourceView(OutDHxKt, 0);

		OutDHzKt = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutDHzKtUAV = RHICreateUnorderedAccessView(OutDHzKt);
		OutDHzKtSRV = RHICreateShaderResourceView(OutDHzKt, 0);

		OutDxz = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutDxzUAV = RHICreateUnorderedAccessView(OutDxz);
		OutDxzSRV = RHICreateShaderResourceView(OutDxz, 0);
		
		OutDxx = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutDxxUAV = RHICreateUnorderedAccessView(OutDxx);
		OutDxxSRV = RHICreateShaderResourceView(OutDxx, 0);
		
		OutDzz = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutDzzUAV = RHICreateUnorderedAccessView(OutDzz);
		OutDzzSRV = RHICreateShaderResourceView(OutDzz, 0);
	}

private:

	FIntPoint CachedRenderTargetSize;
};

class FFTFrequencySpectrumPass
{
public:
	static void RunComputeShader_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FShaderFrequencyParameters& DrawParameters,
		FShaderResourceViewRHIRef InputH0SRV);
};
