#pragma once
#include "Base.h"

BEGIN(Engine)

class CGameInstance;
class CCompute_Shader;

class CEffect_Module : public CBase
{
public:
	enum MODULE_TYPE { MODULE_TRANSLATION, MODULE_KEYFRAME, MODULE_UV };
	enum DATA_APPLY { TRANSLATION, ROTATION, SCALE, COLOR, UV };

protected:
	CEffect_Module();
	CEffect_Module(const CEffect_Module& _Prototype);
	virtual ~CEffect_Module() = default;

public:
	virtual HRESULT Initialize(const json& _jsonModuleInfo);

	virtual void	Update_Translations(_float _fTimeDelta, _float* _pBuffer, _uint _iNumInstance, 
		_uint _iPositionOffset, _uint _iVelocityOffset, _uint _iAccelerationOffset, _uint _iLifeTimeOffset, _uint _iTotalSize);	

	// 개별적인 이펙트에서만 적용
	virtual void	Update_ColorKeyframe(_float  _fCurTime, _float4* _pColor);
	virtual void	Update_ScaleKeyframe(_float _fCurTime, _float4* _pRight, _float4* _pUp, _float4* _pLook);
	
	// 여러개의 파티클에서 적용
	virtual void	Update_ColorKeyframe(_float* _pBuffer, _uint _iNumInstance, _uint _iColorOffset, _uint _iLifeTimeOffset, _uint _iTotalSize);
	virtual void	Update_ScaleKeyframe(_float* _pBuffer, _uint _iNumInstance, _uint _iRightOffset, _uint _iUpOffset, _uint _iLookOffset, _uint _iLifeTimeOffset, _uint _iTotalSize);

	virtual _int	Update_Module(class CCompute_Shader* _pCShader);

public:
	_bool		Is_Init() const { return m_isInit; }
	_int		Get_Order() const { return m_iOrder; }
	MODULE_TYPE	Get_Type() const { return m_eModuleType; }
	DATA_APPLY	Get_ApplyType() const { return m_eApplyTo; }

protected:
	CGameInstance* m_pGameInstance = { nullptr };
protected:
	_bool				m_isInit = { false };
	_int				m_iOrder = { 0 };
	MODULE_TYPE			m_eModuleType = {};
	DATA_APPLY			m_eApplyTo;

public:	
	virtual CEffect_Module* Clone() = 0;
	virtual void Free() override;

#ifdef _DEBUG
public:
	virtual void	Tool_Module_Update();
	virtual HRESULT	Save_Module(json& _jsonModuleInfo);

public:
	void			Set_Order(_int _iOrder) { m_iOrder = _iOrder; }
	const _string	Get_TypeName() const { return m_strTypeName; }
	_bool			Is_Changed() { if (m_isChanged) { m_isChanged = false; return true; } else return false; }

protected:
	_bool	m_isChanged = { false };
	_string m_strTypeName;

#endif

};

END

BEGIN(Engine)

NLOHMANN_JSON_SERIALIZE_ENUM(CEffect_Module::MODULE_TYPE, {
{CEffect_Module::MODULE_TYPE::MODULE_TRANSLATION, "MODULE_TRANSLATION"},
{CEffect_Module::MODULE_TYPE::MODULE_KEYFRAME, "MODULE_KEYFRAME"},
{CEffect_Module::MODULE_TYPE::MODULE_UV, "MODULE_UV"},

	});

NLOHMANN_JSON_SERIALIZE_ENUM(CEffect_Module::DATA_APPLY, {
{CEffect_Module::DATA_APPLY::TRANSLATION, "TRANSLATION"},
{CEffect_Module::DATA_APPLY::ROTATION, "ROTATION"},
{CEffect_Module::DATA_APPLY::SCALE, "SCALE"},
{CEffect_Module::DATA_APPLY::COLOR, "COLOR"},
{CEffect_Module::DATA_APPLY::UV, "UV"},

	});


END