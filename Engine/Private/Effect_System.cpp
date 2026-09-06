#include "Effect_System.h"
#include "GameInstance.h"

#include "Particle_Sprite_Emitter.h"
#include "Particle_Mesh_Emitter.h"
#include "MeshEffect_Emitter.h"
#include "SpriteEffect_Emitter.h"

CEffect_System::CEffect_System(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
	: CPartObject(_pDevice, _pContext)
{
}

CEffect_System::CEffect_System(const CEffect_System& _Prototype)
	: CPartObject(_Prototype)
#ifdef _DEBUG
	, m_strFilePath(_Prototype.m_strFilePath)
	, m_fToolRepeatTime(_Prototype.m_fToolRepeatTime)
#endif // _DEBUG

{
	m_strName = _Prototype.m_strName;
}

// 파티클 시스템이 Prototype 시점에 모든 Emitter을 다 불러옵니다.
HRESULT CEffect_System::Initialize_Prototype(const _tchar* _szFilePath)
{

	json jsonEffectInfo;
	if (FAILED(m_pGameInstance->Load_Json(_szFilePath, &jsonEffectInfo)))
		return E_FAIL;

	if (false == jsonEffectInfo.contains("Name"))
	{
		return E_FAIL;
	}
	_string strName = jsonEffectInfo["Name"];
	m_strName = STRINGTOWSTRING(strName);


	// 갖고있는 Emitter 정보를 불러들입니다.
	for (_int i = 0; i < jsonEffectInfo["Emitters"].size(); ++i)
	{
		if (false == jsonEffectInfo["Emitters"][i].contains("Type"))
			return E_FAIL;
		
		CEmitter::EFFECT_TYPE eType = jsonEffectInfo["Emitters"][i]["Type"];

		if (CEmitter::SPRITE == eType)
		{
			CParticle_Sprite_Emitter* pSpriteEmitter = CParticle_Sprite_Emitter::Create(m_pDevice, m_pContext, jsonEffectInfo["Emitters"][i]);
			if (nullptr == pSpriteEmitter)
				return E_FAIL;
			m_Emitters.push_back(pSpriteEmitter);
		}

		else if (CEmitter::MESH == eType)
		{
			//const jsonEffectInfo["Emitters"][]

			CParticle_Mesh_Emitter* pMeshEmitter = CParticle_Mesh_Emitter::Create(m_pDevice, m_pContext, jsonEffectInfo["Emitters"][i]);
			if (nullptr == pMeshEmitter)
				return E_FAIL;
			m_Emitters.push_back(pMeshEmitter);
		}

		else if (CEmitter::EFFECT == eType)
		{
			CMeshEffect_Emitter* pEffect = CMeshEffect_Emitter::Create(m_pDevice, m_pContext, jsonEffectInfo["Emitters"][i]);
			if (nullptr == pEffect)
					return E_FAIL;
			m_Emitters.push_back(pEffect);
		}

		else if (CEmitter::SINGLE_SPRITE == eType)
		{
			CSpriteEffect_Emitter* pEffect = CSpriteEffect_Emitter::Create(m_pDevice, m_pContext, jsonEffectInfo["Emitters"][i]);
			if (nullptr == pEffect)
				return E_FAIL;
			m_Emitters.push_back(pEffect);
		}

	}

#ifdef _DEBUG
	if (jsonEffectInfo.contains("ToolLoopTime"))
	{
		m_fToolRepeatTime = jsonEffectInfo["ToolLoopTime"];
	}

	m_strFilePath = WSTRINGTOSTRING(_szFilePath);
#endif


	return S_OK;
}

HRESULT CEffect_System::Initialize_Prototype()
{
	return S_OK;
}

// Prototype의 Emitter을 다 Clone 해옵니다.
// Shader에 대한 Desc을 채워줘야 합니다.
HRESULT CEffect_System::Initialize(void* _pArg, const CEffect_System* _pPrototype)
{
	if (nullptr == _pArg)
		return E_FAIL;

	EFFECT_SYSTEM_DESC* pDesc = static_cast<EFFECT_SYSTEM_DESC*>(_pArg);

	// TODO: Particle Coordinate는 2D도 해야할까 ? 
	pDesc->eStartCoord = COORDINATE_3D;
	pDesc->isCoordChangeEnable = false;

	
	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;



	for (_uint i = 0; i < _pPrototype->m_Emitters.size(); ++i)
	{
		if (CEmitter::SINGLE_SPRITE == _pPrototype->m_Emitters[i]->Get_Type())
		{
			CSpriteEffect_Emitter::SPRITE_EMITTER_DESC Desc = {};

			Desc.iCurLevelID = pDesc->iCurLevelID;
			Desc.eStartCoord = COORDINATE_3D;
			Desc.isCoordChangeEnable = false;
			Desc.pParentMatrices[COORDINATE_3D] = &m_WorldMatrices[COORDINATE_3D];
			Desc.iProtoShaderLevel = pDesc->iSingleSpriteShaderLevel;
			Desc.szShaderTag = pDesc->szSingleSpriteShaderTags;
			Desc.iProtoRectLevel = pDesc->iSingleSpriteBufferLevel;
			Desc.szRectTag = pDesc->szSingleSpriteBufferTags;
			Desc.szComputeShaderTag = nullptr;

			CEmitter* pEmitter = static_cast<CEmitter*>(_pPrototype->m_Emitters[i]->Clone(&Desc));
			if (nullptr == pEmitter)
				return E_FAIL;
			m_Emitters.push_back(pEmitter);
		}
		else
		{

			CEmitter::PARTICLE_EMITTER_DESC Desc = {};

			if (CEmitter::MESH == _pPrototype->m_Emitters[i]->Get_Type())
			{
				Desc.iProtoShaderLevel = pDesc->iModelShaderLevel;
				Desc.szShaderTag = pDesc->szModelShaderTags;
				Desc.szComputeShaderTag = pDesc->szMeshComputeShaderTag;

			}

			else if (CEmitter::SPRITE == _pPrototype->m_Emitters[i]->Get_Type())
			{
				Desc.iProtoShaderLevel = pDesc->iSpriteShaderLevel;
				Desc.szShaderTag = pDesc->szSpriteShaderTags;
				Desc.szComputeShaderTag = pDesc->szSpriteComputeShaderTag;

			}

			else if (CEmitter::EFFECT == _pPrototype->m_Emitters[i]->Get_Type())
			{
				Desc.iProtoShaderLevel = pDesc->iEffectShaderLevel;
				Desc.szShaderTag = pDesc->szEffectShaderTags;
				Desc.szComputeShaderTag = nullptr;

			}

			Desc.iCurLevelID = pDesc->iCurLevelID;
			Desc.eStartCoord = COORDINATE_3D;
			Desc.isCoordChangeEnable = false;
			Desc.pParentMatrices[COORDINATE_3D] = &m_WorldMatrices[COORDINATE_3D];

			CEmitter* pEmitter = static_cast<CEmitter*>(_pPrototype->m_Emitters[i]->Clone(&Desc));
			if (nullptr == pEmitter)
				return E_FAIL;
			m_Emitters.push_back(pEmitter);
		}

	}

	return S_OK;
}

// TOOL 전용의 Particle Initialize입니다.
// 새로운 파티클을 툴에서 만들 때 사용합니다.
HRESULT CEffect_System::Initialize(void* _pArg)
{
	if (FAILED(__super::Initialize(_pArg)))
		return E_FAIL;

	return S_OK;

}


// TODO : 여러 로직들 구현하기
void CEffect_System::Priority_Update(_float _fTimeDelta)
{
	for (auto& pEmitter : m_Emitters)
		pEmitter->Priority_Update(_fTimeDelta);


}

void CEffect_System::Update(_float _fTimeDelta)
{
	if (false == m_isActive)
		return;

	_bool isActive = false;
	for (auto& pEmitter : m_Emitters)
	{
#ifdef _DEBUG
		pEmitter->Update(_fTimeDelta * m_fDebugTimeScale);
#else 
		pEmitter->Update(_fTimeDelta);
#endif

		if (pEmitter->Is_Active())
			isActive = true;
	}
	if (false == isActive)
		Set_Active(false);

}

void CEffect_System::Late_Update(_float _fTimeDelta)
{
	if (false == m_isActive)
		return;

	// 부모 오브젝트의 World에 따라 먼저 Update된 이후에 갱신됩니다.
	__super::Late_Update(_fTimeDelta);

	_bool isActive = false;
	for (auto& pEmitter : m_Emitters)
	{
#ifdef _DEBUG
		pEmitter->Late_Update(_fTimeDelta * m_fDebugTimeScale);
#else 
		pEmitter->Late_Update(_fTimeDelta);
#endif

		if (pEmitter->Is_Active())
			isActive = true;
	}
	if (false == isActive)
		Set_Active(false);

}

HRESULT CEffect_System::Render()
{
	return S_OK;
}

void CEffect_System::Active_Effect(_bool _isReset, _uint _iEffectID)
{
	for (auto& pEmitter : m_Emitters)
	{
		if (_iEffectID == pEmitter->Get_EventID())
		{
			pEmitter->Set_Active(true);

			if (_isReset)
				pEmitter->Reset();
		}
	}
	Set_Active(true);
}

void CEffect_System::Active_All(_bool _isReset)
{
	for (auto& pEmitter : m_Emitters)
	{		
		pEmitter->Set_Active(true);

		if (_isReset)
			pEmitter->Reset();
		
	}
	Set_Active(true);
}

void CEffect_System::Stop_Spawn(_float _fDelayTime, _uint _iEffectID)
{
	for (auto& pEmitter : m_Emitters)
	{
		if (_iEffectID == pEmitter->Get_EventID() && pEmitter->Is_Active())
		{
			pEmitter->Stop_Spawn(_fDelayTime);
		}
	}
}

void CEffect_System::Stop_SpawnAll(_float _fDelayTime)
{
	for (auto& pEmitter : m_Emitters)
	{
		if (pEmitter->Is_Active())
		{
			pEmitter->Stop_Spawn(_fDelayTime);
		}
	}
}

void CEffect_System::Inactive_Effect(_uint _iEffectID)
{
	for (auto& pEmitter : m_Emitters)
	{
		if (_iEffectID == pEmitter->Get_EventID() && pEmitter->Is_Active())
		{
			pEmitter->Set_Active(false);

		}
	}
}

void CEffect_System::Inactive_All()
{
	for (auto& pEmitter : m_Emitters)
	{
		if (pEmitter->Is_Active())
		{
			pEmitter->Set_Active(false);

		}
	}
}

void CEffect_System::Reset_All()
{
	for (auto& pEmitter : m_Emitters)
	{
		pEmitter->Reset();
	}
}

void CEffect_System::Set_SpawnMatrix(const _float4x4* _pSpawnMatrix)
{
	m_pSpawnMatrix = _pSpawnMatrix;

	for (auto& pEmitter : m_Emitters)
		pEmitter->Set_SpawnMatrix(_pSpawnMatrix);
}

void CEffect_System::Set_EffectPosition(_fvector _vPosition)
{
	m_pControllerTransform->Set_State(CTransform::STATE_POSITION, _vPosition);
}

void CEffect_System::Set_EffectMatrix(_matrix _WorldMatrix)
{
	m_pControllerTransform->Set_WorldMatrix(_WorldMatrix);
}


CEffect_System* CEffect_System::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const _tchar* _szFilePath)
{
	CEffect_System* pInstance = new CEffect_System(_pDevice, _pContext);

	if (FAILED(pInstance->Initialize_Prototype(_szFilePath)))
	{
		MSG_BOX("Failed to Created : CEffect_System");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CEffect_System::Clone(void* _pArg)
{
	CEffect_System* pInstance = new CEffect_System(*this);

#ifdef _DEBUG
	// 툴 전용
	if (0 == m_Emitters.size())
	{
		if (FAILED(pInstance->Initialize(_pArg)))
		{
			MSG_BOX("Failed to Cloned : CEffect_System");
			Safe_Release(pInstance);
		}

		return pInstance;
	}

#endif

	// 이미 Emitter 존재하는 상태 -> Emitter 복사
	if (FAILED(pInstance->Initialize(_pArg, this)))
	{
		MSG_BOX("Failed to Cloned : CEffect_System");
		Safe_Release(pInstance);
	}


	return pInstance;
}

void CEffect_System::Free()
{
	for (auto& pEmitter : m_Emitters)
	{
		Safe_Release(pEmitter);
	}
	m_Emitters.clear();

	__super::Free();
}

HRESULT CEffect_System::Cleanup_DeadReferences()
{
	return S_OK;
}


#ifdef _DEBUG
HRESULT CEffect_System::Add_New_Emitter(CEmitter::EFFECT_TYPE _eType, void* _pArg)
{

	if (CEmitter::SPRITE == _eType)
	{
		CParticle_Sprite_Emitter* pOut = CParticle_Sprite_Emitter::Create(m_pDevice, m_pContext, _pArg);
		if (nullptr == pOut)
			return E_FAIL;

		m_Emitters.push_back(pOut);
	}

	else if (CEmitter::MESH == _eType)
	{
		CParticle_Mesh_Emitter* pOut = CParticle_Mesh_Emitter::Create(m_pDevice, m_pContext, _pArg);
		if (nullptr == pOut)
			return E_FAIL;

		m_Emitters.push_back(pOut);

	}

	else if (CEmitter::EFFECT == _eType)
	{
		CMeshEffect_Emitter* pOut = CMeshEffect_Emitter::Create(m_pDevice, m_pContext, _pArg);
		if (nullptr == pOut)
			return E_FAIL;

		m_Emitters.push_back(pOut);
	}

	else if (CEmitter::SINGLE_SPRITE == _eType)
	{
		CSpriteEffect_Emitter* pOut = CSpriteEffect_Emitter::Create(m_pDevice, m_pContext, _pArg);
		if (nullptr == pOut)
			return E_FAIL;

		m_Emitters.push_back(pOut);
	}



	return S_OK;
}

void CEffect_System::Tool_ShowList()
{
	if (ImGui::TreeNode("Emitter List"))
	{
		if (ImGui::BeginListBox("List"))
		{
			_int n = 0;
			static _int iNowIndex = 0;
			for (auto& Item : m_Emitters)
			{
				_char _szNumber[10] = "";
				_itoa_s(n, _szNumber, 10);
				const _bool is_selected = (iNowIndex == n);
				if (ImGui::Selectable(_szNumber, is_selected))
				{
					iNowIndex = n;
					m_pNowItem = m_Emitters[n];
					m_pNowItem->Tool_Setting();
				}
				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
				++n;
			}

			if (m_pNowItem && ImGui::Button("Delete Emitter"))
			{
				if (m_pNowItem == m_Emitters[iNowIndex])
				{
					Safe_Release(m_pNowItem);
					m_pNowItem = nullptr;
					m_Emitters.erase(m_Emitters.begin() + iNowIndex);
				}
			}

			ImGui::EndListBox();
		}

		ImGui::TreePop();
	}


}

void CEffect_System::Tool_InputText()
{
	static _char szInputName[MAX_PATH] = "";

	strcpy_s(szInputName, WSTRINGTOSTRING(m_strName).c_str());
	if (ImGui::InputText("Name", szInputName, MAX_PATH))
	{
		if (0 != strcmp(szInputName, ""))
			m_strName = STRINGTOWSTRING(szInputName);
	}

	static _char szInputPath[MAX_PATH] = "";
	
	strcpy_s(szInputPath, m_strFilePath.c_str());
	if (ImGui::InputText("File Path", szInputPath, MAX_PATH))
	{
		m_strFilePath = szInputPath;
	}

	if (ImGui::DragFloat("Debug Loop Time", &m_fToolRepeatTime, 0.1f))
	{
		if (0.f > m_fToolRepeatTime)
			m_fToolRepeatTime = 1.f;
	}
	if (ImGui::DragFloat("Time Scale", &m_fDebugTimeScale, 0.01f))
	{
		if (0.f > m_fDebugTimeScale)
			m_fDebugTimeScale = 1.f;
	}


	if (ImGui::InputInt("Event ID", (_int*)(&m_iToolEventID)))
	{
		Tool_Reset(m_iToolEventID);
	}


	if (ImGui::Button("Resume"))
	{
		m_fDebugTimeScale = 1.f;
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop"))
	{
		m_fDebugTimeScale = 0.f;
	}
	ImGui::SameLine();
	if (ImGui::Button("ReStart"))
	{
		Tool_Reset(m_iToolEventID);
	}

}

void CEffect_System::Tool_Transform()
{
	static _float3 vRot = { 0.f, 0.f, 0.f };
	static _float3 vPosition = { 0.f, 0.f, 0.f };
	static _float4x4 Mat = {1.0f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

	if (ImGui::InputFloat3("Debugging Rotation", (_float*)&vRot))
	{
		XMStoreFloat4x4(&Mat, XMMatrixRotationRollPitchYaw(vRot.x, vRot.y, vRot.z));

			if (m_pNowItem)
				m_pNowItem->Set_SpawnMatrix(&Mat);
	}
	if (ImGui::InputFloat3("Debugging Position", (_float*)&vPosition))
	{
		Mat._41 = vPosition.x;
		Mat._42 = vPosition.y;
		Mat._43 = vPosition.z;
	}
}

void CEffect_System::Tool_Update(_float _fTimeDelta)
{

	m_fToolAccTime += _fTimeDelta * m_fDebugTimeScale;

	if (m_fToolRepeatTime <= m_fToolAccTime)
	{
		Tool_Reset(m_iToolEventID);
	}

	ImGui::Text("Debug Loop Time : %f", m_fToolAccTime);
	
	Tool_InputText();
	Tool_ShowList();
	Tool_Transform();
	
	if (m_pNowItem)
	{
		m_pNowItem->Tool_Update(_fTimeDelta * m_fDebugTimeScale);

		if (m_pNowItem->Is_ToolChanged())
		{
			Tool_Reset(m_iToolEventID);
		}
	}



}
void CEffect_System::Tool_Reset(_uint _iEvent)
{
	for (auto& pEmitter : m_Emitters)
	{
		if (_iEvent == pEmitter->Get_EventID())
		{
			pEmitter->Set_Active(true);
			pEmitter->Reset();
		}

	}
	Set_Active(true);
	m_fToolAccTime = 0.f;
}
void CEffect_System::Set_Texture(CTexture* _pTextureCom, _uint _iType)
{
	if (m_pNowItem)
	{
		if (CEmitter::SPRITE == m_pNowItem->Get_Type())
		{
			static_cast<CParticle_Sprite_Emitter*>(m_pNowItem)->Set_Texture(_pTextureCom, _iType);
		}

		else if (CEmitter::EFFECT == m_pNowItem->Get_Type())
		{
			static_cast<CMeshEffect_Emitter*>(m_pNowItem)->Set_Texture(_pTextureCom, _iType);
		}

		else if (CEmitter::SINGLE_SPRITE == m_pNowItem->Get_Type())
		{
			static_cast<CSpriteEffect_Emitter*>(m_pNowItem)->Set_Texture(_pTextureCom, _iType);
		}
	}
}

HRESULT CEffect_System::Save_File()
{
	std::ofstream OutputFile(m_strFilePath);

	if (OutputFile.is_open())
	{
		json jsonRoot;

		jsonRoot["Name"] = WSTRINGTOSTRING(m_strName).c_str();

		jsonRoot["ToolLoopTime"] = m_fToolRepeatTime;
		
		for (auto& pEmitter : m_Emitters)
		{
			json jsonLeaf;
			if (SUCCEEDED(pEmitter->Save(jsonLeaf)))
				jsonRoot["Emitters"].push_back(jsonLeaf);
		}
		
		
		OutputFile << jsonRoot.dump(4);
		OutputFile.close();
	}
	else
	{
		MessageBoxA(NULL, m_strFilePath.c_str(), "파일 경로 없음", MB_OK);
	}

	return S_OK;
}

CEffect_System* CEffect_System::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
{
	CEffect_System* pInstance = new CEffect_System(_pDevice, _pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Cloned : CEffect_System");
		Safe_Release(pInstance);
	}

	return pInstance;
}
#endif