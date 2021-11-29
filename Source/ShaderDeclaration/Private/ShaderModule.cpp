#include "ShaderModule.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "Interfaces/IPluginManager.h"

IMPLEMENT_MODULE(FShaderDeclarationModule, ShaderDeclaration)

// Declare some GPU stats so we can track them later
DECLARE_GPU_STAT_NAMED(ShaderPlugin_Render, TEXT("ShaderPlugin: Root Render"));
DECLARE_GPU_STAT_NAMED(ShaderPlugin_Compute, TEXT("ShaderPlugin: Render Compute Shader"));
DECLARE_GPU_STAT_NAMED(ShaderPlugin_Pixel, TEXT("ShaderPlugin: Render Pixel Shader"));


void FShaderDeclarationModule::StartupModule()
{
	OnPostResolvedSceneColorHandle.Reset();
	bCachedParametersValid = false;
	 IModuleInterface::StartupModule();

	// Maps virtual shader source directory to the plugin's actual shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("FFTOcean"))->GetBaseDir(), TEXT("Shaders"));
	
	AddShaderSourceDirectoryMapping(TEXT("/OceanShaders"), PluginShaderDir);
}

void FShaderDeclarationModule::ShutdownModule()
{
	EndRendering();
	 IModuleInterface::ShutdownModule();
}

void FShaderDeclarationModule::BeginRendering()
{
	if (OnPostResolvedSceneColorHandle.IsValid())
		return;

	bCachedParametersValid = false;
	
	const FName RenderModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RenderModuleName);

	if (RendererModule)
	{
		OnPostResolvedSceneColorHandle = RendererModule->GetResolvedSceneColorCallbacks().AddRaw(
			this, &FShaderDeclarationModule::PostResolveSceneColor_RenderThread);
	}
}

void FShaderDeclarationModule::EndRendering()
{
	if (!OnPostResolvedSceneColorHandle.IsValid())
		return;

	const FName RenderModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RenderModuleName);

	if (RendererModule)
	{
		RendererModule->GetResolvedSceneColorCallbacks().Remove(OnPostResolvedSceneColorHandle);
	}
	OnPostResolvedSceneColorHandle.Reset();
}

void FShaderDeclarationModule::UpdateParameters(const FString& Key, FRenderFFTPassParams& FFTParams)
{
	RenderEveryFrameLock.Lock();
	
	if (FFTCachedDatas.Contains(Key))
	{
		if (!FFTCachedDatas[Key].InitialDataGenerated)
		{
			FFTCachedDatas[Key].TwiddleFactorParams = FFTParams.TwiddleFactorParams;
		}
		
		FFTCachedDatas[Key].H0Params = FFTParams.H0Params;
		FFTCachedDatas[Key].FrequencyParams = FFTParams.FrequencyParams;
		
		FFTCachedDatas[Key].ButterflyParamsX = FFTParams.ButterflyParamsX;
		FFTCachedDatas[Key].ButterflyParamsY = FFTParams.ButterflyParamsY;
		FFTCachedDatas[Key].ButterflyParamsZ = FFTParams.ButterflyParamsZ;

		FFTCachedDatas[Key].ButterflyParamsDHxKt = FFTParams.ButterflyParamsDHxKt;
		FFTCachedDatas[Key].ButterflyParamsDHzKt = FFTParams.ButterflyParamsDHzKt;

		FFTCachedDatas[Key].ButterflyParamsDxz = FFTParams.ButterflyParamsDxz;
		FFTCachedDatas[Key].ButterflyParamsDxx = FFTParams.ButterflyParamsDxx;
		FFTCachedDatas[Key].ButterflyParamsDzz = FFTParams.ButterflyParamsDzz;

		FFTCachedDatas[Key].DisplacementRT = FFTParams.DisplacementRT;
		FFTCachedDatas[Key].DerivativesRT = FFTParams.DerivativesRT;
		FFTCachedDatas[Key].TurbulenceRT = FFTParams.TurbulenceRT;

		
		FFTCachedDatas[Key].DebugRT = FFTParams.DebugRT;
	}
	else
	{
		FFTCachedDatas.Add(Key, FFTParams);
	}

	bCachedParametersValid = true;
	RenderEveryFrameLock.Unlock();
}

void FShaderDeclarationModule::PostResolveSceneColor_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FSceneRenderTargets& SceneContext)
{
	if (!bCachedParametersValid) return;

	RenderEveryFrameLock.Lock();

	TMap<FString, FRenderFFTPassParams> FFTPass_Copy;
	FFTPass_Copy.Append(FFTCachedDatas);

	RenderEveryFrameLock.Unlock();
	for (TMap<FString, FRenderFFTPassParams>::TIterator It = FFTPass_Copy.CreateIterator(); It; ++It)
	{
		Draw_RenderThread(It.Value(), It.Key());
	}
}

void FShaderDeclarationModule::Draw_RenderThread(FRenderFFTPassParams& FFTParams, const FString& Key)
{
	check(IsInRenderingThread());

	if (!FFTCachedDatas.Contains(Key))
		return;

	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	// Used to gather CPU profiling data for the UE4 session frontend
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_Render); 
	// Used to profile GPU activity and add metadata to be consumed by for example RenderDoc
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_Render);

	// 0. compute twiddle indicies for divide and conquer for IFFT
	if (!FFTCachedDatas[Key].InitialDataGenerated)
	{
		FTwiddleFactorPass::RunComputeShader_RenderThread(RHICmdList, FFTParams.TwiddleFactorParams);
		FFTCachedDatas[Key].InitialDataGenerated = true;
	}
	
	// 1. Compute HZero Pass
	FH0Pass::RunComputeShader_RenderThread(RHICmdList, FFTParams.H0Params);
	
	// 2. Frequency Spectrum Update
	if (FFTParams.H0Params.OutputH0KSRV != nullptr)
	{
		FFTFrequencySpectrumPass::RunComputeShader_RenderThread(RHICmdList, FFTParams.FrequencyParams, FFTParams.H0Params.OutputH0KSRV);
	}
	
	// 3. Compute Butterfly in 3 direction
	if (FFTParams.TwiddleFactorParams.OutputTwiddleSRV != nullptr && FFTParams.FrequencyParams.OutDHxKtSRV != nullptr)
	{
		FFTIFFTButterflyPass::RunComputeShader_RenderThread(
			RHICmdList,
			FFTParams.ButterflyParamsDHxKt,
			FFTParams.TwiddleFactorParams.OutputTwiddleSRV,
			FFTParams.H0Params.OutputH0KSRV,
			FFTParams.H0Params.OutputH0KUAV,
			FFTParams.FrequencyParams.OutDHxKtSRV,
			FFTParams.FrequencyParams.OutDHxKtUAV
		);
	}

	if (FFTParams.TwiddleFactorParams.OutputTwiddleSRV != nullptr && FFTParams.FrequencyParams.OutDHzKtSRV != nullptr)
	{
		FFTIFFTButterflyPass::RunComputeShader_RenderThread(
			RHICmdList,
			FFTParams.ButterflyParamsDHzKt,
			FFTParams.TwiddleFactorParams.OutputTwiddleSRV,
			FFTParams.H0Params.OutputH0KSRV,
			FFTParams.H0Params.OutputH0KUAV,
			FFTParams.FrequencyParams.OutDHzKtSRV,
			FFTParams.FrequencyParams.OutDHzKtUAV
		);
	}

	if (FFTParams.TwiddleFactorParams.OutputTwiddleSRV != nullptr &&  FFTParams.FrequencyParams.OutDxzSRV != nullptr)
	{
		FFTIFFTButterflyPass::RunComputeShader_RenderThread(
			RHICmdList,
			FFTParams.ButterflyParamsDxz,
			FFTParams.TwiddleFactorParams.OutputTwiddleSRV,
			FFTParams.H0Params.OutputH0KSRV,
			FFTParams.H0Params.OutputH0KUAV,
			FFTParams.FrequencyParams.OutDxzSRV,
			FFTParams.FrequencyParams.OutDxzUAV
		);
	}

	if (FFTParams.TwiddleFactorParams.OutputTwiddleSRV != nullptr &&  FFTParams.FrequencyParams.OutDxxSRV != nullptr)
	{
		FFTIFFTButterflyPass::RunComputeShader_RenderThread(
			RHICmdList,
			FFTParams.ButterflyParamsDxx,
			FFTParams.TwiddleFactorParams.OutputTwiddleSRV,
			FFTParams.H0Params.OutputH0KSRV,
			FFTParams.H0Params.OutputH0KUAV,
			FFTParams.FrequencyParams.OutDxxSRV,
			FFTParams.FrequencyParams.OutDxxUAV
		);
	}

	if (FFTParams.TwiddleFactorParams.OutputTwiddleSRV != nullptr &&  FFTParams.FrequencyParams.OutDzzSRV != nullptr)
	{
		FFTIFFTButterflyPass::RunComputeShader_RenderThread(
			RHICmdList,
			FFTParams.ButterflyParamsDzz,
			FFTParams.TwiddleFactorParams.OutputTwiddleSRV,
			FFTParams.H0Params.OutputH0KSRV,
			FFTParams.H0Params.OutputH0KUAV,
			FFTParams.FrequencyParams.OutDzzSRV,
			FFTParams.FrequencyParams.OutDzzUAV
		);
	}

	if (FFTParams.TwiddleFactorParams.OutputTwiddleSRV != nullptr && FFTParams.FrequencyParams.OutSurfaceYSRV != nullptr)
	{
		FFTIFFTButterflyPass::RunComputeShader_RenderThread(
			RHICmdList,
			FFTParams.ButterflyParamsY,
			FFTParams.TwiddleFactorParams.OutputTwiddleSRV,
			FFTParams.H0Params.OutputH0KSRV,
			FFTParams.H0Params.OutputH0KUAV,
			FFTParams.FrequencyParams.OutSurfaceYSRV,
			FFTParams.FrequencyParams.OutSurfaceYUAV);
	}
	
	if (FFTParams.TwiddleFactorParams.OutputTwiddleSRV != nullptr && FFTParams.FrequencyParams.OutSurfaceXSRV != nullptr)
	{
		FFTIFFTButterflyPass::RunComputeShader_RenderThread(
			RHICmdList,
			FFTParams.ButterflyParamsX,
			FFTParams.TwiddleFactorParams.OutputTwiddleSRV,
			FFTParams.H0Params.OutputH0KSRV,
			FFTParams.H0Params.OutputH0KUAV,
			FFTParams.FrequencyParams.OutSurfaceXSRV,
			FFTParams.FrequencyParams.OutSurfaceXUAV);
	}
	
	if (FFTParams.TwiddleFactorParams.OutputTwiddleSRV != nullptr && FFTParams.FrequencyParams.OutSurfaceZSRV != nullptr)
	{
		FFTIFFTButterflyPass::RunComputeShader_RenderThread(
			RHICmdList,
			FFTParams.ButterflyParamsZ,
			FFTParams.TwiddleFactorParams.OutputTwiddleSRV,
			FFTParams.H0Params.OutputH0KSRV,
			FFTParams.H0Params.OutputH0KUAV,
			FFTParams.FrequencyParams.OutSurfaceZSRV,
			FFTParams.FrequencyParams.OutSurfaceZUAV);
	}

	if (FFTParams.FrequencyParams.OutSurfaceXSRV != nullptr &&
		FFTParams.FrequencyParams.OutSurfaceYSRV != nullptr &&
		FFTParams.FrequencyParams.OutSurfaceZSRV != nullptr &&
		FFTParams.FrequencyParams.OutDHxKtSRV != nullptr &&
		FFTParams.FrequencyParams.OutDHzKtSRV != nullptr &&
		FFTParams.FrequencyParams.OutDxzSRV != nullptr &&
		FFTParams.FrequencyParams.OutDxxSRV != nullptr &&
		FFTParams.FrequencyParams.OutDzzSRV != nullptr
		)
	{
		FFTFinalWavesPass::RunComputeShader_RenderThread(
			RHICmdList,
			FFTParams.FinalWavesParams,
			FFTParams.FrequencyParams.OutSurfaceXSRV,
			FFTParams.FrequencyParams.OutSurfaceYSRV,
			FFTParams.FrequencyParams.OutSurfaceZSRV,
			FFTParams.FrequencyParams.OutDHxKtSRV,
			FFTParams.FrequencyParams.OutDHzKtSRV,
			FFTParams.FrequencyParams.OutDxzSRV,
			FFTParams.FrequencyParams.OutDxxSRV,
			FFTParams.FrequencyParams.OutDzzSRV);
	}

	if (FFTParams.DisplacementRT != nullptr, FFTParams.FinalWavesParams.OutDisplacementSRV != nullptr)
	{
		FCopyRT::DrawToRenderTarget_RenderThread(RHICmdList, FFTParams.DisplacementRT, FFTParams.FinalWavesParams.OutDisplacementSRV);
	}
	
	if (FFTParams.DerivativesRT != nullptr, FFTParams.FinalWavesParams.OutDerivativesSRV != nullptr)
	{
		FCopyRT::DrawToRenderTarget_RenderThread(RHICmdList, FFTParams.DerivativesRT, FFTParams.FinalWavesParams.OutDerivativesSRV);
	}
	
	if (FFTParams.TurbulenceRT != nullptr, FFTParams.FinalWavesParams.OutTurbulenceSRV != nullptr)
	{
		FCopyRT::DrawToRenderTarget_RenderThread(RHICmdList, FFTParams.TurbulenceRT, FFTParams.FinalWavesParams.OutTurbulenceSRV);
	}

	if (FFTParams.DebugRT != nullptr, FFTParams.TwiddleFactorParams.OutputTwiddleSRV != nullptr)
	{
		FCopyRT::DrawToRenderTarget_RenderThread(RHICmdList, FFTParams.DebugRT, FFTParams.TwiddleFactorParams.OutputTwiddleSRV);
	}
}