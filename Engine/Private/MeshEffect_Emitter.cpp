

#include "MeshEffect_Emitter.h"
#include "EffectModel.h"
#include "GameInstance.h"

#include "Effect_Module.h"
#include "Keyframe_Module.h"

CMeshEffect_Emitter::CMeshEffect_Emitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
	: CEmitter(_pDevice, _pContext)
{
	m_eEffectType = EFFECT;
	m_Textures.resize(TEXTURE_END, nullptr);
}

CMeshEffect_Emitter::CMeshEffect_Emitter(const CMeshEffect_Emitter& _Prototype)
	: CEmitter(_Prototype)
	, m_pEffectModelCom(_Prototype.m_pEffectModelCom)
	, m_Textures(_Prototype.m_Textures)
	, m_vColor(_Prototype.m_vColor)
	//, m_eShaderPass(_Prototype.m_eShaderPass)
#ifdef _DEBUG
	, m_strModelPath(_Prototype.m_strModelPath)
	, m_PreTransformMatrix(_Prototype.m_PreTransformMatrix)
	, m_vDefaultColor(_Prototype.m_vDefaultColor)
#endif
{
	Safe_AddRef(m_pEffectModelCom);
	
	for (auto& pTexture : m_Textures)
		Safe_AddRef(pTexture);

}

HRESULT CMeshEffect_Emitter::Initialize_Prototype(const json& _jsonInfo)
{
	if (FAILED(__super::Initialize_Prototype(_jsonInfo)))
		return E_FAIL;


	_float4x4 PreTransformMatrix;
	if (false == _jsonInfo.contains("PreTransform"))
	{
		XMStoreFloat4x4(&PreTransformMatrix, XMMatrixIdentity());
	}
	else
	{
		for (size_t i = 0; i < sizeof(_float4x4) / sizeof(_float); ++i)
		{
			*((_float*)(&PreTransformMatrix) + i) = _jsonInfo["PreTransform"][i];
		}
	}


	if (false == _jsonInfo.contains("Model"))
		return E_FAIL;
	_string strModelPath = _jsonInfo["Model"];
	m_pEffectModelCom = CEffectModel::Create(m_pDevice, m_pContext, strModelPath.c_str(), XMLoadFloat4x4(&PreTransformMatrix));

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
	m_strModelPath = strModelPath;
	m_PreTransformMatrix = PreTransformMatrix;
	m_vDefaultColor = m_vColor;
#endif
	return S_OK;
}

HRESULT CMeshEffect_Emitter::Initialize(void* _pArg)
{
	if (FAILED(__super::Initialize(_pArg)))
		return E_FAIL;

	return S_OK;
}

void CMeshEffect_Emitter::Priority_Update(_float _fTimeDelta)
{
}

void CMeshEffect_Emitter::Update(_float _fTimeDelta)
{
	__super::Update(_fTimeDelta);

}

void CMeshEffect_Emitter::Late_Update(_float _fTimeDelta)
{
	if (STOP_SPAWN == m_eNowEvent)
		Set_Active(false);

	if (FOLLOW_PARENT == m_eSpawnPosition)
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



	if (m_isActive && m_iAccLoop)
		m_pGameInstance->Add_RenderObject_New(s_iRG_3D, s_iRGP_EFFECT, this);
}

HRESULT CMeshEffect_Emitter::Render()
{
	if (nullptr == m_pShaderCom || nullptr == m_pEffectModelCom)
		return S_OK;

	if (FAILED(Bind_ShaderResources()))
		return E_FAIL;

	_uint iNumMeshes = m_pEffectModelCom->Get_NumMeshes();

	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		//m_pEffectModelCom->Bind_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE, 0)

		if (FAILED(m_pShaderCom->Begin(m_iShaderPass)))
			return E_FAIL;

		m_pEffectModelCom->Render(i);
	}


	return S_OK;
}

void CMeshEffect_Emitter::Reset()
{
	m_fAccTime = 0.f;

}


void CMeshEffect_Emitter::Update_Emitter(_float _fTimeDelta)
{
	if (STOP_SPAWN == m_eNowEvent)
		Set_Active(false);

	if (nullptr == m_pEffectModelCom)
		return;

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
				if (FOLLOW_PARENT != m_eSpawnPosition)
					pModule->Update_ScaleKeyframe(m_fAccTime, ((_float4*)(&m_WorldMatrices[COORDINATE_3D])), ((_float4*)(&m_WorldMatrices[COORDINATE_3D])+1), ((_float4*)(&m_WorldMatrices[COORDINATE_3D]) + 2));
				else
				{
					const _float4x4* pMatrix = m_pControllerTransform->Get_WorldMatrix_Ptr();
					pModule->Update_ScaleKeyframe(m_fAccTime, ((_float4*)(pMatrix)), ((_float4*)(pMatrix) + 1), ((_float4*)(pMatrix) + 2));
				}
			}
		}
		else if (CEffect_Module::MODULE_TYPE::MODULE_TRANSLATION == eType)
		{

		}
	}
}

HRESULT CMeshEffect_Emitter::Bind_ShaderResources()
{
	//if (RELATIVE_WORLD == m_eSpawnPosition)
	//{
		if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_WorldMatrices[COORDINATE_3D])))
			return E_FAIL;
	//}


	if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_pGameInstance->Get_TransformFloat4x4(CPipeLine::D3DTS_VIEW))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_pGameInstance->Get_TransformFloat4x4(CPipeLine::D3DTS_PROJ))))
		return E_FAIL;


	if (FAILED(Bind_ShaderValue_ByPass()))
		return E_FAIL;


	return S_OK;
}

HRESULT CMeshEffect_Emitter::Bind_ShaderValue_ByPass()
{

	switch (m_iShaderPass)
	{
	case DEFAULT:
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;
		break;
	case DEFAULT_DISSOLVE:
	{
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;
		if (FAILED(m_pShaderCom->Bind_RawValue("g_fTimeAcc", &m_fAccTime, sizeof(_float))))
			return E_FAIL;

		//if (FAILED(m_pShaderCom->Bind_RawValue("g_fTimeAcc", &m_fAccTime, sizeof(_float))))
		//	return E_FAIL;

		//Bind_Texture(ALPHA, "g_AlphaTexture");
		Bind_Texture(MASK, "g_MaskTexture");
		Bind_Texture(NOISE, "g_NoiseTexture");

		//if (FAILED(Bind_Float("AlphaValue", "g_fAlpha")))
		//	return E_FAIL;
		if (FAILED(Bind_Float("DissolveFactor", "g_fDissolveFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveTimeFactor", "g_fDissolveTimeFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveEdge", "g_fDissolveEdgeWidth")))
			return E_FAIL;
		if (FAILED(Bind_Float("AlphaTest", "g_fAlphaTest")))
			return E_FAIL;
		//if (FAILED(Bind_Float("ColorTest", "g_fColorTest")))
		//	return E_FAIL;

		if (FAILED(Bind_Float4("DissolveEdgeColor", "g_vEdgeColor")))
			return E_FAIL;
		//if (FAILED(Bind_Float4("AlphaUVScale", "g_AlphaUVScale")))
		//	return E_FAIL;
		if (FAILED(Bind_Float4("MaskUVScale", "g_MaskUVScale")))
			return E_FAIL;
		if (FAILED(Bind_Float4("NoiseUVScale", "g_NoiseUVScale")))
			return E_FAIL;


		break;
	}

	case BLOOM:
	{
		if (FAILED(m_pShaderCom->Bind_RawValue("g_fTimeAcc", &m_fAccTime, sizeof(_float))))
			return E_FAIL;
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;

		Bind_Texture(MASK, "g_MaskTexture");

		if (FAILED(Bind_Float("AlphaValue", "g_fAlpha")))
			return E_FAIL;
		if (FAILED(Bind_Float("AlphaTest", "g_fAlphaTest")))
			return E_FAIL;
		if (FAILED(Bind_Float("ColorTest", "g_fColorTest")))
			return E_FAIL;
		if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
			return E_FAIL;

		if (FAILED(Bind_Float4("AlphaUVScale", "g_AlphaUVScale")))
			return E_FAIL;
		if (FAILED(Bind_Float4("MaskUVScale", "g_MaskUVScale")))
			return E_FAIL;

		break;
	}
	case BLOOM_DISSOLVE:
	{
		if (FAILED(m_pShaderCom->Bind_RawValue("g_fTimeAcc", &m_fAccTime, sizeof(_float))))
			return E_FAIL;
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;

		//Bind_Texture(ALPHA, "g_AlphaTexture");
		Bind_Texture(MASK, "g_MaskTexture");
		Bind_Texture(NOISE, "g_NoiseTexture");


		//if (FAILED(Bind_Float("AlphaValue", "g_fAlpha")))
		//	return E_FAIL;
		if (FAILED(Bind_Float("DissolveFactor", "g_fDissolveFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveTimeFactor", "g_fDissolveTimeFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveEdge", "g_fDissolveEdgeWidth")))
			return E_FAIL;
		if (FAILED(Bind_Float("AlphaTest", "g_fAlphaTest")))
			return E_FAIL;
		//if (FAILED(Bind_Float("ColorTest", "g_fColorTest")))
		//	return E_FAIL;
		if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
			return E_FAIL;

		if (FAILED(Bind_Float4("DissolveEdgeColor", "g_vEdgeColor")))
			return E_FAIL;

		//if (FAILED(Bind_Float4("AlphaUVScale", "g_AlphaUVScale")))
		//	return E_FAIL;
		if (FAILED(Bind_Float4("MaskUVScale", "g_MaskUVScale")))
			return E_FAIL;
		if (FAILED(Bind_Float4("NoiseUVScale", "g_NoiseUVScale")))
			return E_FAIL;

		break;
	}
	case BLOOM_DISSOLVE_BILLBOARD:
	{
		_float4 vLook;
		XMStoreFloat4(&vLook, m_pGameInstance->Get_TransformInverseMatrix(CPipeLine::D3DTS_VIEW).r[2]);
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vLook", &vLook, sizeof(_float4))))
			return E_FAIL;

		if (FAILED(m_pShaderCom->Bind_RawValue("g_fTimeAcc", &m_fAccTime, sizeof(_float))))
			return E_FAIL;
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;

		//Bind_Texture(ALPHA, "g_AlphaTexture");
		Bind_Texture(MASK, "g_MaskTexture");
		Bind_Texture(NOISE, "g_NoiseTexture");

		//if (FAILED(Bind_Float("AlphaValue", "g_fAlpha")))
		//	return E_FAIL;
		if (FAILED(Bind_Float("DissolveFactor", "g_fDissolveFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveTimeFactor", "g_fDissolveTimeFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveEdge", "g_fDissolveEdgeWidth")))
			return E_FAIL;
		if (FAILED(Bind_Float("AlphaTest", "g_fAlphaTest")))
			return E_FAIL;
		//if (FAILED(Bind_Float("ColorTest", "g_fColorTest")))
		//	return E_FAIL;
		if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
			return E_FAIL;

		if (FAILED(Bind_Float4("DissolveEdgeColor", "g_vEdgeColor")))
			return E_FAIL;

		//if (FAILED(Bind_Float4("AlphaUVScale", "g_AlphaUVScale")))
		//	return E_FAIL;
		if (FAILED(Bind_Float4("MaskUVScale", "g_MaskUVScale")))
			return E_FAIL;
		if (FAILED(Bind_Float4("NoiseUVScale", "g_NoiseUVScale")))
			return E_FAIL;

		break;
	}
	case DISTORTION:
	{
		_float4 vLook;
		XMStoreFloat4(&vLook, m_pGameInstance->Get_TransformInverseMatrix(CPipeLine::D3DTS_VIEW).r[2]);
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vLook", &vLook, sizeof(_float4))))
			return E_FAIL;

		Bind_Texture(NOISE, "g_NoiseTexture");

		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;
		if (FAILED(Bind_Float4("NoiseUVScale", "g_NoiseUVScale")))
			return E_FAIL;
		break;
	}
	case SUB_DISSOLVE:
	{
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;


		if (FAILED(m_pShaderCom->Bind_RawValue("g_fTimeAcc", &m_fAccTime, sizeof(_float))))
			return E_FAIL;

		Bind_Texture(MASK, "g_MaskTexture");
		Bind_Texture(NOISE, "g_NoiseTexture");
		Bind_Texture(SECOND, "g_SecondTexture");

		if (FAILED(Bind_Float("DissolveFactor", "g_fDissolveFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveTimeFactor", "g_fDissolveTimeFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveEdge", "g_fDissolveEdgeWidth")))
			return E_FAIL;
		if (FAILED(Bind_Float("SubIntensity", "g_fSubIntensity")))
			return E_FAIL;
		if (FAILED(Bind_Float("AlphaTest", "g_fAlphaTest")))
			return E_FAIL;

		if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
			return E_FAIL;

		if (FAILED(Bind_Float4("DissolveEdgeColor", "g_vEdgeColor")))
			return E_FAIL;

		if (FAILED(Bind_Float4("MaskUVScale", "g_MaskUVScale")))
			return E_FAIL;
		if (FAILED(Bind_Float4("NoiseUVScale", "g_NoiseUVScale")))
			return E_FAIL;
		if (FAILED(Bind_Float4("SubUVScale", "g_SubUVScale")))
			return E_FAIL;

		break;
	}
	case SUBCOLOR_BLOOM_DISSOLVE:
	{
		if (FAILED(m_pShaderCom->Bind_RawValue("g_fTimeAcc", &m_fAccTime, sizeof(_float))))
			return E_FAIL;
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;

		//Bind_Texture(ALPHA, "g_AlphaTexture");
		Bind_Texture(MASK, "g_MaskTexture");
		Bind_Texture(NOISE, "g_NoiseTexture");


		//if (FAILED(Bind_Float("AlphaValue", "g_fAlpha")))
		//	return E_FAIL;
		if (FAILED(Bind_Float("DissolveFactor", "g_fDissolveFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveTimeFactor", "g_fDissolveTimeFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveEdge", "g_fDissolveEdgeWidth")))
			return E_FAIL;
		if (FAILED(Bind_Float("AlphaTest", "g_fAlphaTest")))
			return E_FAIL;
		//if (FAILED(Bind_Float("ColorTest", "g_fColorTest")))
		//	return E_FAIL;
		if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
			return E_FAIL;

		if (FAILED(Bind_Float4("DissolveEdgeColor", "g_vEdgeColor")))
			return E_FAIL;
		if (FAILED(Bind_Float4("SubColor", "g_vSubColor")))
			return E_FAIL;

		//if (FAILED(Bind_Float4("AlphaUVScale", "g_AlphaUVScale")))
		//	return E_FAIL;
		if (FAILED(Bind_Float4("MaskUVScale", "g_MaskUVScale")))
			return E_FAIL;
		if (FAILED(Bind_Float4("NoiseUVScale", "g_NoiseUVScale")))
			return E_FAIL;

		break;
	}
	case SUBCOLOR_BLOOMDISSOLVE_BILLBOARD:
	{
		_float4 vLook;
		XMStoreFloat4(&vLook, m_pGameInstance->Get_TransformInverseMatrix(CPipeLine::D3DTS_VIEW).r[2]);
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vLook", &vLook, sizeof(_float4))))
			return E_FAIL;

		if (FAILED(m_pShaderCom->Bind_RawValue("g_fTimeAcc", &m_fAccTime, sizeof(_float))))
			return E_FAIL;
		if (FAILED(m_pShaderCom->Bind_RawValue("g_vColor", &m_vColor, sizeof(_float4))))
			return E_FAIL;

		//Bind_Texture(ALPHA, "g_AlphaTexture");
		Bind_Texture(MASK, "g_MaskTexture");
		Bind_Texture(NOISE, "g_NoiseTexture");


		//if (FAILED(Bind_Float("AlphaValue", "g_fAlpha")))
		//	return E_FAIL;
		if (FAILED(Bind_Float("DissolveFactor", "g_fDissolveFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveTimeFactor", "g_fDissolveTimeFactor")))
			return E_FAIL;
		if (FAILED(Bind_Float("DissolveEdge", "g_fDissolveEdgeWidth")))
			return E_FAIL;
		if (FAILED(Bind_Float("AlphaTest", "g_fAlphaTest")))
			return E_FAIL;
		//if (FAILED(Bind_Float("ColorTest", "g_fColorTest")))
		//	return E_FAIL;
		if (FAILED(Bind_Float("BloomThreshold", "g_fBloomThreshold")))
			return E_FAIL;

		if (FAILED(Bind_Float4("DissolveEdgeColor", "g_vEdgeColor")))
			return E_FAIL;
		if (FAILED(Bind_Float4("SubColor", "g_vSubColor")))
			return E_FAIL;

		//if (FAILED(Bind_Float4("AlphaUVScale", "g_AlphaUVScale")))
		//	return E_FAIL;
		if (FAILED(Bind_Float4("MaskUVScale", "g_MaskUVScale")))
			return E_FAIL;
		if (FAILED(Bind_Float4("NoiseUVScale", "g_NoiseUVScale")))
			return E_FAIL;
	}

	}


	return S_OK;
}

HRESULT CMeshEffect_Emitter::Bind_Texture(TEXTURE_TYPE _eType, const _char* _szConstantName)
{
	if (nullptr != m_Textures[_eType] && _eType < TEXTURE_END)
	{
		return m_Textures[_eType]->Bind_ShaderResource(m_pShaderCom, _szConstantName);
	}
	else
		return E_FAIL;

	return S_OK;
}


HRESULT CMeshEffect_Emitter::Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc)
{
	if (FAILED(__super::Ready_Components(_pDesc)))
		return E_FAIL;

	if (nullptr != m_pEffectModelCom)
	{
		m_Components.emplace(L"Com_Effect", m_pEffectModelCom);
		Safe_AddRef(m_pEffectModelCom);
	}

	return S_OK;
}

HRESULT CMeshEffect_Emitter::Load_TextureInfo(const json& _jsonInfo)
{
	if (_jsonInfo.contains("Textures"))
	{
		for (_int i = 0; i < _jsonInfo["Textures"].size(); ++i)
		{
			TEXTURE_TYPE eType = _jsonInfo["Textures"][i]["Type"];
			_string strPath = _jsonInfo["Textures"][i]["Path"];

			if (nullptr != m_pGameInstance->Find_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strPath)))
			{
			}
			else
			{
				m_pGameInstance->Add_Prototype(m_pGameInstance->Get_StaticLevelID(), STRINGTOWSTRING(strPath), CTexture::Create(m_pDevice, m_pContext, strPath.c_str()));
			}

			Safe_Release(m_Textures[eType]);

			m_Textures[eType] = static_cast<CTexture*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::PROTO_COMPONENT, m_pGameInstance->Get_StaticLevelID(),
				STRINGTOWSTRING(strPath), nullptr));
			#ifdef _DEBUG
						m_Textures[eType]->Add_SRVName(STRINGTOWSTRING(strPath));
			#endif
		}
	}

	return S_OK;
}



CMeshEffect_Emitter* CMeshEffect_Emitter::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonInfo)
{
	CMeshEffect_Emitter* pInstance = new CMeshEffect_Emitter(_pDevice, _pContext);

	if (FAILED(pInstance->Initialize_Prototype(_jsonInfo)))
	{
		MSG_BOX("Failed to Created : CMeshEffect_Emitter");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CMeshEffect_Emitter::Clone(void* _pArg)
{
	CMeshEffect_Emitter* pInstance = new CMeshEffect_Emitter(*this);

	if (FAILED(pInstance->Initialize(_pArg)))
	{
		MSG_BOX("Failed to Cloned : CMeshEffect_Emitter");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMeshEffect_Emitter::Free()
{
	Safe_Release(m_pEffectModelCom);

	for (auto& pTexture : m_Textures)
		Safe_Release(pTexture);


	__super::Free();
}

HRESULT CMeshEffect_Emitter::Cleanup_DeadReferences()
{
	return S_OK;
}

#ifdef _DEBUG

void CMeshEffect_Emitter::Tool_Setting()
{
}

void CMeshEffect_Emitter::Tool_Update(_float _fTimeDelta)
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

HRESULT CMeshEffect_Emitter::Save(json& _jsonOut)
{
	if (nullptr == m_pEffectModelCom)
	{
		MSG_BOX("Texture 혹은 Buffer 없음");
		return E_FAIL;
	}

	if (FAILED(__super::Save(_jsonOut)))
		return E_FAIL;

	_jsonOut["Model"] = m_strModelPath.c_str();
	for (_int i = 0; i < 16; ++i)
	{
		_jsonOut["PreTransform"].push_back(*((_float*)(&m_PreTransformMatrix) + i));
	}

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

void CMeshEffect_Emitter::Set_Texture(CTexture* _pTextureCom, _uint _iType)
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
void CMeshEffect_Emitter::Tool_SetEffect()
{
	if (m_pEffectModelCom)
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

			const _char* items[] = { "Default", "Dissolve", "Bloom", "Bloom_Dissolve", "Bloom_Dissolve_BillBoard", "Distortion", 
				"Sub_Dissolve", "SubColor_Bloom_Dissolve", "SubColor_Bloom_Billboard"};
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
					//const _char* Moduleitems[] = { "COLOR_KEYFRAME", "SCALE_KEYFRAME"};
					//const _char* Moduleitems[] = CKeyframe_Module::g_szModuleNames[2];
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
			//if (ImGui::TreeNode("Alpha"))
			//{
			//	if (nullptr != m_Textures[ALPHA])
			//	{
			//		ImVec2 imageSize(300, 300); // 이미지 크기 설정
			//		ID3D11ShaderResourceView* pSelectImage = m_Textures[ALPHA]->Get_SRV(0);
			//		if (nullptr != pSelectImage)
			//		{
			//			ImGui::Image((ImTextureID)pSelectImage, imageSize);
			//		}
			//		
			//		ImGui::Text(WSTRINGTOSTRING(*m_Textures[ALPHA]->Get_SRVName(0)).c_str());

			//		if (ImGui::Button("Delete"))
			//		{
			//			Safe_Release(m_Textures[ALPHA]);
			//		}
			//	}
			//	ImGui::TreePop();
			//}

			if (ImGui::TreeNode("Mask"))
			{
				if (nullptr != m_Textures[MASK])
				{
					ImVec2 imageSize(300, 300); // 이미지 크기 설정
					ID3D11ShaderResourceView* pSelectImage = m_Textures[MASK]->Get_SRV(0);
					if (nullptr != pSelectImage)
					{
						ImGui::Image((ImTextureID)pSelectImage, imageSize);
					}

					ImGui::Text(WSTRINGTOSTRING(*m_Textures[MASK]->Get_SRVName(0)).c_str());

					if (ImGui::Button("Delete"))
					{
						Safe_Release(m_Textures[MASK]);
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

			if (ImGui::TreeNode("Sub_Texture"))
			{
				if (nullptr != m_Textures[SECOND])
				{
					ImVec2 imageSize(300, 300); // 이미지 크기 설정
					ID3D11ShaderResourceView* pSelectImage = m_Textures[SECOND]->Get_SRV(0);
					if (nullptr != pSelectImage)
					{
						ImGui::Image((ImTextureID)pSelectImage, imageSize);
					}

					ImGui::Text(WSTRINGTOSTRING(*m_Textures[SECOND]->Get_SRVName(0)).c_str());


					if (ImGui::Button("Delete"))
					{
						Safe_Release(m_Textures[SECOND]);
					}
				}
				ImGui::TreePop();
			}

			

			ImGui::TreePop();

		}
	}
	else
	{
		static _char szInputPath[MAX_PATH] = "../Bin/Resources/Models/FX/";
		ImGui::InputText("Import Model Path", szInputPath, MAX_PATH);
		if (ImGui::Button("Create_Buffer"))
		{
			_matrix matPretransform = XMMatrixScaling(1 / 150.0f, 1 / 150.0f, 1 / 150.0f);

			m_pEffectModelCom = CEffectModel::Create(m_pDevice, m_pContext, szInputPath, matPretransform);

			XMStoreFloat4x4(&m_PreTransformMatrix, matPretransform);

			if (nullptr != m_pEffectModelCom)
			{
				m_Components.emplace(L"Com_Effect", m_pEffectModelCom);
				m_strModelPath = szInputPath;
			}
		}
	}
}
CMeshEffect_Emitter* CMeshEffect_Emitter::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, void* _pArg)
{
	CMeshEffect_Emitter* pInstance = new CMeshEffect_Emitter(_pDevice, _pContext);

	if (FAILED(pInstance->Initialize(_pArg)))
	{
		MSG_BOX("Failed to Created : CMeshEffect_Emitter");
		Safe_Release(pInstance);
	}
	return pInstance;
}
#endif