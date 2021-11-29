#pragma once

class FShaderFinalWavesParameters
{
public:
	float mSize;
	float mChoppyness;
	float mWorldTimeSeconds;
	float mNormalStrength;
	FVector2D mOceanSizeLxLz;

	FTextureRHIRef OutDisplacement;
	FUnorderedAccessViewRHIRef OutDisplacementUAV;
	FShaderResourceViewRHIRef OutDisplacementSRV;

	FTextureRHIRef OutDerivatives;
	FUnorderedAccessViewRHIRef OutDerivativesUAV;
	FShaderResourceViewRHIRef OutDerivativesSRV;

	FTextureRHIRef OutTurbulence;
	FUnorderedAccessViewRHIRef OutTurbulenceUAV;
	FShaderResourceViewRHIRef OutTurbulenceSRV;

	FShaderFinalWavesParameters() {}

	FShaderFinalWavesParameters(
		float TextureSize,
		float Time,
		float Choppyness,
		float NormalStrength,
		FVector2D OceanSize_LxLz
	)
	{
		CachedRenderTargetSize = FIntPoint(TextureSize, TextureSize);
		mSize = TextureSize;
		mChoppyness = Choppyness;
		mWorldTimeSeconds = Time;
		mNormalStrength = NormalStrength;
		mOceanSizeLxLz = OceanSize_LxLz;

		FRHIResourceCreateInfo CreateInfo;
		
		OutDisplacement = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutDisplacementUAV = RHICreateUnorderedAccessView(OutDisplacement);
		OutDisplacementSRV = RHICreateShaderResourceView(OutDisplacement, 0);

		OutDerivatives = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutDerivativesUAV = RHICreateUnorderedAccessView(OutDerivatives);
		OutDerivativesSRV = RHICreateShaderResourceView(OutDerivatives, 0);

		OutTurbulence = RHICreateTexture2D(
			TextureSize, TextureSize,
			PF_A32B32G32R32F, 1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutTurbulenceUAV = RHICreateUnorderedAccessView(OutTurbulence);
		OutTurbulenceSRV = RHICreateShaderResourceView(OutTurbulence, 0);
	}
	
private:

	FIntPoint CachedRenderTargetSize;
};

class FFTFinalWavesPass
{
public:
	static void RunComputeShader_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FShaderFinalWavesParameters& DrawParameters,
		FShaderResourceViewRHIRef InputDx,
		FShaderResourceViewRHIRef InputDy,
		FShaderResourceViewRHIRef InputDz,
		FShaderResourceViewRHIRef InputDHxKt,
		FShaderResourceViewRHIRef InputDHzKt,
		FShaderResourceViewRHIRef InputDxz,
		FShaderResourceViewRHIRef InputDxx,
		FShaderResourceViewRHIRef InputDzz);
};
