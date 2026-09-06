#pragma once
#include "VIBuffer_Instance.h"

BEGIN(Engine)

class ENGINE_DLL CVIBuffer_Mesh_Particle : public CVIBuffer_Instance
{
private:
	class CVIBuffer_Mesh_Particle(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	class CVIBuffer_Mesh_Particle(const CVIBuffer_Mesh_Particle& _Prototype);
	virtual ~CVIBuffer_Mesh_Particle() = default;

public:
	HRESULT Initialize_Prototype(ifstream& _inFile, const json& _jsonBufferInfo, _fmatrix _PreTransformMatrix, _float _fSpawnRate);
	virtual HRESULT	Initialize_Module(class CEffect_Module* _pModule) override;
	virtual HRESULT Initialize_Buffers() override;

	virtual HRESULT Initialize(void* _pArg) override;
	virtual void	Begin_Update(_float _fTimeDelta) override;
	virtual void	Spawn_Burst(_float _fTimeDelta, const _float4x4* _pSpawnMatrix) override;
	virtual void	Spawn_Rate(_float _fTimeDelta, _float _fSpawnRate, const _float4x4* _pSpawnMatrix) override;
	virtual void	Update_Buffer(_float _fTimeDelta, _bool _isPooling) override;
	virtual void	End_Update(_float _fTimeDelta) override;
	//virtual void	Reset_Buffers(class CCompute_Shader* _pCShader) override;

	virtual	void	Update_Translation(_float _fTimeDelta, class CEffect_Module* _pTranslationModule) override;
	virtual void	Update_ColorKeyframe(class CEffect_Module* _pColorModule) override;
	virtual void	Update_ScaleKeyframe(class CEffect_Module* _pColorModule) override;

public:
	virtual HRESULT Bind_BufferBySRV() override;
	virtual HRESULT Render_BySRV() override;


public:
	// Get
	_char* Get_Name() { return m_szName; }
	_uint Get_MaterialIndex() const { return m_iMaterialIndex; }

	// Set
	void Set_Name(string _strName) { strcpy_s(m_szName, _strName.c_str()); }

private:
	_float4x4			 m_PreTransformMatrix = {};

private:
	/* Mesh Data */
	_char m_szName[MAX_PATH] = ""; // Mesh의 이름은 추후 중요하게 사용 된다.
	_uint m_iMaterialIndex = 0; // 내가 렌더링에 사용하고자하는 Material Index;


private:
	VTXMESHINSTANCE* m_pInstanceVertices = { nullptr };
	VTXMESHINSTANCE* m_pUpdateVertices = { nullptr };

private:
	HRESULT Ready_VertexBuffer(ifstream& _inFile, _fmatrix _PreTransformMatrix);
	void			  Set_Instance(_int _iIndex, _float _fSpawnRate);
	void			  Set_Position(_int _iIndex);

public:
	static CVIBuffer_Mesh_Particle* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, ifstream& _inFile, const json& _jsonBufferInfo, _fmatrix _PreTransformMatrix, _float _fSpawnRate);
	virtual CComponent* Clone(void* _pArg);
	virtual void Free() override;

#ifdef _DEBUG
public:
	HRESULT Initialize_Prototype(ifstream& _inFile, _uint _iNumInstances, _fmatrix _PreTransformMatrix, _float _fSpawnRate);

public:
	virtual void		 Tool_Setting() override;
	void				 Tool_Reset_Buffers(_float _fSpawnRate, CCompute_Shader* _pCShader, vector<class CEffect_Module*>& _Modules);

	virtual void		 Tool_Update(_float _fTimeDelta) override;
	virtual HRESULT		 Save_Buffer(json& _jsonBufferInfo) override;

public:
	static CVIBuffer_Mesh_Particle* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, ifstream& _inFile, _uint _iNumInstances, _fmatrix _PreTransformMatrix, _float _fSpawnRate);


private:
	_float2* m_pTexcoords = { nullptr };
	_float3* m_pTangents = { nullptr };
	_uint	 m_iNumFaces = { 0 };
#endif

};

END