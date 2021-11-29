#include "FFTTwiddleFactorPass.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"
#include "RenderGraphUtils.h"
#include "Shader.h"


#define NUM_THREADS_PER_GROUP_DIMENSION 32

class FTwiddleFactorCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTwiddleFactorCS);
	SHADER_USE_PARAMETER_STRUCT(FTwiddleFactorCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutputTwiddle)
		SHADER_PARAMETER(float, Size)
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

IMPLEMENT_GLOBAL_SHADER(FTwiddleFactorCS, "/OceanShaders/Private/FFTTwiddleFactorCompute.usf", "TwiddleFactorsCS", SF_Compute);

void FTwiddleFactorPass::RunComputeShader_RenderThread(FRHICommandListImmediate& RHICmdList,
	const FShaderTwiddleFactorParameters& DrawParameters)
{
	FTwiddleFactorCS::FParameters PassParameters;
	PassParameters.OutputTwiddle = DrawParameters.OutputTwiddleUAV;
	PassParameters.Size = DrawParameters.mSize;

	TShaderMapRef<FTwiddleFactorCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters,
		FIntVector(
			DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION,
			DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION, 1));
}
