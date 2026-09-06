#include "Emitter.h"

#include "Effect_Module.h"
#include "Keyframe_Module.h"
#include "Translation_Module.h"
#include "UV_Module.h"

#include "GameInstance.h"

_int			CEmitter::s_iRG_3D = 0;
_int			CEmitter::s_iRG_2D = 0;
_int			CEmitter::s_iRGP_PARTICLE = 0;
_int			CEmitter::s_iRGP_EFFECT = 0;
_float4x4		CEmitter::s_IdentityMatrix = _float4x4(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

CEmitter::CEmitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
	: CPartObject(_pDevice, _pContext)
{
	XMStoreFloat4x4(&m_SpawnMatrix, XMMatrixIdentity());
}

CEmitter::CEmitter(const CEmitter& _Prototype)
	: CPartObject(_Prototype)
	, m_eEffectType(_Prototype.m_eEffectType)
	, m_eSpawnType(_Prototype.m_eSpawnType)
	, m_ePooling(_Prototype.m_ePooling)
	, m_iShaderPass(_Prototype.m_iShaderPass)
	, m_eSpawnPosition(_Prototype.m_eSpawnPosition)
	, m_iEventID(_Prototype.m_iEventID)
	, m_fActiveTime(_Prototype.m_fActiveTime)
	, m_fSpawnDelayTime(_Prototype.m_fSpawnDelayTime)
	, m_fAccTime(_Prototype.m_fAccTime)
	, m_iLoopTime(_Prototype.m_iLoopTime)
	, m_SpawnMatrix(_Prototype.m_SpawnMatrix)
	, m_FloatDatas(_Prototype.m_FloatDatas)
	, m_Float2Datas(_Prototype.m_Float2Datas)
	, m_Float4Datas(_Prototype.m_Float4Datas)
{
	m_isActive = _Prototype.m_isActive;


	for (auto& pModule : _Prototype.m_Modules)
	{
		m_Modules.push_back(pModule->Clone());
	}
}

HRESULT CEmitter::Initialize_Prototype(const json& _jsonInfo)
{
	if (_jsonInfo.contains("SpawnPosition"))
		m_eSpawnPosition = _jsonInfo["SpawnPosition"];
	if (_jsonInfo.contains("SpawnType"))
		m_eSpawnType = _jsonInfo["SpawnType"];
	if (_jsonInfo.contains("Pooling"))
		m_ePooling = _jsonInfo["Pooling"];
	if (_jsonInfo.contains("Event"))
		m_iEventID = _jsonInfo["Event"];
	//if (_jsonInfo.contains("Progress"))
	//	m_isActive = _jsonInfo["Progress"];
	if (_jsonInfo.contains("Time"))
		m_fActiveTime = _jsonInfo["Time"];
	if (_jsonInfo.contains("DelayTime"))
		m_fSpawnDelayTime = _jsonInfo["DelayTime"];
	if (_jsonInfo.contains("LoopTime"))
		m_iLoopTime = _jsonInfo["LoopTime"];
	if (_jsonInfo.contains("Transform"))
	{
		for (_int i = 0; i < 16; ++i)
		{
			*((_float*)(&m_SpawnMatrix) + i) = _jsonInfo["Transform"][i];			
		}
	}


	if (_jsonInfo.contains("Modules"))
	{
		for (_int i = 0; i < _jsonInfo["Modules"].size(); ++i)
		{
			CEffect_Module::MODULE_TYPE eType = _jsonInfo["Modules"][i]["Type"];
			CEffect_Module* pOutModule = { nullptr };

			if (CEffect_Module::MODULE_KEYFRAME == eType)
				pOutModule = CKeyframe_Module::Create(m_pDevice, m_pContext, _jsonInfo["Modules"][i], 1);
			if (CEffect_Module::MODULE_TRANSLATION == eType)
				pOutModule = CTranslation_Module::Create(_jsonInfo["Modules"][i]);
			if (CEffect_Module::MODULE_UV == eType)
				pOutModule = CUV_Module::Create(_jsonInfo["Modules"][i]);

			if (nullptr != pOutModule)
			{
				m_Modules.push_back(pOutModule);
			}
		}

		sort(m_Modules.begin(), m_Modules.end(), [](const CEffect_Module* pSrc, const CEffect_Module* pDst)
			{
				return pSrc->Get_Order() < pDst->Get_Order();
			}
		);
	}

	if (_jsonInfo.contains("Float"))
	{
		for (_int i = 0; i < _jsonInfo["Float"].size(); ++i)
		{
			_string str = _jsonInfo["Float"][i]["Name"];
			_float fValue = _jsonInfo["Float"][i]["Value"];
			m_FloatDatas.emplace(str, fValue);
		}
	}

	if (_jsonInfo.contains("Float2"))
	{
		for (_int i = 0; i < _jsonInfo["Float2"].size(); ++i)
		{
			_string str = _jsonInfo["Float2"][i]["Name"];
			_float2 vValue;
			vValue.x = _jsonInfo["Float2"][i]["Value"][0];
			vValue.y = _jsonInfo["Float2"][i]["Value"][1];

			m_Float2Datas.emplace(str, vValue);
		}
	}

	if (_jsonInfo.contains("Float4"))
	{
		for (_int i = 0; i < _jsonInfo["Float4"].size(); ++i)
		{
			_string str = _jsonInfo["Float4"][i]["Name"];
			_float4 vValue;
			vValue.x = _jsonInfo["Float4"][i]["Value"][0];
			vValue.y = _jsonInfo["Float4"][i]["Value"][1];
			vValue.z = _jsonInfo["Float4"][i]["Value"][2];
			vValue.w = _jsonInfo["Float4"][i]["Value"][3];

			m_Float4Datas.emplace(str, vValue);
		}
	}

	return S_OK;
}

HRESULT CEmitter::Initialize(void* _pArg)
{
	PARTICLE_EMITTER_DESC* pDesc = static_cast<PARTICLE_EMITTER_DESC*>(_pArg);

	if (nullptr == pDesc)
		return E_FAIL;

	pDesc->eStartCoord = COORDINATE_3D;
	pDesc->isCoordChangeEnable = false;
	
	if (FAILED(__super::Initialize(_pArg)))
		return E_FAIL;

	m_pControllerTransform->Set_WorldMatrix(XMLoadFloat4x4(&m_SpawnMatrix));

	if (FAILED(Ready_Components(pDesc)))
		return E_FAIL;

	Set_Active(false);

	return S_OK;
}

void CEmitter::Update(_float _fTimeDelta)
{
	if (m_isActive)
	{
		m_fAccTime += _fTimeDelta;

		// Delay 시간 적용
		if (0 >= m_iAccLoop && m_fSpawnDelayTime <= m_fAccTime)
		{
			++m_iAccLoop;
			m_fAccTime = 0.f;
			Set_Matrix();
		}
		else if (0 < m_iAccLoop)
		{
			if (m_fActiveTime <= m_fAccTime && m_iAccLoop <= m_iLoopTime)
			{
				++m_iAccLoop;

				if (m_iAccLoop >= m_iLoopTime)
				{
					m_fInactiveDelayTime = m_fActiveTime;
					m_eNowEvent = STOP_SPAWN;
				}
				else if (0 != m_iLoopTime)
				{
					Reset();
				}

				m_fAccTime = 0.f;
			}			

			Update_Emitter(_fTimeDelta);
		}

		if (STOP_SPAWN == m_eNowEvent)
		{
			m_fAccEventTime += _fTimeDelta;

			if (m_fInactiveDelayTime <= m_fAccEventTime)
			{
				Set_Active(false);
			}
		}
	}

}

void CEmitter::Late_Update(_float _fTimeDelta)
{

}


void CEmitter::Active_OnEnable()
{
	m_fAccTime = 0.f;
	m_iAccLoop = 0;

	m_fAccEventTime = 0.f;
	m_fInactiveDelayTime = D3D11_FLOAT32_MAX;

	m_eNowEvent = NO_EVENT;
}

void CEmitter::Active_OnDisable()
{
	m_fAccTime = 0.f;
	m_iAccLoop = 0;
	m_fAccEventTime = 0.f;
	m_fInactiveDelayTime = D3D11_FLOAT32_MAX;

	m_eNowEvent = NO_EVENT;

	ZeroMemory(&m_WorldMatrices[COORDINATE_3D], sizeof(_float4x4));

}

void CEmitter::Set_Matrix()
{
	//if (FOLLOW_PARENT != m_eSpawnPosition)
	{
		_matrix SpawnMatrix = {};
		if (nullptr != m_pSpawnMatrix)
		{
			SpawnMatrix = XMLoadFloat4x4(m_pSpawnMatrix);
			SpawnMatrix.r[0] = XMVector3Normalize(SpawnMatrix.r[0]);
			SpawnMatrix.r[1] = XMVector3Normalize(SpawnMatrix.r[1]);
			SpawnMatrix.r[2] = XMVector3Normalize(SpawnMatrix.r[2]);
		}
		else
			SpawnMatrix = XMMatrixIdentity();


		XMStoreFloat4x4(&m_WorldMatrices[COORDINATE_3D], m_pControllerTransform->Get_WorldMatrix(COORDINATE_3D) * SpawnMatrix * XMLoadFloat4x4(m_pParentMatrices[COORDINATE_3D]));
	}	
}

HRESULT CEmitter::Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc)
{
	if (FAILED(Add_Component(_pDesc->iProtoShaderLevel, _pDesc->szShaderTag,
		TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom))))
		return E_FAIL;

	if (nullptr != _pDesc->szComputeShaderTag)
	{
		if (FAILED(Add_Component(_pDesc->iProtoShaderLevel, _pDesc->szComputeShaderTag,
			TEXT("Com_ComputeShader"), reinterpret_cast<CComponent**>(&m_pComputeShader))))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CEmitter::Bind_Float(const _char* _szDataName, const _char* _szConstantName)
{
	return m_pShaderCom->Bind_RawValue(_szConstantName, &m_FloatDatas[_szDataName], sizeof(_float));

}

HRESULT CEmitter::Bind_Float2(const _char* _szDataName, const _char* _szConstantName)
{
	return m_pShaderCom->Bind_RawValue(_szConstantName, &m_Float2Datas[_szDataName], sizeof(_float2));
}

HRESULT CEmitter::Bind_Float4(const _char* _szDataName, const _char* _szConstantName)
{
	return m_pShaderCom->Bind_RawValue(_szConstantName, &m_Float4Datas[_szDataName], sizeof(_float4));
}

void CEmitter::Free()
{
	Safe_Release(m_pShaderCom);
	Safe_Release(m_pComputeShader);

	for (auto& pModule : m_Modules)
		Safe_Release(pModule);

	m_FloatDatas.clear();
	m_Float2Datas.clear();
	m_Float4Datas.clear();

	__super::Free();
}

#ifdef _DEBUG
void CEmitter::Tool_Update(_float fTimeDelta)
{
	if (ImGui::TreeNode("Position Type"))
	{
		const char* items[] = { "Relative Position", "Absolute Position", "Follow Parent" };
		static _int item_selected_idx = 0;
		const char* combo_preview_value = items[item_selected_idx];

		item_selected_idx = (SPAWN_POSITION)m_eSpawnPosition;
		if (ImGui::BeginCombo("Spawn Position", combo_preview_value))
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				const bool is_selected = (item_selected_idx == n);

				if (ImGui::Selectable(items[n], is_selected))
				{
					item_selected_idx = n;

					m_eSpawnPosition = (SPAWN_POSITION)n;
					m_isToolChanged = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Spawn Type"))
	{
		const char* items[] = { "Spawn_Rate", "Burst_Spawn" };
		static _int item_selected_idx = 0;
		const char* combo_preview_value = items[item_selected_idx];

		item_selected_idx = (SPAWN_TYPE)m_eSpawnType;
		if (ImGui::BeginCombo("Spawn Type", combo_preview_value))
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				const bool is_selected = (item_selected_idx == n);

				if (ImGui::Selectable(items[n], is_selected))
				{
					item_selected_idx = n;

					m_eSpawnType = (SPAWN_TYPE)n;
					m_isToolChanged = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (SPAWN_RATE == m_eSpawnType)
		{
			ImGui::DragFloat("Spawn Rate", &m_FloatDatas["SpawnRate"], 1.f, 10.f);			
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Pooling Type"))
	{
		const char* items[] = { "Kill", "Revive" };
		static _int item_selected_idx = 0;
		const char* combo_preview_value = items[item_selected_idx];

		item_selected_idx = (POOLING_TYPE)m_ePooling;
		if (ImGui::BeginCombo("Spawn Type", combo_preview_value))
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				const bool is_selected = (item_selected_idx == n);

				if (ImGui::Selectable(items[n], is_selected))
				{
					item_selected_idx = n;

					m_ePooling = (POOLING_TYPE)n;
					m_isToolChanged = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::TreePop();
	}


	if (ImGui::InputInt("Event", (_int*)&m_iEventID))
		m_isToolChanged = true;
	if (ImGui::InputFloat("Time", &m_fActiveTime))
		m_isToolChanged = true;
	
	ImGui::SameLine();
	if (ImGui::Button("Infinite Time"))
	{
		m_fActiveTime = D3D11_FLOAT32_MAX;
		m_isToolChanged = true;
	}

	if (ImGui::InputFloat("Delay Time", &m_fSpawnDelayTime))
		m_isToolChanged = true;
	if (ImGui::InputInt("Loop Count", (_int*)&m_iLoopTime))
		m_isToolChanged = true;
	ImGui::SameLine();
	if (ImGui::Button("Infinite Loop"))
	{
		m_iLoopTime = UINT32_MAX;
		m_isToolChanged = true;
	}

	if (ImGui::TreeNode("Transform"))
	{
		static _float3 vScale = { 1.f, 1.f, 1.f };
		static _float3 vRotaiton = { 0.f, 0.f, 0.f };
		static _float4 vQuaternion = { 0.f, 0.f, 0.f, 1.f };
		static _float3 vPosition = { 0.f, 0.f, 0.f };
		if (ImGui::Button("Get_Transform_Info"))
		{
			_matrix matWorld = m_pControllerTransform->Get_WorldMatrix();

			for (_int i = 0; i < 3; ++i)
			{
				*((_float*)(&vScale) + i) = XMVectorGetX(XMVector3Length(matWorld.r[i]));
				matWorld.r[i] = XMVector3Normalize(matWorld.r[i]);
			}

			XMStoreFloat4(&vQuaternion, XMQuaternionRotationMatrix(matWorld));

			XMStoreFloat3(&vPosition, matWorld.r[3]);
		}

		ImGui::Text("Rotation : %.3f, %.3f, %.3f, %.3f", vQuaternion.x, vQuaternion.y, vQuaternion.z, vQuaternion.w);

		if (ImGui::DragFloat3("Scaling", (_float*)(&vScale), 0.01f))
		{
			if (vScale.x != 0.f && vScale.y != 0.f && vScale.z != 0.f)
				m_pControllerTransform->Set_Scale(vScale);
		}

		if (ImGui::DragFloat3("Rotation", (_float*)(&vRotaiton), 0.01f))
		{
			m_pControllerTransform->RotationXYZ(vRotaiton);
		}

		if (ImGui::DragFloat3("Position", (_float*)(&vPosition), 0.01f))
		{
			m_pControllerTransform->Set_State(CTransform::STATE_POSITION, XMVectorSetW(XMLoadFloat3(&vPosition), 1.f));
		}

		ImGui::TreePop();
	}
	
	if (ImGui::TreeNode("Float"))
	{
		static _char szInput[MAX_PATH] = "";

		for (auto iter = m_FloatDatas.begin(); iter != m_FloatDatas.end(); )
		{
			ImGui::DragFloat(iter->first.c_str(), (_float*)&iter->second, 0.005f);
			ImGui::SameLine();
			if (ImGui::Button(_string("Delete" + iter->first).c_str()))
			{
				iter = m_FloatDatas.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Float2"))
	{
		for (auto iter = m_Float2Datas.begin(); iter != m_Float2Datas.end(); )
		{
			ImGui::DragFloat2(iter->first.c_str(), (_float*)&iter->second, 0.005f);
			ImGui::SameLine();
			if (ImGui::Button(_string("Delete" + iter->first).c_str()))
			{
				iter = m_Float2Datas.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Float4"))
	{
		for (auto iter = m_Float4Datas.begin(); iter != m_Float4Datas.end(); )
		{
			ImGui::DragFloat4(iter->first.c_str(), (_float*)&iter->second, 0.005f);
			ImGui::SameLine();
			if (ImGui::Button(_string("Delete" + iter->first).c_str()))
			{
				iter = m_Float4Datas.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		ImGui::TreePop();
	}
}
void CEmitter::Tool_Setting()
{

}

HRESULT CEmitter::Save(json& _jsonOut)
{
	_jsonOut["SpawnPosition"] = m_eSpawnPosition;
	_jsonOut["SpawnType"] = m_eSpawnType;
	_jsonOut["Pooling"] = m_ePooling;
	_jsonOut["Event"] = m_iEventID;
	//_jsonOut["Progress"] = m_isToolProgress;
	_jsonOut["Time"] = m_fActiveTime;
	_jsonOut["LoopTime"] = m_iLoopTime;
	_jsonOut["Type"] = m_eEffectType;
	_jsonOut["DelayTime"] = m_fSpawnDelayTime;

	_float4x4 matWorld;
	XMStoreFloat4x4(&matWorld, m_pControllerTransform->Get_WorldMatrix());
	for (_int i = 0; i < 16; ++i)
		_jsonOut["Transform"].push_back(*((_float*)(&matWorld) + i));

	for (auto& pModule : m_Modules)
	{
		json jsonModule;

		if (FAILED(pModule->Save_Module(jsonModule)))
			return E_FAIL;

		_jsonOut["Modules"].push_back(jsonModule);
	}


	for (auto& Pair : m_FloatDatas)
	{
		json jsonInfo;
		jsonInfo["Name"] = Pair.first.c_str();
		jsonInfo["Value"] = Pair.second;
		_jsonOut["Float"].push_back(jsonInfo);
	}

	for (auto& Pair : m_Float2Datas)
	{
		json jsonInfo;
		jsonInfo["Name"] = Pair.first.c_str();
		jsonInfo["Value"].push_back(Pair.second.x);
		jsonInfo["Value"].push_back(Pair.second.y);
		_jsonOut["Float2"].push_back(jsonInfo);
	}

	for (auto& Pair : m_Float4Datas)
	{
		json jsonInfo;
		jsonInfo["Name"] = Pair.first.c_str();
		jsonInfo["Value"].push_back(Pair.second.x);
		jsonInfo["Value"].push_back(Pair.second.y);
		jsonInfo["Value"].push_back(Pair.second.z);
		jsonInfo["Value"].push_back(Pair.second.w);
		_jsonOut["Float4"].push_back(jsonInfo);
	}

	return S_OK;
}
#endif

