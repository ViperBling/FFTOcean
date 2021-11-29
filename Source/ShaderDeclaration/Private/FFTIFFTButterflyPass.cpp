#include "FFTIFFTButterflyPass.h"

#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "RHIResources.h"
#include "RHICommandList.h"
#include "CoreMinimal.h"

#define NUM_THREADS_PER_GROUP_DIMENSION 32

class FButterflyComputeCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FButterflyComputeCS);
	SHADER_USE_PARAMETER_STRUCT(FButterflyComputeCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D, TwiddleSRV)
		SHADER_PARAMETER_SRV(Texture2D, InputSRV)
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutputUAV)
		SHADER_PARAMETER(int, Dir)
		SHADER_PARAMETER(int, Step)
	END_SHADER_PARAMETER_STRUCT()
	
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FButterflyComputeCS, "/OceanShaders/Private/FFTButterflyCompute.usf", "ComputeButterflyCS", SF_Compute);

void FFTIFFTButterflyPass::RunComputeShader_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FShaderIFFTButterflyParameters& DrawParameters,
	FShaderResourceViewRHIRef TwiddleSRV,
	FShaderResourceViewRHIRef BufferSRV,
	FUnorderedAccessViewRHIRef BufferUAV,
	FShaderResourceViewRHIRef FrequencySpectrumSRV,
	FUnorderedAccessViewRHIRef FrequencySpectrumUAV)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_ComputeShader); // Used to gather CPU profiling data for the UE4 session frontend 
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_Compute); // Used to profile GPU activity and add metadata to be consumed by for example RenderDoc

	TShaderMapRef<FButterflyComputeCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	int logSize = (int)FMath::Log2(DrawParameters.mSize);
	int pingPong = 1;

	FButterflyComputeCS::FParameters PassParameters;
	PassParameters.TwiddleSRV = TwiddleSRV;

	for (int Dir = 0; Dir < 2; ++Dir)
	{
		PassParameters.Dir = Dir;
	
		for (int i = 0; i < logSize; i++)
		{
			pingPong = (pingPong == 0) ? 1 : 0;
	
			PassParameters.Step = i;
	
			if (pingPong == 0)
			{
				PassParameters.OutputUAV = BufferUAV;
				PassParameters.InputSRV = FrequencySpectrumSRV;
			}
			else
			{
				PassParameters.OutputUAV = FrequencySpectrumUAV;
				PassParameters.InputSRV = BufferSRV;
			}
	
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters,
					FIntVector(
						DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION,
						DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION, 1));
		}
	}
}
