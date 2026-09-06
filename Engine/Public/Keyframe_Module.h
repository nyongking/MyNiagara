#pragma once
#include "Effect_Module.h"

BEGIN(Engine)

class CKeyframe_Module : public CEffect_Module
{

public:
	enum MODULE_NAME { COLOR_KEYFRAME, SCALE_KEYFRAME };

private:
	CKeyframe_Module(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CKeyframe_Module(const CKeyframe_Module& _Prototype);
	virtual ~CKeyframe_Module() = default;

public:
	HRESULT Initialize(const json& _jsonModuleInfo, _int _iNumInstance);
	virtual void	Update_ColorKeyframe(_float  _fCurTime, _float4* _pColor) override; 
	virtual void	Update_ScaleKeyframe(_float _fCurTime, _float4* _pRight, _float4* _pUp, _float4* _pLook) override;

	// 파티클 전용
	virtual void	Update_ColorKeyframe(_float* _pBuffer, _uint _iNumInstance, _uint _iColorOffset, _uint _iLifeTimeOffset, _uint _iTotalSize) override;
	virtual void	Update_ScaleKeyframe(_float* _pBuffer, _uint _iNumInstance, _uint _iRightOffset, _uint _iUpOffset, _uint _iLookOffset, _uint _iLifeTimeOffset, _uint _iTotalSize) override;

	virtual _int	Update_Module(class CCompute_Shader* _pCShader) override;

private:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };

private:
	MODULE_NAME				m_eModuleName;
	vector<_float>			m_Keyframes;
	vector<_float4>			m_KeyframeDatas;
	vector<_uint>			m_KeyCurrentIndicies;
	ID3D11Texture2D*		m_pKeyframeDataTexture = { nullptr };
	ID3D11ShaderResourceView* m_pSRVKeyframeDatas = { nullptr };

public:
	static CKeyframe_Module* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonModuleInfo, _int _iNumInstance);
	virtual CEffect_Module* Clone();
	virtual void Free() override;


#ifdef _DEBUG
public:
	HRESULT Initialize(MODULE_NAME _eModuleName, _int _iNumInstance);

public:
	virtual void	Tool_Module_Update() override;
	virtual HRESULT	Save_Module(json& _jsonModuleInfo) override;


public:
	static CKeyframe_Module* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, MODULE_NAME _eModuleName, _int _iNumInstance);


	static const _char* g_szModuleNames[2];
#endif 
};

END

BEGIN(Engine)

NLOHMANN_JSON_SERIALIZE_ENUM(CKeyframe_Module::MODULE_NAME, {
	{CKeyframe_Module::MODULE_NAME::COLOR_KEYFRAME, "COLOR_KEYFRAME"},
	{CKeyframe_Module::MODULE_NAME::SCALE_KEYFRAME, "SCALE_KEYFRAME"},


	});


END