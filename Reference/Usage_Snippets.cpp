/*==============================================================================
    Usage_Snippets.cpp

    파티클 시스템이 실제 게임 코드에서 어떻게 등록되고 사용되는지 보여주는
    참고용 발췌본입니다. 이 파일은 빌드 대상이 아니며, 원본 위치는 각 블록의
    주석에 표시되어 있습니다.
==============================================================================*/

//------------------------------------------------------------------------------
// 1) 셰이더 / 컴퓨트 셰이더 프로토타입 등록
//    원본: Client/Private/Loader.cpp  (Loading_For_Static)
//------------------------------------------------------------------------------

/* Sprite 파티클 렌더 셰이더 (Point 인스턴싱) */
if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxPointInstance"),
    CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPointInstance.hlsl"),
        VTXPOINTPARTICLE::Elements, VTXPOINTPARTICLE::iNumElements))))
    return E_FAIL;

/* Mesh 파티클 렌더 셰이더 (Mesh 인스턴싱) */
if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxMeshInstance"),
    CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxMeshInstance.hlsl"),
        VTXMESHID::Elements, VTXMESHID::iNumElements))))
    return E_FAIL;

/* 단일 메시 이펙트 셰이더 */
if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxMeshEffect"),
    CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_Effect.hlsl"),
        VTXMESH::Elements, VTXMESH::iNumElements))))
    return E_FAIL;

/* 단일 스프라이트(Point) 셰이더 */
if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxPoint"),
    CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPoint.hlsl"),
        VTXPOINT::Elements, VTXPOINT::iNumElements))))
    return E_FAIL;

/* GPU 시뮬레이션용 컴퓨트 셰이더 - Mesh */
if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Compute_Shader_MeshInstance"),
    CCompute_Shader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_CS_Mesh.hlsl")))))
    return E_FAIL;

/* GPU 시뮬레이션용 컴퓨트 셰이더 - Sprite */
if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Compute_Shader_SpriteInstance"),
    CCompute_Shader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_CS_Sprite.hlsl")))))
    return E_FAIL;


//------------------------------------------------------------------------------
// 2) JSON 데이터로부터 이펙트 프로토타입 일괄 생성
//    원본: Client/Private/Loader.cpp  (CLoader::Load_Directory_Effects)
//------------------------------------------------------------------------------

HRESULT CLoader::Load_Directory_Effects(LEVEL_ID _iLevID, const _tchar* _szJsonFilePath)
{
    std::filesystem::path path;

    json jsonLoadInfo;
    m_pGameInstance->Load_Json(_szJsonFilePath, &jsonLoadInfo);

    for (_int i = 0; i < jsonLoadInfo["Path"].size(); ++i)
    {
        path = STRINGTOWSTRING(jsonLoadInfo["Path"][i]);

        /* JSON 한 개 = CEffect_System 프로토타입 한 개.
           내부의 Emitter / Module / Buffer 구성이 전부 JSON에서 복원된다. */
        CEffect_System* pParticleSystem = CEffect_System::Create(m_pDevice, m_pContext, path.c_str());

        if (FAILED(m_pGameInstance->Add_Prototype(_iLevID, path.filename(), pParticleSystem)))
            return E_FAIL;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
// 3) 게임 오브젝트에서 이펙트 시스템 붙이기
//    원본: Client/Private/Portal.cpp  (CPortal::Ready_DefaultParticle)
//    이 코드가 불러오는 Portal.json 은 Samples/ 에 들어 있습니다.
//------------------------------------------------------------------------------

HRESULT CPortal::Ready_DefaultParticle()
{
    if (false == m_isReady_3D)
        return E_FAIL;

    Safe_Release(m_pDefaultEffect);
    Safe_Release(m_PartObjects[PORTAL_PART_DEFAULTEFFECT]);

    _vector f3DPosition = Get_FinalPosition(COORDINATE_3D);

    CEffect_System::EFFECT_SYSTEM_DESC EffectDesc = {};
    EffectDesc.eStartCoord         = COORDINATE_3D;
    EffectDesc.isCoordChangeEnable = false;

    /* Portal.json 은 SPRITE / EFFECT 이미터만 사용하므로 두 종류의 태그만 지정한다.
       MESH, SINGLE_SPRITE 이미터를 쓰는 이펙트라면 해당 셰이더/버퍼 태그도 함께 채워야 한다. */
    EffectDesc.iSpriteShaderLevel       = LEVEL_STATIC;
    EffectDesc.szSpriteShaderTags       = L"Prototype_Component_Shader_VtxPointInstance";
    EffectDesc.iEffectShaderLevel       = LEVEL_STATIC;
    EffectDesc.szEffectShaderTags       = L"Prototype_Component_Shader_VtxMeshEffect";

    /* GPU 시뮬레이션용 컴퓨트 셰이더 태그 */
    EffectDesc.szSpriteComputeShaderTag = L"Prototype_Component_Compute_Shader_SpriteInstance";

    /* Loader가 등록해 둔 "Portal.json" 프로토타입을 복제한다. */
    m_pDefaultEffect = static_cast<CEffect_System*>(
        m_pGameInstance->Clone_Prototype(PROTOTYPE::PROTO_GAMEOBJ, LEVEL_STATIC,
            TEXT("Portal.json"), &EffectDesc));
    if (nullptr == m_pDefaultEffect)
        return E_FAIL;

    /* ... 포탈이 붙은 벽의 법선 방향에 맞춰 WorldMatrix를 회전시키는 부분 생략 ... */
    _matrix WorldMatrix = XMMatrixIdentity();
    WorldMatrix.r[3] = XMVectorSetW(f3DPosition, 1.f);

    if (nullptr != m_pDefaultEffect)
    {
        /* 이펙트 전체를 이 행렬 위에 올린다. */
        m_pDefaultEffect->Set_EffectMatrix(WorldMatrix);
        m_PartObjects[PORTAL_PART_DEFAULTEFFECT] = m_pDefaultEffect;
        Safe_AddRef(m_pDefaultEffect);
    }

    return S_OK;
}


//------------------------------------------------------------------------------
// 4) 런타임 재생 제어
//    원본: Client/Private/Portal.cpp
//------------------------------------------------------------------------------

/* 매 프레임, 꺼져 있으면 다시 재생시킨다 (포탈은 상시 이펙트) */
void CPortal::Update(_float fTimeDelta)
{
    if (nullptr != m_pDefaultEffect && false == m_pDefaultEffect->Is_Active())
        m_pDefaultEffect->Active_Effect(false);

    __super::Update(fTimeDelta);
}

/* 오브젝트가 켜질 때 — EventID 0번 이미터만 재생 */
void CPortal::Active_OnEnable()
{
    if (m_isFirstActive)
    {
        __super::Active_OnEnable();
        if (m_pDefaultEffect)
            m_pDefaultEffect->Active_Effect(false, 0);
    }
    // ...
}

/* 플레이어가 포탈을 통과하는 순간 — 다른 이펙트의 EventID 1번 이미터를 재생 */
void CPortal::Use_Portal(CPlayer* _pUser)
{
    // ... 좌표계 전환 처리 생략 ...

    if (nullptr != m_pInOutEffect)
        m_pInOutEffect->Active_Effect(false, 1);
}

/* 오브젝트가 꺼질 때 — 전체 비활성 */
void CPortal::Active_OnDisable()
{
    __super::Active_OnDisable();

    if (m_pDefaultEffect && m_pDefaultEffect->Is_Active())
        m_pDefaultEffect->Inactive_All();

    if (m_pInOutEffect && m_pInOutEffect->Is_Active())
        m_pInOutEffect->Inactive_All();
}

/* 그 밖의 제어 함수 */
// pEffect->Active_All(true);      // 전체 Emitter를 리셋 후 재생
// pEffect->Stop_Spawn(0.5f);      // 0.5초 뒤 스폰 중단 (살아있는 파티클은 수명까지 유지)
// pEffect->Set_SpawnMatrix(pBoneMatrix);  // 매 프레임 이 행렬을 스폰 기준으로 사용
