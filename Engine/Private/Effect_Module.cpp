#include "Effect_Module.h"
#include "GameInstance.h"

static int iCount = 0;
CEffect_Module::CEffect_Module()
    : m_pGameInstance(CGameInstance::GetInstance())
{
    Safe_AddRef(m_pGameInstance);
}

CEffect_Module::CEffect_Module(const CEffect_Module& _Prototype)
    : m_isInit(_Prototype.m_isInit)
    , m_iOrder(_Prototype.m_iOrder)
    , m_eModuleType(_Prototype.m_eModuleType)
    , m_eApplyTo(_Prototype.m_eApplyTo)
    , m_pGameInstance(_Prototype.m_pGameInstance)
#ifdef _DEBUG
    , m_strTypeName(_Prototype.m_strTypeName)
#endif
{
    Safe_AddRef(m_pGameInstance);
}

HRESULT CEffect_Module::Initialize(const json& _jsonModuleInfo)
{
    if (false == _jsonModuleInfo.contains("Type"))
        return E_FAIL;
    m_eModuleType = _jsonModuleInfo["Type"];

    if (false == _jsonModuleInfo.contains("Order"))
        return E_FAIL;
    m_iOrder = _jsonModuleInfo["Order"];

    if (false == _jsonModuleInfo.contains("Init"))
        return E_FAIL;
    m_isInit = _jsonModuleInfo["Init"];

    if (false == _jsonModuleInfo.contains("Apply"))
        return E_FAIL;
    m_eApplyTo = _jsonModuleInfo["Apply"];


    return S_OK;
}

void CEffect_Module::Update_Translations(_float _fTimeDelta, _float* _pBuffer, _uint _iNumInstance, _uint _iPositionOffset, _uint _iVelocityOffset, _uint _iAccelerationOffset, _uint _iLifeTimeOffset, _uint _iTotalSize)
{
}


void CEffect_Module::Update_ColorKeyframe(_float _fCurTime, _float4* _pColor)
{
}

void CEffect_Module::Update_ScaleKeyframe(_float _fCurTime, _float4* _pRight, _float4* _pUp, _float4* _pLook)
{
}

void CEffect_Module::Update_ColorKeyframe(_float* _pBuffer, _uint _iNumInstance, _uint _iColorOffset, _uint _iLifeTimeOffset, _uint _iTotalSize)
{
}

void CEffect_Module::Update_ScaleKeyframe(_float* _pBuffer, _uint _iNumInstance, _uint _iRightOffset, _uint _iUpOffset, _uint _iLookOffset, _uint _iLifeTimeOffset, _uint _iTotalSize)
{
}

_int CEffect_Module::Update_Module(CCompute_Shader* _pCShader)
{
    return 0;
}

void CEffect_Module::Free()
{
    Safe_Release(m_pGameInstance);
    --iCount;

    __super::Free();
}

#ifdef _DEBUG

void CEffect_Module::Tool_Module_Update()
{
    ImGui::Text("Type :");
    ImGui::SameLine();
    ImGui::Text(m_strTypeName.c_str());

    if (ImGui::RadioButton("Active", m_isActive))
    {
        m_isActive = !m_isActive;
    }

    static _char szName[MAX_PATH] = "";
    if (ImGui::InputText("Set_Module_Name", szName, MAX_PATH))
    {
        if (0 != strcmp(szName, ""))
            m_strTypeName = szName;
    }

    if (ImGui::RadioButton("Is_Init ?", m_isInit))
    {
        m_isInit = !m_isInit;
        m_isChanged = true;
    }

}

HRESULT CEffect_Module::Save_Module(json& _jsonModuleInfo)
{
    _jsonModuleInfo["Init"] = m_isInit;
    _jsonModuleInfo["Order"] = m_iOrder;
    _jsonModuleInfo["Type"] = m_eModuleType;
    _jsonModuleInfo["Apply"] = m_eApplyTo;


    return S_OK;
}

#endif // _DEBUG
