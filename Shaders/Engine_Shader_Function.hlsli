float3 Get_WorldNormal(float3 _vNormalMapValue, float3 _vNormal, float3 _vTangent, int invertNormalMapY)
{
    // Convert UV range to [-1, 1]
    _vNormalMapValue = _vNormalMapValue * 2.0f - 1.0f;

    // Normalize inputs
    _vNormalMapValue.y = invertNormalMapY > 0 ? _vNormalMapValue.y * -1.0f : _vNormalMapValue.y;
    _vNormal.y = invertNormalMapY ? -_vNormal.y : _vNormal.y;
    float3 vNormal = normalize(_vNormal);
    float3 vTangent = normalize(_vTangent - dot(_vTangent, vNormal) * vNormal);
    float3 vBiTangent = cross(vNormal, vTangent); // 좌표계 확인 필요

    // TBN matrix
    float3x3 TBN = float3x3(vTangent, vBiTangent, vNormal);

    // Tangent Space -> World Space
    return normalize(mul(_vNormalMapValue, TBN));
}


float4 TexcoordToWorld(float2 _vTexcoord, float _fDepth, float _fViewSpaceZ, float4x4 fProjMatrixInverse, float4x4 fViewMatrixInverse)
{
    float4 vPosition = { 0.0f, 0.0f, 0.0f, 0.0f };
    // 1. texcoord 를 기반으로 Ndc Space Position을 구한다. >>> 이때 z 값은 미리 저장해둔 depthRenderTarget의 값을 활용한다.
    vPosition.x = _vTexcoord.x * 2.0f - 1.0f;
    vPosition.y = _vTexcoord.y * -2.0f + 1.0f;
    vPosition.z = _fDepth;
    vPosition.w = 1.0f;
    // 2. ViewSpace로 변환하기 위해서 w 나누기를 했던 vPosition의 값을 viewspace상의 z 값을 곱하여준다.
    vPosition *= _fViewSpaceZ;
    // 3. InverseProjectionMatrix를 이용하여 viewspace로 변환한다.
    vPosition = mul(vPosition, fProjMatrixInverse);
    // 4. World Space로 변환.
    vPosition = mul(vPosition, fViewMatrixInverse);
    
    return vPosition;
}


float Wrap(float value, float min, float max)
{
    return fmod(value - min, max - min) + min;
}