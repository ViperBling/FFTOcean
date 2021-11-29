#include "FFTFrequencySpectrumPass.h"

#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"

#define NUM_THREADS_PER_GROUP_DIMENSION 32

class FFrequencySpectrumCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFrequencySpectrumCS);
	SHADER_USE_PARAMETER_STRUCT(FFrequencySpectrumCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(float, Time)
		SHADER_PARAMETER(float, Choppyness)
		SHADER_PARAMETER(float, Size)
		SHADER_PARAMETER(FVector2D, OceanSizeLxLz)
		SHADER_PARAMETER_SRV(Texture2D<float4>, InputH0SRV)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, OutSurfaceX)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, OutSurfaceY)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, OutSurfaceZ)
	
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, OutDHxKt)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, OutDHzKt)

		SHADER_PARAMETER_UAV(RWTexture2D<float2>, OutDxz)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, OutDxx)
		SHADER_PARAMETER_UAV(RWTexture2D<float2>, OutDzz)
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

IMPLEMENT_GLOBAL_SHADER(FFrequencySpectrumCS, "/OceanShaders/Private/FFTFrequencySpectrum.usf", "ComputeFrequencyCS", SF_Compute);

void FFTFrequencySpectrumPass::RunComputeShader_RenderThread(FRHICommandListImmediate& RHICmdList,
	const FShaderFrequencyParameters& DrawParameters, FShaderResourceViewRHIRef InputH0SRV)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_ComputeShader); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_Compute); // Used to profile GPU activity and add metadata to be consumed by for example RenderDoc

	FFrequencySpectrumCS::FParameters PassParameters;
	PassParameters.Time = DrawParameters.mWorldTimeSeconds;
	PassParameters.Choppyness = DrawParameters.mChoppyness;
	PassParameters.Size = DrawParameters.mSize;
	PassParameters.OceanSizeLxLz = DrawParameters.mOceanSizeLxLz;
	PassParameters.InputH0SRV = InputH0SRV;
	PassParameters.OutSurfaceX = DrawParameters.OutSurfaceXUAV;
	PassParameters.OutSurfaceY = DrawParameters.OutSurfaceYUAV;
	PassParameters.OutSurfaceZ = DrawParameters.OutSurfaceZUAV;

	PassParameters.OutDHxKt = DrawParameters.OutDHxKtUAV;
	PassParameters.OutDHzKt = DrawParameters.OutDHzKtUAV;

	PassParameters.OutDxz = DrawParameters.OutDxzUAV;
	PassParameters.OutDxx = DrawParameters.OutDxxUAV;
	PassParameters.OutDzz = DrawParameters.OutDzzUAV;

	TShaderMapRef<FFrequencySpectrumCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	
	FComputeShaderUtils::Dispatch(
		RHICmdList, ComputeShader, PassParameters, 
		FIntVector(
			DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION,
			DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION, 1));
}
