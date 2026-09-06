#include "Engine_Shader_Define.hlsli"

struct Mesh_Particle
{
    row_major float4x4 InstancingMatrix : WORLD;
    float2 vLifeTime : TEXCOORD0;
    float4 vColor : TEXCOORD1;
    float3 vVelocity : TEXCOORD2;
    float3 vAcceleration : TEXCOORD3;
    float fAlive : TEXCOORD4;
};

struct Keyframe
{
    float fKeyframe;
};

struct KeyframeData
{
    float4 vKeyframeData;
};


RWStructuredBuffer<Mesh_Particle> Particles : register(u0);
StructuredBuffer<Mesh_Particle> InitialParticle : register(t0);
Texture2D g_KeyframeTexture;

float g_fAmount;
float g_Pull;
float3 g_vOrigin;
float3 g_vAxis;
float3 g_vAmount;

float g_Pos1, g_Pos2;
float4 g_Data1, g_Data2;


float g_fTimeDelta;
float4x4 g_SpawnMatrix;
float4x4 g_WorldMatrix;
float g_fAbsolute;
float g_fKill;



[numthreads(256, 1, 1)]
void CS_Update(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Particles[dispatchThreadID.x].vLifeTime.y += g_fTimeDelta;
    
    // kill or revive
    float fLifeOn = step(0.f, Particles[dispatchThreadID.x].vLifeTime.y);
    // y가 크면 -> 0의 크기
    float fDead = step(Particles[dispatchThreadID.x].vLifeTime.x, Particles[dispatchThreadID.x].vLifeTime.y) * fLifeOn;    
     
    
    //  revive
    Particles[dispatchThreadID.x].vLifeTime.y = Particles[dispatchThreadID.x].vLifeTime.y * (1.f - fDead)
    + min(InitialParticle[dispatchThreadID.x].vLifeTime.y, g_fKill) * fDead;
    Particles[dispatchThreadID.x].fAlive = Particles[dispatchThreadID.x].fAlive * (1.f - fDead)
    + InitialParticle[dispatchThreadID.x].fAlive * fDead; 
        
    
    // 속도 적용
    Particles[dispatchThreadID.x].vVelocity += Particles[dispatchThreadID.x].vAcceleration * g_fTimeDelta;
   
    float4 vPosition = Particles[dispatchThreadID.x].InstancingMatrix[3] + float4(Particles[dispatchThreadID.x].vVelocity, 0.f) * g_fTimeDelta;
    Particles[dispatchThreadID.x].InstancingMatrix[3] = vPosition;

}

[numthreads(256, 1, 1)]
void CS_Spawn(int3 dispatchThreadID : SV_DispatchThreadID)
{ 
    // LifeTime 더하기
    
    float fSpawnOn = step(0.f, Particles[dispatchThreadID.x].vLifeTime.y) * (1 - Particles[dispatchThreadID.x].fAlive);
    float4 vTranslate = mul(InitialParticle[dispatchThreadID.x].InstancingMatrix[3], mul(g_WorldMatrix, g_SpawnMatrix));
    float4x4 AbsoluteMatrix = InitialParticle[dispatchThreadID.x].InstancingMatrix * g_WorldMatrix * g_SpawnMatrix;
    AbsoluteMatrix[3] = vTranslate;
    
    Particles[dispatchThreadID.x].InstancingMatrix 
    = Particles[dispatchThreadID.x].InstancingMatrix * (1.f - fSpawnOn)
    + (InitialParticle[dispatchThreadID.x].InstancingMatrix * (1.f - g_fAbsolute) +
    AbsoluteMatrix * g_fAbsolute) * fSpawnOn;
    
    Particles[dispatchThreadID.x].fAlive = max(Particles[dispatchThreadID.x].fAlive, fSpawnOn);
    Particles[dispatchThreadID.x].vColor = Particles[dispatchThreadID.x].vColor * (1.f - fSpawnOn)
    + InitialParticle[dispatchThreadID.x].vColor * fSpawnOn;
    Particles[dispatchThreadID.x].vVelocity = Particles[dispatchThreadID.x].vVelocity * (1.f - fSpawnOn)
    + InitialParticle[dispatchThreadID.x].vVelocity * fSpawnOn;
    Particles[dispatchThreadID.x].vAcceleration = Particles[dispatchThreadID.x].vAcceleration * (1.f - fSpawnOn)
    + InitialParticle[dispatchThreadID.x].vAcceleration * fSpawnOn;
    
}

[numthreads(256, 1, 1)]
void CS_Burst(int3 dispatchThreadID : SV_DispatchThreadID)
{    

    
    //Particles[dispatchThreadID.x].InstancingMatrix = InitialParticle[dispatchThreadID.x].InstancingMatrix * (1.f - g_fAbsolute)
    //+ g_SpawnMatrix * g_fAbsolute;
    
    float4 vTranslate = mul(InitialParticle[dispatchThreadID.x].InstancingMatrix[3], mul(g_WorldMatrix, g_SpawnMatrix));
    float4x4 AbsoluteMatrix = InitialParticle[dispatchThreadID.x].InstancingMatrix * g_WorldMatrix * g_SpawnMatrix;
    AbsoluteMatrix[3] = vTranslate;
    
    Particles[dispatchThreadID.x].InstancingMatrix 
    = InitialParticle[dispatchThreadID.x].InstancingMatrix * (1.f - g_fAbsolute) +
    AbsoluteMatrix * g_fAbsolute;
    
    Particles[dispatchThreadID.x].fAlive = 1.f;
    Particles[dispatchThreadID.x].vLifeTime.y = 0.f;
    Particles[dispatchThreadID.x].vColor = InitialParticle[dispatchThreadID.x].vColor;
    Particles[dispatchThreadID.x].vVelocity = InitialParticle[dispatchThreadID.x].vVelocity;
    Particles[dispatchThreadID.x].vAcceleration = InitialParticle[dispatchThreadID.x].vAcceleration;
}

[numthreads(256, 1, 1)]
void CS_Reset(int3 dispatchThreadID : SV_DispatchThreadID)
{
    
    Particles[dispatchThreadID.x].InstancingMatrix = InitialParticle[dispatchThreadID.x].InstancingMatrix;
    Particles[dispatchThreadID.x].fAlive = InitialParticle[dispatchThreadID.x].fAlive;
    Particles[dispatchThreadID.x].vLifeTime = InitialParticle[dispatchThreadID.x].vLifeTime;
    Particles[dispatchThreadID.x].vColor = InitialParticle[dispatchThreadID.x].vColor;
    Particles[dispatchThreadID.x].vVelocity = InitialParticle[dispatchThreadID.x].vVelocity;
    Particles[dispatchThreadID.x].vAcceleration = InitialParticle[dispatchThreadID.x].vAcceleration;
}



[numthreads(256, 1, 1)]
void CS_POINTVELOCITY(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Particles[dispatchThreadID.x].vVelocity = Particles[dispatchThreadID.x].InstancingMatrix[3].xyz - g_vOrigin * g_fAmount;
}

[numthreads(256, 1, 1)]
void CS_LINEARVELOCITY(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Particles[dispatchThreadID.x].vVelocity = g_vAmount;
}

[numthreads(256, 1, 1)]
void CS_INIT_ACCELERATION(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Particles[dispatchThreadID.x].vAcceleration = Particles[dispatchThreadID.x].vVelocity;
}

[numthreads(256, 1, 1)]
void CS_GRAVITY(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Particles[dispatchThreadID.x].vAcceleration += g_vAmount * g_fTimeDelta;   
}

[numthreads(256, 1, 1)]
void CS_DRAG(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Particles[dispatchThreadID.x].vVelocity = (1.f - g_fAmount * g_fTimeDelta) * Particles[dispatchThreadID.x].vVelocity;
}

[numthreads(256, 1, 1)]
void CS_VORTEX_ACCELERATION(int3 dispatchThreadID : SV_DispatchThreadID)
{
    float3 vDiff = Particles[dispatchThreadID.x].InstancingMatrix[3].xyz - g_vOrigin;
    float3 vR = vDiff - dot(normalize(g_vAxis), vDiff) * normalize(g_vAxis);
    
    float3 vVortex = normalize(cross(normalize(g_vAxis), vR)) * g_fAmount;
    float3 vPull = vR * g_Pull * -1.f;
    
    Particles[dispatchThreadID.x].vAcceleration = Particles[dispatchThreadID.x].vAcceleration
    + (vVortex + vPull) * g_fTimeDelta;

}

[numthreads(256, 1, 1)]
void CS_POINT_ACCELERATION(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Particles[dispatchThreadID.x].vAcceleration += length((Particles[dispatchThreadID.x].InstancingMatrix[3].xyz - g_vOrigin)) * g_vAmount * g_fTimeDelta;
}

[numthreads(256, 1, 1)]
void CS_LIMIT_ACCELERATION(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Particles[dispatchThreadID.x].vAcceleration = normalize(Particles[dispatchThreadID.x].vAcceleration) * min(g_fAmount, length(Particles[dispatchThreadID.x].vAcceleration));

}

[numthreads(256, 1, 1)]
void CS_COLORKEYFRAME(int3 dispatchThreadID : SV_DispatchThreadID)
{
    //// pos1이 크면 Before = 1
    //float fBefore = step(Particles[dispatchThreadID.x].vLifeTime.y, g_Pos1) * step(Particles[dispatchThreadID.x].vLifeTime.y, g_Pos2);
    //// lifetime이 pos2 보다 크면 after = 1
    //float fAfter = step(g_Pos2, Particles[dispatchThreadID.x].vLifeTime.y) * step(g_Pos1, Particles[dispatchThreadID.x].vLifeTime.y);
    //float fBetween = step(g_Pos1, Particles[dispatchThreadID.x].vLifeTime.y) * step(Particles[dispatchThreadID.x].vLifeTime.y, g_Pos2);
    
    //float fRatio = (Particles[dispatchThreadID.x].vLifeTime.y - g_Pos1) / (g_Pos2 - g_Pos1) * fBetween;
    float fNormalizeTime = clamp(Particles[dispatchThreadID.x].vLifeTime.y / Particles[dispatchThreadID.x].vLifeTime.x, 0.f, 1.f);
    Particles[dispatchThreadID.x].vColor = g_KeyframeTexture.SampleLevel(LinearSampler, float2(fNormalizeTime, 0.5f), 0);

}


[numthreads(256, 1, 1)]
void CS_SCALEKEYFRAME(int3 dispatchThreadID : SV_DispatchThreadID)
{
    //// pos1이 크면 Before = 1
    //float fBefore = step(Particles[dispatchThreadID.x].vLifeTime.y, g_Pos1) * step(Particles[dispatchThreadID.x].vLifeTime.y, g_Pos2);
    //// lifetime이 pos2 보다 크면 after = 1
    //float fAfter = step(g_Pos2, Particles[dispatchThreadID.x].vLifeTime.y) * step(g_Pos1, Particles[dispatchThreadID.x].vLifeTime.y);
    //float fBetween = step(g_Pos1, Particles[dispatchThreadID.x].vLifeTime.y) * step(Particles[dispatchThreadID.x].vLifeTime.y, g_Pos2);
    
    //float fRatio = (Particles[dispatchThreadID.x].vLifeTime.y - g_Pos1) / (g_Pos2 - g_Pos1) * fBetween;
    float fNormalizeTime = clamp(Particles[dispatchThreadID.x].vLifeTime.y / Particles[dispatchThreadID.x].vLifeTime.x, 0.f, 1.f);
    float4 vScale = g_KeyframeTexture.SampleLevel(LinearSampler_Clamp, float2(fNormalizeTime + 0.01f, 0.5f), 0);
    
    float3 vRight = normalize(Particles[dispatchThreadID.x].InstancingMatrix[0].xyz);
    float3 vUp = normalize(Particles[dispatchThreadID.x].InstancingMatrix[1].xyz);
    float3 vLook = normalize(Particles[dispatchThreadID.x].InstancingMatrix[2].xyz);
    
    Particles[dispatchThreadID.x].InstancingMatrix[0] = float4(vRight.xyz * clamp(vScale.x, 0.001f, 100.f), 0.f);
    Particles[dispatchThreadID.x].InstancingMatrix[1] = float4(vUp.xyz * clamp(vScale.y, 0.001f, 100.f), 0.f);
    Particles[dispatchThreadID.x].InstancingMatrix[2] = float4(vLook.xyz * clamp(vScale.z, 0.001f, 100.f), 0.f);
    
}



technique11 DefaultTechnique
{
    pass Update // 0
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_Update();
    }

    pass Spawn // 1
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_Spawn();
    }

    pass Burst // 2
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_Burst();
    }

    pass Reset // 3
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_Reset();
    }

    pass PointVelocity // 4
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_POINTVELOCITY();
    }

    pass LinearVelocity // 5
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_LINEARVELOCITY();
    }
    pass InitAccel // 6
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_INIT_ACCELERATION();
    }
    pass Gravity // 7
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_GRAVITY();

    }
    pass Drag // 8
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_DRAG();
    }
    pass Vortex // 9
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_VORTEX_ACCELERATION();
    }

    pass PointAccel // 10
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_POINT_ACCELERATION();
    }

    pass LimitAccel // 11
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_LIMIT_ACCELERATION();
    }


    pass ColorKeyframe // 12
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_COLORKEYFRAME();
    }

    pass ScaleKeyframe // 13
    {
        VertexShader = NULL;
        GeometryShader = NULL;
        PixelShader = NULL;
        ComputeShader = compile cs_5_0 CS_SCALEKEYFRAME();
    }


}