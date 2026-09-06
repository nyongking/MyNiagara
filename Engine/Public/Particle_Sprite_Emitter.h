#pragma once
#include "Emitter.h"

BEGIN(Engine)


class ENGINE_DLL CParticle_Sprite_Emitter : public CEmitter
{
public:
	enum UPDATE_MODE { UPDATE_GPU, UPDATE_CPU };
	enum EFFECT_SHADERPASS { DEFAULT, DEFAULT_BLOOM, 
		ROTATION_BILLBOARD, ROTATION_BILLBOARD_BLOOM,
		VELOCITY_BILLBOARD, VELOCITY_BILLBOARD_BLOOM,
		SUBCOLORBLOOM,		DEFAULT_DISSOLVE, VELOCITY_BILLBOARD_SUBCOLORBLOOM,
		DISSOLVE_SUBCOLORBLOOM, FIRESMOKE, FIRE, ROTATION_BILLBOARD_SUBBLOOM,
		ROT_SUB_DISSOLVE, VEL_SUB_DISSOLVE, ROT2_SUB_DISSOLVE, DEFAULT_NO_BILLBOARD
	};

private:
	CParticle_Sprite_Emitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CParticle_Sprite_Emitter(const CParticle_Sprite_Emitter& _Prototype);
	virtual ~CParticle_Sprite_Emitter() = default;

public:
	//HRESULT						Initialize_Prototype(const _tchar* _szFilePath);
	virtual HRESULT				Initialize_Prototype(const json& _jsonInfo) override;
	virtual HRESULT				Initialize(void* _pArg) override; 
	virtual void				Priority_Update(_float _fTimeDelta) override ;
	virtual void				Update(_float _fTimeDelta) override;
	virtual void				Late_Update(_float _fTimeDelta) override ;
	virtual HRESULT				Render() override;

public:
	virtual	void				Reset() override;
	void						Set_UpdateMode(UPDATE_MODE _eUpdateMode);
	UPDATE_MODE				Get_UpdateMode() const { return m_eUpdateMode; }

private:
	UPDATE_MODE					m_eUpdateMode = { UPDATE_GPU };
	class CVIBuffer_Point_Particle* m_pParticleBufferCom = { nullptr };
	class CTexture*					m_pMaskTextureCom = { nullptr };
	class CTexture*					m_pDissolveTextureCom = { nullptr };
	class CTexture*					m_pDistortionTextureCom = { nullptr };

private:
	//_float							m_fAlphaDiscard = { 0.f };
	//_float							m_fRGBDiscard = { 0.f };
	//_float							m_fBloomThreshold = { 0.f };
	

private:
	virtual void					Update_Emitter(_float _fTimeDelta) override;
	void							Update_Emitter_GPU(_float _fTimeDelta);
	void							Update_Emitter_CPU(_float _fTimeDelta);
	const _float4x4*				Get_CPU_SpawnMatrix(_float4x4& _SpawnMatrix) const;


	HRESULT							Bind_ShaderResources();
	HRESULT							Bind_ShaderValue_ByPass();
	virtual HRESULT					Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc) override;

public:
	//static	CParticle_Sprite_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const _tchar* _szFilePath);
	static	CParticle_Sprite_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonInfo);
	virtual CGameObject* Clone(void* _pArg) override;
	virtual void		 Free() override;
	virtual HRESULT		 Cleanup_DeadReferences() override;
	
	
#ifdef _DEBUG 
public:
	virtual void				Tool_Setting() override;
	virtual void				Tool_Update(_float _fTimeDelta) override;
	virtual HRESULT				Save(json& _jsonOut);
public:
	void						Set_Texture(class CTexture* _pTextureCom, _uint _iTextureIndex);

public:
	// DEBUG용 처음 만든 Sprite
	static	CParticle_Sprite_Emitter* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, void* _pArg);

#endif
};

END

BEGIN(Engine)
NLOHMANN_JSON_SERIALIZE_ENUM(CParticle_Sprite_Emitter::UPDATE_MODE, {
{CParticle_Sprite_Emitter::UPDATE_MODE::UPDATE_GPU, "GPU"},
{CParticle_Sprite_Emitter::UPDATE_MODE::UPDATE_CPU, "CPU"},
	});

NLOHMANN_JSON_SERIALIZE_ENUM(CParticle_Sprite_Emitter::EFFECT_SHADERPASS, {
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::DEFAULT, "DEFAULT"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::DEFAULT_BLOOM, "DEFAULT_BLOOM"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::SUBCOLORBLOOM, "SUBCOLOR_BLOOM"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::VELOCITY_BILLBOARD_SUBCOLORBLOOM, "VELOCITY_BILLBOARD_SUBCOLORBLOOM"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::ROTATION_BILLBOARD, "ROTATION_BILLBOARD"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::ROTATION_BILLBOARD_BLOOM, "ROTATION_BILLBOARD_BLOOM"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::VELOCITY_BILLBOARD, "VELOCITY_BILLBOARD"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::VELOCITY_BILLBOARD_BLOOM, "VELOCITY_BILLBOARD_BLOOM"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::DEFAULT_DISSOLVE, "DEFAULT_DISSOLVE"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::DISSOLVE_SUBCOLORBLOOM, "DISSOLVE_SUBCOLORBLOOM"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::FIRESMOKE, "FIRESMOKE"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::FIRE, "FIRE"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::ROTATION_BILLBOARD_SUBBLOOM, "ROTATION_BILLBOARD_SUBBLOOM"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::ROT_SUB_DISSOLVE, "ROT_SUB_DISSOLVE"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::VEL_SUB_DISSOLVE, "VEL_SUB_DISSOLVE"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::ROT2_SUB_DISSOLVE, "ROT2_SUB_DISSOLVE"},
{CParticle_Sprite_Emitter::EFFECT_SHADERPASS::DEFAULT_NO_BILLBOARD, "DEFAULT_NO_BILLBOARD"},

	});
END
