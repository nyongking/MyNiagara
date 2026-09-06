#include "UV_Module.h"
#include "GameInstance.h"
#include "Compute_Shader.h"

#ifdef _DEBUG
const _char* CUV_Module::g_szModuleNames[2] = {"UV_ANIM", "RANDOM_UV_ANIM"};
#endif
CUV_Module::CUV_Module()
    : CEffect_Module()
{
    m_eModuleType = MODULE_UV;
    m_eApplyTo = UV;
}

CUV_Module::CUV_Module(const CUV_Module& _Prototype)
    : CEffect_Module(_Prototype)
    , m_Float2Datas(_Prototype.m_Float2Datas)
    , m_FloatDatas(_Prototype.m_FloatDatas)
    , m_eModuleName(_Prototype.m_eModuleName) {
}

HRESULT CUV_Module::Initialize(const json& _jsonModuleInfo)
{
    if (FAILED(__super::Initialize(_jsonModuleInfo)))
        return E_FAIL;

    if (false == _jsonModuleInfo.contains("Module"))
        return E_FAIL;
    m_eModuleName = _jsonModuleInfo["Module"];
#ifdef _DEBUG
    m_strTypeName = _jsonModuleInfo["Module"];
#endif

    if (_jsonModuleInfo.contains("Datas"))
    {
        if (_jsonModuleInfo["Datas"].contains("Float"))
        {
            for (_int i = 0; i < _jsonModuleInfo["Datas"]["Float"].size(); ++i)
            {
                m_FloatDatas.emplace(_jsonModuleInfo["Datas"]["Float"][i]["Name"], _jsonModuleInfo["Datas"]["Float"][i]["Value"]);
            }
        }

        if (_jsonModuleInfo["Datas"].contains("Float2"))
        {
            for (_int i = 0; i < _jsonModuleInfo["Datas"]["Float2"].size(); ++i)
            {
                _float2 vData;
                vData.x = _jsonModuleInfo["Datas"]["Float2"][i]["Value"][0];
                vData.y = _jsonModuleInfo["Datas"]["Float2"][i]["Value"][1];

                m_Float2Datas.emplace(_jsonModuleInfo["Datas"]["Float2"][i]["Name"], vData);
            }
        }
    }

    return S_OK;
}

_int CUV_Module::Update_Module(CCompute_Shader* _pCShader)
{
    switch (m_eModuleName)
    {
    case UV_ANIM:
    {
        _pCShader->Bind_RawValue("g_Size", &m_Float2Datas["UVSize"], sizeof(_float2));
        _pCShader->Bind_RawValue("g_Count", &m_Float2Datas["Count"], sizeof(_float2));

        return 14;
        break;
    }

    case RANDOM_UV_ANIM:
    {
        _pCShader->Bind_RawValue("g_Size", &m_Float2Datas["UVSize"], sizeof(_float2));
        _pCShader->Bind_RawValue("g_Count", &m_Float2Datas["Count"], sizeof(_float2));

        return 15;
        break;
    }
    }

	return - 1;
}

void CUV_Module::Update_UV_CPU(_float* _pBuffer, _uint _iNumInstance, _uint _iUVOffset,
	_uint _iLifeTimeOffset, _uint _iRandomOffset, _uint _iTotalSize)
{
	if (nullptr == _pBuffer)
		return;

	const auto SizeIter = m_Float2Datas.find("UVSize");
	const auto CountIter = m_Float2Datas.find("Count");
	if (m_Float2Datas.end() == SizeIter || m_Float2Datas.end() == CountIter)
		return;

	const _float2 vSize = SizeIter->second;
	const _float2 vCount = CountIter->second;
	const _float fTotalCount = vCount.x * vCount.y;
	if (0.f >= vCount.x || 0.f >= vCount.y || 0.f >= fTotalCount)
		return;

	for (_uint i = 0; i < _iNumInstance; ++i)
	{
		_float* pParticle = _pBuffer + i * _iTotalSize;
		const _float2* pLifeTime = reinterpret_cast<const _float2*>(pParticle + _iLifeTimeOffset);
		const _float fNormalizedTime = max(0.f, min(1.f,
			pLifeTime->y / max(pLifeTime->x, 0.0001f)));

		_float fIndexTime = fNormalizedTime;
		if (RANDOM_UV_ANIM == m_eModuleName)
		{
			fIndexTime += *(pParticle + _iRandomOffset);
			if (1.f <= fIndexTime)
				fIndexTime -= 1.f;
		}

		const _float fIndex = floor(fIndexTime * fTotalCount);
		const _float fIndexY = floor((fIndex + 0.01f) / vCount.x);
		const _float fIndexX = fIndex - fIndexY * vCount.x;

		_float4* pUV = reinterpret_cast<_float4*>(pParticle + _iUVOffset);
		*pUV = _float4(fIndexX * vSize.x, fIndexY * vSize.y,
			(fIndexX + 1.f) * vSize.x, (fIndexY + 1.f) * vSize.y);
	}
}

CUV_Module* CUV_Module::Create(const json& _jsonModuleInfo)
{
    CUV_Module* pInstance = new CUV_Module();

    if (FAILED(pInstance->Initialize(_jsonModuleInfo)))
    {
        MSG_BOX("Failed to Created : CUV_Module");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CEffect_Module* CUV_Module::Clone()
{
    return new CUV_Module(*this);
}

void CUV_Module::Free()
{
    m_Float2Datas.clear();
    m_FloatDatas.clear();
    
    __super::Free();
}
#ifdef _DEBUG
HRESULT CUV_Module::Initialize(MODULE_NAME _eType)
{
    m_eModuleName = _eType;

    switch (m_eModuleName)
    {
    case UV_ANIM:
        m_strTypeName = "UV_ANIM";
        break;
    case RANDOM_UV_ANIM:
        m_strTypeName = "RANDOM_UV_ANIM";
        break;
    }

    return S_OK;
}

void CUV_Module::Tool_Module_Update()
{
    __super::Tool_Module_Update();

    for (auto& Pair : m_FloatDatas)
    {
        if (ImGui::DragFloat(Pair.first.c_str(), &(Pair.second), 0.01f))
            m_isChanged = true;
    }

    for (auto& Pair : m_Float2Datas)
    {
        if (ImGui::DragFloat2(Pair.first.c_str(), (_float*)&(Pair.second), 0.01f))
            m_isChanged = true;

    }
}

HRESULT CUV_Module::Save_Module(json& _jsonModuleInfo)
{
    if (FAILED(__super::Save_Module(_jsonModuleInfo)))
        return E_FAIL;

    _jsonModuleInfo["Module"] = m_eModuleName;

    for (auto& Pair : m_FloatDatas)
    {
        json jsonInfo;
        jsonInfo["Value"] = Pair.second;
        jsonInfo["Name"] = Pair.first.c_str();

        _jsonModuleInfo["Datas"]["Float"].push_back(jsonInfo);
    }

    for (auto& Pair : m_Float2Datas)
    {
        json jsonInfo;
        jsonInfo["Value"][0] = Pair.second.x;
        jsonInfo["Value"][1] = Pair.second.y;

        jsonInfo["Name"] = Pair.first.c_str();

        _jsonModuleInfo["Datas"]["Float2"].push_back(jsonInfo);
    }

    return S_OK;

}

CUV_Module* CUV_Module::Create(MODULE_NAME _eType)
{
    CUV_Module* pInstance = new CUV_Module();

    if (FAILED(pInstance->Initialize(_eType)))
    {
        MSG_BOX("Failed to Created : CUV_Module");
        Safe_Release(pInstance);
    }

    return pInstance;
}
#endif
