#pragma once
#include "PartObject.h"

BEGIN(Engine)

/* 모든 파티클의 부모 클래스 */
/* 한 종류의 파티클만 나온다 */
/* Shader Component만 보유 */
/* 기본 함수들 보유 */

class ENGINE_DLL CEmitter : public CPartObject
{
public:
	enum EFFECT_TYPE { SINGLE_SPRITE, MESH, SPRITE, EFFECT, NONE };
	enum SPAWN_TYPE { SPAWN_RATE, BURST_SPAWN };
	enum SPAWN_POSITION { RELATIVE_WORLD, ABSOLUTE_WORLD, FOLLOW_PARENT  };
	enum POOLING_TYPE {KILL, REVIVE};
	enum EMITTER_EVENT_TYPE { STOP_SPAWN, NO_EVENT, };

public:
	typedef struct tagParticleEmitterDesc : public CPartObject::PARTOBJECT_DESC
	{
		_uint iProtoShaderLevel;
		const _tchar* szShaderTag = L"";
		const _tchar* szComputeShaderTag = L"";
	} PARTICLE_EMITTER_DESC;

protected:
	CEmitter(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CEmitter(const CEmitter& _Prototype);
	virtual ~CEmitter() = default;

public:
	static void SetID_3D(_int _iID) { s_iRG_3D = _iID; }
	static void SetID_2D(_int _iID) { s_iRG_2D = _iID; }
	static void SetID_Effect(_int _iID) { s_iRGP_EFFECT = _iID; }
	static void SetID_Particle(_int _iID) { s_iRGP_PARTICLE = _iID; }

public:
	virtual HRESULT				Initialize_Prototype(const json& _jsonInfo);
	virtual HRESULT				Initialize(void* _pArg) override;
	//virtual void				Priority_Update(_float _fTimeDelta) override;
	virtual void				Update(_float _fTimeDelta) override;
	virtual void				Late_Update(_float _fTimeDelta) override;

public:
	virtual	void				Reset() = 0; 
	void						Stop_Spawn(_float _fDelayTime)	{ m_fAccEventTime = 0.f; m_fInactiveDelayTime = _fDelayTime; m_eNowEvent = STOP_SPAWN; }
	void						Set_SpawnMatrix(const _float4x4* _pSpawnMatrix) { m_pSpawnMatrix = _pSpawnMatrix; }

public:
	const EFFECT_TYPE			Get_Type() const { return m_eEffectType; }
	_uint						Get_EventID() const { return m_iEventID; }

protected:
	static _int			s_iRG_3D;
	static _int			s_iRG_2D;
	static _int			s_iRGP_EFFECT;
	static _int			s_iRGP_PARTICLE;

	static _float4x4	s_IdentityMatrix;

protected:
	class CShader* m_pShaderCom = { nullptr };			
	class CCompute_Shader* m_pComputeShader = { nullptr };

protected:
	EFFECT_TYPE		m_eEffectType = { NONE };								// Sprite or Mesh?
	SPAWN_TYPE		m_eSpawnType = { SPAWN_RATE };							// Spawn Type
	SPAWN_POSITION	m_eSpawnPosition = { RELATIVE_WORLD };				// System에 따라 위치가 결정? World 좌표 그대로로 설정? 여부
	POOLING_TYPE	m_ePooling = { KILL };									// 재사용 여부
	EMITTER_EVENT_TYPE m_eNowEvent = {NO_EVENT};
	_uint			m_iShaderPass = 0;					// Particle 설정 여부에 따라서 Pass도 바뀐다.
			
	// 이벤트로 인한 Particle 진행여부는 Active로 판단.
	_uint			m_iEventID = { 0 };					// 파티클이 진행될 ID
	_float			m_fActiveTime = { 3.f };			// 이벤트 총 진행시간

	_float			m_fAccTime = { 0.f };				// 진행시간
	_float			m_fSpawnDelayTime = { 0.f };		// Delay 시간

	_float			m_fAccEventTime = { 0.f };			// 어떤 이벤트가 진행되고 있을 때,
	_float			m_fInactiveDelayTime = { 0.f };			// StopDelay 시간, 동적 설정

	_uint			m_iLoopTime = { 1 };				// 루프 횟수
	_uint			m_iAccLoop = { 0 };					// Delay가 지나면 ++, 

	const _float4x4* m_pSpawnMatrix = { nullptr };
	_float4x4		 m_SpawnMatrix;
	vector<class CEffect_Module*> m_Modules;

protected:
	map<const _string, _float> m_FloatDatas;
	map<const _string, _float2> m_Float2Datas;
	map<const _string, _float4> m_Float4Datas;

protected:
	virtual void Active_OnEnable() override;
	virtual void Active_OnDisable() override;

	virtual void Update_Emitter(_float _fTimeDelta) = 0;

	void Set_Matrix();

protected:
	virtual HRESULT Ready_Components(const PARTICLE_EMITTER_DESC* _pDesc);

	HRESULT							Bind_Float(const _char* _szDataName, const _char* _szConstantName);
	HRESULT							Bind_Float2(const _char* _szDataName, const _char* _szConstantName);
	HRESULT							Bind_Float4(const _char* _szDataName, const _char* _szConstantName);


public:
	virtual CGameObject* Clone(void* _pArg) = 0;
	virtual void		 Free() override;
	virtual HRESULT		 Cleanup_DeadReferences() = 0;

#ifdef _DEBUG
public:
	virtual void				Tool_Update(_float fTimeDelta);
	virtual void				Tool_Setting();
	virtual HRESULT				Save(json& _jsonOut);

	_bool						Is_ToolChanged() { if (m_isToolChanged) { m_isToolChanged = false; return true; } else return false; }

protected:
	//_bool						m_isToolProgress = { true };
	_bool						m_isToolChanged = { false };
	_int						m_iNowModuleIndex = { 0 };
#endif

};

END

BEGIN(Engine)
NLOHMANN_JSON_SERIALIZE_ENUM(CEmitter::SPAWN_TYPE, {
	{CEmitter::SPAWN_TYPE::SPAWN_RATE, "SPAWN_RATE"},
	{CEmitter::SPAWN_TYPE::BURST_SPAWN, "BURST_SPAWN"},
	});

NLOHMANN_JSON_SERIALIZE_ENUM(CEmitter::SPAWN_POSITION, {
	{CEmitter::SPAWN_POSITION::RELATIVE_WORLD, "RELATIVE_WORLD"},
	{CEmitter::SPAWN_POSITION::ABSOLUTE_WORLD, "ABSOLUTE_WORLD"},
	{CEmitter::SPAWN_POSITION::FOLLOW_PARENT, "FOLLOW_PARENT"},
	});

NLOHMANN_JSON_SERIALIZE_ENUM(CEmitter::POOLING_TYPE, {
	{CEmitter::POOLING_TYPE::KILL, "KILL"},
	{CEmitter::POOLING_TYPE::REVIVE, "REVIVE"},
	});

END