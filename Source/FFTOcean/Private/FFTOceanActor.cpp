#include "FFTOceanActor.h"


AFFTOcean::AFFTOcean()
{
	PrimaryActorTick.bCanEverTick = true;
	// PrimaryActorTick.bStartWithTickEnabled = true;
	
	bIsGaussianInitialized = false;
}

void AFFTOcean::BeginPlay()
{
	Super::BeginPlay();
	
	ConstructGaussianNoise();
	CopyGaussianNoiseTextureToResourceViewOnce();
	
	FShaderDeclarationModule::Get("ShaderDeclaration").BeginRendering();
}

void AFFTOcean::BeginDestroy()
{
	Super::BeginDestroy();
	FShaderDeclarationModule::Get("ShaderDeclaration").EndRendering();
}

// bool AFFTOcean::ShouldTickIfViewportsOnly() const
// {
// 	return true;
// }

void AFFTOcean::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TotalElaspedTime += DeltaSeconds * TimeFactor;

	ENQUEUE_RENDER_COMMAND(OceanRenderer)(
	[this](FRHICommandListImmediate& RHICmdList)
	{
		FRenderFFTPassParams FFTPassParams = FRenderFFTPassParams();

		if (bIsGaussianInitialized)
		{
			FFTPassParams.H0Params = FShaderH0Pass(
				GaussianNoiseSRV,
				WindDirection,
				OceanSizeLxLz,
				WaveAmplitude,
				WindScale,
				WindSpeed,
				RTSize
			);
		}
		
		
		FFTPassParams.FrequencyParams = FShaderFrequencyParameters(
			TotalElaspedTime,
			WaveChoppyness,
			OceanSizeLxLz,
			RTSize);
		
		FFTPassParams.TwiddleFactorParams = FShaderTwiddleFactorParameters(RTSize);
		
		FFTPassParams.ButterflyParamsX = FShaderIFFTButterflyParameters(RTSize);
		FFTPassParams.ButterflyParamsY = FShaderIFFTButterflyParameters(RTSize);
		FFTPassParams.ButterflyParamsZ = FShaderIFFTButterflyParameters(RTSize);

		FFTPassParams.ButterflyParamsDHxKt = FShaderIFFTButterflyParameters(RTSize);
		FFTPassParams.ButterflyParamsDHzKt = FShaderIFFTButterflyParameters(RTSize);

		FFTPassParams.ButterflyParamsDxz = FShaderIFFTButterflyParameters(RTSize);
		FFTPassParams.ButterflyParamsDxx = FShaderIFFTButterflyParameters(RTSize);
		FFTPassParams.ButterflyParamsDzz = FShaderIFFTButterflyParameters(RTSize);

		FFTPassParams.FinalWavesParams = FShaderFinalWavesParameters(RTSize, TotalElaspedTime, WaveChoppyness, NormalStrength, OceanSizeLxLz);

		if (FFTPassParams.DisplacementRT == nullptr)
		{
			FFTPassParams.DisplacementRT = DisplacementRT;
		}
		
		if (FFTPassParams.DerivativesRT == nullptr)
		{
			FFTPassParams.DerivativesRT = DerivativesRT;
		}

		if (FFTPassParams.TurbulenceRT == nullptr)
		{
			FFTPassParams.TurbulenceRT = TurbulenceRT;
		}
		
		if (FFTPassParams.DebugRT == nullptr)
		{
			FFTPassParams.DebugRT = Debug_RT;
		}
		
		if (FShaderDeclarationModule::IsAvailable("ShaderDeclaration"))
		{
			FShaderDeclarationModule::Get("ShaderDeclaration").UpdateParameters("Pass", FFTPassParams);
		}
		});
	
}

static float GetGaussianRandomFloat()
{
	float u1 = FMath::Rand() / ((float)RAND_MAX);
	float u2 = FMath::Rand() / ((float)RAND_MAX);
	if (u1 < 1e-6f)
	{
		u1 = 1e-6f;
	}
	return sqrtf(-2.0f * logf(u1)) * cosf(2.0f * PI * u2);
}

void AFFTOcean::ConstructGaussianNoise()
{
	ENQUEUE_RENDER_COMMAND(GaussianNoiseConstruct)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			// GaussianNoise 随机生成 512x512 个像素点 对应顶点信息
			uint32 resolution = RTSize * RTSize;
			float rawData[resolution * 4];
			// rawData.SetNumZeroed(resolution * 4);
			for (uint32 i = 0; i < resolution * 4; i++)
			{
				rawData[i] = GetGaussianRandomFloat();
			}

			FRHIResourceCreateInfo CreateInfo(TEXT("CreateGaussianNoise"));

			GaussianNoiseRHI = RHICreateTexture2D(
				RTSize, RTSize, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource, CreateInfo);

			FUpdateTextureRegion2D UpdateTextureRegion2D = FUpdateTextureRegion2D(0, 0, 0, 0, RTSize, RTSize);

			RHICmdList.UpdateTexture2D(
				GaussianNoiseRHI, 0, UpdateTextureRegion2D,
				GPixelFormats[GaussianNoiseRHI->GetFormat()].BlockBytes * RTSize,
				reinterpret_cast<uint8*>(rawData));
			
			GaussianNoiseSRV = RHICreateShaderResourceView(GaussianNoiseRHI, 0);

			bIsGaussianInitialized = true;
		});
}

void AFFTOcean::CopyGaussianNoiseTextureToResourceViewOnce()
{
	// Let's enqueue a render command to create our gaussian noise distribution and store it into a shader resource view that later we'll pass to the ocean simulator
	// This one will be a one shot operation. A better place for this operation will be in the BeginPlay method of this actor.
	if (GaussianNoise != nullptr && GaussianNoiseSRV == nullptr)
	{
		ENQUEUE_RENDER_COMMAND(GaussianNoiseSRVCreationCommand)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				FTexture2DRHIRef Tex2DRHIRef = ((GaussianNoise->Resource))->GetTexture2DRHI();
				GaussianNoiseSRV = RHICreateShaderResourceView(Tex2DRHIRef, 0);
			});
		FlushRenderingCommands();
	}
}


