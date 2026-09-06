#include "SpriteEffect_Emitter.h"
#include "GameInstance.h"

#include "Effect_Module.h"
#include "Keyframe_Module.h"


CSpriteEffect_Emitter::CSpriteEffect_Emitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
	: CEmitter(_pDevice, _pContext)
{
	m_eEffectType = SINGLE_SPRITE;
	m_Textures.resize(TEXTURE_END, nullptr);
}

CSpriteEffect_Emitter::CSpriteEffect_Emitter(const CSpriteEffect_Emitter& _Prototype)
	: CEmitter(_Prototype)
	, m_Textures(_Prototype.m_Textures)
	, m_vColor(_Prototype.m_vColor)
#ifdef _DEBUG
	, m_vDefaultColor(_Prototype.m_vDefaultColor)
#endif
{
	for (auto& pTexture : m_Textures)
		Safe_AddRef(pTexture);
}

HRESULT CSpriteEffect_Emitter::Initialize_Prototype(const json& _jsonInfo)
{
	if (FAILED(__super::Initialize_Prototype(_jsonInfo)))
		return E_FAIL;

	if (_jsonInfo.contains("Color"))
	{
		for (_int i = 0; i < 4; ++i)
		{
			*((_float*)(&m_vColor) + i) = _jsonInfo["Color"][i];
		}
	}

	if (_jsonInfo.contains("ShaderPass"))
	{
		EFFECT_SHADERPASS eShaderPass = _jsonInfo["ShaderPass"];
		m_iShaderPass = eShaderPass;
	}


	if (FAILED(Load_TextureInfo(_jsonInfo)))
		return E_FAIL;

#ifdef _DEBUG
	m_vDefaultColor = m_vColor;
#endif
	return S_OK;
}

HRESULT CSpriteEffect_Emitter::Initialize(void* _pArg)
{
	if (FAILED(__super::Initialize(_pArg)))
		return E_FAIL;



	return S_OK;
}

void CSpriteEffect_Emitter::Priority_Update(_float _fTimeDelta)
{
}

void CSpriteEffect_Emitter::Update(_float _fTimeDelta)
{
	__super::Update(_fTimeDelta);
}

void CSpriteEffect_Emitter::Late_Update(_float _fTimeDelta)
{
	if (ABSOLUTE_WORLD == m_eSpawnPosition)
	{
		if (m_pSpawnMatrix)
			XMStoreFloat4x4(&m_WorldMatrices[COORDINATE_3D], m_pControllerTransform->Get_WorldMatrix(COORDINATE_3D) * XMLoadFloat4x4(m_pSpawnMatrix));

		//else
			//XMStoreFloat4x4(&m_WorldMatrices[COORDINATE_3D], m_pControllerTransform->Get_WorldMatrix(COORDINATE_3D));
	}

	if (m_isActive && m_iAccLoop)
		m_pGameInstance->Add_RenderObject_New(s_iRG_3D, s_iRGP_PARTICLE, this);
}

HRESULT CSpriteEffect_Emitter::Render()
{
	if (nullptr == m_pShaderCom || nullptr == m_pBufferCom)
		return S_OK;

	if (FAILED(Bind_ShaderResources()))
		return E_FAIL;

		//m_pEffectModelCom->Bind_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE, 0)

		if (FAILED(m_pShaderCom->Begin(m_iShaderPass)))
			return E_FAIL;

		if (FAILED(m_pBufferCom->Bind_BufferDesc()))
			return E_FAIL;

		m_pBufferCom->Render();


	return S_OK;
}

void CSpriteEffect_Emitter::Reset()
{
	m_fAccTime = 0.f;
}



void CSpriteEffect_Emitter::Update_Emitter(_float _fTimeDelta)
{

	if (STOP_SPAWN == m_eNowEvent)
		Set_Active(false);

	for (auto& pModule : m_Modules)
	{
		CEffect_Module::DATA_APPLY eData = pModule->Get_ApplyType();
		CEffect_Module::MODULE_TYPE eType = pModule->Get_Type();

		if (CEffect_Module::MODULE_TYPE::MODULE_KEYFRAME == eType)
		{
			if (CEffect_Module::DATA_APPLY::COLOR == eData)
			{
				pModule->Update_ColorKeyframe(m_fAccTime, &m_vColor);
			}

			else if (CEffect_Module::DATA_APPLY::SCALE == eData)
			{
				const _float4x4* pWorldMatrix = m_pControllerTransform->Get_WorldMatrix_Ptr();
				pModule->Update_ScaleKeyframe(m_fAccTime, ((_float4*)(pWorldMatrix)), ((_float4*)(pWorldMatrix)+1), ((_float4*)(pWorldMatrix)+2));
			}
		}
		else if (CEffect_Module::MODULE_TYPE::MODULE_TRANSLATION == eType)
		{

		}
	}
}

HRESULT CSpriteEffect_Emitter::Bind_ShaderResources()
{
	//if (RELATIVE_WORLD == m_eSpawnPosition)
	//{
		if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_WorldMatrices[COORDINATE_3D])))
			return E_FAIL;
	//}
	//else if (ABSOLUTE_WORLD == m_eSpawnPosition)
	//{
		//if (FAILED(m_pControllerTransform->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
		//	return E_FAIL;
	//}

	if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_pGameInstance->Get_TransformFloat4x4(CPipeLine::D3DTS_VIEW))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_pGameInstance->Get_TransformFloat4x4(CPipeLine::D3DTS_PROJ))))
		return E_FAIL;


	if (FAILED(Bind_UV()))
		return E_FAIL;

	if (FAILED(Bind_ShaderValue_ByPass()))
		return E_FAIL;


	_float4 vLook;
	XMStoreFloat4(&vLook, m_pGameInstance->Get_TransformInverseMatrix(CPipeLine::D3DTS_VIEW).r[2]);
	if (FAILED(m_pShaderCom->Bind_RawValue("g_vLook", &vLook, sizeof(_float4))))
		return E_FAIL;

	return S_OK;
}

HRESULT CSpriteEffect_Emitter::Bind_ShaderValue_ByPass()
{

	switch (m_iShaderPass)
	{
	case DEFAULT:
	{
		if (FAILED(Bind_Texture(DIFFUSE, "g_DiffuseTexture")))
			return E_FAIL;
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;
		if (FAILED(Bind_Float("AlphaTest", "g_AlphaTest")))
			return E_FAIL;
		if (FAILED(Bind_Float("ColorTest", "g_RGBTest")))
			return E_FAIL;
		if (FAILED(Bind_Float4("ColorIntensity", "g_vColorIntensity")))
			return E_FAIL;
		break;
	}
	case BLOOM_DEFAULT:
	{
		if (FAILED(Bind_Texture(DIFFUSE, "g_DiffuseTexture")))
			return E_FAIL;
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;
		if (FAILED(Bind_Float("AlphaTest", "g_AlphaTest")))
			return E_FAIL;
		if (FAILED(Bind_Float("ColorTest", "g_RGBTest")))
			return E_FAIL;
		if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
			return E_FAIL;
		if (FAILED(Bind_Float4("ColorIntensity", "g_vColorIntensity")))
			return E_FAIL;
	}
	}

	return S_OK;
}

HRESULT CSpriteEffect_Emitter::Bind_Texture(TEXTURE_TYPE _eType, const _char* _szConstantName)
{
	if (nullptr != m_Textures[_eType] && _eType < TEXTURE_END)
	{
		return m_Textures[_eType]->Bind_ShaderResource(m_pShaderCom, _szConstantName);
	}
	else
		return E_FAIL;

	return S_OK;
}

HRESULT CSpriteEffect_Emitter::Bind_UV()
{
	_float4 vUV;
	_float2 vUVPerAnim = m_Float2Datas["UV_Per_Anim"];

	_uint iIndex = (_uint)(m_fAccTime / max(m_FloatDatas["UV_Factor"], 0.01f));

	_float fLeft = vUVPerAnim.x * iIndex;
	_float fTop = vUVPerAnim.y * (_int)(fLeft / 1.f);

	fLeft = fLeft - (_int)fLeft;
	
	vUV = _float4(fLeft, fTop, fLeft + vUVPerAnim.x, fTop + vUVPerAnim.y);

	return m_pShaderCom->Bind_RawValue("g_vUVLTRB", &vUV, sizeof(_float4));

}

HRESULT CSpriteEffect_Emitter::Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc)
{
	if (FAILED(__super::Ready_Components(_pDesc)))
		return E_FAIL;

	if (nullptr != m_pBufferCom)
	{
		m_Components.emplace(L"Com_Buffer", m_pBufferCom);
		Safe_AddRef(m_pBufferCom);
	}
	else
	{
		const SPRITE_EMITTER_DESC* pDesc = static_cast<const SPRITE_EMITTER_DESC*>(_pDesc);
		m_pBufferCom = dynamic_cast<CVIBuffer_Point*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::PROTO_COMPONENT, pDesc->iProtoRectLevel, pDesc->szRectTag, nullptr));
		if (nullptr == m_pBufferCom)
			return E_FAIL;

	}


	return S_OK;
}

HRESULT CSpriteEffect_Emitter::Load_TextureInfo(const json& _jsonInfo)
{
	if (_jsonInfo.contains("Textures"))
	{
		for (_int i = 0; i < _jsonInfo["Textures"].size(); ++i)
		{
			TEXTURE_TYPE eType = _jsonInfo["Textures"][i]["Type"];
			_string strPath = _jsonInfo["Textures"][i]["Path"];

			if (m_pGameInstance->Find_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strPath)))
			{
				m_Textures[eType] = static_cast<CTexture*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::PROTO_COMPONENT, m_pGameInstance->Get_StaticLevelID(),
					STRINGTOWSTRING(strPath), nullptr));
			}
			else
			{
				CTexture* pTexture = CTexture::Create(m_pDevice, m_pContext, strPath.c_str());
				if (nullptr != pTexture)
					m_Textures[eType] = pTexture;

				m_pGameInstance->Add_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strPath), pTexture);
			}



#ifdef _DEBUG
			m_Textures[eType]->Add_SRVName(STRINGTOWSTRING(strPath));
#endif
		}
	}

	return S_OK;
}

CSpriteEffect_Emitter* CSpriteEffect_Emitter::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonInfo)
{
	CSpriteEffect_Emitter* pInstance = new CSpriteEffect_Emitter(_pDevice, _pContext);

	if (FAILED(pInstance->Initialize_Prototype(_jsonInfo)))
	{
		MSG_BOX("Failed to Created : CSpriteEffect_Emitter");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CSpriteEffect_Emitter::Clone(void* _pArg)
{
	CSpriteEffect_Emitter* pInstance = new CSpriteEffect_Emitter(*this);

	if (FAILED(pInstance->Initialize(_pArg)))
	{
		MSG_BOX("Failed to Cloned : CSpriteEffect_Emitter");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CSpriteEffect_Emitter::Free()
{
	Safe_Release(m_pBufferCom);

	for (auto& pTexture : m_Textures)
		Safe_Release(pTexture);

	__super::Free();
}

HRESULT CSpriteEffect_Emitter::Cleanup_DeadReferences()
{
	return S_OK;
}

#ifdef _DEBUG
void CSpriteEffect_Emitter::Tool_Setting()
{
}

void CSpriteEffect_Emitter::Tool_Update(_float _fTimeDelta)
{
	ImGui::Begin("Adjust_Effect_Emitter");

	if (ImGui::TreeNode("Set Emitter State"))
	{
		__super::Tool_Update(_fTimeDelta);

		ImGui::TreePop();
	}

	Tool_SetEffect();


	ImGui::End();
}



HRESULT CSpriteEffect_Emitter::Save(json& _jsonOut)
{
	if (FAILED(__super::Save(_jsonOut)))
		return E_FAIL;

	for (_int i = 0; i < 4; ++i)
	{
		_jsonOut["Color"].push_back(*((_float*)(&m_vDefaultColor) + i));
	}

	EFFECT_SHADERPASS eShaderPass = (EFFECT_SHADERPASS)m_iShaderPass;
	_jsonOut["ShaderPass"] = eShaderPass;

	for (_int i = 0; i < TEXTURE_END; ++i)
	{
		if (nullptr != m_Textures[i])
		{
			json jsonInfo;
			jsonInfo["Path"] = WSTRINGTOSTRING(*m_Textures[i]->Get_SRVName(0)).c_str();
			jsonInfo["Type"] = (TEXTURE_TYPE)i;

			_jsonOut["Textures"].push_back(jsonInfo);
		}
	}


	return S_OK;
}
void CSpriteEffect_Emitter::Set_Texture(CTexture* _pTextureCom, _uint _iType)
{
	if (_iType >= m_Textures.size())
		return;

	if (nullptr != m_Textures[_iType])
	{
		Safe_Release(m_Textures[_iType]);
	}

	m_Textures[_iType] = _pTextureCom;
	Safe_AddRef(m_Textures[_iType]);

}

void CSpriteEffect_Emitter::Tool_SetEffect()
{

		if (ImGui::TreeNode("Color"))
		{
			ImGui::Text("Now Color : %.4f, %.4f, %.4f, %.4f", m_vColor.x, m_vColor.y, m_vColor.z, m_vColor.w);
			if (ImGui::DragFloat4("Default Color", (_float*)(&m_vDefaultColor), 0.01f))
			{
				m_vColor = m_vDefaultColor;
			}


			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Pass"))
		{

			const _char* items[] = { "Default", "Bloom"};
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

								CKeyframe_Module* pModule = CKeyframe_Module::Create(m_pDevice, m_pContext, (CKeyframe_Module::MODULE_NAME)n, 1);
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



		if (ImGui::TreeNode("Show Textures"))
		{
			if (ImGui::TreeNode("Diffuse"))
			{
				if (nullptr != m_Textures[DIFFUSE])
				{
					ImVec2 imageSize(300, 300); // 이미지 크기 설정
					ID3D11ShaderResourceView* pSelectImage = m_Textures[DIFFUSE]->Get_SRV(0);
					if (nullptr != pSelectImage)
					{
						ImGui::Image((ImTextureID)pSelectImage, imageSize);
					}

					ImGui::Text(WSTRINGTOSTRING(*m_Textures[DIFFUSE]->Get_SRVName(0)).c_str());

					if (ImGui::Button("Delete"))
					{
						Safe_Release(m_Textures[DIFFUSE]);
					}
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Normal"))
			{
				if (nullptr != m_Textures[NORMAL])
				{
					ImVec2 imageSize(300, 300); // 이미지 크기 설정
					ID3D11ShaderResourceView* pSelectImage = m_Textures[NORMAL]->Get_SRV(0);
					if (nullptr != pSelectImage)
					{
						ImGui::Image((ImTextureID)pSelectImage, imageSize);
					}

					ImGui::Text(WSTRINGTOSTRING(*m_Textures[NORMAL]->Get_SRVName(0)).c_str());

					if (ImGui::Button("Delete"))
					{
						Safe_Release(m_Textures[NORMAL]);
					}
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Noise"))
			{
				if (nullptr != m_Textures[NOISE])
				{
					ImVec2 imageSize(300, 300); // 이미지 크기 설정
					ID3D11ShaderResourceView* pSelectImage = m_Textures[NOISE]->Get_SRV(0);
					if (nullptr != pSelectImage)
					{
						ImGui::Image((ImTextureID)pSelectImage, imageSize);
					}

					ImGui::Text(WSTRINGTOSTRING(*m_Textures[NOISE]->Get_SRVName(0)).c_str());


					if (ImGui::Button("Delete"))
					{
						Safe_Release(m_Textures[NOISE]);
					}
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();

		}

}

CSpriteEffect_Emitter* CSpriteEffect_Emitter::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, void* _pArg)
{
	CSpriteEffect_Emitter* pInstance = new CSpriteEffect_Emitter(_pDevice, _pContext);

	if (FAILED(pInstance->Initialize(_pArg)))
	{
		MSG_BOX("Failed to Created : CSpriteEffect_Emitter");
		Safe_Release(pInstance);
	}
	return pInstance;
}
#endif