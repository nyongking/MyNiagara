#include "Translation_Module.h"
#include "GameInstance.h"
#include "Compute_Shader.h"

#ifdef _DEBUG

const _char* CTranslation_Module::g_szModuleNames[9] = { "POINT_VELOCITY", "LINEAR_VELOCITY", "INIT_ACCELERATION", "GRAVITY", "DRAG",
        "VORTEX_ACCELERATION", "POINT_ACCELERATION", "LIMIT_ACCELERATION", "POSITION_BY_NUMBER" };

#endif // _DEBUG

CTranslation_Module::CTranslation_Module()
    : CEffect_Module()
{
    m_eModuleType = MODULE_TRANSLATION;
    m_eApplyTo = TRANSLATION;
}

CTranslation_Module::CTranslation_Module(const CTranslation_Module& _Prototype)
    : CEffect_Module(_Prototype)
    , m_Float3Datas(_Prototype.m_Float3Datas)
    , m_FloatDatas(_Prototype.m_FloatDatas)
    , m_eModuleName(_Prototype.m_eModuleName)
{
}

HRESULT CTranslation_Module::Initialize(const json& _jsonModuleInfo)
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
            for (_uint i = 0; i < _jsonModuleInfo["Datas"]["Float"].size(); ++i)
            {
                m_FloatDatas.emplace(_jsonModuleInfo["Datas"]["Float"][i]["Name"], _jsonModuleInfo["Datas"]["Float"][i]["Value"]);
            }
        }

        if (_jsonModuleInfo["Datas"].contains("Float3"))
        {
            for (_uint i = 0; i < _jsonModuleInfo["Datas"]["Float3"].size(); ++i)
            {
                _float3 vData;
                vData.x = _jsonModuleInfo["Datas"]["Float3"][i]["Value"][0];
                vData.y = _jsonModuleInfo["Datas"]["Float3"][i]["Value"][1];
                vData.z = _jsonModuleInfo["Datas"]["Float3"][i]["Value"][2];

                m_Float3Datas.emplace(_jsonModuleInfo["Datas"]["Float3"][i]["Name"], vData);
            }
        }
    }
 


    return S_OK;
}



//
//void CTranslation_Module::Update_Translations(_float _fTimeDelta, _float4* _pPosition, _float3* _pVelocity, _float3* _pAcceleration)
//{
//    if (false == m_isActive)
//        return;
//
//    switch (m_eModuleName)
//    {
//    case POINT_VELOCITY:
//    {
//        XMStoreFloat3(_pVelocity, XMLoadFloat4(_pPosition) - XMLoadFloat3(&m_Float3Datas["Origin"]) * m_FloatDatas["Amount"]);
//        break;
//    }   
//    case LINEAR_VELOCITY:
//    {
//
//        *_pVelocity = m_Float3Datas["Amount"];
//        break;
//    }
//    case INIT_ACCELERATION:
//    {
//        *_pAcceleration = *_pVelocity;
//        break;
//    }
//    case GRAVITY:
//    {
//        XMStoreFloat3(_pAcceleration, XMLoadFloat3(_pAcceleration) + XMLoadFloat3(&m_Float3Datas["Amount"]) * _fTimeDelta);
//        break;
//    }
//    case DRAG:
//    {
//        _vector vVelocity = XMLoadFloat3(_pVelocity) * (1.f - m_FloatDatas["Amount"] * _fTimeDelta);
//        //_float fLength = XMVectorGetX(XMVector3Length(vVelocity));
//       
//
//        //XMStoreFloat3(_pVelocity, XMLoadFloat3(_pVelocity) - XMLoadFloat3(_pVelocity) * m_FloatDatas["Amount"] * _fTimeDelta);
//        XMStoreFloat3(_pVelocity, vVelocity);
//        break;
//    }
//    case VORTEX_ACCELERATION:
//    {   
//        _vector vDiff = XMVectorSetW(XMLoadFloat4(_pPosition) - XMLoadFloat3(&m_Float3Datas["Origin Point"]), 0.f);
//        _vector vR = vDiff - XMVector3Dot(XMVector3Normalize(XMLoadFloat3(&m_Float3Datas["Axis"])), vDiff) * XMLoadFloat3(&m_Float3Datas["Axis"]);
//        //// 밀어내는 힘 + 당기는 힘
//        _vector vVortex = XMVector3Normalize(XMVector3Cross(XMVector3Normalize(XMLoadFloat3(&m_Float3Datas["Axis"])), vR)) * m_FloatDatas["Amount"];
//        _vector vPull = - XMVector3Normalize(vR) * m_FloatDatas["Pull Amount"];
//        //
//        XMStoreFloat3(_pAcceleration, XMLoadFloat3(_pAcceleration) + (vVortex + vPull) * _fTimeDelta);
//
//
//        break;
//    }
//
//    case POINT_ACCELERATION:
//    {
//        XMStoreFloat3(_pAcceleration, XMLoadFloat3(_pAcceleration) +
//        XMVectorGetX(XMVector3Length(XMLoadFloat4(_pPosition) - XMLoadFloat3(&m_Float3Datas["Origin"]))) * _fTimeDelta * XMLoadFloat3(&m_Float3Datas["Amount"]));
//
//        break;
//    }
//    case LIMIT_ACCELERATION:
//    {
//        _vector vAccel = XMLoadFloat3(_pAcceleration);
//        _float fLength = XMVectorGetX(XMVector3Length(vAccel));
//
//        XMStoreFloat3(_pAcceleration, XMVector3Normalize(vAccel) * max(fLength, m_FloatDatas["Amount"]));
//
//        break;
//    }
//
//    default:
//        break;
//    }
//}

void CTranslation_Module::Update_Translations(_float _fTimeDelta, _float* _pBuffer, _uint _iNumInstance, _uint _iPositionOffset, _uint _iVelocityOffset, _uint _iAccelerationOffset, _uint _iLifeTimeOffset, _uint _iTotalSize)
{
    if (false == m_isActive)
        return;

    switch (m_eModuleName)
    {
    case POINT_VELOCITY:
    {
        _vector vOrigin = XMLoadFloat3(&m_Float3Datas["Origin"]);
        _float fAmount = m_FloatDatas["Amount"];
        for (_uint i = 0; i < _iNumInstance; ++i)
        {

            XMStoreFloat3((_float3*)(_pBuffer + (i * _iTotalSize) + _iVelocityOffset),
                (XMLoadFloat4((_float4*)(_pBuffer + (i * _iTotalSize) + _iPositionOffset)) - vOrigin) * fAmount);
        }
        
        break;
    }
    case LINEAR_VELOCITY:
    {
        _float3 vAmount = m_Float3Datas["Amount"];
        for (_uint i = 0; i < _iNumInstance; ++i)
        {
            *(_float3*)(_pBuffer + (i * _iTotalSize) + _iVelocityOffset) = vAmount;
        }
        break;
    }
    case INIT_ACCELERATION:
    {
        for (_uint i = 0; i < _iNumInstance; ++i)
        {
           XMStoreFloat3((_float3*)(_pBuffer + (i * _iTotalSize) + _iAccelerationOffset), XMLoadFloat3((_float3*)(_pBuffer + (i * _iTotalSize) + _iVelocityOffset)) * m_FloatDatas["Amount"]);
        }

        break;
    }
    case GRAVITY:
    {
        _vector vAmount = XMLoadFloat3(&m_Float3Datas["Amount"]) * _fTimeDelta;

        for (_uint i = 0; i < _iNumInstance; ++i)
        {

            _float3* pAcceleration = (_float3*)(_pBuffer + (i * _iTotalSize) + _iAccelerationOffset);
            XMStoreFloat3(pAcceleration, XMLoadFloat3(pAcceleration) + vAmount);
        }

        break;
    }
    case DRAG:
    {
        _float fAmount = m_FloatDatas["Amount"];
        for (_uint i = 0; i < _iNumInstance; ++i)
        {

            _float3* pVelocity = (_float3*)(_pBuffer + (i * _iTotalSize) + _iVelocityOffset);        
            XMStoreFloat3(pVelocity, XMLoadFloat3(pVelocity) * (1.f - fAmount * _fTimeDelta));
        }
        break;
    }
    case VORTEX_ACCELERATION:
    {
        _vector vOrigin = XMLoadFloat3(&m_Float3Datas["Origin Point"]);
        _vector vAxis = XMVector3Normalize(XMLoadFloat3(&m_Float3Datas["Axis"]));
        _float fAmount = m_FloatDatas["Amount"];
        _float fPullAmount = m_FloatDatas["Pull Amount"];
        for (_uint i = 0; i < _iNumInstance; ++i)
        {

            _vector vDiff = XMVectorSetW(XMLoadFloat4((_float4*)(_pBuffer + (i * _iTotalSize) + _iPositionOffset)) - vOrigin, 0.f);
            _vector vR = vDiff - XMVector3Dot(vAxis, vDiff) * vAxis;
            //// 밀어내는 힘 + 당기는 힘
            _vector vVortex = XMVector3Normalize(XMVector3Cross(vAxis, vR)) * fAmount;
            _vector vPull = -XMVector3Normalize(vR) * fPullAmount;
            //
            XMStoreFloat3((_float3*)(_pBuffer + (i * _iTotalSize) + _iAccelerationOffset), XMLoadFloat3((_float3*)(_pBuffer + (i * _iTotalSize) + _iAccelerationOffset)) + (vVortex + vPull) * _fTimeDelta);
        }

        break;
    }
    case POINT_ACCELERATION:
    {
        _vector vOrigin = XMLoadFloat3(&m_Float3Datas["Origin"]);
        _vector vAmount = XMLoadFloat3(&m_Float3Datas["Amount"]);
        for (_uint i = 0; i < _iNumInstance; ++i)
        {

            _float3* pAcceleration = (_float3*)(_pBuffer + (i * _iTotalSize) + _iAccelerationOffset);
            _float4* pPosition = (_float4*)(_pBuffer + (i * _iTotalSize) + _iPositionOffset);

            XMStoreFloat3(pAcceleration, XMLoadFloat3(pAcceleration) +
                XMVectorGetX(XMVector3Length(XMLoadFloat4(pPosition) - vOrigin)) * _fTimeDelta * vAmount);

        }
      
        break;
    }
    case LIMIT_ACCELERATION:
    {
        _float fAmount = m_FloatDatas["Amount"];
        for (_uint i = 0; i < _iNumInstance; ++i)
        {

            _float3* pAcceleration = (_float3*)(_pBuffer + (i * _iTotalSize) + _iAccelerationOffset);
            _vector vAccel = XMLoadFloat3(pAcceleration);
            _float fLength = XMVectorGetX(XMVector3Length(vAccel));

            XMStoreFloat3(pAcceleration, XMVector3Normalize(vAccel) * max(fLength, fAmount));
        }
        
        break;
    }
    case POSITION_BY_NUMBER:
    {
        _vector vAxis = XMVector3Normalize(XMLoadFloat3(&m_Float3Datas["Axis"]));
        _float fOffset = m_FloatDatas["Offset"];
        _float3 vRandomOffset = m_Float3Datas["Random Offset"];
        for (_uint i = 0; i < _iNumInstance; ++i)
        {

            _float4* pPosition = (_float4*)(_pBuffer + (i * _iTotalSize) + _iPositionOffset);
            
            XMStoreFloat4(pPosition, vAxis * fOffset * (_float)i + XMVectorSet(m_pGameInstance->Compute_Random(-vRandomOffset.x, vRandomOffset.x),
                m_pGameInstance->Compute_Random(-vRandomOffset.y, vRandomOffset.y),
                m_pGameInstance->Compute_Random(-vRandomOffset.z, vRandomOffset.z)
                , 1.f));
        }

        break;
    }

    default:
        break;

    }
}

_int CTranslation_Module::Update_Module(CCompute_Shader* _pCShader)
{
    if (false == m_isActive)
        return -1;

    switch (m_eModuleName)
    {
    case POINT_VELOCITY:
    {
        _pCShader->Bind_RawValue("g_vOrigin", &m_Float3Datas["Origin"], sizeof(_float3));
        _pCShader->Bind_RawValue("g_fAmount", &m_FloatDatas["Amount"], sizeof(_float));

        return 4;
        break;
    }
    case LINEAR_VELOCITY:
    {
        _pCShader->Bind_RawValue("g_vAmount", &m_Float3Datas["Amount"], sizeof(_float3));

        return 5;
        break;
    }
    case INIT_ACCELERATION:
    {
        _pCShader->Bind_RawValue("g_fAmount", &m_FloatDatas["Amount"], sizeof(_float));
        return 6;

        break;
    }
    case GRAVITY:
    {
        _pCShader->Bind_RawValue("g_vAmount", &m_Float3Datas["Amount"], sizeof(_float3));
        return 7;

        break;
    }
    case DRAG:
    {
        _pCShader->Bind_RawValue("g_fAmount", &m_FloatDatas["Amount"], sizeof(_float));

        return 8;
        break;
    }
    case VORTEX_ACCELERATION:
    {
        _pCShader->Bind_RawValue("g_fAmount", &m_FloatDatas["Amount"], sizeof(_float));
        _pCShader->Bind_RawValue("g_Pull", &m_FloatDatas["Pull Amount"], sizeof(_float));
        _pCShader->Bind_RawValue("g_vOrigin", &m_Float3Datas["Origin Point"], sizeof(_float3));
        _pCShader->Bind_RawValue("g_vAxis", &m_Float3Datas["Axis"], sizeof(_float3));


        return 9;
        break;
    }
    case POINT_ACCELERATION:
    {
        _pCShader->Bind_RawValue("g_vOrigin", &m_Float3Datas["Origin"], sizeof(_float3));
        _pCShader->Bind_RawValue("g_vAmount", &m_Float3Datas["Amount"], sizeof(_float3));

        return 10;
        break;
    }
    case LIMIT_ACCELERATION:
    {
        _pCShader->Bind_RawValue("g_fAmount", &m_FloatDatas["Amount"], sizeof(_float));

        return 11;
        break;
    }
    default:
        break;
    }

    return -1;
}

CTranslation_Module* CTranslation_Module::Create(const json& _jsonModuleInfo)
{
    CTranslation_Module* pInstance = new CTranslation_Module();

    if (FAILED(pInstance->Initialize(_jsonModuleInfo)))
    {
        MSG_BOX("Failed to Created : CTranslation_Module");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CEffect_Module* CTranslation_Module::Clone()
{
    return new CTranslation_Module(*this);
}

void CTranslation_Module::Free()
{
    m_Float3Datas.clear();
    m_FloatDatas.clear();

    __super::Free();

}
#ifdef _DEBUG

HRESULT CTranslation_Module::Initialize(MODULE_NAME _eType, const _string& _strTypeName)
{
    m_eModuleName = _eType;
    m_strTypeName = _strTypeName;

    switch (m_eModuleName)
    {
    case LINEAR_VELOCITY:
        m_isInit = true;
        //m_Float3Datas.emplace("Amount", _float3(0.f, 0.f, 0.f));
        break;
    case POINT_VELOCITY:
        m_isInit = true;
       //m_Float3Datas.emplace("Origin", _float3(0.f, 0.f, 0.f));
       //m_FloatDatas.emplace("Amount", 0.f);
        break;
    case INIT_ACCELERATION:
        m_isInit = true;
        break;
    case GRAVITY:
        m_isInit = false;
        //m_Float3Datas.emplace("Amount", _float3(0.f, -5.f, 0.f));
        break;
    case DRAG:
        m_isInit = false;
        //m_FloatDatas.emplace("Amount", 0.f);
        break;
    case VORTEX_ACCELERATION:
        m_isInit = false;
        //m_FloatDatas.emplace("Amount", 0.f);
        //m_FloatDatas.emplace("Pull Amount", 0.f);
        //m_Float3Datas.emplace("Axis", _float3(0.f, 1.f, 0.f));
        //m_Float3Datas.emplace("Origin Point", _float3(0.f, 0.f, 0.f));
        break;
    case POINT_ACCELERATION:
        m_isInit = false;
        //m_Float3Datas.emplace("Origin", _float3(0.f, 0.f, 0.f));
        //m_Float3Datas.emplace("Amount", _float3(0.f, 0.f, 0.f));
        break;
    case LIMIT_ACCELERATION:
        m_isInit = false;
        //m_FloatDatas.emplace("Amount", 10.f);
        break;
    case POSITION_BY_NUMBER:
        m_isInit = true;
        break;
    }

    return S_OK;
}

void CTranslation_Module::Tool_Module_Update()
{
    __super::Tool_Module_Update();

    for (auto& Pair : m_FloatDatas)
    {
        if (ImGui::DragFloat(Pair.first.c_str(), &(Pair.second), 0.01f))
            m_isChanged = true;
    }

    for (auto& Pair : m_Float3Datas)
    {
        if (ImGui::DragFloat3(Pair.first.c_str(), (_float*)&(Pair.second), 0.01f))
            m_isChanged = true;

    }

}
HRESULT CTranslation_Module::Save_Module(json& _jsonModuleInfo)
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

    for (auto& Pair : m_Float3Datas)
    {
        json jsonInfo;
        jsonInfo["Value"][0] = Pair.second.x;
        jsonInfo["Value"][1] = Pair.second.y;
        jsonInfo["Value"][2] = Pair.second.z;

        jsonInfo["Name"] = Pair.first.c_str();

        _jsonModuleInfo["Datas"]["Float3"].push_back(jsonInfo);
    }


   /* for (auto& Pair : m_FloatDatas)
    {
        _jsonModuleInfo[Pair.first.c_str()] = Pair.second;
    }

    for (auto& Pair : m_Float3Datas)
    {
        _jsonModuleInfo[Pair.first.c_str()].push_back(Pair.second.x);
        _jsonModuleInfo[Pair.first.c_str()].push_back(Pair.second.y);
        _jsonModuleInfo[Pair.first.c_str()].push_back(Pair.second.z);
    }*/


    return S_OK;
}
CTranslation_Module* CTranslation_Module::Create(MODULE_NAME eType , const _string& _strTypeName)
{
    CTranslation_Module* pInstance = new CTranslation_Module();

    if (FAILED(pInstance->Initialize(eType, _strTypeName)))
    {
        MSG_BOX("Failed to Created : CTranslation_Module");
        Safe_Release(pInstance);
    }

    return pInstance;
}
#endif