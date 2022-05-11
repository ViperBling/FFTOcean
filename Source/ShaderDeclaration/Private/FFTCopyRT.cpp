#include "FFTCopyRT.h"

#include "CommonRenderResources.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"


class FCopyRTVertexBuffer : public FVertexBuffer
{
public:
	void InitRHI() override
	{
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);

		Vertices[0].Position = FVector4f(1, 1, 0, 1);
		Vertices[0].UV = FVector2f(1, 1);

		Vertices[1].Position = FVector4f(-1, 1, 0, 1);
		Vertices[1].UV = FVector2f(0, 1);

		Vertices[2].Position = FVector4f(1, -1, 0, 1);
		Vertices[2].UV = FVector2f(1, 0);

		Vertices[3].Position = FVector4f(-1, -1, 0, 1);
		Vertices[3].UV = FVector2f(0, 0);

		// Create vertex buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(TEXT("VertexBufferCreateInfo"), &Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

TGlobalResource<FCopyRTVertexBuffer> GCopyRTVertexBuffer;

class FCopyRTVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FCopyRTVS);

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FCopyRTVS() { }
	FCopyRTVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) { }
};

class FCopyRTPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FCopyRTPS);
	SHADER_USE_PARAMETER_STRUCT(FCopyRTPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, CopyRTSampler)
		//SHADER_PARAMETER(FVector2D, TextureSize) // Metal doesn't support GetDimensions(), so we send in this data via our parameters. FFT 不考虑Metal
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

IMPLEMENT_GLOBAL_SHADER(FCopyRTVS, "/OceanShaders/Private/FFTCopyRT.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FCopyRTPS, "/OceanShaders/Private/FFTCopyRT.usf", "MainPS", SF_Pixel);

void FCopyRT::DrawToRenderTarget_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FShaderCopyRT& CopyRTParams,
	FShaderResourceViewRHIRef InputTextureRT)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_PixelShader);
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_Pixel);

	FRHIRenderPassInfo RenderPassInfo(
		CopyRTParams.RenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(),
		ERenderTargetActions::Clear_Store);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("ShaderPlugin_OutputToRenderTarget"));

	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FCopyRTVS> VertexShader(ShaderMap);
	TShaderMapRef<FCopyRTPS> PixelShader(ShaderMap);

	// Set the graphic pipeline state.
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
	GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	// Setup the pixel shader
	FCopyRTPS::FParameters PassParameters;
	PassParameters.InputTexture = InputTextureRT;
	PassParameters.CopyRTSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	
	SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PassParameters);

	// Draw
	RHICmdList.SetStreamSource(0, GCopyRTVertexBuffer.VertexBufferRHI, 0);
	RHICmdList.DrawPrimitive(0, 2, 1);
	// Resolve render target
	RHICmdList.CopyToResolveTarget(
	CopyRTParams.RenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(),
		CopyRTParams.RenderTarget->GetRenderTargetResource()->TextureRHI, FResolveParams());

	RHICmdList.EndRenderPass();
}



class FNormalPassVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FNormalPassVS);

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FNormalPassVS() { }
	FNormalPassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) { }
};

class FNormalPassPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FNormalPassPS);
	SHADER_USE_PARAMETER_STRUCT(FNormalPassPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D, InputTextureX)
		SHADER_PARAMETER_SRV(Texture2D, InputTextureY)
		SHADER_PARAMETER_SRV(Texture2D, InputTextureZ)
		SHADER_PARAMETER_SRV(Texture2D, InputDHxKt)
		SHADER_PARAMETER_SRV(Texture2D, InputDHzKt)
		SHADER_PARAMETER_SRV(Texture2D, InputDxzKt)
		SHADER_PARAMETER_SRV(Texture2D, InputDxxKt)
		SHADER_PARAMETER_SRV(Texture2D, InputDzzKt)
		SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)
		SHADER_PARAMETER(FVector2f, OceanSizeLxLz)
		SHADER_PARAMETER(float, Size)
		SHADER_PARAMETER(float, Choppyness)
		SHADER_PARAMETER(float, NormalStrength)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

IMPLEMENT_GLOBAL_SHADER(FNormalPassVS, "/OceanShaders/Private/FFTNormalPass.usf", "MainVertexShader", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FNormalPassPS, "/OceanShaders/Private/FFTNormalPass.usf", "NormalAndFoldingMain", SF_Pixel);

void FFTNormalPass::RunPixelShader_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FShaderNormalParameters& NormalParams,
	FShaderResourceViewRHIRef InputTextureX,
	FShaderResourceViewRHIRef InputTextureY,
	FShaderResourceViewRHIRef InputTextureZ,
	FShaderResourceViewRHIRef InputDHxKt,
	FShaderResourceViewRHIRef InputDHzKt,
	FShaderResourceViewRHIRef InputDxzKt,
	FShaderResourceViewRHIRef InputDxxKt,
	FShaderResourceViewRHIRef InputDzzKt)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_PixelShader);
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_Pixel);

	FRHIRenderPassInfo RenderPassInfo(
		NormalParams.RenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(),
		ERenderTargetActions::Clear_Store);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("ShaderPlugin_OutputNormalToRT"));

	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FNormalPassVS> VertexShader(ShaderMap);
	TShaderMapRef<FNormalPassPS> PixelShader(ShaderMap);

	// Set the graphic pipeline state.
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
	GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	// Setup the pixel shader
	FNormalPassPS::FParameters PassParameters;
	PassParameters.InputTextureX = InputTextureX;
	PassParameters.InputTextureY = InputTextureY;
	PassParameters.InputTextureZ = InputTextureZ;
	PassParameters.InputDHxKt = InputDHxKt;
	PassParameters.InputDHzKt = InputDHzKt;

	PassParameters.InputDxzKt = InputDxzKt;
	PassParameters.InputDxxKt = InputDxxKt;
	PassParameters.InputDzzKt = InputDzzKt;
	
	PassParameters.LinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters.OceanSizeLxLz = NormalParams.mOceanSizeLxLz;
	PassParameters.Size = NormalParams.mSize;
	PassParameters.Choppyness = NormalParams.mChoppyness;
	PassParameters.NormalStrength = NormalParams.mNormalStrength;
	
	SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PassParameters);

	// Draw
	RHICmdList.SetStreamSource(0, GCopyRTVertexBuffer.VertexBufferRHI, 0);
	RHICmdList.DrawPrimitive(0, 2, 1);
	// Resolve render target
	RHICmdList.CopyToResolveTarget(
	NormalParams.RenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(),
		NormalParams.RenderTarget->GetRenderTargetResource()->TextureRHI, FResolveParams());

	RHICmdList.EndRenderPass();
}
