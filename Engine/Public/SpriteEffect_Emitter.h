#pragma once
#include "Emitter.h"

BEGIN(Engine)

class CVIBuffer_Point;
class CTexture;
class CEffect_Module;

class ENGINE_DLL CSpriteEffect_Emitter : public CEmitter
{
public:
	typedef struct tagSpriteEmitterDesc : public CEmitter::PARTICLE_EMITTER_DESC
	{
		_uint iProtoRectLevel;
		const _tchar* szRectTag = L"";
	} SPRITE_EMITTER_DESC;

public:
	enum TEXTURE_TYPE { DIFFUSE, NORMAL, NOISE, TEXTURE_END };
	enum EFFECT_SHADERPASS { DEFAULT, BLOOM_DEFAULT, DISSOLVE, BLOOM_DISSOLVE };

private:
	CSpriteEffect_Emitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CSpriteEffect_Emitter(const CSpriteEffect_Emitter& _Prototype);
	virtual ~CSpriteEffect_Emitter() = default;

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
	vector<class CTexture*>	m_Textures;
	CVIBuffer_Point* m_pBufferCom = { nullptr };

private:
	_float4		m_vColor = { 1.f, 1.f, 1.f, 1.f };


private:
	virtual void					Update_Emitter(_float _fTimeDelta) override;


	HRESULT							Bind_ShaderResources();
	HRESULT							Bind_ShaderValue_ByPass();
	HRESULT							Bind_Texture(TEXTURE_TYPE _eType, const _char* _szConstantName);
	HRESULT							Bind_UV();
	virtual HRESULT					Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc) override;
	HRESULT							Load_TextureInfo(const json& _jsonInfo);

public:
	static	CSpriteEffect_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonInfo);

	virtual CGameObject* Clone(void* _pArg) override;
	virtual void		 Free() override;
	virtual HRESULT		 Cleanup_DeadReferences() override;

#ifdef _DEBUG

public:
	virtual void				Tool_Setting() override;
	virtual void				Tool_Update(_float _fTimeDelta) override;
	virtual HRESULT				Save(json& _jsonOut);

public:
	void						Set_Texture(class CTexture* _pTextureCom, _uint _iType);

private:
	_float4						m_vDefaultColor;

private:
	void						Tool_SetEffect();

public:
	static	CSpriteEffect_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, void* _pArg);
#endif
};

END

BEGIN(Engine)
NLOHMANN_JSON_SERIALIZE_ENUM(CSpriteEffect_Emitter::TEXTURE_TYPE, {
{CSpriteEffect_Emitter::TEXTURE_TYPE::DIFFUSE, "DIFFUSE"},
{CSpriteEffect_Emitter::TEXTURE_TYPE::NORMAL, "NORMAL"},
{CSpriteEffect_Emitter::TEXTURE_TYPE::NOISE, "NOISE"},
	});

NLOHMANN_JSON_SERIALIZE_ENUM(CSpriteEffect_Emitter::EFFECT_SHADERPASS, {
{CSpriteEffect_Emitter::EFFECT_SHADERPASS::DEFAULT, "DEFAULT"},
{CSpriteEffect_Emitter::EFFECT_SHADERPASS::BLOOM_DEFAULT, "BLOOM_DEFAULT"},
{CSpriteEffect_Emitter::EFFECT_SHADERPASS::DISSOLVE, "DEFAULT_DISSOLVE"},
{CSpriteEffect_Emitter::EFFECT_SHADERPASS::BLOOM_DISSOLVE, "BLOOM_DISSOLVE"},

	});
END