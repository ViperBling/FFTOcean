#pragma once

#include "CoreMinimal.h"
#include "FFTCopyRT.h"
#include "FFTH0Pass.h"
#include "FFTTwiddleFactorPass.h"
#include "FFTFrequencySpectrumPass.h"
#include "FFTIFFTButterflyPass.h"
// #include "FFTNormalPass.h"
#include "FFTFinalWavesPass.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "RenderGraphResources.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "ShaderModule.generated.h"

USTRUCT()
struct SHADERDECLARATION_API FRenderFFTPassParams
{
	GENERATED_BODY()

	FShaderH0Pass H0Params;
	
	FShaderCopyRT CopyRTParams;

	FShaderFrequencyParameters FrequencyParams;

	FShaderTwiddleFactorParameters TwiddleFactorParams;

	FShaderIFFTButterflyParameters ButterflyParamsX;
	FShaderIFFTButterflyParameters ButterflyParamsY;
	FShaderIFFTButterflyParameters ButterflyParamsZ;

	FShaderIFFTButterflyParameters ButterflyParamsDHxKt;
	FShaderIFFTButterflyParameters ButterflyParamsDHzKt;

	FShaderIFFTButterflyParameters ButterflyParamsDxz;
	FShaderIFFTButterflyParameters ButterflyParamsDxx;
	FShaderIFFTButterflyParameters ButterflyParamsDzz;

	FShaderFinalWavesParameters FinalWavesParams;

	UPROPERTY()
	UTextureRenderTarget2D* DisplacementRT;
	UPROPERTY()
	UTextureRenderTarget2D* DerivativesRT;
	UPROPERTY()
	UTextureRenderTarget2D* TurbulenceRT;
	
	UPROPERTY()
	UTextureRenderTarget2D* DebugRT;

	bool InitialDataGenerated = false;
};

class SHADERDECLARATION_API FShaderDeclarationModule : public IModuleInterface
{
public:

	static FShaderDeclarationModule& Get(FName ModuleName)
	{
		return FModuleManager::LoadModuleChecked<FShaderDeclarationModule>(ModuleName);
	}

	static bool IsAvailable(FName ModuleName)
	{
		return FModuleManager::Get().IsModuleLoaded(ModuleName);
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:

	void BeginRendering();

	void EndRendering();

	// Update Parameters from FFTOceanActor.Cpp Tick() to pass in datas from CPU
	void UpdateParameters(const FString& Key, FRenderFFTPassParams& FFTParams);

	FDelegateHandle OnPostResolvedSceneColorHandle;
	FCriticalSection RenderEveryFrameLock;

	volatile bool bCachedParametersValid;

	// auto called by render thread every tick()
	void PostResolveSceneColor_RenderThread(FRDGBuilder& RDGBuilder, const FSceneTextures& SceneTextures);

private:
	// This holds all datas that would need to be updated into FFT render thread.
	TMap<FString, FRenderFFTPassParams> FFTCachedDatas;

	// Draw Render thread
	void Draw_RenderThread(FRenderFFTPassParams& FFTParams, const FString& Key);

};
