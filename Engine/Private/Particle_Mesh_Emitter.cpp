#include "Particle_Mesh_Emitter.h"
#include "GameInstance.h"
#include "VIBuffer_Mesh_Particle.h"
#include "Material.h"
#include "Bone.h"
#include "Effect_Module.h"
#include "Translation_Module.h"
#include "Keyframe_Module.h"

CParticle_Mesh_Emitter::CParticle_Mesh_Emitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
	: CEmitter(_pDevice, _pContext)
{
	m_eEffectType = MESH;
}

CParticle_Mesh_Emitter::CParticle_Mesh_Emitter(const CParticle_Mesh_Emitter& _Prototype)
	: CEmitter(_Prototype)
	, m_PreTransformMatrix(_Prototype.m_PreTransformMatrix)
	, m_iNumMeshes(_Prototype.m_iNumMeshes)
	, m_iNumMaterials(_Prototype.m_iNumMaterials)
	, m_Materials(_Prototype.m_Materials)
#ifdef _DEBUG
	, m_strModelPath(_Prototype.m_strModelPath)
#endif
{
	if (_Prototype.m_ParticleMeshes.size())
	{
		for (auto& pPrototypeMesh : _Prototype.m_ParticleMeshes)
		{
			CVIBuffer_Mesh_Particle* pMesh = static_cast<CVIBuffer_Mesh_Particle*>(pPrototypeMesh->Clone(nullptr));
			if (nullptr != pMesh)
				m_ParticleMeshes.push_back(pMesh);
		}
	}

	if (_Prototype.m_Bones.size())
	{
		for (auto& pPrototypeBone : _Prototype.m_Bones)
		{
			m_Bones.push_back(pPrototypeBone->Clone());
		}
	}

	for (auto& pMaterial : m_Materials)
		Safe_AddRef(pMaterial);

}


HRESULT CParticle_Mesh_Emitter::Initialize_Prototype(const json& _jsonInfo)
{
	if (FAILED(__super::Initialize_Prototype(_jsonInfo)))
		return E_FAIL;

	if (false == _jsonInfo.contains("PreTransform"))
	{
		XMStoreFloat4x4(&m_PreTransformMatrix, XMMatrixIdentity());
	}
	else
	{
		for (size_t i = 0; i < sizeof(_float4x4) / sizeof(_float); ++i)
		{
			*((_float*)(&m_PreTransformMatrix) + i) = _jsonInfo["PreTransform"][i];
		}
	}

	if (false == _jsonInfo.contains("Model"))
		return E_FAIL;

	_string strModelPath = _jsonInfo["Model"];
#ifdef _DEBUG
	m_strModelPath = strModelPath;
#endif

	std::ifstream inFile(strModelPath.c_str(), std::ios::binary);
	if (!inFile) {
		string str = "파일을 열 수 없습니다.";
		str += strModelPath;
		MessageBoxA(NULL, str.c_str(), "에러", MB_OK);
		return E_FAIL;
	}

	_bool bAnim;
	inFile.read(reinterpret_cast<char*>(&bAnim), 1);
	if (FAILED(Ready_Bones(inFile, -1)))
		return E_FAIL;


	if (FAILED(Ready_Meshes(inFile, _jsonInfo["Buffer"])))
		return E_FAIL;

	if (FAILED(Ready_Materials(inFile, strModelPath.c_str())))
		return E_FAIL;

	inFile.close();

	if (_jsonInfo.contains("ShaderPass"))
	{
		EFFECT_SHADERPASS eShaderPass = _jsonInfo["ShaderPass"];
		m_iShaderPass = eShaderPass;
	}

	// Initial Buffer
	for (auto pModule : m_Modules)
	{
		if (false == pModule->Is_Init())
			continue;

		m_ParticleMeshes[0]->Initialize_Module(pModule);
	}

	if (FAILED(m_ParticleMeshes[0]->Initialize_Buffers()))
		return E_FAIL;

	return S_OK;
}


HRESULT CParticle_Mesh_Emitter::Initialize(void* _pArg)
{
	if (FAILED(__super::Initialize(_pArg)))
		return E_FAIL;

	//if (false == m_isWorld)
	//{
	//	for (auto& pParticle : m_ParticleMeshes)
	//		pParticle->Set_SpawnMatrix(m_pParentMatrices[COORDINATE_3D]);
	//}

	return S_OK;
}

void CParticle_Mesh_Emitter::Priority_Update(_float _fTimeDelta)
{
}

void CParticle_Mesh_Emitter::Update(_float _fTimeDelta)
{
	__super::Update(_fTimeDelta);
}

void CParticle_Mesh_Emitter::Late_Update(_float _fTimeDelta)
{
	__super::Late_Update(_fTimeDelta);

	if (m_isActive && m_iAccLoop)
		m_pGameInstance->Add_RenderObject_New(s_iRG_3D, s_iRGP_PARTICLE, this);

}

HRESULT CParticle_Mesh_Emitter::Render()
{
	if (nullptr == m_pShaderCom || 0 == m_ParticleMeshes.size())
		return S_OK;

	if (FAILED(Bind_ShaderResources()))
		return E_FAIL;

	for (_uint i = 0; i < m_iNumMeshes; ++i)
	{
		// TODO : 어떻게 하지
		if (FAILED(Bind_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE, 0)))
			return E_FAIL;

		// PASS 설정
		if (FAILED(m_pShaderCom->Begin(m_iShaderPass)))
			return E_FAIL;

		if (FAILED(m_ParticleMeshes[i]->Bind_BufferBySRV()))
			return E_FAIL;
		if (FAILED(m_ParticleMeshes[i]->Render_BySRV()))
			return E_FAIL;

		//m_ParticleMeshes[i]->Bind_BufferDesc();
		//m_ParticleMeshes[i]->Render();
	}


	return S_OK;
}

void CParticle_Mesh_Emitter::Reset()
{
	if (0 < m_ParticleMeshes.size())
		m_ParticleMeshes[0]->Reset_Buffers(m_pComputeShader);
}



void CParticle_Mesh_Emitter::Update_Emitter(_float _fTimeDelta)
{
	if (0 ==  m_ParticleMeshes.size())
		return;

	for (_int i = 0; i < m_ParticleMeshes.size(); ++i)
	{
		m_pComputeShader->Bind_RawValue("g_fTimeDelta", &_fTimeDelta, sizeof(_float));

		_float fKill;
		if (KILL == m_ePooling)
		{
			fKill = D3D11_FLOAT32_MAX * -1.f;
		}
		else
		{
			fKill = 0.f;
		}
		m_pComputeShader->Bind_RawValue("g_fKill", &fKill, sizeof(_float));
		m_pComputeShader->Begin(0);
		m_ParticleMeshes[i]->Begin_Compute(m_pComputeShader);
		m_ParticleMeshes[i]->Compute(m_pComputeShader);
		m_pComputeShader->End_Compute();

		if (STOP_SPAWN != m_eNowEvent)
		{
			if (SPAWN_RATE == m_eSpawnType)
			{
				_float fAbsolute;
				if (RELATIVE_WORLD == m_eSpawnPosition)
				{
					fAbsolute = 0.f;
					m_pComputeShader->Bind_RawValue("g_fAbsolute", &fAbsolute, sizeof(_float));
				}
				else if (ABSOLUTE_WORLD == m_eSpawnPosition)
				{
					fAbsolute = 1.f;
					m_pComputeShader->Bind_RawValue("g_fAbsolute", &fAbsolute, sizeof(_float));
					if (m_pSpawnMatrix)
						m_pComputeShader->Bind_Matrix("g_SpawnMatrix", m_pSpawnMatrix);
					else
						m_pComputeShader->Bind_Matrix("g_SpawnMatrix", &s_IdentityMatrix);
				}
				m_pComputeShader->Begin(1);

			}
			else if (BURST_SPAWN == m_eSpawnType)
			{
				_float fAbsolute;
				if (RELATIVE_WORLD == m_eSpawnPosition)
				{
					fAbsolute = 0.f;
					m_pComputeShader->Bind_RawValue("g_fAbsolute", &fAbsolute, sizeof(_float));
				}
				else if (ABSOLUTE_WORLD == m_eSpawnPosition)
				{
					fAbsolute = 1.f;
					m_pComputeShader->Bind_RawValue("g_fAbsolute", &fAbsolute, sizeof(_float));
					if (m_pSpawnMatrix)
						m_pComputeShader->Bind_Matrix("g_SpawnMatrix", m_pSpawnMatrix);
					else
						m_pComputeShader->Bind_Matrix("g_SpawnMatrix", &s_IdentityMatrix);
				}
				m_pComputeShader->Begin(2);

				m_eNowEvent = STOP_SPAWN;
				m_fInactiveDelayTime = m_fActiveTime;
			}
			m_ParticleMeshes[i]->Begin_Compute(m_pComputeShader);
			m_ParticleMeshes[i]->Compute(m_pComputeShader);
			m_pComputeShader->End_Compute();
		}

		for (auto pModule : m_Modules)
		{
			if (pModule->Is_Init())
				continue;

			m_ParticleMeshes[i]->Update_Module(pModule, m_pComputeShader);
		}
	}
}

// Map
//m_ParticleMeshes[i]->Begin_Update(_fTimeDelta);
//
//// Spawn
//if (STOP_SPAWN != m_eNowEvent)
//{
//	if (SPAWN_RATE == m_eSpawnType)
//	{
//		if (RELATIVE_WORLD == m_eSpawnPosition)
//			m_ParticleMeshes[i]->Spawn_Rate(_fTimeDelta, m_FloatDatas["SpawnRate"], nullptr);
//		else if (ABSOLUTE_WORLD == m_eSpawnPosition)
//			m_ParticleMeshes[i]->Spawn_Rate(_fTimeDelta, m_FloatDatas["SpawnRate"],
//				&m_WorldMatrices[COORDINATE_3D]);
//
//	}
//	else if (BURST_SPAWN == m_eSpawnType)
//	{
//		if (RELATIVE_WORLD == m_eSpawnPosition)
//			m_ParticleMeshes[i]->Spawn_Rate(_fTimeDelta, m_FloatDatas["SpawnRate"], nullptr);
//		else if (ABSOLUTE_WORLD == m_eSpawnPosition)
//			m_ParticleMeshes[i]->Spawn_Rate(_fTimeDelta, m_FloatDatas["SpawnRate"],
//				&m_WorldMatrices[COORDINATE_3D]);
//
//		m_eNowEvent = STOP_SPAWN;
//		m_fInactiveDelayTime = m_fActiveTime;
//	}
//}
//
//// Module
//for (auto pModule : m_Modules)
//{
//	if (pModule->Is_Init())
//		continue;
//
//	CEffect_Module::DATA_APPLY eData = pModule->Get_ApplyType();
//	CEffect_Module::MODULE_TYPE eType = pModule->Get_Type();
//
//	if (CEffect_Module::MODULE_TYPE::MODULE_KEYFRAME == eType)
//	{
//		if (CEffect_Module::DATA_APPLY::COLOR == eData)
//		{
//			m_ParticleMeshes[i]->Update_ColorKeyframe(pModule);
//		}
//
//		else if (CEffect_Module::DATA_APPLY::SCALE == eData)
//		{
//			m_ParticleMeshes[i]->Update_ScaleKeyframe(pModule);
//		}
//	}
//	else if (CEffect_Module::MODULE_TYPE::MODULE_TRANSLATION == eType)
//	{
//		if (CEffect_Module::DATA_APPLY::TRANSLATION == eData)
//		{
//			m_ParticleMeshes[i]->Update_Translation(_fTimeDelta, pModule);
//		}
//	}
//}
//
//// Kill Or Revive
//if (KILL == m_ePooling)
//m_ParticleMeshes[i]->Update_Buffer(_fTimeDelta, false);
//else if (REVIVE == m_ePooling)
//m_ParticleMeshes[i]->Update_Buffer(_fTimeDelta, true);
//
//// UnMap
//m_ParticleMeshes[i]->End_Update(_fTimeDelta);


HRESULT CParticle_Mesh_Emitter::Bind_ShaderResources()
{
	if (RELATIVE_WORLD == m_eSpawnPosition)
	{
		if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_WorldMatrices[COORDINATE_3D])))
			return E_FAIL;
	}
	else if (ABSOLUTE_WORLD == m_eSpawnPosition)
	{
		if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &s_IdentityMatrix)))
			return E_FAIL;
	}

	if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_pGameInstance->Get_TransformFloat4x4(CPipeLine::D3DTS_VIEW))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_pGameInstance->Get_TransformFloat4x4(CPipeLine::D3DTS_PROJ))))
		return E_FAIL;



	return S_OK;
}

HRESULT CParticle_Mesh_Emitter::Bind_Material(CShader* _pShader, const _char* _pConstantName, _uint _iMeshIndex, aiTextureType _eTextureType, _uint _iTextureIndex)
{
	_uint iMaterialIndex = m_ParticleMeshes[_iMeshIndex]->Get_MaterialIndex();

	ID3D11ShaderResourceView* pSRV = m_Materials[iMaterialIndex]->Find_Texture(_eTextureType, _iTextureIndex);
	if (nullptr == pSRV)
		return E_FAIL;

	return _pShader->Bind_SRV(_pConstantName, pSRV);

}

HRESULT CParticle_Mesh_Emitter::Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc)
{
	if (FAILED(__super::Ready_Components(_pDesc)))
		return E_FAIL;

	for (_int i = 0; i < m_ParticleMeshes.size(); ++i)
	{
		_wstring wstrName = L"Particle" + to_wstring(i);

		m_Components.emplace(wstrName, m_ParticleMeshes[i]);
		Safe_AddRef(m_ParticleMeshes[i]);
	}

	return S_OK;
}

HRESULT CParticle_Mesh_Emitter::Ready_Bones(ifstream& _inFile, _uint _iParentBoneIndex)
{
	CBone* pBone = CBone::Create(_inFile, _iParentBoneIndex);

	if (nullptr == pBone)
		return E_FAIL;

	m_Bones.push_back(pBone);

	_iParentBoneIndex = (_uint)m_Bones.size() - 1;
	_uint iNumChildren = 0;
	_inFile.read(reinterpret_cast<char*>(&iNumChildren), sizeof(_uint));
	//cout << iNumChildren << endl;
	//for (_uint i = 0; i < iNumChildren; ++i)
	//{
	//	Ready_Bones(_inFile, _iParentBoneIndex);
	//}

	return S_OK;
}

HRESULT CParticle_Mesh_Emitter::Ready_Meshes(ifstream& _inFile, const json& _jsonBufferInfo)
{
	_inFile.read(reinterpret_cast<char*>(&m_iNumMeshes), sizeof(_uint));
	for (_uint i = 0; i < m_iNumMeshes; i++)
	{
		CVIBuffer_Mesh_Particle* pMesh = CVIBuffer_Mesh_Particle::Create(m_pDevice, m_pContext, _inFile, _jsonBufferInfo, XMLoadFloat4x4(&m_PreTransformMatrix), m_FloatDatas["SpawnRate"]);
		if (nullptr == pMesh)
			return E_FAIL;

		m_ParticleMeshes.push_back(pMesh);
	}

	return S_OK;
}

HRESULT CParticle_Mesh_Emitter::Ready_Materials(ifstream& _inFile, const _char* _szModelFilePath)
{
	_inFile.read(reinterpret_cast<char*>(&m_iNumMaterials), sizeof(_uint));
	//cout << m_iNumMaterials << endl;
	m_Materials.resize(m_iNumMaterials);
	_char		szDrive[MAX_PATH] = "";
	_char		szDirectory[MAX_PATH] = "";
	_splitpath_s(_szModelFilePath, szDrive, MAX_PATH, szDirectory, MAX_PATH, nullptr, 0, nullptr, 0);
	strcat_s(szDrive, szDirectory);
	for (_uint i = 0; i < m_iNumMaterials; i++)
	{
		CMaterial* pMaterial = CMaterial::Create(m_pDevice, m_pContext, szDrive, _inFile);
		m_Materials[i] = pMaterial;
	}
	return S_OK;
}


CParticle_Mesh_Emitter* CParticle_Mesh_Emitter::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonInfo)
{
	CParticle_Mesh_Emitter* pInstance = new CParticle_Mesh_Emitter(_pDevice, _pContext);

	if (FAILED(pInstance->Initialize_Prototype(_jsonInfo)))
	{
		MSG_BOX("Failed to Created : CParticle_Mesh_Emitter");
		Safe_Release(pInstance);
	}
	return pInstance;
}


CGameObject* CParticle_Mesh_Emitter::Clone(void* _pArg)
{
	CParticle_Mesh_Emitter* pInstance = new CParticle_Mesh_Emitter(*this);

	if (FAILED(pInstance->Initialize(_pArg)))
	{
		MSG_BOX("Failed to Cloned : CParticle_Mesh_Emitter");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CParticle_Mesh_Emitter::Free()
{
	for (auto& pBone : m_Bones)
		Safe_Release(pBone);
	m_Bones.clear();

	for (auto& pMaterial : m_Materials)
		Safe_Release(pMaterial);
	m_Materials.clear();

	for (auto& pMesh : m_ParticleMeshes)
		Safe_Release(pMesh);
	m_ParticleMeshes.clear();

	__super::Free();
}

HRESULT CParticle_Mesh_Emitter::Cleanup_DeadReferences()
{
	return S_OK;
}

#ifdef _DEBUG
void CParticle_Mesh_Emitter::Tool_Setting()
{
	if (m_ParticleMeshes.size())
	{
		for (auto& pMesh : m_ParticleMeshes)
			pMesh->Tool_Setting();
	}
}

void CParticle_Mesh_Emitter::Tool_Update(_float _fTimeDelta)
{
	ImGui::Begin("Adjust_Mesh_Emitter");

	if (ImGui::Button("Reset"))
	{
		if (nullptr != m_ParticleMeshes[0])
		{
			m_ParticleMeshes[0]->Tool_Reset_Buffers(m_FloatDatas["SpawnRate"], m_pComputeShader, m_Modules);
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

							CKeyframe_Module* pModule = CKeyframe_Module::Create(m_pDevice, m_pContext, (CKeyframe_Module::MODULE_NAME)n, m_ParticleMeshes[0]->Get_NumInstance());
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
		const _char* items[] = { "Default" };
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


	if (m_ParticleMeshes.size())
	{
		m_ParticleMeshes[0]->Tool_Update(_fTimeDelta);

		if (false == m_isToolChanged)
			m_isToolChanged = m_ParticleMeshes[0]->Is_ToolChanged();


	}
	else
	{
		static _int iNumInstanceInput = 1;
		static _char szInputModelPath[MAX_PATH] = "../Bin/Resources/Models/FX/";

		ImGui::InputText("Model Path", szInputModelPath, MAX_PATH);

		if (ImGui::InputInt("Instance Count", &iNumInstanceInput))
		{
			if (0 >= iNumInstanceInput)
			{
				iNumInstanceInput = 1;
			}
		}

		if (ImGui::Button("Create_Buffer"))
		{
			XMMATRIX matPreTransform = XMMatrixScaling(1 / 150.0f, 1 / 150.0f, 1 / 150.0f);
			matPreTransform *= XMMatrixRotationAxis(_vector{ 0,1,0,0 }, XMConvertToRadians(180));
			XMStoreFloat4x4(&m_PreTransformMatrix, matPreTransform);

			m_strModelPath = szInputModelPath;

			std::ifstream inFile(m_strModelPath.c_str(), std::ios::binary);
			if (!inFile) {
				string str = "파일을 열 수 없습니다.";
				str += m_strModelPath;
				MessageBoxA(NULL, str.c_str(), "에러", MB_OK);
			}
			
			else
			{
				_bool bAnim;
				inFile.read(reinterpret_cast<char*>(&bAnim), 1);
				if (FAILED(Ready_Bones(inFile, -1)))
				{
					MSG_BOX("Bone 실패 !!");
				}

				else
				{
					if (FAILED(Ready_Meshes(inFile, iNumInstanceInput)))
					{
						MSG_BOX("Mesh 실패 !!");
					}

					else
					{
						if (FAILED(Ready_Materials(inFile, m_strModelPath.c_str())))
						{
							MSG_BOX("Material 실패 !!");
						}
					}

				}
				
				

				inFile.close();
			}
			
		}

	}


	ImGui::End();
}

HRESULT CParticle_Mesh_Emitter::Save(json& _jsonOut)
{
	if (0 == m_ParticleMeshes.size())
	{
		MSG_BOX("Model이나 불러오세요.");
	}

	if (FAILED(__super::Save(_jsonOut)))
		return E_FAIL;

	_jsonOut["Model"] = m_strModelPath.c_str();
	
	for (_int i = 0; i < 16; ++i)
	{
		_jsonOut["PreTransform"].push_back(*((_float*)(&m_PreTransformMatrix) + i));
	}

	json jsonBufferInfo;

	if (FAILED(m_ParticleMeshes[0]->Save_Buffer(jsonBufferInfo)))
	{
		return E_FAIL;
	}

	_jsonOut["Buffer"] = jsonBufferInfo;

	EFFECT_SHADERPASS eShaderPass = (EFFECT_SHADERPASS)m_iShaderPass;
	_jsonOut["ShaderPass"] = eShaderPass;


	return S_OK;
}



HRESULT CParticle_Mesh_Emitter::Ready_Meshes(ifstream& _inFile, _uint _iNumInstance)
{
	_inFile.read(reinterpret_cast<char*>(&m_iNumMeshes), sizeof(_uint));
	for (_uint i = 0; i < m_iNumMeshes; i++)
	{
		CVIBuffer_Mesh_Particle* pMesh = CVIBuffer_Mesh_Particle::Create(m_pDevice, m_pContext, _inFile, _iNumInstance, XMLoadFloat4x4(&m_PreTransformMatrix), m_FloatDatas["SpawnRate"]);
		if (nullptr == pMesh)
			return E_FAIL;

		m_ParticleMeshes.push_back(pMesh);
	}

	return S_OK;
}

CParticle_Mesh_Emitter* CParticle_Mesh_Emitter::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, void* _pArg)
{
	CParticle_Mesh_Emitter* pInstance = new CParticle_Mesh_Emitter(_pDevice, _pContext);

	if (FAILED(pInstance->Initialize(_pArg)))
	{
		MSG_BOX("Failed to Cloned : CParticle_Mesh_Emitter");
		Safe_Release(pInstance);
	}

	return pInstance;
}
#endif