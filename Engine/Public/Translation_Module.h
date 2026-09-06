#pragma once
#include "Effect_Module.h"

BEGIN(Engine)

class CTranslation_Module : public CEffect_Module
{
public:
	enum MODULE_NAME { POINT_VELOCITY, LINEAR_VELOCITY, INIT_ACCELERATION, GRAVITY, DRAG,
		VORTEX_ACCELERATION, POINT_ACCELERATION, LIMIT_ACCELERATION, POSITION_BY_NUMBER };

private:
	CTranslation_Module();
	CTranslation_Module(const CTranslation_Module& _Prototype);
	virtual ~CTranslation_Module() = default;

public:
	virtual HRESULT Initialize(const json& _jsonModuleInfo) override;	
	//virtual void	Update_Translations(_float _fTimeDelta, _float4* _pPosition, _float3* _pVelocity, _float3* _pAcceleration) override;
	virtual void	Update_Translations(_float _fTimeDelta, _float* _pBuffer, _uint _iNumInstance,
		_uint _iPositionOffset, _uint _iVelocityOffset, _uint _iAccelerationOffset, _uint _iLifeTimeOffset, _uint _iTotalSize);
	
	virtual _int	Update_Module(class CCompute_Shader* _pCShader) override;

private:
	MODULE_NAME		m_eModuleName;

private:
	map<const _string, _float> m_FloatDatas;
	map<const _string, _float3> m_Float3Datas;


public:
	static CTranslation_Module* Create(const json& _jsonModuleInfo);
	virtual CEffect_Module* Clone();
	virtual void Free() override;

#ifdef _DEBUG

public:
	HRESULT Initialize(MODULE_NAME eType, const _string& _strTypeName);
	virtual void	Tool_Module_Update() override;
	virtual HRESULT	Save_Module(json& _jsonModuleInfo) override;

public:
	static const _char* g_szModuleNames[9];
	static CTranslation_Module* Create(MODULE_NAME _eType, const _string& _strTypeName);

#endif 
};

END

BEGIN(Engine)

NLOHMANN_JSON_SERIALIZE_ENUM(CTranslation_Module::MODULE_NAME, {
{CTranslation_Module::MODULE_NAME::POINT_VELOCITY, "POINT_VELOCITY"},
{CTranslation_Module::MODULE_NAME::LINEAR_VELOCITY, "LINEAR_VELOCITY"},
{CTranslation_Module::MODULE_NAME::INIT_ACCELERATION, "INIT_ACCELERATION"},
{CTranslation_Module::MODULE_NAME::GRAVITY, "GRAVITY"},
{CTranslation_Module::MODULE_NAME::DRAG, "DRAG"},
{CTranslation_Module::MODULE_NAME::VORTEX_ACCELERATION, "VORTEX_ACCELERATION"},
{CTranslation_Module::MODULE_NAME::POINT_ACCELERATION, "POINT_ACCELERATION"},
{CTranslation_Module::MODULE_NAME::LIMIT_ACCELERATION, "LIMIT_ACCELERATION"},
{CTranslation_Module::MODULE_NAME::POSITION_BY_NUMBER, "POSITION_BY_NUMBER"},

	});


END


