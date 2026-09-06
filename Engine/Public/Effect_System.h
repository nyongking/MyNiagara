#pragma once
#include "PartObject.h"
#include "Emitter.h"

BEGIN(Engine)


class ENGINE_DLL CEffect_System : public CPartObject
{
public:
	typedef struct tagParticleSystemDesc : public CPartObject::PARTOBJECT_DESC
	{
		_uint iSpriteShaderLevel = 0;
		const _tchar* szSpriteShaderTags = L"";
		_uint iModelShaderLevel = 0;
		const _tchar* szModelShaderTags = L"";
		_uint iEffectShaderLevel = 0;
		const _tchar* szEffectShaderTags = L"";
		_uint iSingleSpriteShaderLevel = 0;
		const _tchar* szSingleSpriteShaderTags = L"";
		_uint iSingleSpriteBufferLevel = 0;
		const _tchar* szSingleSpriteBufferTags = L"";
		const _tchar* szSpriteComputeShaderTag = L"";
		const _tchar* szMeshComputeShaderTag = L"";

		//vector<_uint> EmitterShaderLevels;
		//vector<const _tchar*> ShaderTags;
	} EFFECT_SYSTEM_DESC;

private:
	CEffect_System(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CEffect_System(const CEffect_System& _Prototype);
	virtual ~CEffect_System() = default;

public:
	HRESULT						Initialize_Prototype(const _tchar* _szFilePath);
	virtual	HRESULT				Initialize_Prototype() override;
	HRESULT						Initialize(void* _pArg, const CEffect_System* _pPrototype);
	virtual HRESULT				Initialize(void* _pArg) override;
	virtual void				Priority_Update(_float _fTimeDelta) override;
	virtual void				Update(_float _fTimeDelta) override;
	virtual void				Late_Update(_float _fTimeDelta) override;
	virtual HRESULT				Render() override;

public:
	void						Active_Effect(_bool _isReset = true, _uint _iEffectID = 0);
	void						Active_All(_bool _isReset = true);
	void						Stop_Spawn(_float _fDelayTime = 0.f, _uint _iEffectID = 0);
	void						Stop_SpawnAll(_float _fDelayTime = 0.f);
	void						Inactive_Effect(_uint _iEffectID = 0);
	void						Inactive_All();
	void						Reset_All();

public:
	void						Set_SpawnMatrix(const _float4x4* _pSpawnMatrix);
	void						Set_EffectPosition(_fvector _vPosition);
	void						Set_EffectMatrix(_matrix _WorldMatrix);

private:
	vector<class CEmitter*> m_Emitters;

	const _float4x4*			m_pSpawnMatrix = { nullptr };

public:
	static	CEffect_System* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const _tchar* _szFilePath);
	virtual CGameObject* Clone(void* _pArg) override;
	virtual void		 Free() override;
	virtual HRESULT		 Cleanup_DeadReferences() override;

#ifdef _DEBUG
public:
	HRESULT						Add_New_Emitter(CEmitter::EFFECT_TYPE _eType, void* _pArg);
	void						Tool_Update(_float _fTimeDelta);
	void						Tool_Reset(_uint _iEvent);
public:
	void						Set_Texture(class CTexture* _pTextureCom, _uint _iType = 0);

public:
	HRESULT						Save_File();

private:
	_string						m_strFilePath = "../Bin/DataFiles/Effects/Untitle.json";
	CEmitter*					m_pNowItem = { nullptr };

	_int						m_iInputNumInstances = { 0 };
	_uint						m_iToolEventID = { 0 };
	_float						m_fToolAccTime = { 0.f };
	_float						m_fToolRepeatTime = { 5.f };
	_float						m_fDebugTimeScale = { 1.f };
private:
	void						Tool_ShowList();
	void						Tool_InputText();
	void						Tool_Transform();
public:
	static	CEffect_System* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext); /* 툴전용의 파티클 생성 코드입니다, 어떤 Emitter도 만들지 않습니다.*/

#endif
};

END

BEGIN(Engine)
NLOHMANN_JSON_SERIALIZE_ENUM(CEmitter::EFFECT_TYPE, {
{CEmitter::EFFECT_TYPE::SPRITE, "SPRITE"},
{CEmitter::EFFECT_TYPE::SINGLE_SPRITE, "SINGLE_SPRITE"},
{CEmitter::EFFECT_TYPE::MESH, "MESH"},
{CEmitter::EFFECT_TYPE::EFFECT, "EFFECT"}
	});
END