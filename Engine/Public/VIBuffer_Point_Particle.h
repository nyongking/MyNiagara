#pragma once
#include "VIBuffer_Instance.h"

BEGIN(Engine)

class ENGINE_DLL CVIBuffer_Point_Particle : public CVIBuffer_Instance
{
public:
	enum EFFECT_SHADERPASS { DEFAULT_BILLBOARD, ROTATION_BILLBOARD, VELOCITY_BILLBOARD, };

private:
	CVIBuffer_Point_Particle(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CVIBuffer_Point_Particle(const CVIBuffer_Point_Particle& _Prototype);
	virtual ~CVIBuffer_Point_Particle() = default;

public:
	HRESULT Initialize_Prototype(const json& _jsonBufferInfo, _float _fSpawnRate);
	virtual HRESULT	Initialize_Module(class CEffect_Module* _pModule) override;
	virtual HRESULT Initialize_Buffers() override;

	virtual HRESULT Initialize(void* _pArg) override;
	virtual void	Begin_Update(_float _fTimeDelta) override;
	virtual void	Spawn_Burst(_float _fTimeDelta, const _float4x4* _pSpawnMatrix);
	virtual void	Spawn_Rate(_float _fTimeDelta, _float _fSpawnRate, const _float4x4* _pSpawnMatrix);
	virtual void	Update_Buffer(_float _fTimeDelta, _bool _isPooling) override;
	virtual void	End_Update(_float _fTimeDelta) override;
	void			Update_CPU(_float _fTimeDelta, _float _fSpawnRate, _bool _isPooling);
	void			Update_Module_CPU(class CEffect_Module* _pModule, _float _fTimeDelta);
	void			Reset_CPU_Buffers();
	//virtual void	Reset_Buffers(class CCompute_Shader* _pCShader) override;			// Buffer을 처음 시점으로 바꾼다.. 

	virtual	void	Update_Translation(_float _fTimeDelta, class CEffect_Module* _pTranslationModule) override;
	virtual void	Update_ColorKeyframe(class CEffect_Module* _pColorModule) override;
	virtual void	Update_ScaleKeyframe(class CEffect_Module* _pColorModule) override;

public:
	virtual HRESULT Bind_BufferBySRV() override;
	virtual HRESULT Render_BySRV() override;

private:
	VTXPOINTINSTANCE* m_pInstanceVertices = { nullptr };
	VTXPOINTINSTANCE* m_pUpdateVertices = { nullptr };
	_float			  m_fCPUSpawnRate = { 1.f };

	// UV 정보
	// 모든 Atlas는 위에서 아래, 좌에서 우로 향한다는 가정.
	// 모든 Texture의 크기는 같다는 가정
	_float			  m_fUVCount = { 1.f };
	_float2			  m_vUVPerAnim = { 1.f, 1.f };



private:
	void			  Set_UV(_Out_ _float4* _pOutUV, _float _fUVCount);
	void			  Set_Position(_int _iIndex);
	void			  Set_Instance(_int _iIndex, _float _fSpawnRate);
	void			  Apply_SpawnMatrix(VTXPOINTINSTANCE& _Particle, const _float4x4* _pSpawnMatrix);


public:
	static CVIBuffer_Point_Particle* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonBufferInfo, _float _fSpawnRate);
	virtual CComponent* Clone(void* _pArg);
	virtual void Free() override;

#ifdef _DEBUG
public:
	virtual void		 Tool_Setting() override;
	virtual void		 Tool_Reset_Buffers(_float _fSpawnRate, CCompute_Shader* _pCShader, vector<class CEffect_Module*>& _Modules);

	virtual void		 Tool_Update(_float _fTimeDelta) override;
	virtual HRESULT		 Save_Buffer(json& _jsonBufferInfo) override;
	

public:
	HRESULT Initialize_Prototype(_uint _iNumInstances, _float _fSpawnRate);
public:
	static CVIBuffer_Point_Particle* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, _uint _iNumInstances, _float _fSpawnRate);

#endif

};
END
