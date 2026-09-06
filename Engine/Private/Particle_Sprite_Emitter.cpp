#include "Particle_Sprite_Emitter.h"
#include "GameInstance.h"

#include "Effect_Module.h"
#include "Keyframe_Module.h"
#include "Translation_Module.h"
#include "UV_Module.h"

CParticle_Sprite_Emitter::CParticle_Sprite_Emitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
	: CEmitter(_pDevice, _pContext)
{
    m_eEffectType = SPRITE;
}

CParticle_Sprite_Emitter::CParticle_Sprite_Emitter(const CParticle_Sprite_Emitter& _Prototype)
	: CEmitter(_Prototype)
	, m_eUpdateMode(_Prototype.m_eUpdateMode)
{
    if (nullptr != _Prototype.m_pMaskTextureCom)
        m_pMaskTextureCom = static_cast<CTexture*>(_Prototype.m_pMaskTextureCom->Clone(nullptr));
    if (nullptr != _Prototype.m_pDissolveTextureCom)
        m_pDissolveTextureCom = static_cast<CTexture*>(_Prototype.m_pDissolveTextureCom->Clone(nullptr));
    if (nullptr != _Prototype.m_pDistortionTextureCom)
        m_pDistortionTextureCom = static_cast<CTexture*>(_Prototype.m_pDistortionTextureCom->Clone(nullptr));

    if (nullptr != _Prototype.m_pParticleBufferCom)
    {
        m_pParticleBufferCom = static_cast<CVIBuffer_Point_Particle*>(_Prototype.m_pParticleBufferCom->Clone(nullptr));
    }
}


HRESULT CParticle_Sprite_Emitter::Initialize_Prototype(const json& _jsonInfo)
{
    if (FAILED(__super::Initialize_Prototype(_jsonInfo)))
        return E_FAIL;

	if (_jsonInfo.contains("UpdateMode"))
		m_eUpdateMode = _jsonInfo["UpdateMode"];

    // Mask Texture
    if (false == _jsonInfo.contains("Texture"))
        return E_FAIL;

    _string strTexturePath = _jsonInfo["Texture"];

    if (m_pGameInstance->Find_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strTexturePath)))
    {

    }
    else
    {
        m_pGameInstance->Add_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strTexturePath), CTexture::Create(m_pDevice, m_pContext, strTexturePath.c_str(), 1));
    }

    m_pMaskTextureCom = static_cast<CTexture*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::PROTO_COMPONENT, m_pGameInstance->Get_StaticLevelID(),
        STRINGTOWSTRING(strTexturePath), nullptr));

    if (nullptr == m_pMaskTextureCom)
        return E_FAIL;

    // Dissolve Texture ?
    if (_jsonInfo.contains("DissolveTexture"))
    {
        strTexturePath = _jsonInfo["DissolveTexture"];

        if (m_pGameInstance->Find_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strTexturePath)))
        {
            
        }
        else
        {
            m_pGameInstance->Add_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strTexturePath), CTexture::Create(m_pDevice, m_pContext, strTexturePath.c_str(), 1));

        }

        m_pDissolveTextureCom = static_cast<CTexture*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::PROTO_COMPONENT, m_pGameInstance->Get_StaticLevelID(),
            STRINGTOWSTRING(strTexturePath), nullptr));
    }


    // Second Texture ?
    if (_jsonInfo.contains("DistortionTexture"))
    {
        strTexturePath = _jsonInfo["DistortionTexture"];

        if (m_pGameInstance->Find_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strTexturePath)))
        {

        }
        else
        {
            m_pGameInstance->Add_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strTexturePath), CTexture::Create(m_pDevice, m_pContext, strTexturePath.c_str(), 1));
        }
        m_pDistortionTextureCom = static_cast<CTexture*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::PROTO_COMPONENT, m_pGameInstance->Get_StaticLevelID(),
            STRINGTOWSTRING(strTexturePath), nullptr));
    }
#ifdef _DEBUG
    if (m_pMaskTextureCom)
        m_pMaskTextureCom->Add_SRVName(STRINGTOWSTRING(_jsonInfo["Texture"]));
    if (m_pDissolveTextureCom)
        m_pDissolveTextureCom->Add_SRVName(STRINGTOWSTRING(_jsonInfo["DissolveTexture"]));
    if (m_pDistortionTextureCom)
        m_pDistortionTextureCom->Add_SRVName(STRINGTOWSTRING(_jsonInfo["DistortionTexture"]));
    
#endif //  _DEBUG


    if (_jsonInfo.contains("ShaderPass"))
    {
        EFFECT_SHADERPASS eShaderPass = _jsonInfo["ShaderPass"];
        m_iShaderPass = eShaderPass;
    }

    if (false == _jsonInfo.contains("Buffer"))
        return E_FAIL;
    m_pParticleBufferCom = CVIBuffer_Point_Particle::Create(m_pDevice, m_pContext, _jsonInfo["Buffer"], m_FloatDatas["SpawnRate"]);
    if (nullptr == m_pParticleBufferCom)
        return E_FAIL;

    for (auto pModule : m_Modules)
    {
        if (false == pModule->Is_Init())
            continue;

        m_pParticleBufferCom->Initialize_Module(pModule);
    }

    if (FAILED(m_pParticleBufferCom->Initialize_Buffers()))
        return E_FAIL;

    return S_OK;
}



HRESULT CParticle_Sprite_Emitter::Initialize(void* _pArg)
{
    if (FAILED(__super::Initialize(_pArg)))
        return E_FAIL;
  

    return S_OK;
}

void CParticle_Sprite_Emitter::Priority_Update(_float _fTimeDelta)
{
}

void CParticle_Sprite_Emitter::Update(_float _fTimeDelta)
{
    __super::Update(_fTimeDelta);
}

void CParticle_Sprite_Emitter::Late_Update(_float _fTimeDelta)
{
    __super::Late_Update(_fTimeDelta);

    if (FOLLOW_PARENT == m_eSpawnPosition)
        Set_Matrix();

    if (m_isActive && m_iAccLoop)
        m_pGameInstance->Add_RenderObject_New(s_iRG_3D, s_iRGP_PARTICLE, this);
}

HRESULT CParticle_Sprite_Emitter::Render()
{
    // 렌더 예외 처리
    if (nullptr == m_pMaskTextureCom || nullptr == m_pParticleBufferCom || nullptr == m_pShaderCom)
        return S_OK;

    // World, View, Proj 행렬 Bind/Texture Bind 등.. 
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;
    
    // 렌더 시작 함수, HLSL의 Shader Pass 번호에 맞게 렌더한다.
    if (FAILED(m_pShaderCom->Begin_NoInput(m_iShaderPass)))
        return E_FAIL;

    // 파티클 인스턴스 정보(SRV)를 넘겨준다.
    if (FAILED(m_pParticleBufferCom->Bind_BufferBySRV()))
        return E_FAIL;

    // 렌더한다.
    if (FAILED(m_pParticleBufferCom->Render_BySRV()))
        return E_FAIL;


    return S_OK;
}

void CParticle_Sprite_Emitter::Reset()
{
	if (nullptr == m_pParticleBufferCom)
		return;

	if (UPDATE_CPU == m_eUpdateMode)
		m_pParticleBufferCom->Reset_CPU_Buffers();
	else
		m_pParticleBufferCom->Reset_Buffers(m_pComputeShader);
}

void CParticle_Sprite_Emitter::Set_UpdateMode(UPDATE_MODE _eUpdateMode)
{
	if (m_eUpdateMode == _eUpdateMode)
		return;

	m_eUpdateMode = _eUpdateMode;
	if (nullptr == m_pParticleBufferCom)
		return;

	if (UPDATE_CPU == m_eUpdateMode)
		m_pParticleBufferCom->Reset_CPU_Buffers();
	else
		m_pParticleBufferCom->Reset_Buffers(m_pComputeShader);
}

void CParticle_Sprite_Emitter::Update_Emitter(_float _fTimeDelta)
{
	if (UPDATE_CPU == m_eUpdateMode)
		Update_Emitter_CPU(_fTimeDelta);
	else
		Update_Emitter_GPU(_fTimeDelta);
}

void CParticle_Sprite_Emitter::Update_Emitter_GPU(_float _fTimeDelta)
{
    if (nullptr == m_pParticleBufferCom)
        return;

    m_pComputeShader->Bind_RawValue("g_fTimeDelta", &_fTimeDelta, sizeof(_float));

    _float fKill;
    if (KILL == m_ePooling)
    {
        fKill = -100'000'000.f;
    }
    else
    {
        fKill = 0.f;
    }
    m_pComputeShader->Bind_RawValue("g_fKill", &fKill, sizeof(_float));

    _float fCount = (_float)m_pParticleBufferCom->Get_NumInstance();
    m_pComputeShader->Bind_RawValue("g_fCount", &fCount, sizeof(_float));
    m_pComputeShader->Bind_RawValue("g_Rate", &m_FloatDatas["SpawnRate"], sizeof(_float));

    m_pComputeShader->Begin(0);
    m_pParticleBufferCom->Begin_Compute(m_pComputeShader);
    m_pParticleBufferCom->Compute(m_pComputeShader);
    m_pComputeShader->End_Compute();


    if (STOP_SPAWN != m_eNowEvent)
    {
        if (SPAWN_RATE == m_eSpawnType)
        {
            _float fAbsolute;
            if (RELATIVE_WORLD == m_eSpawnPosition || FOLLOW_PARENT == m_eSpawnPosition)
            {
                m_pComputeShader->Bind_Matrix("g_SpawnMatrix", &s_IdentityMatrix);

                fAbsolute = 0.f;
            }
            else if (ABSOLUTE_WORLD == m_eSpawnPosition)
            {
                fAbsolute = 1.f;
                if (m_pSpawnMatrix)
                {
                    _float4x4 WorldMatrix;
                    _matrix SocketMatrix = XMLoadFloat4x4(m_pSpawnMatrix);
                    for (_int i = 0; i < 3; ++i)
                        SocketMatrix.r[i] = XMVector3Normalize(SocketMatrix.r[i]);
                    XMStoreFloat4x4(&WorldMatrix, SocketMatrix * XMLoadFloat4x4(m_pParentMatrices[COORDINATE_3D]));

                    m_pComputeShader->Bind_Matrix("g_SpawnMatrix", &WorldMatrix);
                }
                else
                {
                    m_pComputeShader->Bind_Matrix("g_SpawnMatrix", m_pParentMatrices[COORDINATE_3D]);
                }
                m_pComputeShader->Bind_Matrix("g_WorldMatrix", m_pControllerTransform->Get_WorldMatrix_Ptr(COORDINATE_3D));
            }
            m_pComputeShader->Bind_RawValue("g_fAbsolute", &fAbsolute, sizeof(_float));
          
            m_pComputeShader->Begin(1);

        }
        else if (BURST_SPAWN == m_eSpawnType)
        {
            _float fAbsolute;
            if (RELATIVE_WORLD == m_eSpawnPosition || FOLLOW_PARENT == m_eSpawnPosition)
            {
                fAbsolute = 0.f;
                m_pComputeShader->Bind_Matrix("g_SpawnMatrix", &s_IdentityMatrix);
            }
            else if (ABSOLUTE_WORLD == m_eSpawnPosition )
            {
                fAbsolute = 1.f;
                if (m_pSpawnMatrix)
                {
                    _float4x4 WorldMatrix;
                    _matrix SocketMatrix = XMLoadFloat4x4(m_pSpawnMatrix);
                    for (_int i = 0; i < 3; ++i)
                        SocketMatrix.r[i] = XMVector3Normalize(SocketMatrix.r[i]);
                    XMStoreFloat4x4(&WorldMatrix, SocketMatrix * XMLoadFloat4x4(m_pParentMatrices[COORDINATE_3D]));

                    m_pComputeShader->Bind_Matrix("g_SpawnMatrix", &WorldMatrix);
                }
                else
                {
                    m_pComputeShader->Bind_Matrix("g_SpawnMatrix", m_pParentMatrices[COORDINATE_3D]);
                }
                m_pComputeShader->Bind_Matrix("g_WorldMatrix", m_pControllerTransform->Get_WorldMatrix_Ptr(COORDINATE_3D));
            }
            m_pComputeShader->Bind_RawValue("g_fAbsolute", &fAbsolute, sizeof(_float));
            m_pComputeShader->Begin(2);

            m_eNowEvent = STOP_SPAWN;
            m_fInactiveDelayTime = m_fActiveTime;
        }
        m_pParticleBufferCom->Begin_Compute(m_pComputeShader);
        m_pParticleBufferCom->Compute(m_pComputeShader);
        m_pComputeShader->End_Compute();
    }

    for (auto pModule : m_Modules)
    {
        if (pModule->Is_Init())
            continue;
        
        m_pParticleBufferCom->Update_Module(pModule, m_pComputeShader);
    }
}

void CParticle_Sprite_Emitter::Update_Emitter_CPU(_float _fTimeDelta)
{
	if (nullptr == m_pParticleBufferCom)
		return;

	m_pParticleBufferCom->Update_CPU(_fTimeDelta, m_FloatDatas["SpawnRate"], REVIVE == m_ePooling);

	if (STOP_SPAWN != m_eNowEvent)
	{
		_float4x4 SpawnMatrix;
		const _float4x4* pSpawnMatrix = Get_CPU_SpawnMatrix(SpawnMatrix);

		if (SPAWN_RATE == m_eSpawnType)
		{
			m_pParticleBufferCom->Spawn_Rate(_fTimeDelta, m_FloatDatas["SpawnRate"], pSpawnMatrix);
		}
		else if (BURST_SPAWN == m_eSpawnType)
		{
			m_pParticleBufferCom->Spawn_Burst(_fTimeDelta, pSpawnMatrix);
			m_eNowEvent = STOP_SPAWN;
			m_fInactiveDelayTime = m_fActiveTime;
		}
	}

	for (auto pModule : m_Modules)
	{
		if (pModule->Is_Init())
			continue;

		m_pParticleBufferCom->Update_Module_CPU(pModule, _fTimeDelta);
	}

	m_pParticleBufferCom->End_Update(_fTimeDelta);
}

const _float4x4* CParticle_Sprite_Emitter::Get_CPU_SpawnMatrix(_float4x4& _SpawnMatrix) const
{
	if (RELATIVE_WORLD == m_eSpawnPosition || FOLLOW_PARENT == m_eSpawnPosition)
		return nullptr;

	_matrix WorldMatrix = m_pControllerTransform->Get_WorldMatrix(COORDINATE_3D);
	_matrix SpawnMatrix = XMMatrixIdentity();

	if (nullptr != m_pSpawnMatrix)
	{
		SpawnMatrix = XMLoadFloat4x4(m_pSpawnMatrix);
		for (_int i = 0; i < 3; ++i)
			SpawnMatrix.r[i] = XMVector3Normalize(SpawnMatrix.r[i]);
	}

	if (nullptr != m_pParentMatrices[COORDINATE_3D])
		SpawnMatrix *= XMLoadFloat4x4(m_pParentMatrices[COORDINATE_3D]);

	XMStoreFloat4x4(&_SpawnMatrix, WorldMatrix * SpawnMatrix);
	return &_SpawnMatrix;
}

HRESULT CParticle_Sprite_Emitter::Bind_ShaderResources()
{
    _float fAbsolute = 0.f;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_WorldMatrices[COORDINATE_3D])))
        return E_FAIL;
    if (RELATIVE_WORLD == m_eSpawnPosition || FOLLOW_PARENT == m_eSpawnPosition)
    {
        fAbsolute = 0.f;
    }
    else if (ABSOLUTE_WORLD == m_eSpawnPosition)
    {
        fAbsolute = 1.f;
    }
    if (FAILED(m_pShaderCom->Bind_RawValue("g_fAbsolute", &fAbsolute, sizeof(_float))))
        return E_FAIL;
    

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_pGameInstance->Get_TransformFloat4x4(CPipeLine::D3DTS_VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_pGameInstance->Get_TransformFloat4x4(CPipeLine::D3DTS_PROJ))))
        return E_FAIL;

    if (FAILED(m_pMaskTextureCom->Bind_ShaderResource(m_pShaderCom, "g_DiffuseTexture", 0)))
        return E_FAIL;

    if (FAILED(Bind_ShaderValue_ByPass()))
        return E_FAIL;

    _float4 vLook;
    XMStoreFloat4(&vLook, m_pGameInstance->Get_TransformInverseMatrix(CPipeLine::D3DTS_VIEW).r[2]);
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLook", &vLook, sizeof(_float4))))
        return E_FAIL;

    return S_OK;
}

HRESULT CParticle_Sprite_Emitter::Bind_ShaderValue_ByPass()
{

    if (FAILED(Bind_Float("AlphaTest", "g_AlphaTest")))
        return E_FAIL;
    //if (FAILED(Bind_Float("RGBTest", "g_RGBTest")))
    //    return E_FAIL;
    if (FAILED(Bind_Float("MaskBrightness", "g_fMaskBrightness")))
        return E_FAIL;
    if (FAILED(Bind_Float("ColorScale", "g_fColorScale")))
        return E_FAIL;
    if (FAILED(Bind_Float4("ColorIntensity", "g_vColorIntensity")))
        return E_FAIL;


    switch (m_iShaderPass)
    {
    case DEFAULT_BLOOM:
    {
        if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
            return E_FAIL;
        break;
    }        
    case DEFAULT:
    {
        break;
    }
    case ROTATION_BILLBOARD_BLOOM:
    {
        if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
            return E_FAIL;
        break;
    }
    case ROTATION_BILLBOARD:
    {
        break;
    }
    case VELOCITY_BILLBOARD_BLOOM:
    {
        if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
            return E_FAIL;
        break;
    }
    case VELOCITY_BILLBOARD:
    {
        break;
    }
    case SUBCOLORBLOOM:
    case ROTATION_BILLBOARD_SUBBLOOM:
    {
        if (FAILED(Bind_Float4("SubColor", "g_vSubColor")))
            return E_FAIL;
        if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
            return E_FAIL;
        break;
    }
    case DEFAULT_DISSOLVE:
    {
        if (FAILED(Bind_Float4("NoiseUVScale", "g_vNoiseUVScale")))
            return E_FAIL;
        if (FAILED(Bind_Float4("EdgeColor", "g_vEdgeColor")))
            return E_FAIL;
        if (FAILED(Bind_Float("EdgeWidth", "g_fEdgeWidth")))
            return E_FAIL;
        if (FAILED(Bind_Float("DissolveTimeFactor", "g_fDissolveTimeFactor")))
            return E_FAIL;
        if (FAILED(Bind_Float("DissolveFactor", "g_fDissolveFactor")))
            return E_FAIL;

        if (nullptr != m_pDissolveTextureCom && FAILED(m_pDissolveTextureCom->Bind_ShaderResource(m_pShaderCom, "g_NoiseTexture")))
            return E_FAIL;

        break;
    }
    case VELOCITY_BILLBOARD_SUBCOLORBLOOM:
    {
        if (FAILED(Bind_Float4("SubColor", "g_vSubColor")))
            return E_FAIL;
        if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
            return E_FAIL;
        break;
    }
    case DISSOLVE_SUBCOLORBLOOM:
    case ROT_SUB_DISSOLVE:
    case VEL_SUB_DISSOLVE:
    case ROT2_SUB_DISSOLVE:
    {
        if (FAILED(Bind_Float4("NoiseUVScale", "g_vNoiseUVScale")))
            return E_FAIL;
        if (FAILED(Bind_Float4("EdgeColor", "g_vEdgeColor")))
            return E_FAIL;
        if (FAILED(Bind_Float("EdgeWidth", "g_fEdgeWidth")))
            return E_FAIL;
        if (FAILED(Bind_Float("DissolveTimeFactor", "g_fDissolveTimeFactor")))
            return E_FAIL;
        if (FAILED(Bind_Float("DissolveFactor", "g_fDissolveFactor")))
            return E_FAIL;

        if (FAILED(Bind_Float4("SubColor", "g_vSubColor")))
            return E_FAIL;
        if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
            return E_FAIL;

        if (nullptr != m_pDissolveTextureCom && FAILED(m_pDissolveTextureCom->Bind_ShaderResource(m_pShaderCom, "g_NoiseTexture")))
            return E_FAIL;
        break;
    }
    case FIRESMOKE:
    {
        if (FAILED(Bind_Float4("NoiseUVScale", "g_vNoiseUVScale")))
            return E_FAIL;
        if (FAILED(Bind_Float("EdgeWidth", "g_fEdgeWidth")))
            return E_FAIL;
        if (FAILED(Bind_Float("DissolveTimeFactor", "g_fDissolveTimeFactor")))
            return E_FAIL;
        if (FAILED(Bind_Float("DissolveFactor", "g_fDissolveFactor")))
            return E_FAIL;
        if (nullptr != m_pDissolveTextureCom && FAILED(m_pDissolveTextureCom->Bind_ShaderResource(m_pShaderCom, "g_NoiseTexture")))
            return E_FAIL;
        break;
    }
    case FIRE:
    {
        if (FAILED(Bind_Float2("NoiseIntensity", "g_NoiseIntensity")))
            return E_FAIL;
        if (FAILED(Bind_Float("Strength", "g_fStrength")))
            return E_FAIL;
        break;
    }
    default:
        break;
    }

    return S_OK;
}

HRESULT CParticle_Sprite_Emitter::Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc)
{
    if (FAILED(__super::Ready_Components(_pDesc)))
        return E_FAIL;

    if (nullptr != m_pMaskTextureCom)
    {
        m_Components.emplace(L"Com_Texture", m_pMaskTextureCom);
        Safe_AddRef(m_pMaskTextureCom);
    }
    if (nullptr != m_pDissolveTextureCom)
    {
        m_Components.emplace(L"Com_Dissolve", m_pDissolveTextureCom);
        Safe_AddRef(m_pDissolveTextureCom);
    }
    if (nullptr != m_pDistortionTextureCom)
    {
        m_Components.emplace(L"Com_Second", m_pDistortionTextureCom);
        Safe_AddRef(m_pDistortionTextureCom);
    }

    if (nullptr != m_pParticleBufferCom)
    {
        m_Components.emplace(L"Com_Buffer", m_pParticleBufferCom);
        Safe_AddRef(m_pParticleBufferCom);        
    }

    return S_OK;
}


CParticle_Sprite_Emitter* CParticle_Sprite_Emitter::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonInfo)
{
    CParticle_Sprite_Emitter* pInstance = new CParticle_Sprite_Emitter(_pDevice, _pContext);

    if (FAILED(pInstance->Initialize_Prototype(_jsonInfo)))
    {
        MSG_BOX("Failed to Cloned : CParticle_Sprite_Emitter");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CParticle_Sprite_Emitter::Clone(void* _pArg)
{
    CParticle_Sprite_Emitter* pInstance = new CParticle_Sprite_Emitter(*this);

    if (FAILED(pInstance->Initialize(_pArg)))
    {
        MSG_BOX("Failed to Cloned : CParticle_Sprite_Emitter");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CParticle_Sprite_Emitter::Free()
{
    Safe_Release(m_pParticleBufferCom);
    Safe_Release(m_pMaskTextureCom);
    Safe_Release(m_pDissolveTextureCom);
    Safe_Release(m_pDistortionTextureCom);

    __super::Free();
}

HRESULT CParticle_Sprite_Emitter::Cleanup_DeadReferences()
{
	return S_OK;
}

#ifdef _DEBUG
void CParticle_Sprite_Emitter::Tool_Setting()
{
    if (m_pParticleBufferCom)
        m_pParticleBufferCom->Tool_Setting();
}
void CParticle_Sprite_Emitter::Tool_Update(_float _fTimeDelta)
{
    ImGui::Begin("Adjust_Sprite_Emitter");   

	const _char* UpdateModes[] = { "GPU", "CPU" };
	_int iUpdateMode = static_cast<_int>(m_eUpdateMode);
	if (ImGui::BeginCombo("Update Mode", UpdateModes[iUpdateMode]))
	{
		for (_int i = 0; i < IM_ARRAYSIZE(UpdateModes); ++i)
		{
			const _bool isSelected = (iUpdateMode == i);
			if (ImGui::Selectable(UpdateModes[i], isSelected))
			{
				Set_UpdateMode(static_cast<UPDATE_MODE>(i));
				m_isToolChanged = true;
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

    if (ImGui::Button("Reset"))
    {
        if (nullptr != m_pParticleBufferCom)
        {
            m_pParticleBufferCom->Tool_Reset_Buffers(m_FloatDatas["SpawnRate"], m_pComputeShader, m_Modules);
            m_isToolChanged = true;
        }
    }

    if (ImGui::TreeNode("Set Emitter State"))
    {
        __super::Tool_Update(_fTimeDelta);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Modules"))
    {
        if (ImGui::BeginListBox("List"))
        {
            if (ImGui::ArrowButton("Order Up", ImGuiDir::ImGuiDir_Up))
            {
                if (0 < m_iNowModuleIndex && 1 < m_Modules.size())
                {
                    swap(m_Modules[m_iNowModuleIndex], m_Modules[m_iNowModuleIndex - 1]);

                    for (_int i = 0; i < m_Modules.size(); ++i)
                    {
                        m_Modules[i]->Set_Order(i);
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("Order Down", ImGuiDir::ImGuiDir_Down))
            {
                if (m_Modules.size() - 1 > m_iNowModuleIndex && 1 < m_Modules.size())
                {
                    swap(m_Modules[m_iNowModuleIndex], m_Modules[m_iNowModuleIndex + 1]);

                    for (_int i = 0; i < m_Modules.size(); ++i)
                    {
                        m_Modules[i]->Set_Order(i);
                    }

                }
            }

            int n = 0;
            for (auto& pModule : m_Modules)
            {
                const bool is_selected = (m_iNowModuleIndex == n);
                if (ImGui::Selectable(pModule->Get_TypeName().c_str(), is_selected))
                {
                    m_iNowModuleIndex = is_selected ? -1 : n;
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
                ++n;
            }

            if (-1 != m_iNowModuleIndex)
            {
                if (ImGui::Button("Delete Module"))
                {
                    Safe_Release(m_Modules[m_iNowModuleIndex]);

                    auto iter = m_Modules.begin() + m_iNowModuleIndex;
                    m_Modules.erase(iter);

                    m_iNowModuleIndex = -1;
                    m_isToolChanged = true;
                }
            }

            ImGui::EndListBox();

            if (ImGui::TreeNode("Add Keyframe Module"))
            {
                static _int item_selected_idx = 0;
                const char* combo_preview_value = CKeyframe_Module::g_szModuleNames[item_selected_idx];

                if (ImGui::BeginCombo("Keyframe Module", combo_preview_value))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(CKeyframe_Module::g_szModuleNames); n++)
                    {
                        const bool is_selected = (item_selected_idx == n);

                        if (ImGui::Selectable(CKeyframe_Module::g_szModuleNames[n], is_selected))
                        {
                            item_selected_idx = n;

                            CKeyframe_Module* pModule = CKeyframe_Module::Create(m_pDevice, m_pContext, (CKeyframe_Module::MODULE_NAME)n, m_pParticleBufferCom->Get_NumInstance());
                            if (nullptr != pModule)
                            {
                                pModule->Set_Order((_int)m_Modules.size());
                                m_Modules.push_back(pModule);

                                sort(m_Modules.begin(), m_Modules.end(), [](const CEffect_Module* pSrc, const CEffect_Module* pDst)
                                    {
                                        return pSrc->Get_Order() < pDst->Get_Order();
                                    }
                                );
                            }

                            m_isToolChanged = true;
                        }
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Add UV Module"))
            {
                static _int item_selected_idx = 0;
                const char* combo_preview_value = CUV_Module::g_szModuleNames[item_selected_idx];

                if (ImGui::BeginCombo("UV Module", combo_preview_value))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(CUV_Module::g_szModuleNames); n++)
                    {
                        const bool is_selected = (item_selected_idx == n);

                        if (ImGui::Selectable(CUV_Module::g_szModuleNames[n], is_selected))
                        {
                            item_selected_idx = n;

                            CUV_Module* pModule = CUV_Module::Create((CUV_Module::MODULE_NAME)n);
                            if (nullptr != pModule)
                            {
                                pModule->Set_Order((_int)m_Modules.size());
                                m_Modules.push_back(pModule);

                                sort(m_Modules.begin(), m_Modules.end(), [](const CEffect_Module* pSrc, const CEffect_Module* pDst)
                                    {
                                        return pSrc->Get_Order() < pDst->Get_Order();
                                    }
                                );
                            }

                            m_isToolChanged = true;
                        }
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Add Translation Module"))
            {                
                static _int item_selected_idx = 0;
                const char* combo_preview_value = CTranslation_Module::g_szModuleNames[item_selected_idx];

                if (ImGui::BeginCombo("Module Type", combo_preview_value))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(CTranslation_Module::g_szModuleNames); n++)
                    {
                        const bool is_selected = (item_selected_idx == n);

                        if (ImGui::Selectable(CTranslation_Module::g_szModuleNames[n], is_selected))
                        {
                            item_selected_idx = n;

                            CTranslation_Module* pModule = CTranslation_Module::Create((CTranslation_Module::MODULE_NAME)n, CTranslation_Module::g_szModuleNames[n]);
                            if (nullptr != pModule)
                            {
                                pModule->Set_Order((_int)m_Modules.size());
                                m_Modules.push_back(pModule);

                                sort(m_Modules.begin(), m_Modules.end(), [](const CEffect_Module* pSrc, const CEffect_Module* pDst)
                                    {
                                        return pSrc->Get_Order() < pDst->Get_Order();
                                    }
                                );
                                m_isToolChanged = true;

                            }
                        }
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }


                ImGui::TreePop();
            }


            if (-1 != m_iNowModuleIndex && m_iNowModuleIndex < m_Modules.size())
            {
                if (ImGui::TreeNode("Adjust Modules"))
                {
                    m_Modules[m_iNowModuleIndex]->Tool_Module_Update();

                    ImGui::TreePop();
                }
            }
        }



        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Pass"))
    {
        const _char* items[] = { "Default", "DEFAULT_BLOOM", "ROTATION_BILLBOARD", 
            "ROTATION_BILLBOARD_BLOOM", "VELOCITY_BILLBOARD",
            "VELOCITY_BILLBOARD_BLOOM", "SUBCOLOR_BLOOM",
            "DEFAULT_DISSOLVE", "VELOCITY_BILLBOARD_SUBCOLORBLOOM",
            "DISSOLVE_SUBCOLORBLOOM", "FIRESMOKE", "FIRE", "ROTATION_BILLBOARD_SUBBLOOM",
            "ROT_SUB_DISSOLVE", "VEL_SUB_DISSOLVE", "ROT2_SUB_DISSOLVE", "DEFAULT_NO_BILLBOARD"
        };
        static _int item_selected_idx = 0;
        const char* combo_preview_value = items[item_selected_idx];

        if (ImGui::BeginCombo("Shader Type", combo_preview_value))
        {
            item_selected_idx = m_iShaderPass;
            ImGui::SetItemDefaultFocus();

            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_selected_idx == n);

                if (ImGui::Selectable(items[n], is_selected))
                {
                    item_selected_idx = n;
                    if (m_iShaderPass != n)
                    {
                        m_iShaderPass = (EFFECT_SHADERPASS)n;

                    }
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Show Textures"))
    {
        if (ImGui::TreeNode("Mask"))
        {
            if (nullptr != m_pMaskTextureCom)
            {
                ImVec2 imageSize(300, 300); // 이미지 크기 설정
                ID3D11ShaderResourceView* pSelectImage = m_pMaskTextureCom->Get_SRV(0);
                if (nullptr != pSelectImage)
                {
                    ImGui::Image((ImTextureID)pSelectImage, imageSize);
                }

                ImGui::Text(WSTRINGTOSTRING(*m_pMaskTextureCom->Get_SRVName(0)).c_str());

                if (ImGui::Button("Delete"))
                {
                    Safe_Release(m_pMaskTextureCom);
                }
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Noise"))
        {
            if (nullptr != m_pDissolveTextureCom)
            {
                ImVec2 imageSize(300, 300); // 이미지 크기 설정
                ID3D11ShaderResourceView* pSelectImage = m_pDissolveTextureCom->Get_SRV(0);
                if (nullptr != pSelectImage)
                {
                    ImGui::Image((ImTextureID)pSelectImage, imageSize);
                }

                ImGui::Text(WSTRINGTOSTRING(*m_pDissolveTextureCom->Get_SRVName(0)).c_str());


                if (ImGui::Button("Delete"))
                {
                    Safe_Release(m_pDissolveTextureCom);
                }
            }
            ImGui::TreePop();
        }


        if (ImGui::TreeNode("Second"))
        {
            if (nullptr != m_pDistortionTextureCom)
            {
                ImVec2 imageSize(300, 300); // 이미지 크기 설정
                ID3D11ShaderResourceView* pSelectImage = m_pDistortionTextureCom->Get_SRV(0);
                if (nullptr != pSelectImage)
                {
                    ImGui::Image((ImTextureID)pSelectImage, imageSize);
                }

                ImGui::Text(WSTRINGTOSTRING(*m_pDistortionTextureCom->Get_SRVName(0)).c_str());


                if (ImGui::Button("Delete"))
                {
                    Safe_Release(m_pDistortionTextureCom);
                }
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();


    }


    if (m_pParticleBufferCom)
    {
        m_pParticleBufferCom->Tool_Update(_fTimeDelta);

        if (false == m_isToolChanged)
            m_isToolChanged = m_pParticleBufferCom->Is_ToolChanged();

        if (false == m_isToolChanged)
        {
            for (auto& pModule : m_Modules)
            {
                if (pModule->Is_Changed())
                {
                    m_isToolChanged = true;
                    break;
                }
            }
        }    

        if (m_isToolChanged)
        {
            m_pParticleBufferCom->Tool_Reset_Buffers(m_FloatDatas["SpawnRate"], m_pComputeShader, m_Modules);
        }

    }
    else
    {
        static _int iNumInstanceInput = 1;
        
        if (ImGui::InputInt("Instance Count", &iNumInstanceInput))
        {
            if (0 >= iNumInstanceInput)
            {
                iNumInstanceInput = 1;
            }
        }

        if (ImGui::Button("Create_Buffer"))
        {
            m_pParticleBufferCom = CVIBuffer_Point_Particle::Create(m_pDevice, m_pContext, iNumInstanceInput, m_FloatDatas["SpawnRate"]);

            if (nullptr != m_pParticleBufferCom)
                m_Components.emplace(L"Com_Buffer", m_pParticleBufferCom);
        }
    }

    ImGui::End();

}
HRESULT CParticle_Sprite_Emitter::Save(json& _jsonOut)
{
    if (nullptr == m_pParticleBufferCom || nullptr == m_pMaskTextureCom)
    {
        MSG_BOX("Texture 혹은 Buffer 없음");
        return E_FAIL;
    }

    if (FAILED(__super::Save(_jsonOut)))
        return E_FAIL;

    _jsonOut["Texture"] = WSTRINGTOSTRING(*m_pMaskTextureCom->Get_SRVName(0)).c_str();
    if (nullptr != m_pDissolveTextureCom)
        _jsonOut["DissolveTexture"] = WSTRINGTOSTRING(*m_pDissolveTextureCom->Get_SRVName(0)).c_str();
    if (nullptr != m_pDistortionTextureCom)
        _jsonOut["DistortionTexture"] = WSTRINGTOSTRING(*m_pDistortionTextureCom->Get_SRVName(0)).c_str();

    EFFECT_SHADERPASS eShaderPass = (EFFECT_SHADERPASS)m_iShaderPass;
    _jsonOut["ShaderPass"] = eShaderPass;
	_jsonOut["UpdateMode"] = m_eUpdateMode;

    json jsonBufferInfo;

    if (FAILED(m_pParticleBufferCom->Save_Buffer(jsonBufferInfo)))
    {
        return E_FAIL;
    }

    _jsonOut["Buffer"] = jsonBufferInfo;

    return S_OK;
}
void CParticle_Sprite_Emitter::Set_Texture(CTexture* _pTextureCom, _uint _iTextureIndex)
{
    if (0 == _iTextureIndex)
    {
        if (m_pMaskTextureCom)
        {
            Safe_Release(m_pMaskTextureCom);
        }

        m_pMaskTextureCom = _pTextureCom;
        m_Components[L"Com_Texture"] = m_pMaskTextureCom;
        Safe_AddRef(m_pMaskTextureCom);
    }

    if (1 == _iTextureIndex)
    {
        if (m_pDissolveTextureCom)
        {
            Safe_Release(m_pDissolveTextureCom);
        }

        m_pDissolveTextureCom = _pTextureCom;
        m_Components[L"Com_Dissolve"] = m_pDissolveTextureCom;
        Safe_AddRef(m_pDissolveTextureCom);
    }


    if (2 == _iTextureIndex)
    {
        if (m_pDistortionTextureCom)
        {
            Safe_Release(m_pDistortionTextureCom);
        }

        m_pDistortionTextureCom = _pTextureCom;
        m_Components[L"Com_Second"] = m_pDistortionTextureCom;
        Safe_AddRef(m_pDistortionTextureCom);
    }


}
CParticle_Sprite_Emitter* CParticle_Sprite_Emitter::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, void* _pArg)
{
    CParticle_Sprite_Emitter* pInstance = new CParticle_Sprite_Emitter(_pDevice, _pContext);

    if (FAILED(pInstance->Initialize(_pArg)))
    {
        MSG_BOX("Failed to Cloned : CParticle_Sprite_Emitter");
        Safe_Release(pInstance);
    }

    return pInstance;
}

#endif
