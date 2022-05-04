#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "ShaderModule.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FFTOceanActor.generated.h"

UCLASS()
class FFTOCEAN_API AFFTOcean : public AActor
{
	GENERATED_BODY()

public:
	AFFTOcean();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// called when begin to destory
	virtual void BeginDestroy() override;

	// virtual bool ShouldTickIfViewportsOnly() const override;

public:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	UTextureRenderTarget2D* DisplacementRT = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	UTextureRenderTarget2D* DerivativesRT = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	UTextureRenderTarget2D* TurbulenceRT = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	UTextureRenderTarget2D* Debug_RT = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ocean")
	UTexture2D* GaussianNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean")
	float RTSize = 512.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	FVector2D WindDirection = FVector2D(0.5, 0.5);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	FVector2D OceanSizeLxLz = FVector2D(2000., 2000.);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	float WindScale = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	float WindSpeed = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	float WaveAmplitude = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	float WaveChoppyness = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	float TimeFactor = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	float NormalStrength = 1.0f;

protected:
	void ConstructGaussianNoise();
	void CopyGaussianNoiseTextureToResourceViewOnce();

	float TotalElaspedTime = 0;

	// FRenderFFTPassParams FFTPassParams;

	//Gaussian noise shader resource view
	TAtomic<bool> bIsGaussianInitialized;
	FTexture2DRHIRef GaussianNoiseRHI;
	FShaderResourceViewRHIRef GaussianNoiseSRV;
};