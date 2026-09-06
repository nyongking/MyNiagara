#pragma once
#include "Component.h"

BEGIN(Engine)
class CMesh;
class CBone;
class CMaterial;

class ENGINE_DLL CEffectModel : public CComponent
{
private:
	CEffectModel(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CEffectModel(const CEffectModel& _Prototype);
	virtual ~CEffectModel() = default;

public:
	HRESULT					Initialize_Prototype(const _char* pModelFilePath, _fmatrix PreTransformMatrix);
	virtual HRESULT			Initialize(void* _pArg) override;
	virtual HRESULT			Render(_uint iMeshIndex);

public:
	HRESULT					Bind_Material(class CShader* _pShader, const _char* _pConstantName, _uint _iMeshIndex, aiTextureType _eTextureType, _uint _iTextureIndex = 0);

public:
	// Get
	_uint					Get_NumMeshes() const { return m_iNumMeshes; }
	CMesh*					Get_Mesh(_uint iMeshIndex) const { return m_Meshes[iMeshIndex]; }
	_uint					Get_MeshIndex(const _char* _szName) const;


private:
	_uint						m_iNumMeshes = 0;
	vector<CMesh*>				m_Meshes;

	_uint						m_iNumMaterials = 0;
	vector<CMaterial*>			m_Materials;
	vector<CBone*>				m_Bones;
	_float4x4					m_PreTransformMatrix = {};

private:
	HRESULT Ready_Bones(ifstream& inFile, _uint iParentBoneIndex);
	HRESULT Ready_Meshes(ifstream& inFile);
	HRESULT Ready_Materials(ifstream& inFile, const _char* pModelFilePath);

public:
	static CEffectModel* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _char* pModelFilePath, _fmatrix PreTransformMatrix);
	virtual CComponent* Clone(void* _pArg) override;
	virtual void Free() override;

};

END