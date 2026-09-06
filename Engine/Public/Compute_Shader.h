#pragma once
#include "Component.h"

BEGIN(Engine)

class ENGINE_DLL CCompute_Shader : public CComponent
{
private:
	CCompute_Shader(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CCompute_Shader(const CCompute_Shader& _Prototype);
	virtual ~CCompute_Shader() = default;

public:
	HRESULT Initialize_Prototype(const _tchar* _pShaderFilePath);
	virtual HRESULT Initialize(void* _pArg) override;

public:
	HRESULT Bind_RawValue(const _char* _pConstantName, const void* _pData, _uint _iLength);
	HRESULT Bind_Matrix(const _char* _pConstantName, const _float4x4* _pMatrix);
	HRESULT Bind_SRV(const _char* _pConstantName, ID3D11ShaderResourceView* _pSRV);

	HRESULT Set_SRVs(ID3D11ShaderResourceView** _ppSRVs, _uint _iCount, _uint _iStartSlot = 0);
	HRESULT Set_UAVs(ID3D11UnorderedAccessView** _ppUAVs, _uint _iCount, _uint _iStartSlot = 0);
public:
	HRESULT Begin(_uint _iPassIndex);
	HRESULT Compute(_uint _iThreadGroupCountX, _uint _iThreadGroupCountY, _uint _iThreadGroupCountZ);
	HRESULT End_Compute();

private:
	ID3DX11Effect* m_pEffect = { nullptr };
	//ID3D11Query* m_pEventQuery = { nullptr };
	_uint			m_iNumLayouts = { 0 }; // NumLayouts == NumPasses
	//map<_string, ID3D11ComputeShader*> m_ComputeShaders;

public:
	static CCompute_Shader* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const _tchar* _pShaderFilePath);
	virtual CComponent* Clone(void* _pArg) override;
	virtual void Free() override;
};

END