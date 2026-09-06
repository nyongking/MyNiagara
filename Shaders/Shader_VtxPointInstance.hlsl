#include "Engine_Shader_Define.hlsli"
/* 상수 테이블 */
float4x4        g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;
float4x4        g_InverseMatrix;
float4x4        g_WorldPosMatrix;
Texture2D       g_DiffuseTexture, g_NoiseTexture, g_DistortionTexture;

vector          g_vCamPosition, g_vLook;
float4          g_vParticleColor = { 1.0f, 1.0f, 1.0f, 1.0f };
float4          g_vSubColor = { 1.0f, 1.0f, 1.0f, 1.0f };
float4          g_vNoiseUVScale, g_vEdgeColor, g_vDissolveColor, g_vDistortionUVScale ;
float           g_fEdgeWidth, g_fDissolveTimeFactor, g_fDissolveFactor;

float4          g_vColorIntensity = { 1.0f, 1.0f, 1.0f, 1.0f };
float g_fMaskBrightness = { 1.0f }, g_fColorScale = { 1.0f };
float g_fStrength;
float g_Near, g_Far;
float g_AlphaTest;
float g_fBloomThreshold;
float g_fAbsolute;
float2 g_NoiseIntensity;


// Weight에 대한 식은 총 4개
float FUNC_WEIGHT1(float fDepth, float fAlpha)
{
    return fAlpha * clamp(10.f / (1e-5 + pow(fDepth / 5.f, 2.0f) + pow(fDepth / 200.f, 6.f)), 1e-2, 3e3);
}

float FUNC_WEIGHT2(float fDepth, float fAlpha)
{
    return fAlpha * clamp(10.f / (1e-5 + pow(fDepth / 10.f, 3.0f) + pow(fDepth / 200.f, 6.f)), 1e-2, 3e3);
}

float FUNC_WEIGHT3(float fDepth, float fAlpha)
{
    return fAlpha * clamp(10.f / (1e-5 + pow(fDepth / 200.f, 4.f)), 1e-2, 3e3);
}

float FUNC_WEIGHT4(float fDepth, float fAlpha)
{
    float fdZ = (g_Near * g_Far) / (fDepth - g_Far) / (g_Near - g_Far);
    return fAlpha * clamp(pow(1 - fdZ, 3.f), 1e-2, 3e3);
}


/* 구조체 */
struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vPSize : PSIZE;              // Point Size
    
    /* Instancing Buffer Data */
    row_major float4x4 InstancingMatrix : WORLD;
    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float4 vTexcoord : TEXCOORD2;
    float3 vVelocity : TEXCOORD3;
    float3 vAcceleration : TEXCOORD4;
    float fAlive : TEXCOORD5;
    float fRandom : TEXCOORD6;
};



struct VS_OUT
{
    float4 vPosition : POSITION; // SV_POSITION : 그리기전에 필요한 변환들이 끝났어! >>> z 나누기 및 래스터라이져 과정을 수행해!  없다면 >>> 할일이 남았어.
    float2 vPSize : PSIZE;
    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float4 vTexcoord : TEXCOORD2;
    float3 vVelocity : TEXCOORD3;
    float fRandom : TEXCOORD4;

};

struct VS_ROUT
{
    float4 vPosition : POSITION;
    float2 vPSize : PSIZE;
    
    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float4 vTexcoord : TEXCOORD2;
    float3 vVelocity : TEXCOORD3;
    float3 vUp : TEXCOORD4;
    float fRandom : TEXCOORD5;

};

struct VS_NBOUT
{
    float4 vPosition : POSITION;
    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float4 vTexcoord : TEXCOORD2;
    float3 vRight : TEXCOORD3;
    float3 vUp : TEXCOORD4;
    float fRandom : TEXCOORD5;
};




struct GS_IN
{
    float4 vPosition : POSITION;
    float2 vPSize : PSIZE;
    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float4 vTexcoord : TEXCOORD2;
    float3 vVelocity : TEXCOORD3;
    float fRandom : TEXCOORD4;

};



struct GS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float2 vTexcoord : TEXCOORD2;
    float fWeight : TEXCOORD3;
    float2 vOriginTexcoord : TEXCOORD4;
    float fRandom : TEXCOORD5;

};

struct GS_RIN
{
    float4 vPosition : POSITION;
    float2 vPSize : PSIZE;

    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float4 vTexcoord : TEXCOORD2;
    float3 vVelocity : TEXCOORD3;
    float3 vUp : TEXCOORD4;
    float fRandom : TEXCOORD5;


};

// Rendering PipeLine : PixelShader //
struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float2 vTexcoord : TEXCOORD2;
    float fWeight : TEXCOORD3;
    float2 vOriginTexcoord : TEXCOORD4;
    float fRandom : TEXCOORD5;

};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

struct  Sprite_Particle
{
    row_major float4x4 InstancingMatrix : WORLD;
    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float4 vTexcoord : TEXCOORD2;
    float3 vVelocity : TEXCOORD3;
    float3 vAcceleration : TEXCOORD4;
    float fAlive : TEXCOORD5;
    float fRandom : TEXCOORD6;

};

StructuredBuffer<Sprite_Particle> Particles : register(t0);

VS_OUT VS_SRV_MAIN(uint iVertexID : SV_VertexID)
{
    VS_OUT Out;
 
        
    float fScaleX = length(Particles[iVertexID].InstancingMatrix._11_12_13) * Particles[iVertexID].fAlive;
    float fScaleY = length(Particles[iVertexID].InstancingMatrix._21_22_23) * Particles[iVertexID].fAlive;
   
    matrix FinalMatrix = float4x4(Particles[iVertexID].InstancingMatrix._11_12_13_14,
                                    Particles[iVertexID].InstancingMatrix._21_22_23_24,
                                    Particles[iVertexID].InstancingMatrix._31_32_33_34,
                                    Particles[iVertexID].InstancingMatrix._41_42_43_44);
    
    vector vPostion = mul(float4(0.f, 0.f, 0.f, 1.0f), FinalMatrix);

    Out.vPosition = mul(vPostion, g_WorldMatrix) * (1.f - g_fAbsolute) + g_fAbsolute * vPostion;
    Out.vPSize = float2(fScaleX, fScaleY);
    Out.vLifeTime = Particles[iVertexID].vLifeTime;
    Out.vTexcoord = Particles[iVertexID].vTexcoord;
    Out.vColor = Particles[iVertexID].vColor;
    
    float fScaleVelocity = length(Particles[iVertexID].vVelocity);
    
    Out.vVelocity = normalize(mul(float4(Particles[iVertexID].vVelocity, 0.f), g_WorldMatrix)) * fScaleVelocity;
    Out.fRandom = Particles[iVertexID].fRandom;
    
    return Out;
}

VS_ROUT VS_SRV_RMAIN(uint iVertexID : SV_VertexID)
{
    VS_ROUT Out;
    
    float fAlive = step(0.f, Particles[iVertexID].vLifeTime.y) * step(Particles[iVertexID].vLifeTime.y, Particles[iVertexID].vLifeTime.x);

    float fScaleX = length(Particles[iVertexID].InstancingMatrix._11_12_13) * Particles[iVertexID].fAlive;
    float fScaleY = length(Particles[iVertexID].InstancingMatrix._21_22_23) * Particles[iVertexID].fAlive;
   
    matrix FinalMatrix = float4x4(Particles[iVertexID].InstancingMatrix._11_12_13_14,
                                    Particles[iVertexID].InstancingMatrix._21_22_23_24,
                                    Particles[iVertexID].InstancingMatrix._31_32_33_34,
                                    Particles[iVertexID].InstancingMatrix._41_42_43_44);
    
    vector vPostion = mul(float4(0.f, 0.f, 0.f, 1.0f), FinalMatrix);

    Out.vPosition = mul(vPostion, g_WorldMatrix) * (1.f - g_fAbsolute) + g_fAbsolute * vPostion;
    Out.vPSize = float2(fScaleX, fScaleY);
    Out.vLifeTime = Particles[iVertexID].vLifeTime;
    Out.vTexcoord = Particles[iVertexID].vTexcoord;
    Out.vColor = Particles[iVertexID].vColor;
    
    float fScaleVelocity = length(Particles[iVertexID].vVelocity);
    
    Out.vVelocity = normalize(/*mul(*/float4(Particles[iVertexID].vVelocity, 0.f)/*, g_WorldMatrix)*/) * fScaleVelocity;
    //Out.vUp = mul((Particles[iVertexID].InstancingMatrix._21_22_23_24), g_WorldMatrix) * (1.f - g_fAbsolute)
    //+ g_fAbsolute * Particles[iVertexID].InstancingMatrix._21_22_23_24;
    Out.vUp = mul(Particles[iVertexID].InstancingMatrix._21_22_23_24, g_WorldMatrix);
    Out.fRandom = Particles[iVertexID].fRandom;

    return Out;
}

VS_ROUT VS_SRV_RMAIN2(uint iVertexID : SV_VertexID)
{
    VS_ROUT Out;
    
    float fAlive = step(0.f, Particles[iVertexID].vLifeTime.y) * step(Particles[iVertexID].vLifeTime.y, Particles[iVertexID].vLifeTime.x);

    float fScaleX = length(Particles[iVertexID].InstancingMatrix._11_12_13) * Particles[iVertexID].fAlive;
    float fScaleY = length(Particles[iVertexID].InstancingMatrix._21_22_23) * Particles[iVertexID].fAlive;
   
    matrix FinalMatrix = float4x4(Particles[iVertexID].InstancingMatrix._11_12_13_14,
                                    Particles[iVertexID].InstancingMatrix._21_22_23_24,
                                    Particles[iVertexID].InstancingMatrix._31_32_33_34,
                                    Particles[iVertexID].InstancingMatrix._41_42_43_44);
    
    vector vPostion = mul(float4(0.f, 0.f, 0.f, 1.0f), FinalMatrix);

    Out.vPosition = mul(vPostion, g_WorldMatrix) * (1.f - g_fAbsolute) + g_fAbsolute * vPostion;
    Out.vPSize = float2(fScaleX, fScaleY);
    Out.vLifeTime = Particles[iVertexID].vLifeTime;
    Out.vTexcoord = Particles[iVertexID].vTexcoord;
    Out.vColor = Particles[iVertexID].vColor;
    
    float fScaleVelocity = length(Particles[iVertexID].vVelocity);
    
    Out.vVelocity = normalize(mul(float4(Particles[iVertexID].vVelocity, 0.f), g_WorldMatrix)) * fScaleVelocity;
    //Out.vUp = mul((Particles[iVertexID].InstancingMatrix._21_22_23_24), g_WorldMatrix) * (1.f - g_fAbsolute)
    //+ g_fAbsolute * Particles[iVertexID].InstancingMatrix._21_22_23_24;
    Out.vUp = mul(Particles[iVertexID].InstancingMatrix._21_22_23_24, g_WorldMatrix);
    Out.fRandom = Particles[iVertexID].fRandom;

    return Out;
}

VS_NBOUT VS_SRV_NO_BILLBOARD(uint iVertexID : SV_VertexID)
{
    VS_NBOUT Out = (VS_NBOUT) 0;

    float4 vPosition = Particles[iVertexID].InstancingMatrix._41_42_43_44;
    float3 vRight = Particles[iVertexID].InstancingMatrix._11_12_13;
    float3 vUp = Particles[iVertexID].InstancingMatrix._21_22_23;
    float fAlive = Particles[iVertexID].fAlive;

    float4 vRelativePosition = mul(vPosition, g_WorldMatrix);
    float3 vRelativeRight = mul(float4(vRight, 0.f), g_WorldMatrix).xyz;
    float3 vRelativeUp = mul(float4(vUp, 0.f), g_WorldMatrix).xyz;

    Out.vPosition = vRelativePosition * (1.f - g_fAbsolute) + vPosition * g_fAbsolute;
    Out.vRight = (vRelativeRight * (1.f - g_fAbsolute) + vRight * g_fAbsolute) * fAlive;
    Out.vUp = (vRelativeUp * (1.f - g_fAbsolute) + vUp * g_fAbsolute) * fAlive;
    Out.vLifeTime = Particles[iVertexID].vLifeTime;
    Out.vColor = Particles[iVertexID].vColor;
    Out.vTexcoord = Particles[iVertexID].vTexcoord;
    Out.fRandom = Particles[iVertexID].fRandom;

    return Out;
}

// Rendering PipeLine : Vertex Shader // 
VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;
    
    // y가 크면 -> 0의 크기
    float fAlive = step(In.vLifeTime.y, In.vLifeTime.x) /** step(0.f, In.vLifeTime.y)*/;
    
    // Scale 값을 별도로 추출하여, PSize에 곱하여 줄 것이다. 그리고 , Instancing Matrix에서는 Scale을 1로 고정시킬 것 이다. 
    float fScaleX = length(In.InstancingMatrix._11_12_13) * fAlive;
    float fScaleY = length(In.InstancingMatrix._21_22_23) * fAlive;
    //float fScaleZ = length(In.InstancingMatrix._31_32_33);
   
    matrix FinalMatrix = float4x4(  In.InstancingMatrix._11_12_13_14,
                                    In.InstancingMatrix._21_22_23_24,
                                    In.InstancingMatrix._31_32_33_34,
                                    In.InstancingMatrix._41_42_43_44);
    
    // Size에 대한 값을 psize 하나로 처리하기 위해 기존 InstancingMatrix의 Scale을 1로 밀어버리고 곱하였다. 
    vector vPostion = mul(float4(In.vPosition, 1.0f), FinalMatrix);
    // Geometry Shader에서 기하도형을 구성하기위해 Camera의 WorldPos 를 사용할 예정이다. >> 정점을 World까지만 변환 후 GeometryShader에서 도형 찍고 View, Projection 곱하자. 
    Out.vPosition = mul(vPostion, g_WorldMatrix);
    Out.vPSize = float2(In.vPSize.x * fScaleX, In.vPSize.y * fScaleY);
    Out.vLifeTime = In.vLifeTime;
    Out.vTexcoord = In.vTexcoord;
    Out.vColor = In.vColor;
    Out.fRandom = In.fRandom;

    
    float fScaleVelocity = length(In.vVelocity);
    
    Out.vVelocity = normalize(mul(float4(In.vVelocity, 0.f), g_WorldMatrix)) * fScaleVelocity;
    
    return Out;
}

VS_ROUT VS_RMAIN(VS_IN In)
{
    VS_ROUT Out = (VS_ROUT) 0;
    float fAlive = step(In.vLifeTime.y, In.vLifeTime.x) /** step(0.f, In.vLifeTime.y)*/;

    // Scale 값을 별도로 추출하여, PSize에 곱하여 줄 것이다. 그리고 , Instancing Matrix에서는 Scale을 1로 고정시킬 것 이다. 
    float fScaleX = length(In.InstancingMatrix._11_12_13) * fAlive;
    float fScaleY = length(In.InstancingMatrix._21_22_23) * fAlive;
    //float fScaleZ = length(In.InstancingMatrix._31_32_33);
   
    matrix FinalMatrix = float4x4(In.InstancingMatrix._11_12_13_14,
                                    In.InstancingMatrix._21_22_23_24,
                                    In.InstancingMatrix._31_32_33_34,
                                    In.InstancingMatrix._41_42_43_44);
    
    // Size에 대한 값을 psize 하나로 처리하기 위해 기존 InstancingMatrix의 Scale을 1로 밀어버리고 곱하였다. 
    vector vPostion = mul(float4(In.vPosition, 1.0f), FinalMatrix);
    // Geometry Shader에서 기하도형을 구성하기위해 Camera의 WorldPos 를 사용할 예정이다. >> 정점을 World까지만 변환 후 GeometryShader에서 도형 찍고 View, Projection 곱하자. 
    Out.vPosition = mul(vPostion, g_WorldMatrix);
    Out.vPSize = float2(In.vPSize.x * fScaleX, In.vPSize.y * fScaleY);
    Out.vLifeTime = In.vLifeTime;
    Out.vTexcoord = In.vTexcoord;
    Out.vColor = In.vColor;
    Out.vVelocity = In.vVelocity;
    Out.vUp = mul((In.InstancingMatrix._21_22_23_24), g_WorldMatrix);
    Out.fRandom = In.fRandom;

    //VS_ROUT Out = (VS_ROUT) 0;
  
    //Out.vPosition = float4(In.vPosition, 1.f);
    //Out.vPSize = float2(In.vPSize.x, In.vPSize.y);
    
    //Out.InstancingMatrix = In.InstancingMatrix;
    //Out.vLifeTime = In.vLifeTime;
    //Out.vTexcoord = In.vTexcoord;
    //Out.vColor = In.vColor;
    //Out.vVelocity = In.vVelocity;
    
    return Out;
}



//GS_MAIN(triangle GS_IN In[3])
//GS_MAIN(line GS_IN In[2])
[maxvertexcount(6)] // VertexCount를 reserve 한다고 보면 된다. 꼭 6개의 정점을전부 보내야하는 것은아니다.
void GS_MAIN(point GS_IN In[1], inout TriangleStream<GS_OUT> OutStream)
{
    GS_OUT Out[4]; /* Output으로 내보낼 타입의 구조체 4개를 선언. 즉, 4개의 정점을 구성하고, 6개의 OutStream으로 내보낸다. */
    
    /* Camera World Position을 기반으로 현재 Shader에 들어온 Vertex를 중점으로하는 4각형을 찍을 것이다. */
    /* 주의 : 사각형 기준으로 정점을 찍지만, 카메라가 바라보는 방향기준으로 시계방향으로 정점을 찍어야한다. */
    //vector vLookDir = g_vCamPosition - In[0].vPosition;
    vector vLookDir = g_vLook;
    float3 vRightDir = normalize(cross(float3(0.0f, 1.0f, 0.0f), vLookDir.xyz));
    float3 vUpDir = normalize(cross(vLookDir.xyz, vRightDir));
    
    float3 vCenter = In[0].vPosition.xyz;
    float3 vRightDist = vRightDir * In[0].vPSize.x * 0.5f;
    float3 vUpDist = vUpDir * In[0].vPSize.y * 0.5f;
    
    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);
        
    // 정점 기준 우상단 (카메라가 바라보는 기준 좌상단.)
    Out[0].vPosition = float4(vCenter + vRightDist + vUpDist, 1.0f);
    Out[0].vPosition = mul(Out[0].vPosition, matVP);
    Out[0].vTexcoord = float2(In[0].vTexcoord.x, In[0].vTexcoord.y);
    Out[0].vLifeTime = In[0].vLifeTime;
    Out[0].vOriginTexcoord = float2(0.f, 0.f);
    //Out[0].fWeight = Out[0].vPosition.w;
    
    // 정점 기준 좌상단 (카메라가 바라보는 기준 우상단.)
    Out[1].vPosition = float4(vCenter - vRightDist + vUpDist, 1.0f);
    Out[1].vPosition = mul(Out[1].vPosition, matVP);
    Out[1].vTexcoord = float2(In[0].vTexcoord.z, In[0].vTexcoord.y);
    Out[1].vLifeTime = In[0].vLifeTime;
    Out[1].vOriginTexcoord = float2(1.f, 0.f);
    //Out[1].fWeight = Out[1].vPosition.w;

    // 정점 기준 좌하단 (카메라가 바라보는 기준 우하단.)
    Out[2].vPosition = float4(vCenter - vRightDist - vUpDist, 1.0f);
    Out[2].vPosition = mul(Out[2].vPosition,  matVP);
    Out[2].vTexcoord = float2(In[0].vTexcoord.z, In[0].vTexcoord.w);
    Out[2].vLifeTime = In[0].vLifeTime;
    Out[2].vOriginTexcoord = float2(1.f, 1.f);

    //Out[2].fWeight = Out[2].vPosition.w;

    // 정점 기준 우하단 (카메라가 바라보는 기준 좌하단.)
    Out[3].vPosition = float4(vCenter + vRightDist - vUpDist, 1.0f);
    Out[3].vPosition = mul(Out[3].vPosition, matVP);
    Out[3].vTexcoord = float2(In[0].vTexcoord.x, In[0].vTexcoord.w);
    Out[3].vLifeTime = In[0].vLifeTime;
    Out[3].vOriginTexcoord = float2(0.f, 1.f);

    //Out[3].fWeight = Out[3].vPosition.w;
    
    //float fdZ = (g_Near * g_Far) / (mul(In[0].vPosition, matVP).w - g_Far) / (g_Near - g_Far);
    //float fWeight = clamp(pow(1 - fdZ, 3.f), 1e-2, 3e3);
    float fDepth = mul(In[0].vPosition, matVP).w;
    float fWeight = clamp(10.f / (1e-5 + pow(fDepth / 10.f, 3.0f) + pow(fDepth / 200.f, 6.f)), 1e-2, 3e3);
    //float fWeight = clamp(10.f / (1e-5 + pow(mul(In[0].vPosition, matVP).w / 5.f, 2.0f) + pow(mul(In[0].vPosition, matVP).w / 200.f, 6.f)), 1e-2, 3e3);
    Out[0].fWeight = fWeight;
    Out[1].fWeight = fWeight;
    Out[2].fWeight = fWeight;
    Out[3].fWeight = fWeight;
    
    Out[0].vColor = In[0].vColor;
    Out[1].vColor = In[0].vColor;
    Out[2].vColor = In[0].vColor;
    Out[3].vColor = In[0].vColor;
    
    Out[0].fRandom = In[0].fRandom;
    Out[1].fRandom = In[0].fRandom;
    Out[2].fRandom = In[0].fRandom;
    Out[3].fRandom = In[0].fRandom;

    
    OutStream.Append(Out[0]);
    OutStream.Append(Out[1]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();

    OutStream.Append(Out[0]);
    OutStream.Append(Out[2]);
    OutStream.Append(Out[3]);
    OutStream.RestartStrip();
}

[maxvertexcount(6)]
void GS_NO_BILLBOARD(point VS_NBOUT In[1], inout TriangleStream<GS_OUT> OutStream)
{
    GS_OUT Out[4];

    float3 vCenter = In[0].vPosition.xyz;
    float3 vRightDist = In[0].vRight * 0.5f;
    float3 vUpDist = In[0].vUp * 0.5f;
    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);

    float3 vPositions[4];
    vPositions[0] = vCenter + vRightDist + vUpDist;
    vPositions[1] = vCenter - vRightDist + vUpDist;
    vPositions[2] = vCenter - vRightDist - vUpDist;
    vPositions[3] = vCenter + vRightDist - vUpDist;

    float2 vTexcoords[4];
    vTexcoords[0] = float2(In[0].vTexcoord.x, In[0].vTexcoord.y);
    vTexcoords[1] = float2(In[0].vTexcoord.z, In[0].vTexcoord.y);
    vTexcoords[2] = float2(In[0].vTexcoord.z, In[0].vTexcoord.w);
    vTexcoords[3] = float2(In[0].vTexcoord.x, In[0].vTexcoord.w);

    float2 vOriginTexcoords[4];
    vOriginTexcoords[0] = float2(0.f, 0.f);
    vOriginTexcoords[1] = float2(1.f, 0.f);
    vOriginTexcoords[2] = float2(1.f, 1.f);
    vOriginTexcoords[3] = float2(0.f, 1.f);

    float fDepth = mul(In[0].vPosition, matVP).w;
    float fWeight = clamp(10.f / (1e-5f + pow(fDepth / 10.f, 3.f) + pow(fDepth / 200.f, 6.f)), 1e-2f, 3e3f);

    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        Out[i].vPosition = mul(float4(vPositions[i], 1.f), matVP);
        Out[i].vLifeTime = In[0].vLifeTime;
        Out[i].vColor = In[0].vColor;
        Out[i].vTexcoord = vTexcoords[i];
        Out[i].fWeight = fWeight;
        Out[i].vOriginTexcoord = vOriginTexcoords[i];
        Out[i].fRandom = In[0].fRandom;
    }

    OutStream.Append(Out[0]);
    OutStream.Append(Out[1]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();

    OutStream.Append(Out[0]);
    OutStream.Append(Out[2]);
    OutStream.Append(Out[3]);
    OutStream.RestartStrip();
}

[maxvertexcount(6)]
void GS_NEWBILLBOARD(point GS_RIN In[1], inout TriangleStream<GS_OUT> OutStream)
{
    GS_OUT Out[4];
        
    
    vector vLookDir = g_vLook;
    float3 vRightDir = normalize(cross(normalize(In[0].vUp.xyz), vLookDir.xyz));
    float3 vUpDir = normalize(cross(vLookDir.xyz, vRightDir));
        
    float3 vCenter = In[0].vPosition.xyz;
    float3 vRightDist = vRightDir * In[0].vPSize.x * 0.5f;
    float3 vUpDist = vUpDir * In[0].vPSize.y;
    
    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);
        
    // 정점 기준 우상단 (카메라가 바라보는 기준 좌상단.)
    Out[0].vPosition = float4(vCenter + vRightDist + vUpDist, 1.0f);
    Out[0].vPosition = mul(Out[0].vPosition, matVP);
    Out[0].vTexcoord = float2(In[0].vTexcoord.x, In[0].vTexcoord.y);
    Out[0].vLifeTime = In[0].vLifeTime;
    Out[0].vOriginTexcoord = float2(0.f, 0.f);

    //Out[0].fWeight = Out[0].vPosition.w;
    
    // 정점 기준 좌상단 (카메라가 바라보는 기준 우상단.)
    Out[1].vPosition = float4(vCenter - vRightDist + vUpDist, 1.0f);
    Out[1].vPosition = mul(Out[1].vPosition, matVP);
    Out[1].vTexcoord = float2(In[0].vTexcoord.z, In[0].vTexcoord.y);
    Out[1].vLifeTime = In[0].vLifeTime;
    Out[1].vOriginTexcoord = float2(1.f, 0.f);

    //Out[1].fWeight = Out[1].vPosition.w;

    // 정점 기준 좌하단 (카메라가 바라보는 기준 우하단.)
    Out[2].vPosition = float4(vCenter - vRightDist, 1.0f);
    Out[2].vPosition = mul(Out[2].vPosition, matVP);
    Out[2].vTexcoord = float2(In[0].vTexcoord.z, In[0].vTexcoord.w);
    Out[2].vLifeTime = In[0].vLifeTime;
    Out[2].vOriginTexcoord = float2(1.f, 1.f);

    //Out[2].fWeight = Out[2].vPosition.w;

    // 정점 기준 우하단 (카메라가 바라보는 기준 좌하단.)
    Out[3].vPosition = float4(vCenter + vRightDist, 1.0f);
    Out[3].vPosition = mul(Out[3].vPosition, matVP);
    Out[3].vTexcoord = float2(In[0].vTexcoord.x, In[0].vTexcoord.w);
    Out[3].vLifeTime = In[0].vLifeTime;
    Out[3].vOriginTexcoord = float2(0.f, 1.f);

    //Out[3].fWeight = Out[3].vPosition.w;
    
    //float fdZ = (g_Near * g_Far) / (mul(In[0].vPosition, matVP).w - g_Far) / (g_Near - g_Far);
    //float fWeight = clamp(pow(1 - fdZ, 3.f), 1e-2, 3e3);
    float fDepth = mul(In[0].vPosition, matVP).w;
    float fWeight = clamp(10.f / (1e-5 + pow(fDepth / 10.f, 3.0f) + pow(fDepth / 200.f, 6.f)), 1e-2, 3e3);
    //float fWeight = clamp(10.f / (1e-5 + pow(mul(In[0].vPosition, matVP).w / 5.f, 2.0f) + pow(mul(In[0].vPosition, matVP).w / 200.f, 6.f)), 1e-2, 3e3);
    Out[0].fWeight = fWeight;
    Out[1].fWeight = fWeight;
    Out[2].fWeight = fWeight;
    Out[3].fWeight = fWeight;
    
    Out[0].vColor = In[0].vColor;
    Out[1].vColor = In[0].vColor;
    Out[2].vColor = In[0].vColor;
    Out[3].vColor = In[0].vColor;
    
        
    Out[0].fRandom = In[0].fRandom;
    Out[1].fRandom = In[0].fRandom;
    Out[2].fRandom = In[0].fRandom;
    Out[3].fRandom = In[0].fRandom;
    
    OutStream.Append(Out[0]);
    OutStream.Append(Out[1]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();

    OutStream.Append(Out[0]);
    OutStream.Append(Out[2]);
    OutStream.Append(Out[3]);
    OutStream.RestartStrip();
}


[maxvertexcount(6)]
void GS_NEWBILLBOARD2(point GS_RIN In[1], inout TriangleStream<GS_OUT> OutStream)
{
    GS_OUT Out[4];
        
    
    vector vLookDir = g_vLook;
    float3 vRightDir = normalize(cross(normalize(In[0].vUp.xyz), vLookDir.xyz));
    float3 vUpDir = normalize(cross(vLookDir.xyz, vRightDir));
        
    float3 vCenter = In[0].vPosition.xyz;
    float3 vRightDist = vRightDir * In[0].vPSize.x * 0.5f;
    float3 vUpDist = vUpDir * In[0].vPSize.y * 0.5f;
    
    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);
        
    // 정점 기준 우상단 (카메라가 바라보는 기준 좌상단.)
    Out[0].vPosition = float4(vCenter + vRightDist + vUpDist, 1.0f);
    Out[0].vPosition = mul(Out[0].vPosition, matVP);
    Out[0].vTexcoord = float2(In[0].vTexcoord.x, In[0].vTexcoord.y);
    Out[0].vLifeTime = In[0].vLifeTime;
    Out[0].vOriginTexcoord = float2(0.f, 0.f);

    //Out[0].fWeight = Out[0].vPosition.w;
    
    // 정점 기준 좌상단 (카메라가 바라보는 기준 우상단.)
    Out[1].vPosition = float4(vCenter - vRightDist + vUpDist, 1.0f);
    Out[1].vPosition = mul(Out[1].vPosition, matVP);
    Out[1].vTexcoord = float2(In[0].vTexcoord.z, In[0].vTexcoord.y);
    Out[1].vLifeTime = In[0].vLifeTime;
    Out[1].vOriginTexcoord = float2(1.f, 0.f);

    //Out[1].fWeight = Out[1].vPosition.w;

    // 정점 기준 좌하단 (카메라가 바라보는 기준 우하단.)
    Out[2].vPosition = float4(vCenter - vRightDist - vUpDist, 1.0f);
    Out[2].vPosition = mul(Out[2].vPosition, matVP);
    Out[2].vTexcoord = float2(In[0].vTexcoord.z, In[0].vTexcoord.w);
    Out[2].vLifeTime = In[0].vLifeTime;
    Out[2].vOriginTexcoord = float2(1.f, 1.f);

    //Out[2].fWeight = Out[2].vPosition.w;

    // 정점 기준 우하단 (카메라가 바라보는 기준 좌하단.)
    Out[3].vPosition = float4(vCenter + vRightDist - vUpDist, 1.0f);
    Out[3].vPosition = mul(Out[3].vPosition, matVP);
    Out[3].vTexcoord = float2(In[0].vTexcoord.x, In[0].vTexcoord.w);
    Out[3].vLifeTime = In[0].vLifeTime;
    Out[3].vOriginTexcoord = float2(0.f, 1.f);

    //Out[3].fWeight = Out[3].vPosition.w;
    
    //float fdZ = (g_Near * g_Far) / (mul(In[0].vPosition, matVP).w - g_Far) / (g_Near - g_Far);
    //float fWeight = clamp(pow(1 - fdZ, 3.f), 1e-2, 3e3);
    float fDepth = mul(In[0].vPosition, matVP).w;
    float fWeight = clamp(10.f / (1e-5 + pow(fDepth / 10.f, 3.0f) + pow(fDepth / 200.f, 6.f)), 1e-2, 3e3);
    //float fWeight = clamp(10.f / (1e-5 + pow(mul(In[0].vPosition, matVP).w / 5.f, 2.0f) + pow(mul(In[0].vPosition, matVP).w / 200.f, 6.f)), 1e-2, 3e3);
    Out[0].fWeight = fWeight;
    Out[1].fWeight = fWeight;
    Out[2].fWeight = fWeight;
    Out[3].fWeight = fWeight;
    
    Out[0].vColor = In[0].vColor;
    Out[1].vColor = In[0].vColor;
    Out[2].vColor = In[0].vColor;
    Out[3].vColor = In[0].vColor;
    
        
    Out[0].fRandom = In[0].fRandom;
    Out[1].fRandom = In[0].fRandom;
    Out[2].fRandom = In[0].fRandom;
    Out[3].fRandom = In[0].fRandom;
    
    OutStream.Append(Out[0]);
    OutStream.Append(Out[1]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();

    OutStream.Append(Out[0]);
    OutStream.Append(Out[2]);
    OutStream.Append(Out[3]);
    OutStream.RestartStrip();
}

[maxvertexcount(6)]
// Biilboard 적용되지 않은 GS
void GS_VELOCITYBILLBOARD(point GS_IN In[1], inout TriangleStream<GS_OUT> OutStream)
{
    GS_OUT Out[4]; 
    
    //matrix matInverse = g_InverseMatrix;
    //matInverse[3] = float4(0.f, 0.f, 0.f, 1.f);
    
    //vector vUpDir = normalize(mul(float4(In[0].vVelocity, 0.f), matInverse));
    //vector vUpDir = normalize(normalize(float4(In[0].vVelocity, 0.f)) - g_vLook);
    //vector vUpDir = normalize(g_vLook - normalize(float4(In[0].vVelocity, 0.f)));
    //float3 vLookDir = normalize(cross(float3(1.0f, 0.f, 0.f), vUpDir.xyz));
    //float3 vRightDir = normalize(cross(vUpDir.xyz, vLookDir));

    vector vLookDir = g_vLook;
    //float3 vRightDir = normalize(cross(float3(0.f, 1.f, 0.f), vLookDir.xyz));
    float3 vRightDir = normalize(cross(normalize(In[0].vVelocity), vLookDir.xyz));
    float3 vUpDir = normalize(cross(vLookDir.xyz, vRightDir));
    
    float fDot = dot(vUpDir, normalize(In[0].vVelocity));
    
    float3 vCenter = In[0].vPosition.xyz;
    //float3 vRightDist = vRightDir * In[0].vPSize.x * 0.5f * (clamp(sqrt(1.f - fDot * fDot), 0.35f, 1.f));
    float3 vRightDist = vRightDir * In[0].vPSize.x * 0.5f * sqrt(1.f - fDot * fDot);
    //float3 vUpDist = vUpDir * In[0].vPSize.y * 0.5f * (clamp(abs(fDot), 0.35f, 1.f));
    float3 vUpDist = vUpDir * In[0].vPSize.y * 0.5f * abs(fDot);
    
    
    
    
    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);
        
    // 정점 기준 우상단 (카메라가 바라보는 기준 좌상단.)
    Out[0].vPosition = float4(vCenter + vRightDist + vUpDist, 1.0f);
    Out[0].vPosition = mul(Out[0].vPosition, matVP);
    Out[0].vTexcoord = float2(In[0].vTexcoord.x, In[0].vTexcoord.y);
    Out[0].vLifeTime = In[0].vLifeTime;
    Out[0].vOriginTexcoord = float2(0.f, 0.f);

    //Out[0].fWeight = Out[0].vPosition.w;
    
    // 정점 기준 좌상단 (카메라가 바라보는 기준 우상단.)
    Out[1].vPosition = float4(vCenter - vRightDist + vUpDist, 1.0f);
    Out[1].vPosition = mul(Out[1].vPosition, matVP);
    Out[1].vTexcoord = float2(In[0].vTexcoord.z, In[0].vTexcoord.y);
    Out[1].vLifeTime = In[0].vLifeTime;
    Out[1].vOriginTexcoord = float2(1.f, 0.f);

    //Out[1].fWeight = Out[1].vPosition.w;

    // 정점 기준 좌하단 (카메라가 바라보는 기준 우하단.)
    Out[2].vPosition = float4(vCenter - vRightDist - vUpDist, 1.0f);
    Out[2].vPosition = mul(Out[2].vPosition, matVP);
    Out[2].vTexcoord = float2(In[0].vTexcoord.z, In[0].vTexcoord.w);
    Out[2].vLifeTime = In[0].vLifeTime;
    Out[2].vOriginTexcoord = float2(1.f, 1.f);

    //Out[2].fWeight = Out[2].vPosition.w;

    // 정점 기준 우하단 (카메라가 바라보는 기준 좌하단.)
    Out[3].vPosition = float4(vCenter + vRightDist - vUpDist, 1.0f);
    Out[3].vPosition = mul(Out[3].vPosition, matVP);
    Out[3].vTexcoord = float2(In[0].vTexcoord.x, In[0].vTexcoord.w);
    Out[3].vLifeTime = In[0].vLifeTime;
    Out[3].vOriginTexcoord = float2(0.f, 1.f);

    float fDepth = mul(In[0].vPosition, matVP).w;
    float fWeight = clamp(10.f / (1e-5 + pow(fDepth / 10.f, 3.0f) + pow(fDepth / 200.f, 6.f)), 1e-2, 3e3);
    //float fWeight = clamp(10.f / (1e-5 + pow(mul(In[0].vPosition, matVP).w / 5.f, 2.0f) + pow(mul(In[0].vPosition, matVP).w / 200.f, 6.f)), 1e-2, 3e3);
    Out[0].fWeight = fWeight;
    Out[1].fWeight = fWeight;
    Out[2].fWeight = fWeight;
    Out[3].fWeight = fWeight;
    
    Out[0].vColor = In[0].vColor;
    Out[1].vColor = In[0].vColor;
    Out[2].vColor = In[0].vColor;
    Out[3].vColor = In[0].vColor;
    
        
    Out[0].fRandom = In[0].fRandom;
    Out[1].fRandom = In[0].fRandom;
    Out[2].fRandom = In[0].fRandom;
    Out[3].fRandom = In[0].fRandom;
    
    OutStream.Append(Out[0]);
    OutStream.Append(Out[1]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();

    OutStream.Append(Out[0]);
    OutStream.Append(Out[2]);
    OutStream.Append(Out[3]);
    OutStream.RestartStrip();
}



/* PixelShader */
PS_OUT PS_MAIN_DEFAULT(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;

    Out.vColor = g_DiffuseTexture.Sample(PointSampler, In.vTexcoord);
    if (0.1f > Out.vColor.a)
        discard;
    
    
    //Out.vColor *= In.vColor;
    
    //Out.vColor *= g_vParticleColor;
    //if (Out.vColor.a < 0.01f)
    //    discard;

    //// 밝은 영역만 추출.
    //float fBrightness = dot(Out.vColor.rgb, float3(0.2126f, 0.7152f, 0.0722f));
    //float4 vBrightnessColor = (fBrightness > 0.1f) ? Out.vColor : float4(0.0f, 0.0f, 0.0f, 0.0f);
    //vBrightnessColor.a = 1.0f / 4.f;
    
    
    //Out.vColor.a *= In.vLifeTime.y / In.vLifeTime.x;
    
    return Out;
}

struct PS_OUT_WEIGHTEDBLENDED
{
    float4 vAccumulate : SV_TARGET0;
    float vRevealage : SV_TARGET1;
};

struct PS_OUT_WEIGHTEDBLENDED_BLOOM
{
    float4 vAccumulate : SV_TARGET0;
    float   vRevealage : SV_TARGET1;
    float4   vBright : SV_TARGET2;
};


float3 g_fBrightness = float3(0.2126, 0.7152, 0.0722);

PS_OUT_WEIGHTEDBLENDED_BLOOM PS_WEIGHT_BLENDEDBLOOM(PS_IN In)
{
    PS_OUT_WEIGHTEDBLENDED_BLOOM Out = (PS_OUT_WEIGHTEDBLENDED_BLOOM) 0;        
    
    float4 vColor = g_DiffuseTexture.Sample(PointSampler, In.vTexcoord);
    //if (g_RGBTest > vColor.r + vColor.g + vColor.b)
    //    discard;

    vColor.a = vColor.r * In.vColor.a;
    vColor.rgb = In.vColor.rgb * g_fColorScale + vColor.rgb * g_fMaskBrightness;

        
    vColor = vColor + vColor * g_vColorIntensity;    
    if (g_AlphaTest > vColor.a)
        discard;
    
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


PS_OUT_WEIGHTEDBLENDED PS_WEIGHT_BLENDED(PS_IN In)
{
    PS_OUT_WEIGHTEDBLENDED Out = (PS_OUT_WEIGHTEDBLENDED) 0;
        
    
    float4 vColor = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
    //if (g_RGBTest > vColor.r + vColor.g + vColor.b)
    //    discard;
    
    vColor.a = vColor.r * In.vColor.a;
    vColor.rgb = In.vColor.rgb * g_fColorScale + vColor.rgb * g_fMaskBrightness;

    
    vColor = vColor + vColor * g_vColorIntensity;
    if (g_AlphaTest > vColor.a)
        discard;
    
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


PS_OUT_WEIGHTEDBLENDED_BLOOM PS_WEIGHT_BLENDEDSUBCOLORBLOOM(PS_IN In)
{
    PS_OUT_WEIGHTEDBLENDED_BLOOM Out = (PS_OUT_WEIGHTEDBLENDED_BLOOM) 0;
        
    
    float4 vColor = g_DiffuseTexture.Sample(PointSampler, In.vTexcoord);
    //if (g_RGBTest > vColor.r + vColor.g + vColor.b)
    //    discard;


    vColor.a = vColor.r * In.vColor.a;
    vColor.rgb = In.vColor.rgb * g_fColorScale + vColor.rgb * g_fMaskBrightness;

        
    vColor = vColor + vColor * g_vColorIntensity;
    if (g_AlphaTest > vColor.a)
        discard;
    
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
    
    Out.vBright = g_vSubColor * vColor.a * fBrightness;
    
    return Out;
}


PS_OUT_WEIGHTEDBLENDED PS_WEIGHT_BLENDEDDISSOLVE(PS_IN In)
{
    PS_OUT_WEIGHTEDBLENDED Out = (PS_OUT_WEIGHTEDBLENDED) 0;
    
    float4 vColor = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
    float fDissolve = g_NoiseTexture.Sample(LinearSampler, In.vOriginTexcoord * float2(g_vNoiseUVScale.x, g_vNoiseUVScale.y) + float2(g_vNoiseUVScale.z, g_vNoiseUVScale.w) * In.fRandom).r;
    
    vColor.a = vColor.r * In.vColor.a;
    vColor.rgb = In.vColor.rgb * g_fColorScale + vColor.rgb * g_fMaskBrightness;

    vColor = vColor + vColor * g_vColorIntensity;
    
    float fNormalizedTime = clamp(In.vLifeTime.y / In.vLifeTime.x, 0.f, 1.f);
    float fDissolveThreshold = fNormalizedTime * g_fDissolveTimeFactor + g_fDissolveFactor;
    
    float fDissolveMask = step(fDissolveThreshold, fDissolve);
    float fEdgeMask = smoothstep(fDissolveThreshold - g_fEdgeWidth, fDissolveThreshold, fDissolve);
    
    vColor.a = fEdgeMask * vColor.a;
    vColor.rgb = lerp(g_vEdgeColor.rgb, vColor.rgb, fEdgeMask);
    //vColor.rgb = fEdgeMask.xxx;
    if (g_AlphaTest > vColor.a)
        discard;
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

PS_OUT_WEIGHTEDBLENDED_BLOOM PS_WEIGHT_BLENDEDDISSOLVE_SUBBLOOM(PS_IN In)
{
    PS_OUT_WEIGHTEDBLENDED_BLOOM Out = (PS_OUT_WEIGHTEDBLENDED_BLOOM) 0;
    
    float4 vColor = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
    float fDissolve = g_NoiseTexture.Sample(LinearSampler, In.vOriginTexcoord * float2(g_vNoiseUVScale.x, g_vNoiseUVScale.y) + float2(g_vNoiseUVScale.z, g_vNoiseUVScale.w) * In.fRandom).r;
    
    vColor.a = vColor.r * In.vColor.a;
    vColor.rgb = In.vColor.rgb * g_fColorScale + vColor.rgb * g_fMaskBrightness;

    vColor = vColor + vColor * g_vColorIntensity;
    
    float fNormalizedTime = clamp(In.vLifeTime.y / In.vLifeTime.x, 0.f, 1.f);
    float fDissolveThreshold = fNormalizedTime * g_fDissolveTimeFactor + g_fDissolveFactor;
    
    float fDissolveMask = step(fDissolveThreshold, fDissolve);
    float fEdgeMask = smoothstep(fDissolveThreshold - g_fEdgeWidth, fDissolveThreshold, fDissolve);
    
    vColor.a = fEdgeMask * vColor.a;
    vColor.rgb = lerp(g_vEdgeColor.rgb, vColor.rgb, fEdgeMask);
    if (g_AlphaTest > vColor.a)
        discard;
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
    
    Out.vBright = g_vSubColor * vColor.a * fBrightness;
    

    return Out;
}

PS_OUT_WEIGHTEDBLENDED_BLOOM PS_FIRESMOKE(PS_IN In)
{
    PS_OUT_WEIGHTEDBLENDED_BLOOM Out = (PS_OUT_WEIGHTEDBLENDED_BLOOM) 0;
    
    float fNormalizedTime = clamp(In.vLifeTime.y / In.vLifeTime.x, 0.f, 1.f);

    float4 vColor = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
    float fDissolve = g_NoiseTexture.Sample(LinearSampler, In.vOriginTexcoord * float2(g_vNoiseUVScale.x, g_vNoiseUVScale.y) + float2(g_vNoiseUVScale.z, g_vNoiseUVScale.w) * In.fRandom).r;
    
    vColor.a = vColor.r * In.vColor.a;
    vColor.rgb = In.vColor.rgb * g_fColorScale + vColor.rgb * g_fMaskBrightness;

    vColor = vColor + vColor * g_vColorIntensity;
    
    float fDissolveThreshold = fNormalizedTime * g_fDissolveTimeFactor + g_fDissolveFactor;
    
    float fDissolveMask = step(fDissolveThreshold, fDissolve);
    float fEdgeMask = smoothstep(fDissolveThreshold - g_fEdgeWidth, fDissolveThreshold, fDissolve);
    
    //vColor.a = fEdgeMask * vColor.a;
    vColor.rgb = lerp(float3(0.f, 0.f, 0.f), vColor.rgb, fEdgeMask);
    if (g_AlphaTest > vColor.a)
        discard;
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

float2 Random(float2 vParameter)
{
    float2 vRandom;
    vRandom = float2(dot(vParameter, float2(127.1f, 311.7f)), dot(vParameter, float2(269.5f, 183.3f)));
    
    return -1.0f + 2.0f * frac(sin(vParameter) * 43758.54f);
}

const float fK1 = 0.366025f;
const float fK2 = 0.211324f;

float Noise(float2 vParameter)
{
    float2 vInput = floor(vParameter + (vParameter.x + vParameter.y) * fK1);
    float2 vA = vParameter - vInput + (vInput.x + vInput.y) * fK2;
    float2 vO = vA.x > vA.y ? float2(1.0f, 0.f) : float2(0.0f, 1.0f);
    float2 vB = vA - vO + fK2; // 
    float2 vC = vA - 1.0f + 2.0f * fK2; // 
    
    float3 vH = max(0.5f - float3(dot(vA, vA), dot(vB, vB), dot(vC, vC)), 0.0f); // 
    float3 vN = vH * vH * vH * vH * float3(dot(vA, Random(vInput + 0.f)), dot(vB, Random(vInput + vO)), dot(vC, Random(vInput + 1.0f)));
    
    float fReturn = dot(vN, float3(70.f, 70.f, 70.f));
    return fReturn;  
}

float FBM(float2 vUV)
{
    float fResult;
    float2x2 mat2 = float2x2(1.6f, -1.2f, 1.2f, 1.6f);
    
    fResult = 0.5f * Noise(vUV);
    vUV = mul(vUV, mat2);
    fResult += 0.25f * Noise(vUV);
    vUV = mul(vUV, mat2);
    fResult += 0.125f * Noise(vUV);
    vUV = mul(vUV, mat2);
    fResult += 0.0625f * Noise(vUV);
    fResult = 0.5f + 0.5f * fResult;
    
    return fResult;
}


PS_OUT_WEIGHTEDBLENDED_BLOOM PS_FIRE(PS_IN In)
{
    PS_OUT_WEIGHTEDBLENDED_BLOOM Out = (PS_OUT_WEIGHTEDBLENDED_BLOOM) 0;
    
    float2 vUV = float2(In.vTexcoord.x, In.vTexcoord.y);
    
    float fTimeValue =  g_fStrength * In.vLifeTime.y;
    
    vUV.x = fmod(In.vTexcoord.x, 1.f) - 0.5f;
    vUV.y -= 0.25f;
    
    float fN = FBM(g_fStrength * vUV - float2(0.f, fTimeValue));
    //float fC = 1.f - 16.f * pow(max(0.f, length(vUV * float2(1.8f + vUV.y * 1.5f, 0.75f)) - fN * max(0.f, vUV.y + 0.25f)), 1.2);
   
    //float fC1 = fN * fC * (1.5f - pow(2.5f * vUV.y, 4.f));
    //fC1 = clamp(fC1, 0.f, 1.f);
    
    //float3 vColor = float3(1.5f * fC1, 1.5f * pow(fC1, 3.f), pow(fC1, 6.f));
    //float fAlpha = fC * (1.f - pow(In.vTexcoord.y, 3.f));
    //float4 vFragColor = float4(lerp(float3(0.f, 0.f, 0.f), vColor, fAlpha), fAlpha);
    float2 vDistortUV = float2(In.vTexcoord.x - 0.5f, 1.f - In.vTexcoord.y) * fN * g_NoiseIntensity;
    float2 vDistort = In.vTexcoord + vDistortUV;
       
    float4 vColor = g_DiffuseTexture.Sample(LinearSampler_Clamp, vDistort);
    //float fDissolve = g_NoiseTexture.Sample(LinearSampler, In.vOriginTexcoord * float2(g_vNoiseUVScale.x, g_vNoiseUVScale.y) + float2(g_vNoiseUVScale.z, g_vNoiseUVScale.w) * In.fRandom).r;    
    vColor.a = vColor.r * In.vColor.a;
    vColor.rgb = In.vColor.rgb * g_fColorScale + vColor.rgb * g_fMaskBrightness;

    vColor = vColor + vColor * g_vColorIntensity;
    
    //float fDissolveThreshold = fNormalizedTime * g_fDissolveTimeFactor + g_fDissolveFactor;
    
    //float fDissolveMask = step(fDissolveThreshold, fDissolve);
    //float fEdgeMask = smoothstep(fDissolveThreshold - g_fEdgeWidth, fDissolveThreshold, fDissolve);
    
    ////vColor.a = fEdgeMask * vColor.a;
    //vColor.rgb = lerp(float3(0.f, 0.f, 0.f), vColor.rgb, fEdgeMask);
    if (g_AlphaTest > vColor.a)
        discard;

    
    float fWeight = In.fWeight * max(min(1.0, max(max(vColor.r, vColor.g), vColor.b) * vColor.a), vColor);

    
    Out.vAccumulate.rgb = vColor.rgb * vColor.a * fWeight;
    Out.vAccumulate.a = vColor.a * fWeight;
    Out.vRevealage.r = vColor.a * clamp(log(0.6f + vColor.a), 0.25f, 0.6f);

    return Out;
}


technique11 DefaultTechnique
{
    pass BILLBOARD_WEIGHTED_BLENDED // 0
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
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

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDBLOOM();
        ComputeShader = NULL;

    }

    pass BILLBOARDZ_WEIGHTED_BLENDED // 2
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_RMAIN2();
        GeometryShader = compile gs_5_0 GS_NEWBILLBOARD();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDED();
        ComputeShader = NULL;

    }

    pass BILLBOARDZ_WEIGHTED_BLENDEDBLOOM // 3
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_RMAIN2();
        GeometryShader = compile gs_5_0 GS_NEWBILLBOARD();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDBLOOM();
        ComputeShader = NULL;

    }

    pass VELOCITYBILLBOARD_WEIGHTED_BLENDED // 4
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_VELOCITYBILLBOARD();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDED();
        ComputeShader = NULL;

    }   

    pass VELOCITYBILLBOARD_WEIGHTED_BLENDEDBLOOM // 5
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_VELOCITYBILLBOARD();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDBLOOM();
        ComputeShader = NULL;
    }

    pass SUBCOLOR_BLOOM // 6
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDSUBCOLORBLOOM();
        ComputeShader = NULL;
    }

    pass DEFAULT_DISSOLVE // 7
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDDISSOLVE();
        ComputeShader = NULL;
    }

    pass VELOCITYBILLBOARD_SUBCOLORBLOOM// 8
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_VELOCITYBILLBOARD();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDSUBCOLORBLOOM();
        ComputeShader = NULL;
    }

    pass DISSOLVE_SUBCOLORBLOOM // 9
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDDISSOLVE_SUBBLOOM();
        ComputeShader = NULL;
    }

    pass FIRESMOKE // 10
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN();
        PixelShader = compile ps_5_0 PS_FIRESMOKE();
        ComputeShader = NULL;
    }

    pass FIRE // 11
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN();
        PixelShader = compile ps_5_0 PS_FIRE();
        ComputeShader = NULL;
    }

    pass ROTATIONBILL_SUBBLOOM // 12
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_RMAIN2();
        GeometryShader = compile gs_5_0 GS_NEWBILLBOARD();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDSUBCOLORBLOOM();
        ComputeShader = NULL;
    }

    pass ROT_DISSOLVE_SUBCOLORBLOOM // 13
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_RMAIN2();
        GeometryShader = compile gs_5_0 GS_NEWBILLBOARD();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDDISSOLVE_SUBBLOOM();
        ComputeShader = NULL;
    }
    pass VEL_DISSOLVE_SUBCOLORBLOOM // 14
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_MAIN();
        GeometryShader = compile gs_5_0 GS_VELOCITYBILLBOARD();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDDISSOLVE_SUBBLOOM();

        ComputeShader = NULL;
    }

    pass ROT2_DISSOLVE_SUBCOLORBLOOM // 15
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        //SetBlendState(BS_WeightAccumulate, float4(0.f, 0, f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_RMAIN2();
        GeometryShader = compile gs_5_0 GS_NEWBILLBOARD2();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDEDDISSOLVE_SUBBLOOM();

        ComputeShader = NULL;
    }

    pass DEFAULT_NO_BILLBOARD // 16
    {
        SetRasterizerState(RS_Cull_None);
        SetDepthStencilState(DSS_WriteNone, 0);
        SetBlendState(BS_WeightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_SRV_NO_BILLBOARD();
        GeometryShader = compile gs_5_0 GS_NO_BILLBOARD();
        PixelShader = compile ps_5_0 PS_WEIGHT_BLENDED();
        ComputeShader = NULL;
    }
}
