#include "Engine_Shader_Define.hlsli"
/* 상수 테이블 */
float4x4 g_WorldMatrix;
float4x4 g_ViewMatrix;
float4x4 g_ProjMatrix;

float4 g_vLook;
float4 g_vUVLTRB;
float4 g_vColor;

float g_RGBTest;
float g_AlphaTest;

Texture2D g_DiffuseTexture;
float g_fTimeAcc;
float3 g_fBrightness = float3(0.2126, 0.7152, 0.0722);
float4 g_vColorIntensity = { 1.0f, 1.0f, 1.0f, 1.0f };
float g_fBloomThreshold;


/* 구조체 */
struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vPSize : PSIZE;
};

struct VS_OUT
{
    float4 vPosition : POSITION;
    float2 vPSize : PSIZE;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    // 0, 0, / 1, 0 / 1, 1 / 0, 1
    
    float fScaleX = length(g_WorldMatrix._11_12_13);
    float fScaleY = length(g_WorldMatrix._21_22_23);
   
    // Size에 대한 값을 psize 하나로 처리하기 위해 기존 InstancingMatrix의 Scale을 1로 밀어버리고 곱하였다. 
    vector vPostion = mul(float4(In.vPosition, 1.0f), g_WorldMatrix);
    // Geometry Shader에서 기하도형을 구성하기위해 Camera의 WorldPos 를 사용할 예정이다. >> 정점을 World까지만 변환 후 GeometryShader에서 도형 찍고 View, Projection 곱하자. 
    Out.vPosition = mul(vPostion, g_WorldMatrix);
    Out.vPSize = float2(In.vPSize.x * fScaleX, In.vPSize.y * fScaleY);
    
    return Out;
}

struct GS_IN
{
    float4 vPosition : POSITION;
    float2 vPSize : PSIZE;
};

struct GS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float fWeight : TEXCOORD1;
};


[maxvertexcount(6)]
void GS_MAIN(point GS_IN In[1], inout TriangleStream<GS_OUT> OutStream)
{
    GS_OUT Out[4];
    
    vector vLookDir = g_vLook;
    float3 vRightDir = normalize(cross(float3(0.0f, 1.0f, 0.0f), vLookDir.xyz));
    float3 vUpDir = normalize(cross(vLookDir.xyz, vRightDir));
    
    float3 vCenter = In[0].vPosition.xyz;
    float3 vRightDist = vRightDir * In[0].vPSize.x * 0.5f;
    float3 vUpDist = vUpDir * In[0].vPSize.y * 0.5f;
    
    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);
    
     // 정점 기준 우상단 (카메라가 바라보는 기준 좌상단.)
      // 정점 기준 우상단 (카메라가 바라보는 기준 좌상단.)
    Out[0].vPosition = float4(vCenter + vRightDist + vUpDist, 1.0f);
    Out[0].vPosition = mul(Out[0].vPosition, matVP);
    Out[0].vTexcoord = float2(g_vUVLTRB.x, g_vUVLTRB.y);

    // 정점 기준 좌상단 (카메라가 바라보는 기준 우상단.)
    Out[1].vPosition = float4(vCenter - vRightDist + vUpDist, 1.0f);
    Out[1].vPosition = mul(Out[1].vPosition, matVP);
    Out[1].vTexcoord = float2(g_vUVLTRB.z, g_vUVLTRB.y);
    
    // 정점 기준 좌하단 (카메라가 바라보는 기준 우하단.)
    Out[2].vPosition = float4(vCenter - vRightDist - vUpDist, 1.0f);
    Out[2].vPosition = mul(Out[2].vPosition, matVP);
    Out[2].vTexcoord = float2(g_vUVLTRB.z, g_vUVLTRB.w);
    
    // 정점 기준 우하단 (카메라가 바라보는 기준 좌하단.)
    Out[3].vPosition = float4(vCenter + vRightDist - vUpDist, 1.0f);
    Out[3].vPosition = mul(Out[3].vPosition, matVP);
    Out[3].vTexcoord = float2(g_vUVLTRB.x, g_vUVLTRB.w);

    float fDepth = mul(In[0].vPosition, matVP).w;
    float fWeight = clamp(10.f / (1e-5 + pow(fDepth / 10.f, 3.0f) + pow(fDepth / 200.f, 6.f)), 1e-2, 3e3);

    Out[0].fWeight = fWeight;
    Out[1].fWeight = fWeight;
    Out[2].fWeight = fWeight;
    Out[3].fWeight = fWeight;
    
    OutStream.Append(Out[0]);
    OutStream.Append(Out[1]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();

    OutStream.Append(Out[0]);
    OutStream.Append(Out[2]);
    OutStream.Append(Out[3]);
    OutStream.RestartStrip();
    
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float fWeight : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

struct PS_OUT_EFFECT
{
    float4 vColor : SV_TARGET0;
    float4 vBrightness : SV_TARGET1;
};

struct PS_OUT_WEIGHTEDBLENDED
{
    float4 vAccumulate : SV_TARGET0;
    float vRevealage : SV_TARGET1;
};

struct PS_OUT_WEIGHTEDBLENDED_BLOOM
{
    float4 vAccumulate : SV_TARGET0;
    float vRevealage : SV_TARGET1;
    float4 vBright : SV_TARGET2;
};




PS_OUT_WEIGHTEDBLENDED PS_WEIGHT_BLENDED(PS_IN In)
{
    PS_OUT_WEIGHTEDBLENDED Out = (PS_OUT_WEIGHTEDBLENDED) 0;        
    
    float4 vColor = g_DiffuseTexture.Sample(PointSampler, In.vTexcoord);
    
    vColor.a = vColor.r; 
    
    if (g_AlphaTest > vColor.a)
        discard;
    
    vColor *= g_vColor;    
    vColor = vColor + vColor * g_vColorIntensity;

    
    // 아래 식 추가 선택
    /*max(min(1.0, max(max(vColor.r, vColor.g), vColor.b) * vColor.a), vColor.a)*/
    // Weight 식 선택 후 적용, 혹은 1.0
    float fWeight = In.fWeight * max(min(1.0, max(max(vColor.r, vColor.g), vColor.b) * vColor.a), vColor.a);
    //float fWeight = In.fWeight * max(min(1.0, max(max(vColor.r, vColor.g), vColor.b) * vColor.a), vColor.a) * vColor.a;
    //float fWeight = In.fWeight * vColor.a;

    Out.vAccumulate.rgb = vColor.rgb * vColor.a * fWeight;
    Out.vAccumulate.a = vColor.a * fWeight;
    Out.vRevealage.r = vColor.a * clamp(log(0.6f + vColor.a), 0.25f, 0.6f);
        
    return Out;
}



PS_OUT_WEIGHTEDBLENDED_BLOOM PS_WEIGHT_BLENDEDBLOOM(PS_IN In)
{
    PS_OUT_WEIGHTEDBLENDED_BLOOM Out = (PS_OUT_WEIGHTEDBLENDED_BLOOM) 0;       
    
    float4 vColor = g_DiffuseTexture.Sample(PointSampler, In.vTexcoord);
    
    vColor.a = vColor.r;
    
    if (g_AlphaTest > vColor.a)
        discard;
    
    vColor *= g_vColor;
    
    vColor = vColor + vColor * g_vColorIntensity;
        

    
    // 아래 식 추가 선택
    /*max(min(1.0, max(max(vColor.r, vColor.g), vColor.b) * vColor.a), vColor.a)*/
    // Weight 식 선택 후 적용, 혹은 1.0
    float fWeight = In.fWeight * max(min(1.0, max(max(vColor.r, vColor.g), vColor.b) * vColor.a), vColor.a);
    //float fWeight = In.fWeight * max(min(1.0, max(max(vColor.r, vColor.g), vColor.b) * vColor.a), vColor.a) * vColor.a;
    //float fWeight = In.fWeight * vColor.a;
    
    float fLuminance = dot(vColor.rgb, g_fBrightness);
    float fBrightness = max(fLuminance - g_fBloomThreshold, 0.0) / ((length(vColor.rgb) * 0.33f - g_fBloomThreshold));

    Out.vAccumulate.rgb = vColor.rgb * vColor.a * fWeight;
    Out.vAccumulate.a = vColor.a * fWeight;
    Out.vRevealage.r = vColor.a * clamp(log(0.6f + vColor.a), 0.25f, 0.6f);
    
    Out.vBright = Out.vAccumulate * fBrightness;
    
    return Out;
}


technique11 DefaultTechnique
{
	/* 우리가 수행해야할 정점, 픽셀 셰이더의 진입점 함수를 지정한다. */
    pass BILLBOARD_WEIGHTED_BLENDED // 0
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDED();
        ComputeShader = NULL;

    }

    pass BILLBOARD_WEIGHTED_BLENDEDBLOOM // 1
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDBLOOM();
        ComputeShader = NULL;

    }
}