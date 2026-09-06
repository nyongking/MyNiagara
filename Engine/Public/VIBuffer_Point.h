#pragma once
#include "VIBuffer.h"
BEGIN(Engine)

class ENGINE_DLL CVIBuffer_Point final : public CVIBuffer
{
private:
	CVIBuffer_Point(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	CVIBuffer_Point(const CVIBuffer_Point& _Prototype);
	virtual ~CVIBuffer_Point() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* _pArg);

public:
	
public:
	static CVIBuffer_Point* Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext);
	virtual CComponent* Clone(void* _pArg) override;
	virtual void Free() override;
};

END