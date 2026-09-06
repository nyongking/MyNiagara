#pragma once
#include "Emitter.h"


BEGIN(Engine)

class CVIBuffer_Mesh_Particle;
class CMaterial;
class CBone;

class ENGINE_DLL CParticle_Mesh_Emitter : public CEmitter
{
public:
	enum EFFECT_SHADERPASS {DEFAULT};
private:
	CParticle_Mesh_Emitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CParticle_Mesh_Emitter(const CParticle_Mesh_Emitter& _Prototype);
	virtual ~CParticle_Mesh_Emitter() = default;

public:
	HRESULT						Initialize_Prototype(const json& _jsonInfo);
	virtual HRESULT				Initialize(void* _pArg) override;
	virtual void				Priority_Update(_float _fTimeDelta) override;
	virtual void				Update(_float _fTimeDelta) override;
	virtual void				Late_Update(_float _fTimeDelta) override;
	virtual HRESULT				Render() override;

public:
	virtual	void				Reset() override;

private:
	_uint									m_iNumMeshes = 0;
	vector<CVIBuffer_Mesh_Particle*>		m_ParticleMeshes;

	_uint				m_iNumMaterials = 0;
	vector<CMaterial*>	m_Materials;

	vector<CBone*>		m_Bones;
	_float4x4			m_PreTransformMatrix = {};


private:
	virtual void					Update_Emitter(_float _fTimeDelta) override;

	HRESULT					Bind_ShaderResources();
	HRESULT					Bind_Material(class CShader* _pShader, const _char* _pConstantName, _uint _iMeshIndex, aiTextureType _eTextureType, _uint _iTextureIndex = 0);

	virtual HRESULT					Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc) override;

private:
	HRESULT Ready_Bones(ifstream& _inFile, _uint _iParentBoneIndex);
	HRESULT Ready_Meshes(ifstream& _inFile, const json& _jsonBufferInfo);
	HRESULT Ready_Materials(ifstream& _inFile, const _char*_szpModelFilePath);




public:
	//static	CParticle_Mesh_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const _char* _szModelPath, const _tchar* _szInfoPath);
	static	CParticle_Mesh_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonInfo);

	virtual CGameObject* Clone(void* _pArg) override;
	virtual void		 Free() override;
	virtual HRESULT		 Cleanup_DeadReferences() override;

#ifdef _DEBUG
public:
	virtual void				Tool_Setting() override;
	virtual void				Tool_Update(_float _fTimeDelta) override;
	virtual HRESULT				Save(json& _jsonOut);
	
private:
	_string						m_strModelPath;



	HRESULT Ready_Meshes(ifstream& _inFile, _uint _iNumInstance);


public:
	static	CParticle_Mesh_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, void* _pArg);

#endif

};

END


BEGIN(Engine)
NLOHMANN_JSON_SERIALIZE_ENUM(CParticle_Mesh_Emitter::EFFECT_SHADERPASS, {
{CParticle_Mesh_Emitter::EFFECT_SHADERPASS::DEFAULT, "DEFAULT"},


	});
END