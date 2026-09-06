#include "VIBuffer_Instance.h"
#include "GameInstance.h"
#include "Effect_Module.h"


CVIBuffer_Instance::CVIBuffer_Instance(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
	: CVIBuffer(_pDevice, _pContext)
{
}

CVIBuffer_Instance::CVIBuffer_Instance(const CVIBuffer_Instance& _Prototype)
	: CVIBuffer(_Prototype)
	, m_InstanceBufferDesc(_Prototype.m_InstanceBufferDesc)
	, m_InstanceInitialDesc(_Prototype.m_InstanceInitialDesc)
	, m_iNumInstances(_Prototype.m_iNumInstances)
	, m_iNumIndexCountPerInstance(_Prototype.m_iNumIndexCountPerInstance)
	, m_iInstanceStride(_Prototype.m_iInstanceStride)

	//, m_eSpawnType(_Prototype.m_eSpawnType)
	//, m_fSpawnTime(_Prototype.m_fSpawnTime)
	, m_eShapeType(_Prototype.m_eShapeType)
	, m_ShapeDatas(_Prototype.m_ShapeDatas)
	, m_SetDatas(_Prototype.m_SetDatas)
	, m_pSetScales(_Prototype.m_pSetScales)
	, m_pSetRotations(_Prototype.m_pSetRotations)
	, m_pSetPositions(_Prototype.m_pSetPositions)
	, m_pSetLifeTimes(_Prototype.m_pSetLifeTimes)
	, m_pSetColors(_Prototype.m_pSetColors)
	, m_vShapeScale(_Prototype.m_vShapeScale)
	, m_vShapeRotation(_Prototype.m_vShapeRotation)
	, m_vShapePosition(_Prototype.m_vShapePosition)

	, m_pBufferInitial(_Prototype.m_pBufferInitial)
	, m_pSRVInitial(_Prototype.m_pSRVInitial)
{
	Safe_AddRef(m_pBufferInitial);
	Safe_AddRef(m_pSRVInitial);
}

HRESULT CVIBuffer_Instance::Initialize_Prototype()
{
	m_SetDatas.resize(DATAEND, UNSET);

	m_pSetScales = new _float3[2];
	for (_int i = 0; i < 2; ++i)
	{
		for (_int j = 0; j < 3; ++j)
		{
			*((_float*)(&m_pSetScales[i]) + j) = 1.f;
		}
	}

	m_pSetRotations = new _float3[2];
	for (_int i = 0; i < 2; ++i)
	{
		for (_int j = 0; j < 3; ++j)
		{
			*((_float*)(&m_pSetRotations[i]) + j) = 0.f;
		}
	}

	m_pSetPositions = new _float3[2];
	for (_int i = 0; i < 2; ++i)
	{
		for (_int j = 0; j < 3; ++j)
		{
			*((_float*)(&m_pSetPositions[i]) + j) = 0.f;
		}
	}
	m_pSetLifeTimes = new _float[2];
	for (_int i = 0; i < 2; ++i)
	{
		for (_int j = 0; j < 1; ++j)
		{
			*((_float*)(&m_pSetLifeTimes[i]) + j) = 0.f;
		}
	}

	m_pSetColors = new _float4[2];
	for (_int i = 0; i < 2; ++i)
	{
		for (_int j = 0; j < 4; ++j)
		{
			*((_float*)(&m_pSetColors[i]) + j) = 1.f;
		}
	}

	for (_int i = 0; i < 3; ++i)
	{
		*((_float*)(&m_vShapeScale) + i) = 1.f;
		*((_float*)(&m_vShapeRotation) + i) = 0.f;
		*((_float*)(&m_vShapePosition) + i) = 0.f;
	}
	return S_OK;
}

HRESULT CVIBuffer_Instance::Initialize(void* _pArg)
{
	return m_pDevice->CreateBuffer(&m_InstanceBufferDesc, &m_InstanceInitialDesc, &m_pVBInstance);
}


HRESULT CVIBuffer_Instance::Render()
{
	m_pContext->DrawIndexedInstanced(m_iNumIndexCountPerInstance, m_iNumInstances, 0, 0, 0);

	return S_OK;
}

void CVIBuffer_Instance::Begin_Compute(CCompute_Shader* _pCShader)
{
	_pCShader->Set_SRVs(&m_pSRVInitial, 1);
	_pCShader->Set_UAVs(&m_pUAV, 1);
}

void CVIBuffer_Instance::Compute(CCompute_Shader* _pCShader)
{
	_pCShader->Compute((_uint)(ceil(m_iNumInstances / 256.f)), 1, 1);
	Begin_Compute(_pCShader);
}


void CVIBuffer_Instance::Update_Module(CEffect_Module* _pModule, CCompute_Shader* _pCShader)
{
	_int iEntry = _pModule->Update_Module(_pCShader);
	if (0 <= iEntry)
	{
		_pCShader->Begin(iEntry);
		Begin_Compute(_pCShader);
		_pCShader->Compute((_uint)(ceil(m_iNumInstances / 256.f)), 1, 1);
		_pCShader->End_Compute();
	}
}

HRESULT CVIBuffer_Instance::Bind_BufferDesc()
{
	ID3D11Buffer* pVertexBuffer[] = {
		m_pVB,
		m_pVBInstance
	};

	_uint		iVertexStrides[] = {
		m_iVertexStride,
		m_iInstanceStride
	};

	_uint		iOffsets[] = {
		0,
		0
	};

	// 시작 슬롯, Vertex Buffer의 개수, Vertex Buffer 배열 주소, 각 Vertex Buffer byte 크기 주소, 오프셋
	m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pVertexBuffer, iVertexStrides, iOffsets);
	// Index Buffer, Index Buffer의 크기, 오프셋
	m_pContext->IASetIndexBuffer(m_pIB, m_eIndexFormat, 0);
	// 형상 (삼각형, 점,,,)
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveTopology);

	return S_OK;
}

HRESULT CVIBuffer_Instance::Bind_BufferBySRV()
{
	m_pContext->VSSetShaderResources(0, 1, &m_pSRV);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveTopology);


	return S_OK;
}

HRESULT CVIBuffer_Instance::Render_BySRV()
{
	m_pContext->Draw(_uint(m_iNumInstances), 0);

	ID3D11ShaderResourceView* nullSRV[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	m_pContext->VSSetShaderResources(0, 6, nullSRV); // VS에서도 해제

	return S_OK;
}

void CVIBuffer_Instance::Reset_Buffers(CCompute_Shader* _pCShader)
{
	_pCShader->Begin(3);
	Begin_Compute(_pCShader);
	Compute(_pCShader);
	_pCShader->End_Compute();
}




HRESULT CVIBuffer_Instance::Set_ShapeData(const json& _jsonInfo)
{
	if (false == _jsonInfo.contains("ShapeLocation") && false == _jsonInfo["ShapeLocation"].contains("Shape"))
		return E_FAIL;

	m_eShapeType = _jsonInfo["ShapeLocation"]["Shape"];
	
	switch (m_eShapeType)
	{
	case SPHERE:
	{
		if (false == _jsonInfo["ShapeLocation"].contains("Sphere_Radius"))
			return E_FAIL;
		m_ShapeDatas.emplace("Sphere_Radius", _jsonInfo["ShapeLocation"]["Sphere_Radius"]);

		if (false == _jsonInfo["ShapeLocation"].contains("Sphere_Surface"))
			return E_FAIL;
		m_ShapeDatas.emplace("Sphere_Surface", _jsonInfo["ShapeLocation"]["Sphere_Surface"]);

		break;
	}
	case CYLINDER:
	{
		if (false == _jsonInfo["ShapeLocation"].contains("Cylinder_Radius"))
			return E_FAIL;
		m_ShapeDatas.emplace("Cylinder_Radius", _jsonInfo["ShapeLocation"]["Cylinder_Radius"]);

		if (false == _jsonInfo["ShapeLocation"].contains("Cylinder_Height"))
			return E_FAIL;
		m_ShapeDatas.emplace("Cylinder_Height", _jsonInfo["ShapeLocation"]["Cylinder_Height"]);

		if (false == _jsonInfo["ShapeLocation"].contains("MidPoint"))
			return E_FAIL;
		m_ShapeDatas.emplace("MidPoint", _jsonInfo["ShapeLocation"]["MidPoint"]);

		break;
	}
	case BOX:
	{
		if (false == _jsonInfo["ShapeLocation"].contains("Width"))
			return E_FAIL;
		m_ShapeDatas.emplace("Width", _jsonInfo["ShapeLocation"]["Width"]);

		if (false == _jsonInfo["ShapeLocation"].contains("Height"))
			return E_FAIL;
		m_ShapeDatas.emplace("Height", _jsonInfo["ShapeLocation"]["Height"]);

		if (false == _jsonInfo["ShapeLocation"].contains("Depth"))
			return E_FAIL;
		m_ShapeDatas.emplace("Depth", _jsonInfo["ShapeLocation"]["Depth"]);

		if (false == _jsonInfo["ShapeLocation"].contains("MidX"))
			return E_FAIL;
		m_ShapeDatas.emplace("MidX", _jsonInfo["ShapeLocation"]["MidX"]);

		if (false == _jsonInfo["ShapeLocation"].contains("MidY"))
			return E_FAIL;
		m_ShapeDatas.emplace("MidY", _jsonInfo["ShapeLocation"]["MidY"]);

		if (false == _jsonInfo["ShapeLocation"].contains("MidZ"))
			return E_FAIL;
		m_ShapeDatas.emplace("MidZ", _jsonInfo["ShapeLocation"]["MidZ"]);

		if (_jsonInfo["ShapeLocation"].contains("Box_Surface"))
		{
			m_ShapeDatas.emplace("Box_Surface", _jsonInfo["ShapeLocation"]["Box_Surface"]);
		}
		break;
	}
	case TORUS:
	{
		if (false == _jsonInfo["ShapeLocation"].contains("LargeRadius"))
			return E_FAIL;
		m_ShapeDatas.emplace("LargeRadius", _jsonInfo["ShapeLocation"]["LargeRadius"]);

		if (false == _jsonInfo["ShapeLocation"].contains("HandleRadius"))
			return E_FAIL;
		m_ShapeDatas.emplace("HandleRadius", _jsonInfo["ShapeLocation"]["HandleRadius"]);

		if (false == _jsonInfo["ShapeLocation"].contains("U_Distribution"))
			return E_FAIL;
		m_ShapeDatas.emplace("U_Distribution", clamp((_float)(_jsonInfo["ShapeLocation"]["U_Distribution"]), 0.f, 1.f));

		if (false == _jsonInfo["ShapeLocation"].contains("V_Distribution"))
			return E_FAIL;
		m_ShapeDatas.emplace("V_Distribution", clamp((_float)(_jsonInfo["ShapeLocation"]["V_Distribution"]), 0.f, 1.f));

		break;
	}
	case RING:
	{
		if (false == _jsonInfo["ShapeLocation"].contains("Ring_Radius"))
			return E_FAIL;
		m_ShapeDatas.emplace("Ring_Radius", _jsonInfo["ShapeLocation"]["Ring_Radius"]);

		if (false == _jsonInfo["ShapeLocation"].contains("Coverage"))
			return E_FAIL;
		m_ShapeDatas.emplace("Coverage", clamp((_float)_jsonInfo["ShapeLocation"]["Coverage"], 0.f, 1.f));

		if (false == _jsonInfo["ShapeLocation"].contains("Ring_Distribution"))
			return E_FAIL;
		m_ShapeDatas.emplace("Ring_Distribution", clamp((_float)_jsonInfo["ShapeLocation"]["Ring_Distribution"], 0.f, 1.f));


		break;
	}
	case CONE:
	{
		if (false == _jsonInfo["ShapeLocation"].contains("Length"))
			return E_FAIL;
		m_ShapeDatas.emplace("Length", _jsonInfo["ShapeLocation"]["Length"]);

		if (false == _jsonInfo["ShapeLocation"].contains("Outer_Angle"))
			return E_FAIL;
		m_ShapeDatas.emplace("Outer_Angle", _jsonInfo["ShapeLocation"]["Outer_Angle"]);

		if (false == _jsonInfo["ShapeLocation"].contains("Inner_Angle"))
			return E_FAIL;
		m_ShapeDatas.emplace("Inner_Angle", _jsonInfo["ShapeLocation"]["Inner_Angle"]);

		if (false == _jsonInfo["ShapeLocation"].contains("Radial_Angle"))
			return E_FAIL;
		m_ShapeDatas.emplace("Radial_Angle", _jsonInfo["ShapeLocation"]["Radial_Angle"]);

		if (false == _jsonInfo["ShapeLocation"].contains("Cone_Surface"))
			return E_FAIL;
		m_ShapeDatas.emplace("Cone_Surface", clamp((_float)(_jsonInfo["ShapeLocation"]["Cone_Surface"]), 0.f, 1.f));

		if (false == _jsonInfo["ShapeLocation"].contains("Flatten"))
			return E_FAIL;
		m_ShapeDatas.emplace("Flatten", _jsonInfo["ShapeLocation"]["Flatten"]);

		break;
	}
	case SHAPE_NONE:
	{
		break;
	}

	}

	if (false == _jsonInfo["ShapeLocation"].contains("Scale") && 3 > _jsonInfo["ShapeLocation"]["Scale"].size())
		return E_FAIL;
	if (false == _jsonInfo["ShapeLocation"].contains("Rotation") && 3 > _jsonInfo["ShapeLocation"]["Rotation"].size())
		return E_FAIL;
	if (false == _jsonInfo["ShapeLocation"].contains("Position") && 3 > _jsonInfo["ShapeLocation"]["Position"].size())
		return E_FAIL;

	for (_int i = 0; i < 3; ++i)
	{
		*((_float*)(&m_vShapeScale) + i) = _jsonInfo["ShapeLocation"]["Scale"][i];
		*((_float*)(&m_vShapeRotation) + i) = _jsonInfo["ShapeLocation"]["Rotation"][i];
		*((_float*)(&m_vShapePosition) + i) = _jsonInfo["ShapeLocation"]["Position"][i];
	}

	return S_OK;
}

_vector  CVIBuffer_Instance::Get_Sphere_Pos(_float _fRadius, _float _fSurface)
{
	/* fSurface : 중앙에서 해당 비율만큼 떨어진 곳에서부터 만들어진다.
		0 = 중앙에서부터, 1 = 무조건 표면에서만 생성 */
	_vector vReturn;
	_fSurface = clamp(_fSurface, 0.f, 1.f);

	vReturn = XMVectorSetW(XMVector3Normalize(
		XMVectorSet(m_pGameInstance->Compute_Random(-1.f, 1.f), 
			m_pGameInstance->Compute_Random(-1.f, 1.f),
			m_pGameInstance->Compute_Random(-1.f, 1.f), 0.f))
		* _fRadius * m_pGameInstance->Compute_Random(_fSurface, 1.f), 1.f);

	return vReturn;
}

_vector CVIBuffer_Instance::Get_Cylinder_Pos(_float _fHeight, _float _fRadius, _float _fMidPoint)
{
	/* fMidPoint : 중앙점, 0 = 원기둥의 바닥, 1 = 원기둥의 윗면 */
	_vector vReturn;
	_float fOutY = m_pGameInstance->Compute_Random(0.f, _fHeight) - _fHeight * _fMidPoint;

	_vector vBase = XMVector3Normalize(XMVectorSet(
		m_pGameInstance->Compute_Random(-1.f, 1.f), 
		0.f,
		m_pGameInstance->Compute_Random(-1.f, 1.f), 0.f))
		* _fRadius * m_pGameInstance->Compute_Random_Normal();

	vReturn = XMVectorSetW(XMVectorSetY(vBase, fOutY), 1.f);

	return vReturn;
}

_vector CVIBuffer_Instance::Get_Box_Pos(const _float3& _vWHD, const _float3& _vMidPoint, _bool _isSurface, _float _fSurface)
{	
	/* fSurface : 표면에서 그려지는데, 두께는 어느정도인가 ? */
	/* fMidPoint : 중점으로부터 얼마만큼 떨어져 있는가 ? */
	/* vWHD : Widht, Height, Depth */

	_vector vReturn;
	_float fX, fY, fZ;

	if (_isSurface)
	{
		fX = m_pGameInstance->Compute_Random(0.f, _vWHD.x);
		fY = m_pGameInstance->Compute_Random(0.f, _vWHD.y);
		fZ = m_pGameInstance->Compute_Random(0.f, _vWHD.z);
		_int iRandValue = rand() % 3;
		if (0 == iRandValue)
		{
			iRandValue = rand() % 2;
			if (iRandValue)
				fX = _vWHD.x - m_pGameInstance->Compute_Random(0.f, _fSurface);
			else
				fX = 0.f + m_pGameInstance->Compute_Random(0.f, _fSurface);
		}
		else if (1 == iRandValue)
		{
			iRandValue = rand() % 2;
			if (iRandValue)
				fY = _vWHD.y - m_pGameInstance->Compute_Random(0.f, _fSurface);
			else
				fY = 0.f + m_pGameInstance->Compute_Random(0.f, _fSurface);
		}
		else if (2 == iRandValue)
		{
			iRandValue = rand() % 2;
			if (iRandValue)
				fZ = _vWHD.z - m_pGameInstance->Compute_Random(0.f, _fSurface);
			else
				fZ = 0.f + m_pGameInstance->Compute_Random(0.f, _fSurface);
		}

		fX -= _vWHD.x * _vMidPoint.x;
		fY -= _vWHD.y * _vMidPoint.y;
		fZ -= _vWHD.z * _vMidPoint.z;

	}
	else
	{
		fX = m_pGameInstance->Compute_Random(0.f, _vWHD.x) - _vWHD.x * _vMidPoint.x;
		fY = m_pGameInstance->Compute_Random(0.f, _vWHD.y) - _vWHD.y * _vMidPoint.y;
		fZ = m_pGameInstance->Compute_Random(0.f, _vWHD.z) - _vWHD.z * _vMidPoint.z;
	}

	vReturn = XMVectorSet(fX, fY, fZ, 1.f);

	return vReturn;
}

_vector CVIBuffer_Instance::Get_Torus_Pos(_float _fLargeRadius, _float _fHandleRadius, _float _fU, _float _fV)
{
	// Torus : 도넛 형태
	/* LargeRadius : 도넛의 큰 반지름 */
	/* HandleRadius : 도넛 링의 크기, */
	/* U : 큰 도넛의 각도 느낌? */
	/* V : 도넛 링의 각도 느낌? */
	_vector vReturn;

	_float fHandleAngle = XM_2PI - m_pGameInstance->Compute_Random(0.f, XM_2PI * (1.f - _fV));
	_vector vHandleLocal = XMVectorSet(cosf(fHandleAngle), sinf(fHandleAngle), 0.f, 1.f) * m_pGameInstance->Compute_Random(0.f, _fHandleRadius);
	
	_float fLargeAngle = XM_2PI - m_pGameInstance->Compute_Random(0.f, XM_2PI * (1.f - _fU));
	_matrix matTotal = XMMatrixIdentity();
	matTotal.r[3] = XMVectorSetX(matTotal.r[3], _fLargeRadius);
	matTotal = matTotal * XMMatrixRotationY(fLargeAngle);
	
	vReturn = XMVector3TransformCoord(vHandleLocal, matTotal);

	return vReturn;
}

_vector CVIBuffer_Instance::Get_Ring_Pos(_float _fRadius, _float _fCoverage, _float _fU)
{
	/* Coverage: 중앙으로 치고드는 정도, 1 = 원형 */
	/* fU : 링의 최대 각도 느낌? */
	_vector vReturn;

	_fRadius = (1.f - m_pGameInstance->Compute_Random(0.f, _fCoverage)) * _fRadius;
	
	_float fAngle = XM_2PI - m_pGameInstance->Compute_Random(0.f, XM_2PI * (1.f - _fU));	

	vReturn = XMVectorSet(_fRadius * cosf(fAngle), 0.f, _fRadius * sinf(fAngle), 1.f);
	return vReturn;
}

_vector CVIBuffer_Instance::Get_Cone_Pos(_float _fFlatten, _float _fLength, const _float3& _vAngle, _float _fSurface)
{
	/* Flatten : 평평하냐 vs 아니냐, _bool로 쓰임 */
	/* Length : 콘의 길이 */
	/* Angle : Angle, Inner Angle, Outer Angle,
		Angle : 콘의 커버 각도, 360도 이상일 때 전반을 다룸
		Inner Angle : 콘의 안 각도
		OuterAngle : 콘의 바깥 각도
	*/
	/* Surface : 아래에서부터 위까지 다루는 정도*/

	_vector vReturn;

	// 평평함
	if (0.5f < _fFlatten)
	{
		_float fY = _fLength * m_pGameInstance->Compute_Random(_fSurface, 1.f);
		_float fCoverAngle = m_pGameInstance->Compute_Random(XMConvertToRadians(_vAngle.y) * 0.5f, XMConvertToRadians(_vAngle.z) * 0.5f);
		_float fX = fY * tanf(clamp(fCoverAngle, -263.f, 263.f));
		vReturn = XMVectorSet(fX, fY, 0.f, 0.f);
	}

	// 원형
	else
	{
		_float  fLength = _fLength * m_pGameInstance->Compute_Random(_fSurface, 1.f);
		_float fCoverAngle = m_pGameInstance->Compute_Random(XMConvertToRadians(_vAngle.y) * 0.5f, XMConvertToRadians(_vAngle.z) * 0.5f);
		vReturn = XMVectorSet(sinf(fCoverAngle), cosf(fCoverAngle), 0.f, 0.f) * fLength;
	}
	

	vReturn = XMVector3Transform(vReturn, XMMatrixRotationY(m_pGameInstance->Compute_Random(0.f, XMConvertToRadians(_vAngle.x))));

	return vReturn;
}

_float3 CVIBuffer_Instance::Compute_ScaleValue()
{
	_float3 vScale;
	switch (m_SetDatas[P_SCALE])
	{
	case DIRECTSET:
		vScale = m_pSetScales[0];
		break;
	case RANDOMLINEAR:
		XMStoreFloat3(&vScale, XMVectorLerp(XMLoadFloat3(&m_pSetScales[0]), XMLoadFloat3(&m_pSetScales[1]), m_pGameInstance->Compute_Random_Normal()));
		break;
	case RANDOMRANGE:
		vScale = _float3(m_pGameInstance->Compute_Random(m_pSetScales[0].x, m_pSetScales[1].x),
			m_pGameInstance->Compute_Random(m_pSetScales[0].y, m_pSetScales[1].y),
			m_pGameInstance->Compute_Random(m_pSetScales[0].z, m_pSetScales[1].z));
		break;
	case UNSET:
	default:
		vScale = _float3(1.f, 1.f, 1.f);
		break;
	}


	return vScale;
}

_float3 CVIBuffer_Instance::Compute_RotationValue()
{
	_float3 vRotation;
	switch (m_SetDatas[P_ROTATION])
	{
	case DIRECTSET:
		vRotation = m_pSetRotations[0];
		break;
	case RANDOMLINEAR:
		XMStoreFloat3(&vRotation, XMVectorLerp(XMLoadFloat3(&m_pSetRotations[0]), XMLoadFloat3(&m_pSetRotations[1]), m_pGameInstance->Compute_Random_Normal()));
		break;
	case RANDOMRANGE:
		vRotation = _float3(m_pGameInstance->Compute_Random(m_pSetRotations[0].x, m_pSetRotations[1].x),
			m_pGameInstance->Compute_Random(m_pSetRotations[0].y, m_pSetRotations[1].y),
			m_pGameInstance->Compute_Random(m_pSetRotations[0].z, m_pSetRotations[1].z));
		break;
	case UNSET:
	default:
		vRotation = _float3(0.f, 0.f, 0.f);
		break;
	}


	return vRotation;
}

_float3 CVIBuffer_Instance::Compute_PositionValue()
{
	_float3 vPosition;
	switch (m_SetDatas[P_POSITION])
	{
	case DIRECTSET:
		vPosition = m_pSetPositions[0];
		break;
	case RANDOMLINEAR:
		XMStoreFloat3(&vPosition, XMVectorLerp(XMLoadFloat3(&m_pSetPositions[0]), XMLoadFloat3(&m_pSetPositions[1]), m_pGameInstance->Compute_Random_Normal()));
		break;
	case RANDOMRANGE:
		vPosition = _float3(m_pGameInstance->Compute_Random(m_pSetPositions[0].x, m_pSetPositions[1].x),
			m_pGameInstance->Compute_Random(m_pSetPositions[0].y, m_pSetPositions[1].y),
			m_pGameInstance->Compute_Random(m_pSetPositions[0].z, m_pSetPositions[1].z));
		break;
	case UNSET:
	default:
		vPosition = _float3(0.f, 0.f, 0.f);
		break;
	}


	return vPosition;
}

_float CVIBuffer_Instance::Compute_LifeTime()
{
	_float vLifeTime;
	switch (m_SetDatas[P_LIFETIME])
	{
	case DIRECTSET:
		vLifeTime = m_pSetLifeTimes[0];
		break;
	case RANDOMLINEAR:
	case RANDOMRANGE:
		vLifeTime = m_pGameInstance->Compute_Random(m_pSetLifeTimes[0], m_pSetLifeTimes[1]);
		break;
	case UNSET:
		vLifeTime = D3D11_FLOAT32_MAX;
		break;
	default:
		vLifeTime = 1.f;
		break;
	}


	return vLifeTime;
}

_float4 CVIBuffer_Instance::Compute_ColorValue()
{
	_float4 vColor;
	switch (m_SetDatas[P_COLOR])
	{
	case DIRECTSET:
		vColor = m_pSetColors[0];
		break;
	case RANDOMLINEAR:
		XMStoreFloat4(&vColor, XMVectorLerp(XMLoadFloat4(&m_pSetColors[0]), XMLoadFloat4(&m_pSetColors[1]), m_pGameInstance->Compute_Random_Normal()));
		break;
	case RANDOMRANGE:
		vColor = _float4(m_pGameInstance->Compute_Random(m_pSetColors[0].x, m_pSetColors[1].x),
			m_pGameInstance->Compute_Random(m_pSetColors[0].y, m_pSetColors[1].y),
			m_pGameInstance->Compute_Random(m_pSetColors[0].z, m_pSetColors[1].z),
			m_pGameInstance->Compute_Random(m_pSetColors[0].w, m_pSetColors[1].w));
		break;
	case UNSET:
	default:
		vColor = _float4(1.f, 1.f, 1.f, 1.f);
		break;
	}


	return vColor;
}


HRESULT CVIBuffer_Instance::Set_FloatData(PARTICLE_DATA eData, const json& _jsonInfo, const _char* _szTag, _float* _pSaveDatas)
{
	if (false == _jsonInfo.contains(_szTag))
		return E_FAIL;
	else
	{
		if (_jsonInfo[_szTag].contains("SetType"))
		{
			m_SetDatas[eData] = _jsonInfo[_szTag]["SetType"];
			if (false == _jsonInfo[_szTag].contains("Arg1"))
				return E_FAIL;
			_pSaveDatas[0] = _jsonInfo[_szTag]["Arg1"];
			if (_jsonInfo[_szTag].contains("Arg2"))
				_pSaveDatas[1] = _jsonInfo[_szTag]["Arg2"];
			else
				_pSaveDatas[1] = _jsonInfo[_szTag]["Arg1"];
		}
	}

	return S_OK;
}

HRESULT CVIBuffer_Instance::Set_Float2Data(PARTICLE_DATA eData, const json& _jsonInfo, const _char* _szTag, _float2* _pSaveDatas)
{
	if (false == _jsonInfo.contains(_szTag))
		return E_FAIL;
	else
	{
		if (_jsonInfo[_szTag].contains("SetType"))
		{
			m_SetDatas[eData] = _jsonInfo[_szTag]["SetType"];
			if (false == _jsonInfo[_szTag].contains("Arg1"))
				return E_FAIL;

			_float2 vArg = { 0.f, 0.f  };
			for (_int i = 0; i < 2; ++i)
				*((_float*)(&vArg) + i) = _jsonInfo[_szTag]["Arg1"][i];
			_pSaveDatas[0] = vArg;

			if (_jsonInfo[_szTag].contains("Arg2"))
			{
				for (_int i = 0; i < 2; ++i)
					*((_float*)(&vArg) + i) = _jsonInfo[_szTag]["Arg2"][i];
			}
			_pSaveDatas[1] = vArg;
		}
	}


	return S_OK;
}

HRESULT CVIBuffer_Instance::Set_Float3Data(PARTICLE_DATA eData, const json& _jsonInfo, const _char* _szTag, _float3* _pSaveDatas)
{
	if (false == _jsonInfo.contains(_szTag))
		return E_FAIL;
	else
	{
		if (_jsonInfo[_szTag].contains("SetType"))
		{
			m_SetDatas[eData] = _jsonInfo[_szTag]["SetType"];
			if (false == _jsonInfo[_szTag].contains("Arg1"))
				return E_FAIL;

			_float3 vArg = { 0.f, 0.f, 0.f };
			for (_int i = 0; i < 3; ++i)
				*((_float*)(&vArg) + i) = _jsonInfo[_szTag]["Arg1"][i];
			_pSaveDatas[0] = vArg;

			if (_jsonInfo[_szTag].contains("Arg2"))
			{
				for (_int i = 0; i < 3; ++i)
					*((_float*)(&vArg) + i) = _jsonInfo[_szTag]["Arg2"][i];
			}
			_pSaveDatas[1] = vArg;

		}
	}



	return S_OK;
}

HRESULT CVIBuffer_Instance::Set_Float4Data(PARTICLE_DATA eData, const json& _jsonInfo, const _char* _szTag, _float4* _pSaveDatas)
{
	if (false == _jsonInfo.contains(_szTag))
		return E_FAIL;
	else
	{
		if (_jsonInfo[_szTag].contains("SetType"))
		{
			m_SetDatas[eData] = _jsonInfo[_szTag]["SetType"];
			if (false == _jsonInfo[_szTag].contains("Arg1"))
				return E_FAIL;

			_float4 vArg = { 0.f, 0.f, 0.f, 0.f };
			for (_int i = 0; i < 4; ++i)
				*((_float*)(&vArg) + i) = _jsonInfo[_szTag]["Arg1"][i];
			_pSaveDatas[0] = vArg;

			if (_jsonInfo[_szTag].contains("Arg2"))
			{
				for (_int i = 0; i < 4; ++i)
					*((_float*)(&vArg) + i) = _jsonInfo[_szTag]["Arg2"][i];
			}
			_pSaveDatas[1] = vArg;
				
		}
	}


	return S_OK;
}



void CVIBuffer_Instance::Free()
{
	Safe_Release(m_pBuffer);
	Safe_Release(m_pBufferInitial);

	Safe_Release(m_pUAV);
	Safe_Release(m_pSRVInitial);
	Safe_Release(m_pSRV);


	Safe_Release(m_pVBInstance);
	if (false == m_isCloned)
	{
		Safe_Delete_Array(m_pSetScales);
		Safe_Delete_Array(m_pSetRotations);
		Safe_Delete_Array(m_pSetPositions);
		Safe_Delete_Array(m_pSetLifeTimes);
		Safe_Delete_Array(m_pSetColors);
	}
	

	__super::Free();
}

#ifdef _DEBUG

_bool CVIBuffer_Instance::s_isShapeChange = false;
_int CVIBuffer_Instance::s_iShapeFlags = 0;

void CVIBuffer_Instance::Tool_Setting()
{
	s_isShapeChange = true;
	s_iShapeFlags = m_eShapeType;
}

void CVIBuffer_Instance::Tool_Update(_float _fTimeDelta)
{
}
HRESULT CVIBuffer_Instance::Save_Buffer(json& _jsonBufferInfo)
{
	_jsonBufferInfo["Count"] = m_iNumInstances;

	_jsonBufferInfo["ShapeLocation"]["Shape"] = m_eShapeType;
	for (auto& ShapeData : m_ShapeDatas)
	{
		_jsonBufferInfo["ShapeLocation"][ShapeData.first.c_str()] = ShapeData.second;
	}

	for (_int i = 0; i < 3; ++i)
	{
		_jsonBufferInfo["ShapeLocation"]["Scale"].push_back(*((_float*)(&m_vShapeScale) + i));
		_jsonBufferInfo["ShapeLocation"]["Rotation"].push_back(*((_float*)(&m_vShapeRotation) + i));
		_jsonBufferInfo["ShapeLocation"]["Position"].push_back(*((_float*)(&m_vShapePosition) + i));

		_jsonBufferInfo["Scale"]["Arg1"].push_back(*((_float*)(&m_pSetScales[0]) + i));
		_jsonBufferInfo["Scale"]["Arg2"].push_back(*((_float*)(&m_pSetScales[1]) + i));
		_jsonBufferInfo["Rotation"]["Arg1"].push_back(*((_float*)(&m_pSetRotations[0]) + i));
		_jsonBufferInfo["Rotation"]["Arg2"].push_back(*((_float*)(&m_pSetRotations[1]) + i));
		_jsonBufferInfo["Position"]["Arg1"].push_back(*((_float*)(&m_pSetPositions[0]) + i));
		_jsonBufferInfo["Position"]["Arg2"].push_back(*((_float*)(&m_pSetPositions[1]) + i));
		_jsonBufferInfo["Color"]["Arg1"].push_back(*((_float*)(&m_pSetColors[0]) + i));
		_jsonBufferInfo["Color"]["Arg2"].push_back(*((_float*)(&m_pSetColors[1]) + i));
	}

	//_jsonBufferInfo["Spawn"]["Type"] = m_eSpawnType;
	//_jsonBufferInfo["Spawn"]["Time"] = m_fSpawnTime;
	//_jsonBufferInfo["Spawn"]["Count"] = m_iSpawnCount;

	_jsonBufferInfo["Scale"]["SetType"] = m_SetDatas[P_SCALE];
	_jsonBufferInfo["Rotation"]["SetType"] = m_SetDatas[P_ROTATION];
	_jsonBufferInfo["Position"]["SetType"] = m_SetDatas[P_POSITION];
	_jsonBufferInfo["LifeTime"]["SetType"] = m_SetDatas[P_LIFETIME];
	_jsonBufferInfo["Color"]["SetType"] = m_SetDatas[P_COLOR];

	_jsonBufferInfo["LifeTime"]["Arg1"] = m_pSetLifeTimes[0];
	_jsonBufferInfo["LifeTime"]["Arg2"] = m_pSetLifeTimes[1];
	_jsonBufferInfo["Color"]["Arg1"].push_back(m_pSetColors[0].w);
	_jsonBufferInfo["Color"]["Arg2"].push_back(m_pSetColors[1].w);


	return S_OK;
}
void CVIBuffer_Instance::Tool_Adjust_DefaultData()
{
	if (ImGui::TreeNode("Defaults"))
	{
	
		if (ImGui::TreeNode("Change Counts"))
		{
			if (ImGui::InputInt("Instance Count", (_int*)&m_iNumInstances))
			{
				m_isToolChanged = true;
			}

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Particle Scaling"))
		{
			const _char* items[] = { "UnSet", "Direct Set", "Random_Linear", "Random_Range" };

			static _int item_selected_idx = 0;
			const char* combo_preview_value = items[item_selected_idx];

			if (ImGui::BeginCombo("Scale Type", combo_preview_value))
			{
				item_selected_idx = m_SetDatas[P_SCALE];
				ImGui::SetItemDefaultFocus();

				for (int n = 0; n < IM_ARRAYSIZE(items); n++)
				{
					const bool is_selected = (item_selected_idx == n);

					if (ImGui::Selectable(items[n], is_selected))
					{
						item_selected_idx = n;
						if (m_SetDatas[P_SCALE] != n)
						{
							m_SetDatas[P_SCALE] = (SETTING_TYPE)n;
							m_isToolChanged = true;
						}
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			switch (m_SetDatas[P_SCALE])
			{
			case UNSET:
				break;
			case DIRECTSET:
			{
				if (ImGui::InputFloat3("Set_Scale", (_float*)(&m_pSetScales[0])))
				{
					for (_int i = 0; i < 3; ++i)
					{
						if (0.f == *((_float*)(&m_pSetScales[0]) + i))
						{
							*((_float*)(&m_pSetScales[0]) + i) = 1.f;
						}
					}
					m_isToolChanged = true;
				}
				break;
			}
			case RANDOMLINEAR:
			case RANDOMRANGE:
			{
				if (ImGui::InputFloat3("Set_Scale1", (_float*)(&m_pSetScales[0])))
				{
					for (_int i = 0; i < 3; ++i)
					{
						if (0.f == *((_float*)(&m_pSetScales[0]) + i))
						{
							*((_float*)(&m_pSetScales[0]) + i) = 1.f;
						}
					}
					m_isToolChanged = true;
				}
				if (ImGui::InputFloat3("Set_Scale2", (_float*)(&m_pSetScales[1])))
				{
					for (_int i = 0; i < 3; ++i)
					{
						if (0.f == *((_float*)(&m_pSetScales[1]) + i))
						{
							*((_float*)(&m_pSetScales[1]) + i) = 1.f;
						}
					}
					m_isToolChanged = true;
				}
				break;
			}
			}
			

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Particle Rotation"))
		{
			const _char* items[] = { "UnSet", "Direct Set", "Random_Linear", "Random_Range" };

			static _int item_selected_idx = 0;
			const char* combo_preview_value = items[item_selected_idx];

			if (ImGui::BeginCombo("Rotation Type", combo_preview_value))
			{
				item_selected_idx = m_SetDatas[P_ROTATION];
				ImGui::SetItemDefaultFocus();

				for (int n = 0; n < IM_ARRAYSIZE(items); n++)
				{
					const bool is_selected = (item_selected_idx == n);

					if (ImGui::Selectable(items[n], is_selected))
					{
						item_selected_idx = n;
						if (m_SetDatas[P_ROTATION] != n)
						{
							m_SetDatas[P_ROTATION] = (SETTING_TYPE)n;
							m_isToolChanged = true;
						}
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			switch (m_SetDatas[P_ROTATION])
			{
			case UNSET:
				break;
			case DIRECTSET:
			{
				if (ImGui::InputFloat3("Set_Rotation", (_float*)(&m_pSetRotations[0])))
				{
					m_isToolChanged = true;
				}
				break;
			}
			case RANDOMLINEAR:
			case RANDOMRANGE:
			{
				if (ImGui::InputFloat3("Set_Rotation1", (_float*)(&m_pSetRotations[0])))
				{
					m_isToolChanged = true;
				}
				if (ImGui::InputFloat3("Set_Rotation2", (_float*)(&m_pSetRotations[1])))
				{
					m_isToolChanged = true;
				}
				break;
			}
			}


			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Particle Position"))
		{
			const _char* items[] = { "UnSet", "Direct Set", "Random_Linear", "Random_Range" };
			static _int item_selected_idx = 0;
			const char* combo_preview_value = items[item_selected_idx];

			if (ImGui::BeginCombo("Position Type", combo_preview_value))
			{
				item_selected_idx = m_SetDatas[P_POSITION];
				ImGui::SetItemDefaultFocus();

				for (int n = 0; n < IM_ARRAYSIZE(items); n++)
				{
					const bool is_selected = (item_selected_idx == n);

					if (ImGui::Selectable(items[n], is_selected))
					{
						item_selected_idx = n;
						if (m_SetDatas[P_POSITION] != n)
						{
							m_SetDatas[P_POSITION] = (SETTING_TYPE)n;
							m_isToolChanged = true;
						}
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			switch (m_SetDatas[P_POSITION])
			{
			case UNSET:
				break;
			case DIRECTSET:
			{
				if (ImGui::InputFloat3("Set_Position", (_float*)(&m_pSetPositions[0])))
				{
					m_isToolChanged = true;
				}
				break;
			}
			case RANDOMLINEAR:
			case RANDOMRANGE:
			{
				if (ImGui::InputFloat3("Set_Position1", (_float*)(&m_pSetPositions[0])))
				{
					m_isToolChanged = true;
				}
				if (ImGui::InputFloat3("Set_Position2", (_float*)(&m_pSetPositions[1])))
				{
					m_isToolChanged = true;
				}
				break;
			}
			}


			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Particle Color"))
		{
			const _char* items[] = { "UnSet", "Direct Set", "Random_Linear", "Random_Range" };
			static _int item_selected_idx = 0;
			const char* combo_preview_value = items[item_selected_idx];

			if (ImGui::BeginCombo("Color Type", combo_preview_value))
			{
				item_selected_idx = m_SetDatas[P_COLOR];
				ImGui::SetItemDefaultFocus();

				for (int n = 0; n < IM_ARRAYSIZE(items); n++)
				{
					const bool is_selected = (item_selected_idx == n);

					if (ImGui::Selectable(items[n], is_selected))
					{
						item_selected_idx = n;
						if (m_SetDatas[P_COLOR] != n)
						{
							m_SetDatas[P_COLOR] = (SETTING_TYPE)n;
							m_isToolChanged = true;
						}
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			switch (m_SetDatas[P_COLOR])
			{
			case UNSET:
				break;
			case DIRECTSET:
			{
				if (ImGui::InputFloat4("Set_Color", (_float*)(&m_pSetColors[0])))
				{
					m_isToolChanged = true;
				}
				break;
			}
			case RANDOMLINEAR:
			case RANDOMRANGE:
			{
				if (ImGui::InputFloat4("Set_Color1", (_float*)(&m_pSetColors[0])))
				{
					m_isToolChanged = true;
				}
				if (ImGui::InputFloat4("Set_Color2", (_float*)(&m_pSetColors[1])))
				{
					m_isToolChanged = true;
				}
				break;
			}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Particle LifeTime"))
		{
			const _char* items[] = { "Infinite", "Direct Set", "Random" };
			static _int item_selected_idx = 0;
			const char* combo_preview_value = items[item_selected_idx];

			if (ImGui::BeginCombo("LifeTime Type", combo_preview_value))
			{
				item_selected_idx = m_SetDatas[P_LIFETIME];
				ImGui::SetItemDefaultFocus();

				for (int n = 0; n < IM_ARRAYSIZE(items); n++)
				{
					const bool is_selected = (item_selected_idx == n);

					if (ImGui::Selectable(items[n], is_selected))
					{
						item_selected_idx = n;
						if (m_SetDatas[P_LIFETIME] != n)
						{
							m_SetDatas[P_LIFETIME] = (SETTING_TYPE)n;
							m_isToolChanged = true;
						}
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			switch (m_SetDatas[P_LIFETIME])
			{
			case UNSET:
				break;
			case DIRECTSET:
			{
				if (ImGui::InputFloat("Set_LifeTime", (_float*)(&m_pSetLifeTimes[0])))
				{
					m_isToolChanged = true;
				}
				break;
			}
			case RANDOMLINEAR:
			case RANDOMRANGE:
			{
				if (ImGui::InputFloat("Set_LifeTimeMin", (_float*)(&m_pSetLifeTimes[0])))
				{
					m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Set_LifeTimeMax", (_float*)(&m_pSetLifeTimes[1])))
				{
					m_isToolChanged = true;
				}
				break;
			}
			}


			ImGui::TreePop();
		}
		
		ImGui::TreePop();
	}

}
void CVIBuffer_Instance::Tool_Adjust_Shape()
{
	if (ImGui::TreeNode("Shape"))
	{
		const _char* items[] = { "Sphere", "Cylinder", "Box", "Torus", "Ring", "Cone", "None" };
		static _int item_selected_idx = 0;
		const char* combo_preview_value = items[item_selected_idx];

		if (s_isShapeChange)
		{
			item_selected_idx = s_iShapeFlags;
			ImGui::SetItemDefaultFocus();
		}

		if (ImGui::BeginCombo("Select Shape", combo_preview_value))
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				const bool is_selected = (item_selected_idx == n);				

				if (ImGui::Selectable(items[n], is_selected))
				{
					item_selected_idx = n;
					if (m_eShapeType != n)
					{
						m_eShapeType = (SHAPE_TYPE)n;
						Tool_Create_ShapeData();
					}

				}

				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		

		switch (m_eShapeType)
		{
		case SPHERE:
		{
			if (ImGui::DragFloat("Radius", &m_ShapeDatas["Sphere_Radius"], 0.1f))
			{
				m_isToolChanged = true;
			}

			if (ImGui::DragFloat("Surface", &m_ShapeDatas["Sphere_Surface"], 0.1f))
			{
				m_isToolChanged = true;
			}

			if (ImGui::TreeNode("Input Section"))
			{
				if (ImGui::InputFloat("Radius", &m_ShapeDatas["Sphere_Radius"]))
				{
					m_isToolChanged = true;
				}

				if (ImGui::InputFloat("Surface", &m_ShapeDatas["Sphere_Surface"]))
				{
					m_ShapeDatas["Sphere_Surface"] = clamp(m_ShapeDatas["Sphere_Surface"], 0.f, 0.9999f);

					m_isToolChanged = true;
				}


				ImGui::TreePop();
			}


			break;
		}
		case CYLINDER:
		{
			if (ImGui::DragFloat("Radius", &m_ShapeDatas["Cylinder_Radius"], 0.1f))
			{
				m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Height", &m_ShapeDatas["Cylinder_Height"], 0.1f))
			{
				m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Mid Point", &m_ShapeDatas["MidPoint"], 0.1f))
			{
				m_isToolChanged = true;
			}

			if (ImGui::TreeNode("Input Section"))
			{
				if (ImGui::InputFloat("Radius", &m_ShapeDatas["Cylinder_Radius"]))
				{
					m_isToolChanged = true;
				}

				if (ImGui::InputFloat("Height", &m_ShapeDatas["Cylinder_Height"]))
				{
					m_isToolChanged = true;
				}

				if (ImGui::InputFloat("Mid Point", &m_ShapeDatas["MidPoint"]))
				{
					m_isToolChanged = true;
				}

				ImGui::TreePop();
			}


			break;
		}
		case BOX:
		{
			if (ImGui::DragFloat("Width", &m_ShapeDatas["Width"], 0.1f))
			{
				m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Height", &m_ShapeDatas["Height"], 0.1f))
			{
				m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Depth", &m_ShapeDatas["Depth"], 0.1f))
			{
				m_isToolChanged = true;
			}
			if (ImGui::DragFloat("MidX", &m_ShapeDatas["MidX"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("MidY", &m_ShapeDatas["MidY"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("MidZ", &m_ShapeDatas["MidZ"], 0.1f))
			{
					m_isToolChanged = true;
			}

			_bool isSurface = Is_ShapeData("Box_Surface");
			if (ImGui::RadioButton("Surface_On", isSurface))
			{
				if (isSurface)
				{
					m_ShapeDatas.erase("Box_Surface");
					if (Is_ToolData("Box_Surface"))
					{
						m_ToolShapeDatas.erase("Box_Surface");
					}
				}
				else
				{
					m_ShapeDatas.emplace("Box_Surface", 0.f);
				}

					m_isToolChanged = true;
			}
			isSurface = Is_ShapeData("Box_Surface");

			if (isSurface)
			{
				if (ImGui::DragFloat("Surface", &m_ShapeDatas["Box_Surface"], 0.1f))
				{
						m_isToolChanged = true;
				}
			}
	

			if (ImGui::TreeNode("Input Section"))
			{
				if (ImGui::InputFloat("Width", &m_ShapeDatas["Width"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Height", &m_ShapeDatas["Height"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Depth", &m_ShapeDatas["Depth"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("MidX", &m_ShapeDatas["MidX"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("MidY", &m_ShapeDatas["MidY"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("MidZ", &m_ShapeDatas["MidZ"]))
				{
						m_isToolChanged = true;
				}

				if (isSurface)
				{
					if (ImGui::InputFloat("Surface", &m_ShapeDatas["Box_Surface"]))
					{
							m_isToolChanged = true;
					}
				}

				ImGui::TreePop();
			}

			break;
		}
		case TORUS:
		{
			if (ImGui::DragFloat("Large_Radius", &m_ShapeDatas["LargeRadius"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Handle_Radius", &m_ShapeDatas["HandleRadius"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("U_Distribution", &m_ShapeDatas["U_Distribution"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("V_Distribution", &m_ShapeDatas["V_Distribution"], 0.1f))
			{
					m_isToolChanged = true;
			}

			

			if (ImGui::TreeNode("Input Section"))
			{
				if (ImGui::InputFloat("Large_Radius", &m_ShapeDatas["LargeRadius"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Handle_Radius", &m_ShapeDatas["HandleRadius"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("U_Distribution", &m_ShapeDatas["U_Distribution"]))
				{
					m_ShapeDatas["U_Distribution"] = clamp(m_ShapeDatas["U_Distribution"], 0.f, 0.9999f);

						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("V_Distribution", &m_ShapeDatas["V_Distribution"]))
				{
					m_ShapeDatas["V_Distribution"] = clamp(m_ShapeDatas["V_Distribution"], 0.f, 0.9999f);

						m_isToolChanged = true;
				}

				ImGui::TreePop();

			}

			break;
		}
		case RING:
		{
			if (ImGui::DragFloat("Radius", &m_ShapeDatas["Ring_Radius"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Coverage", &m_ShapeDatas["Coverage"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Ring_Distribution", &m_ShapeDatas["Ring_Distribution"], 0.1f))
			{
					m_isToolChanged = true;
			}

			if (ImGui::TreeNode("Input Section"))
			{
				if (ImGui::InputFloat("Radius", &m_ShapeDatas["Ring_Radius"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Coverage", &m_ShapeDatas["Coverage"]))
				{
					m_ShapeDatas["Coverage"] = clamp(m_ShapeDatas["Coverage"], 0.f, 0.9999f);

						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Ring_Distribution", &m_ShapeDatas["Ring_Distribution"]))
				{
					m_ShapeDatas["Ring_Distribution"] = clamp(m_ShapeDatas["Ring_Distribution"], 0.f, 0.9999f);

						m_isToolChanged = true;
				}

				ImGui::TreePop();

			}
			break;
		}

		case CONE:
		{
			if (ImGui::DragFloat("Length", &m_ShapeDatas["Length"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Outer_Angle", &m_ShapeDatas["Outer_Angle"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Inner_Angle", &m_ShapeDatas["Inner_Angle"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Radial_Angle", &m_ShapeDatas["Radial_Angle"], 0.1f))
			{
					m_isToolChanged = true;
			}
			if (ImGui::DragFloat("Surface", &m_ShapeDatas["Cone_Surface"], 0.1f))
			{
					m_isToolChanged = true;
			}
			_bool isFlatten = m_ShapeDatas["Flatten"] > 0.5f ? true : false;
			if (ImGui::RadioButton("Flatten", isFlatten))
			{
				if (isFlatten)
					m_ShapeDatas["Flatten"] = 0.f;
				else
					m_ShapeDatas["Flatten"] = 1.f;

					m_isToolChanged = true;
			}

			if (ImGui::TreeNode("Input Section"))
			{
				if (ImGui::InputFloat("Length", &m_ShapeDatas["Length"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Outer_Angle", &m_ShapeDatas["Outer_Angle"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Inner_Angle", &m_ShapeDatas["Inner_Angle"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Radial_Angle", &m_ShapeDatas["Radial_Angle"]))
				{
						m_isToolChanged = true;
				}
				if (ImGui::InputFloat("Surface", &m_ShapeDatas["Cone_Surface"]))
				{
					m_ShapeDatas["Cone_Surface"] = clamp(m_ShapeDatas["Cone_Surface"], 0.f, 0.9999f);

						m_isToolChanged = true;
				}

				ImGui::TreePop();

			}


			break;
		}
		case SHAPE_NONE:
		{
			break;
		}

		}

		if (SHAPE_NONE != m_eShapeType)
		{
			if (ImGui::InputFloat3("Shape_Scale", (_float*)(&m_vShapeScale)))
			{
				for (_int i = 0; i < 3; ++i)
				{
					if (0.f == *((_float*)(&m_vShapeScale) + i))
					{
						*((_float*)(&m_vShapeScale) + i) = 1.f;
					}
						m_isToolChanged = true;
				}
			}

			if (ImGui::InputFloat3("Shape_Rotation", (_float*)(&m_vShapeRotation)))
			{
					m_isToolChanged = true;
			}


			if (ImGui::InputFloat3("Shape_Position", (_float*)(&m_vShapePosition)))
			{
					m_isToolChanged = true;
			}


		}


		s_isShapeChange = false;
		ImGui::TreePop();
	}
}


void CVIBuffer_Instance::Tool_Create_ShapeData()
{
	for (auto& Pair : m_ShapeDatas)
	{
		m_ToolShapeDatas[Pair.first] = Pair.second;
	}

	m_ShapeDatas.clear();

	switch (m_eShapeType)
	{
	case SPHERE:
	{
		Tool_InsertData("Sphere_Radius", 1.f);
		Tool_InsertData("Sphere_Surface", 0.f);
		
		break;
	}
	case CYLINDER:
	{
		Tool_InsertData("Cylinder_Radius", 1.f);
		Tool_InsertData("Cylinder_Height", 1.f);
		Tool_InsertData("MidPoint", 0.5f);

		break;
	}
	case BOX:
	{
		Tool_InsertData("Width", 1.f);
		Tool_InsertData("Height", 1.f);
		Tool_InsertData("Depth", 1.f);
		Tool_InsertData("MidX", 0.5f);
		Tool_InsertData("MidY", 0.5f);
		Tool_InsertData("MidZ", 0.5f);
		
		if (Is_ToolData("Box_Surface"))
		{
			Tool_InsertData("Box_Surface", 0.f);
		}


		break;
	}
	case TORUS:
	{
		Tool_InsertData("LargeRadius", 3.f);
		Tool_InsertData("HandleRadius", 1.f);
		Tool_InsertData("U_Distribution", 0.f);
		Tool_InsertData("V_Distribution", 0.f);

		break;
	}
	case RING:
	{
		Tool_InsertData("Ring_Radius", 3.f);
		Tool_InsertData("Coverage", 0.f);
		Tool_InsertData("Ring_Distribution", 0.f);
		break;
	}
	case CONE:
	{
		Tool_InsertData("Length", 3.f);
		Tool_InsertData("Outer_Angle", 90.f);
		Tool_InsertData("Inner_Angle", 0.f);
		Tool_InsertData("Radial_Angle", 360.f);
		Tool_InsertData("Cone_Surface", 0.f);
		Tool_InsertData("Flatten", 0.f);

		break;
	}
	case SHAPE_NONE:
		break;
	
	}

		m_isToolChanged = true;

}

#endif