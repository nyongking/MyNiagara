#pragma once
#include "Emitter.h"

BEGIN(Engine)

class CEffectModel;
class CTexture;
class CEffect_Module;

class ENGINE_DLL CMeshEffect_Emitter : public CEmitter
{
public:
	enum TEXTURE_TYPE { MASK, NOISE, SECOND, TEXTURE_END };
	enum EFFECT_SHADERPASS { DEFAULT, DEFAULT_DISSOLVE, BLOOM, BLOOM_DISSOLVE, BLOOM_DISSOLVE_BILLBOARD, DISTORTION, 
		SUB_DISSOLVE, SUBCOLOR_BLOOM_DISSOLVE, SUBCOLOR_BLOOMDISSOLVE_BILLBOARD };

private:
	CMeshEffect_Emitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CMeshEffect_Emitter(const CMeshEffect_Emitter& _Prototype);
	virtual ~CMeshEffect_Emitter() = default;

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
	CEffectModel* m_pEffectModelCom = { nullptr };
	vector<class CTexture*>	m_Textures;
	

private:
	_float4		m_vColor = {1.f, 1.f, 1.f, 1.f};


private:
	virtual void					Update_Emitter(_float _fTimeDelta) override;


	HRESULT							Bind_ShaderResources();
	HRESULT							Bind_ShaderValue_ByPass();
	HRESULT							Bind_Texture(TEXTURE_TYPE _eType, const _char* _szConstantName);
	virtual HRESULT					Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc) override;
	HRESULT							Load_TextureInfo(const json& _jsonInfo);

public:
	static	CMeshEffect_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonInfo);

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
	_float4	m_vDefaultColor = {};
	_string m_strModelPath;
	_float4x4 m_PreTransformMatrix = {};

private:
	void						Tool_SetEffect();
public:
// DEBUG용 처음 만든 Mesh
	static	CMeshEffect_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, void* _pArg);
#endif
};

END

BEGIN(Engine)
NLOHMANN_JSON_SERIALIZE_ENUM(CMeshEffect_Emitter::TEXTURE_TYPE, {
{CMeshEffect_Emitter::TEXTURE_TYPE::MASK, "MASK"},
{CMeshEffect_Emitter::TEXTURE_TYPE::NOISE, "NOISE"},
{CMeshEffect_Emitter::TEXTURE_TYPE::SECOND, "SECOND"},
	});

NLOHMANN_JSON_SERIALIZE_ENUM(CMeshEffect_Emitter::EFFECT_SHADERPASS, {
{CMeshEffect_Emitter::EFFECT_SHADERPASS::DEFAULT, "DEFAULT"},
{CMeshEffect_Emitter::EFFECT_SHADERPASS::DEFAULT_DISSOLVE, "DEFAULT_DISSOLVE"},
{CMeshEffect_Emitter::EFFECT_SHADERPASS::BLOOM, "BLOOM_DEFAULT"},
{CMeshEffect_Emitter::EFFECT_SHADERPASS::BLOOM_DISSOLVE, "BLOOM_DISSOLVE"},
{CMeshEffect_Emitter::EFFECT_SHADERPASS::BLOOM_DISSOLVE_BILLBOARD, "BLOOM_DISSOLVE_BILLBOARD"},
{CMeshEffect_Emitter::EFFECT_SHADERPASS::DISTORTION, "DISTORTION"},
{CMeshEffect_Emitter::EFFECT_SHADERPASS::SUB_DISSOLVE, "SUB_DISSOLVE"},
{CMeshEffect_Emitter::EFFECT_SHADERPASS::SUBCOLOR_BLOOM_DISSOLVE, "SUBCOLOR_BLOOM_DISSOLVE"},
{CMeshEffect_Emitter::EFFECT_SHADERPASS::SUBCOLOR_BLOOMDISSOLVE_BILLBOARD, "SUBCOLOR_BLOOMDISSOLVE_BILLBOARD"},


	});
END