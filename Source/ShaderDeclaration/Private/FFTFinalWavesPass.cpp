#include "FFTFinalWavesPass.h"

#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"

#define NUM_THREADS_PER_GROUP_DIMENSION 32

class FFinalWavesCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFinalWavesCS);
	SHADER_USE_PARAMETER_STRUCT(FFinalWavesCS, FGlobalShader);
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(float, Size)
		SHADER_PARAMETER(float, Choppyness)
		SHADER_PARAMETER(float, Time)
		SHADER_PARAMETER(float, NormalStrength)
		SHADER_PARAMETER(FVector2D, OceanSize_LxLz)
	
		// SHADER_PARAMETER_SRV(Texture2D<float4>, InputTurb)
		SHADER_PARAMETER_SRV(Texture2D<float4>, InputDx)
		SHADER_PARAMETER_SRV(Texture2D<float4>, InputDy)
		SHADER_PARAMETER_SRV(Texture2D<float4>, InputDz)
	
		SHADER_PARAMETER_SRV(Texture2D<float4>, InputDHxKt)
		SHADER_PARAMETER_SRV(Texture2D<float4>, InputDHzKt)
	
		SHADER_PARAMETER_SRV(Texture2D<float4>, InputDxz)
		SHADER_PARAMETER_SRV(Texture2D<float4>, InputDxx)
		SHADER_PARAMETER_SRV(Texture2D<float4>, InputDzz)
	
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutDisplacement)
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutDerivatives)
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutTurbulence)

		SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)
		
	END_SHADER_PARAMETER_STRUCT()
	
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FFinalWavesCS, "/OceanShaders/Private/FFTFinalWavesCompute.usf", "MainCS", SF_Compute);

void FFTFinalWavesPass::RunComputeShader_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FShaderFinalWavesParameters& DrawParameters,
	FShaderResourceViewRHIRef InputDx,
	FShaderResourceViewRHIRef InputDy,
	FShaderResourceViewRHIRef InputDz,
	FShaderResourceViewRHIRef InputDHxKt,
	FShaderResourceViewRHIRef InputDHzKt,
	FShaderResourceViewRHIRef InputDxz,
	FShaderResourceViewRHIRef InputDxx,
	FShaderResourceViewRHIRef InputDzz)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_ComputeShader); // Used to gather CPU profiling data for the UE4 session frontend 
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_Compute); // Used to profile GPU activity and add metadata to be consumed by for example RenderDoc
	
	FFinalWavesCS::FParameters PassParameters;
	PassParameters.Size = DrawParameters.mSize;
	PassParameters.Choppyness = DrawParameters.mChoppyness;
	PassParameters.Time = DrawParameters.mWorldTimeSeconds;
	PassParameters.NormalStrength = DrawParameters.mNormalStrength;
	PassParameters.OceanSize_LxLz = DrawParameters.mOceanSizeLxLz;

	// PassParameters.InputTurb = DrawParameters.OutTurbulenceSRV;

	PassParameters.InputDx = InputDx;
	PassParameters.InputDy = InputDy;
	PassParameters.InputDz = InputDz;
	
	PassParameters.InputDHxKt = InputDHxKt;
	PassParameters.InputDHzKt = InputDHzKt;
	
	PassParameters.InputDxz = InputDxz;
	PassParameters.InputDxx = InputDxx;
	PassParameters.InputDzz = InputDzz;

	PassParameters.OutDisplacement = DrawParameters.OutDisplacementUAV;
	PassParameters.OutDerivatives = DrawParameters.OutDerivativesUAV;
	PassParameters.OutTurbulence = DrawParameters.OutTurbulenceUAV;

	PassParameters.LinearSampler = TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	
	TShaderMapRef<FFinalWavesCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	
	FComputeShaderUtils::Dispatch(
		RHICmdList, ComputeShader, PassParameters, 
		FIntVector(
			DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION,
			DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION, 1));
}
