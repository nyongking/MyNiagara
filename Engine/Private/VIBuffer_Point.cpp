#include "VIBuffer_Point.h"

CVIBuffer_Point::CVIBuffer_Point(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
    : CVIBuffer(_pDevice, _pContext)
{
}

CVIBuffer_Point::CVIBuffer_Point(const CVIBuffer_Point& _Prototype)
    : CVIBuffer(_Prototype)
{
}

HRESULT CVIBuffer_Point::Initialize_Prototype()
{
	// 멤버 변수에 값 저장. >>> 추후 IA에 전송할 데이터들을 미리 저장 해두는 것.
	m_iNumVertexBuffers = 1;

	m_iVertexStride = sizeof(VTXPOINT);
	m_iNumVertices = 1;

	m_iIndexStride = sizeof(_ushort);
	m_iNumIndices = 1;
	m_eIndexFormat = DXGI_FORMAT_R16_UINT; // 16비트 정수형 타입
	m_ePrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

#pragma region VERTEX_BUFFER

	// 1. BufferDesc 값을 채운다.
	ZeroMemory(&m_BufferDesc, sizeof(m_BufferDesc));

	/* 정점버퍼를 몇 바이트 할당할까요? */
	m_BufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
	/* 어떠한 목적으로 사용되는 버퍼인가요? */
	m_BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	/* 매우정적, 정적, 동적, 매우동적 */
	m_BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	/* 버퍼 하나의 메모리 할당 크기 */
	m_BufferDesc.StructureByteStride = m_iVertexStride;
	/* 아래의 두 Flag는 Usage가 Default가 아닌 경우 사용된다. */
	/* CPU에서 리소스에 접근할 수 있는지에 대한 여부를 결정하는 플래그 */
	m_BufferDesc.CPUAccessFlags = 0;
	/* 버퍼에 대한 추가적인 부가 옵션들을 설정하는 플래그 (밉맵, 공유리소스, 간접그리기호출리소스)*/
	m_BufferDesc.MiscFlags = 0;


	// 2. SubResourceData 값을 채운다.
	//m_vecVerticesPos.reserve(m_iNumVertices);
	VTXPOINT* pVertices = new VTXPOINT[m_iNumVertices];
	ZeroMemory(pVertices, sizeof(VTXPOINT) * m_iNumVertices);

	pVertices[0].vPosition = XMFLOAT3(0.f, 0.f, 0.0f);
	pVertices[0].vSize = XMFLOAT2(1.f, 1.f);

	//m_vecVerticesPos.push_back(pVertices[0].vPosition);

	ZeroMemory(&m_SubResourceDesc, sizeof(m_SubResourceDesc));
	m_SubResourceDesc.pSysMem = pVertices; // Buffer 생성시에는 pSysMem 만 사용한다.

	// 3. Buffer를 생성한다.
	if (FAILED(CVIBuffer::Create_Buffer(&m_pVB)))
		return E_FAIL;

	// 4. Buffer 에 값을 채워주기 위해 생성한 구조체* 를 제거한다 (매우 중요!)
	Safe_Delete_Array(pVertices);
#pragma endregion

#pragma region INDEX_BUFFER
	ZeroMemory(&m_BufferDesc, sizeof m_BufferDesc);

	m_BufferDesc.ByteWidth = m_iIndexStride * m_iNumIndices;
	m_BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	m_BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	m_BufferDesc.StructureByteStride = /*m_iIndexStride*/0; // 0으로 값을 채워도 상관 없는 이유 : 추후 IndexBuffer 데이터를 몇바이트 단위로 읽을 지에 대한 값은 따로 설정함.
	m_BufferDesc.CPUAccessFlags = 0;
	m_BufferDesc.MiscFlags = 0;

	// SubResourceData 채우기 //
	
	_ushort* pIndices = new _ushort[m_iNumIndices];
	ZeroMemory(pIndices, sizeof(_ushort) * m_iNumIndices);

	pIndices[0] = 0;
	//m_vecIndexBuffer.push_back(pIndices[0]);

	ZeroMemory(&m_SubResourceDesc, sizeof(D3D11_SUBRESOURCE_DATA));
	m_SubResourceDesc.pSysMem = pIndices;

	if (FAILED(__super::Create_Buffer(&m_pIB)))
		return E_FAIL;

	Safe_Delete_Array(pIndices);


#pragma endregion

    return S_OK;
}

HRESULT CVIBuffer_Point::Initialize(void* _pArg)
{
    return S_OK;
}

CVIBuffer_Point* CVIBuffer_Point::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
{
	CVIBuffer_Point* pInstance = new CVIBuffer_Point(_pDevice, _pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CVIBuffer_Point");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CVIBuffer_Point::Clone(void* _pArg)
{
	CVIBuffer_Point* pInstance = new CVIBuffer_Point(*this);

	if (FAILED(pInstance->Initialize(_pArg)))
	{
		MSG_BOX("Failed to Cloned : CVIBuffer_Point");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CVIBuffer_Point::Free()
{
	__super::Free();
}
