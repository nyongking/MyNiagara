#include "EffectModel.h"
#include "Mesh.h"
#include "Material.h"
#include "Shader.h"
#include "Bone.h"

#include "iostream"


CEffectModel::CEffectModel(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
	: CComponent(_pDevice, _pContext)
{
}

CEffectModel::CEffectModel(const CEffectModel& _Prototype)
	: CComponent(_Prototype)
	, m_iNumMeshes(_Prototype.m_iNumMeshes)
	, m_iNumMaterials{ _Prototype.m_iNumMaterials }
	, m_PreTransformMatrix{ _Prototype.m_PreTransformMatrix }
	, m_Meshes(_Prototype.m_Meshes)
	, m_Materials{ _Prototype.m_Materials }
{
	for (auto& pPrototypeBone : _Prototype.m_Bones)
	{
		m_Bones.push_back(pPrototypeBone->Clone());
	}

	for (auto& pMaterial : m_Materials)
		Safe_AddRef(pMaterial);

	for (auto& pMesh : m_Meshes)
		Safe_AddRef(pMesh);
}

HRESULT CEffectModel::Initialize_Prototype(const _char* pModelFilePath, _fmatrix PreTransformMatrix)
{
	XMStoreFloat4x4(&m_PreTransformMatrix, PreTransformMatrix);

	std::ifstream inFile(pModelFilePath, std::ios::binary);
	if (!inFile) {
		string str = "파일을 열 수 없습니다.";
		str += pModelFilePath;
		MessageBoxA(NULL, str.c_str(), "에러", MB_OK);
		return E_FAIL;
	}

	bool bAnim;
	inFile.read(reinterpret_cast<char*>(&bAnim), 1);

	if (FAILED(Ready_Bones(inFile, -1)))
		return E_FAIL;

	if (FAILED(Ready_Meshes(inFile)))
		return E_FAIL;

	if (FAILED(Ready_Materials(inFile, pModelFilePath)))
		return E_FAIL;

	inFile.close();


	return S_OK;
}

HRESULT CEffectModel::Initialize(void* _pArg)
{
	return S_OK;
}

HRESULT CEffectModel::Render(_uint iMeshIndex)
{
	/* Mesh 단위 렌더. */
	if (iMeshIndex >= m_iNumMeshes)
		return E_FAIL;

	if (FAILED(m_Meshes[iMeshIndex]->Bind_BufferDesc()))
		return E_FAIL;
	if (FAILED(m_Meshes[iMeshIndex]->Render()))
		return E_FAIL;

	return S_OK;
}



HRESULT CEffectModel::Bind_Material(CShader* _pShader, const _char* _pConstantName, _uint _iMeshIndex, aiTextureType _eTextureType, _uint _iTextureIndex)
{
	if (nullptr == _pShader)
		return E_FAIL;

	_uint iMaterialIndex = m_Meshes[_iMeshIndex]->Get_MaterialIndex();

	ID3D11ShaderResourceView* pSRV = m_Materials[iMaterialIndex]->Find_Texture(_eTextureType, _iTextureIndex);
	if (nullptr == pSRV)
		return E_FAIL;

	return _pShader->Bind_SRV(_pConstantName, pSRV);
}

_uint CEffectModel::Get_MeshIndex(const _char* _szName) const
{
	_uint	 iMeshIndex = { 0 };

	auto	iter = find_if(m_Meshes.begin(), m_Meshes.end(), [&](CMesh* pMesh)->_bool
		{
			if (false == strcmp(pMesh->Get_Name(), _szName))
				return true;

			++iMeshIndex;

			return false;
		});

	if (iter == m_Meshes.end())
		MSG_BOX("그런 메쉬가 없어");


	return iMeshIndex;
}

HRESULT CEffectModel::Ready_Bones(ifstream& inFile, _uint iParentBoneIndex)
{
	CBone* pBone = CBone::Create(inFile, iParentBoneIndex);;

	if (nullptr == pBone)
		return E_FAIL;

	m_Bones.push_back(pBone);

	iParentBoneIndex = (_uint)m_Bones.size() - 1;
	_uint iNumChildren = 0;
	inFile.read(reinterpret_cast<char*>(&iNumChildren), sizeof(_uint));
	for (_uint i = 0; i < iNumChildren; ++i)
	{
		Ready_Bones(inFile, iParentBoneIndex);
	}

	return S_OK;
}

HRESULT CEffectModel::Ready_Meshes(ifstream& inFile)
{
	inFile.read(reinterpret_cast<char*>(&m_iNumMeshes), sizeof(_uint));

	for (_uint i = 0; i < m_iNumMeshes; i++)
	{
		CMesh* pMesh = CMesh::Create(m_pDevice, m_pContext, inFile, XMLoadFloat4x4(&m_PreTransformMatrix));
		if (nullptr == pMesh)
			return E_FAIL;

		m_Meshes.push_back(pMesh);
	}

	return S_OK;
}

HRESULT CEffectModel::Ready_Materials(ifstream& inFile, const _char* pModelFilePath)
{
	inFile.read(reinterpret_cast<char*>(&m_iNumMaterials), sizeof(_uint));
	//cout << m_iNumMaterials << endl;
	m_Materials.resize(m_iNumMaterials);
	_char		szDrive[MAX_PATH] = "";
	_char		szDirectory[MAX_PATH] = "";
	_splitpath_s(pModelFilePath, szDrive, MAX_PATH, szDirectory, MAX_PATH, nullptr, 0, nullptr, 0);
	strcat_s(szDrive, szDirectory);
	for (_uint i = 0; i < m_iNumMaterials; i++)
	{
		CMaterial* pMaterial = CMaterial::Create(m_pDevice, m_pContext, szDrive, inFile);
		m_Materials[i] = pMaterial;
	}

	return S_OK;
}

CEffectModel* CEffectModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _char* pModelFilePath, _fmatrix PreTransformMatrix)
{
	CEffectModel* pInstance = new CEffectModel(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(pModelFilePath, PreTransformMatrix)))
	{
		MSG_BOX("Failed to Created : CEffectModel");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CEffectModel::Clone(void* _pArg)
{
	CEffectModel* pInstance = new CEffectModel(*this);

	if (FAILED(pInstance->Initialize(_pArg)))
	{
		MSG_BOX("Failed to Cloned : CEffectModel");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CEffectModel::Free()
{
	for (auto& pBone : m_Bones)
		Safe_Release(pBone);
	m_Bones.clear();

	for (auto& pMaterial : m_Materials)
		Safe_Release(pMaterial);
	m_Materials.clear();

	for (auto& pMesh : m_Meshes)
		Safe_Release(pMesh);
	m_Meshes.clear();

	__super::Free();
}
