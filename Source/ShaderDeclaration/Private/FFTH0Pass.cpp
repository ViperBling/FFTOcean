#include "FFTH0Pass.h"

#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"

#define NUM_THREADS_PER_GROUP_DIMENSION 32

class FH0PassCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FH0PassCS);
	SHADER_USE_PARAMETER_STRUCT(FH0PassCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		// SHADER_PARAMETER(FVector4, A_V_windDependency_T)
		// SHADER_PARAMETER(FVector4, W_unused_unused)
		// SHADER_PARAMETER(FVector4, width_height_Lx_Lz)
		// SHADER_PARAMETER(float, TotalTimeElapsedSeconds)
		SHADER_PARAMETER(FVector2f, WindDirection)
		SHADER_PARAMETER(FVector2f, OceanSizeLxLz)
		SHADER_PARAMETER(float, WaveAmplitude)
		SHADER_PARAMETER(float, WindSpeed)
		SHADER_PARAMETER(float, WindScale)
		SHADER_PARAMETER(float, Size)
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutputSurface)
		SHADER_PARAMETER_SRV(Texture2D, InputNoiseTexture)
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

IMPLEMENT_GLOBAL_SHADER(FH0PassCS, "/OceanShaders/Private/FFTH0GenerationCompute.usf", "ComputeH0CS", SF_Compute);


void FH0Pass::RunComputeShader_RenderThread(FRHICommandListImmediate& RHICmdList,
	const FShaderH0Pass& DrawParameters)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_ComputeShader); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_Compute); // Used to profile GPU activity and add metadata to be consumed by for example RenderDoc

	FH0PassCS::FParameters PassParameters;
	
	// PassParameters.A_V_windDependency_T = DrawParameters.mA_V_WindDependency_T;
	// PassParameters.W_unused_unused = DrawParameters.mW;
	// PassParameters.width_height_Lx_Lz = DrawParameters.mWidth_Height_Lx_Lz;
	// PassParameters.TotalTimeElapsedSeconds = DrawParameters.mTotalElapsedTime;

	PassParameters.WindDirection = DrawParameters.mWindDirection;
	PassParameters.OceanSizeLxLz = DrawParameters.mOceanSizeLxLz;
	PassParameters.WaveAmplitude = DrawParameters.mWaveAmplitude;
	PassParameters.WindScale = DrawParameters.mWindScale;
	PassParameters.WindSpeed = DrawParameters.mWindSpeed;
	PassParameters.Size = DrawParameters.mSize;
	PassParameters.OutputSurface = DrawParameters.OutputH0KUAV;
	PassParameters.InputNoiseTexture = DrawParameters.GaussianNoiseInputSRV;

	TShaderMapRef<FH0PassCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FComputeShaderUtils::Dispatch(
		RHICmdList,
		ComputeShader,
		PassParameters,
		FIntVector(
			DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION,
			DrawParameters.mSize / NUM_THREADS_PER_GROUP_DIMENSION, 1));
}
