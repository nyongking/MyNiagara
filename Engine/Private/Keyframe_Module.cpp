#include "Keyframe_Module.h"
#include "Compute_Shader.h"

#ifdef _DEBUG

const _char* CKeyframe_Module::g_szModuleNames[2] = { "COLOR_KEYFRAME", "SCALE_KEYFRAME" };

#endif // _DEBUG

CKeyframe_Module::CKeyframe_Module(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
    : CEffect_Module()
    , m_pDevice(_pDevice)
    , m_pContext(_pContext)
{
    m_eModuleType = MODULE_KEYFRAME;
    Safe_AddRef(_pDevice);
    Safe_AddRef(_pContext);
}

CKeyframe_Module::CKeyframe_Module(const CKeyframe_Module& _Prototype)
    : CEffect_Module(_Prototype)
    , m_eModuleName(_Prototype.m_eModuleName)
    , m_Keyframes(_Prototype.m_Keyframes)
    , m_KeyframeDatas(_Prototype.m_KeyframeDatas)
    , m_KeyCurrentIndicies(_Prototype.m_KeyCurrentIndicies)
    , m_pDevice(_Prototype.m_pDevice)
    , m_pContext(_Prototype.m_pContext)
    , m_pKeyframeDataTexture(_Prototype.m_pKeyframeDataTexture)
    , m_pSRVKeyframeDatas(_Prototype.m_pSRVKeyframeDatas)
{    
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);

    Safe_AddRef(m_pSRVKeyframeDatas);
    Safe_AddRef(m_pKeyframeDataTexture);
}

HRESULT CKeyframe_Module::Initialize(const json& _jsonModuleInfo, _int _iNumInstance)
{
	if (FAILED(__super::Initialize(_jsonModuleInfo)))
		return E_FAIL;

    _float4* pKeyFrameDatas = new _float4[128];
    if (_jsonModuleInfo.contains("Keyframe"))
    {
        for (_int i = 0; i < _jsonModuleInfo["Keyframe"].size(); ++i)
        {
            _float fKeyframe = _jsonModuleInfo["Keyframe"][i]["Time"];
            
            _float4 vkeyframeData;
            for (_int j = 0; j < _jsonModuleInfo["Keyframe"][i]["Data"].size(); ++j)
            {
                *((_float*)(&vkeyframeData) + j) = _jsonModuleInfo["Keyframe"][i]["Data"][j];
            }

            m_Keyframes.push_back(fKeyframe);
            m_KeyframeDatas.push_back(vkeyframeData);
        }
    }

    for (_uint i = 0; i < (_uint)m_Keyframes.size() - 1; ++i)
    {
        _uint iFirst = clamp(_uint(m_Keyframes[i] * 128.f), 0U, 127U);
        _uint iSecond = clamp(_uint(m_Keyframes[i + 1] * 128.f), 0U, 127U);

        for (_uint j = iFirst; j < iSecond; ++j)
        {
            _float fCurTime = (_float)j / (_float)128.f;
            _float fRatio = (fCurTime - m_Keyframes[i]) / (m_Keyframes[i + 1] - m_Keyframes[i]);
            _float4 vKeyframe;

            XMStoreFloat4(&vKeyframe, XMVectorLerp(XMLoadFloat4(&m_KeyframeDatas[i]), XMLoadFloat4(&m_KeyframeDatas[i + 1]), fRatio));
                   
            pKeyFrameDatas[j] = vKeyframe;
        }
    }


    if (_jsonModuleInfo.contains("Module"))
    {
        m_eModuleName = _jsonModuleInfo["Module"];
#ifdef _DEBUG
        m_strTypeName = _jsonModuleInfo["Module"];
#endif
    }

    m_KeyCurrentIndicies.resize(_iNumInstance, 0);
    

#pragma region SRV

   D3D11_TEXTURE2D_DESC	TextureDesc{};

   TextureDesc.Width = 128;
   TextureDesc.Height = 16;
   TextureDesc.MipLevels = 1;
   TextureDesc.ArraySize = 1;
   TextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

   TextureDesc.SampleDesc.Quality = 0;
   TextureDesc.SampleDesc.Count = 1;

   TextureDesc.Usage = D3D11_USAGE_DEFAULT;
   TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
   TextureDesc.CPUAccessFlags = 0;
   TextureDesc.MiscFlags = 0;

   _float4* pPixel = new _float4[128 * 16];
   ZeroMemory(pPixel, sizeof(_float4) * 128 * 16);

   for (_uint i = 0; i < 16; i++)
   {
       for (_uint j = 0; j < 128; j++)
       {
           _uint		iIndex = i * 128 + j;

           pPixel[iIndex] = pKeyFrameDatas[j];
       }
       pPixel[i * 128 + 127] = pPixel[i * 128 + 126];
   }

   D3D11_SUBRESOURCE_DATA		InitialData{};

   InitialData.pSysMem = pPixel;
   InitialData.SysMemPitch = 128 * sizeof(_float4);

   if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, &InitialData, &m_pKeyframeDataTexture)))
       return E_FAIL;

   Safe_Delete_Array(pPixel);
   Safe_Delete_Array(pKeyFrameDatas);

   // D3D11_SHADER_RESOURCE_VIEW_DESC 설정 (기본적인 2D 텍스처)
   D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
   srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
   srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
   srvDesc.Texture2D.MostDetailedMip = 0;
   srvDesc.Texture2D.MipLevels = 1;

   // SRV 생성
   HRESULT hr = m_pDevice->CreateShaderResourceView(m_pKeyframeDataTexture, nullptr, &m_pSRVKeyframeDatas);
   if (FAILED(hr))
       return E_FAIL;
 
#pragma endregion SRV




	return S_OK;
}

// 하나의 개별적 매쉬 이펙트에서만 적용
void CKeyframe_Module::Update_ColorKeyframe(_float _fCurTime, _float4* _pColor)
{
    if (false == m_isActive || 0 == m_Keyframes.size())
        return;

    _uint iCurrentIndex = m_KeyCurrentIndicies[0];
    
    // 다음 키프레임으로 이동해야할 경우 이동,
    while (iCurrentIndex < m_Keyframes.size() - 1 && m_Keyframes[iCurrentIndex + 1] < _fCurTime)
    {     
        ++iCurrentIndex;
    }

    // 이전 키프레임으로 이동해야 할 경우 이동,
    while (iCurrentIndex > 0 && m_Keyframes[iCurrentIndex] > _fCurTime)
    {
        --iCurrentIndex;
    }

    // 현재 키프레임 인덱스가, 전체 키프레임 초과일 경우.
    if (m_Keyframes.size() - 1 <= iCurrentIndex)
    {
        XMStoreFloat4(_pColor, XMLoadFloat4(&m_KeyframeDatas.back()));
    }

    // 일반적인 상황
    else
    {
        _float fRatio = (_fCurTime - m_Keyframes[iCurrentIndex]) / (m_Keyframes[iCurrentIndex + 1] - m_Keyframes[iCurrentIndex]);
        
        XMStoreFloat4(_pColor, XMVectorLerp(XMLoadFloat4(&m_KeyframeDatas[iCurrentIndex]), XMLoadFloat4(&m_KeyframeDatas[iCurrentIndex + 1]), fRatio));
    }


    m_KeyCurrentIndicies[0] = iCurrentIndex;
}

void CKeyframe_Module::Update_ScaleKeyframe(_float _fCurTime, _float4* _pRight, _float4* _pUp, _float4* _pLook)
{
    if (false == m_isActive || 0 == m_Keyframes.size())
        return;

    _uint iCurrentIndex = m_KeyCurrentIndicies[0];

    // 다음 키프레임으로 이동해야할 경우 이동,
    while (iCurrentIndex < m_Keyframes.size() - 1 && m_Keyframes[iCurrentIndex + 1] < _fCurTime)
    {
        ++iCurrentIndex;
    }

    // 이전 키프레임으로 이동해야 할 경우 이동,
    while (iCurrentIndex > 0 && m_Keyframes[iCurrentIndex] > _fCurTime)
    {
        --iCurrentIndex;
    }

    // 현재 키프레임 인덱스가, 전체 키프레임 초과일 경우.
    if (m_Keyframes.size() - 1 <= iCurrentIndex)
    {
        XMStoreFloat4(_pRight, XMVector3Normalize(XMLoadFloat4(_pRight)) * m_KeyframeDatas.back().x);
        XMStoreFloat4(_pUp, XMVector3Normalize(XMLoadFloat4(_pUp)) * m_KeyframeDatas.back().y);
        XMStoreFloat4(_pLook, XMVector3Normalize(XMLoadFloat4(_pLook)) * m_KeyframeDatas.back().z);
    }

    else
    {
        _float fRatio = (_fCurTime - m_Keyframes[iCurrentIndex]) / (m_Keyframes[iCurrentIndex + 1] - m_Keyframes[iCurrentIndex]);
        _vector vScale = XMVectorLerp(XMLoadFloat4(&m_KeyframeDatas[iCurrentIndex]), XMLoadFloat4(&m_KeyframeDatas[iCurrentIndex + 1]), fRatio);



        XMStoreFloat4(_pRight, XMVector3Normalize(XMLoadFloat4(_pRight)) * XMVectorGetX(vScale));
        XMStoreFloat4(_pUp, XMVector3Normalize(XMLoadFloat4(_pUp)) * XMVectorGetY(vScale));
        XMStoreFloat4(_pLook, XMVector3Normalize(XMLoadFloat4(_pLook)) * XMVectorGetZ(vScale));
    }
}

void CKeyframe_Module::Update_ColorKeyframe(_float* _pBuffer, _uint _iNumInstance, _uint _iColorOffset, _uint _iLifeTimeOffset, _uint _iTotalSize)
{
    if (false == m_isActive || 0 == m_Keyframes.size())
        return;
    
    // Keyframe size부터 확인
    if (m_KeyCurrentIndicies.size() != _iNumInstance)
    {
        m_KeyCurrentIndicies.resize(_iNumInstance, 0);
    }

    for (_uint i = 0; i < _iNumInstance; ++i)
    {
        _uint iCurrentIndex = m_KeyCurrentIndicies[i];
        _float2* pLifeTime = (_float2*)(_pBuffer + (i * _iTotalSize) + _iLifeTimeOffset);
        _float4* pColor = (_float4*)(_pBuffer + (i * _iTotalSize) + _iColorOffset);

        // 다음 키프레임으로 이동해야할 경우 이동,
        while (iCurrentIndex < m_Keyframes.size() - 1 
            && m_Keyframes[iCurrentIndex + 1] < pLifeTime->y)
        {
            ++iCurrentIndex;
        }

        // 이전 키프레임으로 이동해야 할 경우 이동,
        while (iCurrentIndex > 0
            && m_Keyframes[iCurrentIndex] > pLifeTime->y)
        {
            --iCurrentIndex;
        }

        // 현재 키프레임 인덱스가, 전체 키프레임 초과일 경우.
        if (m_Keyframes.size() - 1 <= iCurrentIndex)
        {
            XMStoreFloat4(pColor, XMLoadFloat4(&m_KeyframeDatas.back()));
        }

        else
        {
            _float fRatio = (pLifeTime->y - m_Keyframes[iCurrentIndex]) / (m_Keyframes[iCurrentIndex + 1] - m_Keyframes[iCurrentIndex]);

            XMStoreFloat4(pColor, XMVectorLerp(XMLoadFloat4(&m_KeyframeDatas[iCurrentIndex]), XMLoadFloat4(&m_KeyframeDatas[iCurrentIndex + 1]), fRatio));
        }
    }

    
}

void CKeyframe_Module::Update_ScaleKeyframe(_float* _pBuffer, _uint _iNumInstance, _uint _iRightOffset, _uint _iUpOffset, _uint _iLookOffset, _uint _iLifeTimeOffset, _uint _iTotalSize)
{
    if (false == m_isActive || 0 == m_Keyframes.size())
        return;

    // Keyframe size부터 확인
    if (m_KeyCurrentIndicies.size() != _iNumInstance)
    {
        m_KeyCurrentIndicies.resize(_iNumInstance, 0);
    }

    for (_uint i = 0; i < _iNumInstance; ++i)
    {
        _uint iCurrentIndex = m_KeyCurrentIndicies[i];
        _float2* pLifeTime = (_float2*)(_pBuffer + (i * _iTotalSize) + _iLifeTimeOffset);
        _float4* pRight = (_float4*)(_pBuffer + (i * _iTotalSize) + _iRightOffset);
        _float4* pUp = (_float4*)(_pBuffer + (i * _iTotalSize) + _iUpOffset);
        _float4* pLook = (_float4*)(_pBuffer + (i * _iTotalSize) + _iLookOffset);

        // 다음 키프레임으로 이동해야할 경우 이동,
        while (iCurrentIndex < m_Keyframes.size() - 1
            && m_Keyframes[iCurrentIndex + 1] < pLifeTime->y)
        {
            ++iCurrentIndex;
        }

        // 이전 키프레임으로 이동해야 할 경우 이동,
        while (iCurrentIndex > 0
            && m_Keyframes[iCurrentIndex] > pLifeTime->y)
        {
            --iCurrentIndex;
        }

        // 현재 키프레임 인덱스가, 전체 키프레임 초과일 경우.
        if (m_Keyframes.size() - 1 <= iCurrentIndex)
        {
            XMStoreFloat4(pRight, XMVector3Normalize(XMLoadFloat4(pRight)) * m_KeyframeDatas.back().x);
            XMStoreFloat4(pUp, XMVector3Normalize(XMLoadFloat4(pUp)) * m_KeyframeDatas.back().y);
            XMStoreFloat4(pLook, XMVector3Normalize(XMLoadFloat4(pLook)) * m_KeyframeDatas.back().z);
        }

        else
        {
            _float fRatio = (pLifeTime->y - m_Keyframes[iCurrentIndex]) / (m_Keyframes[iCurrentIndex + 1] - m_Keyframes[iCurrentIndex]);
            _vector vScale = XMVectorLerp(XMLoadFloat4(&m_KeyframeDatas[iCurrentIndex]), XMLoadFloat4(&m_KeyframeDatas[iCurrentIndex + 1]), fRatio);

            XMStoreFloat4(pRight, XMVector3Normalize(XMLoadFloat4(pRight)) * XMVectorGetX(vScale));
            XMStoreFloat4(pUp, XMVector3Normalize(XMLoadFloat4(pUp)) * XMVectorGetY(vScale));
            XMStoreFloat4(pLook, XMVector3Normalize(XMLoadFloat4(pLook)) * XMVectorGetZ(vScale));
        }
    }
}

_int CKeyframe_Module::Update_Module(CCompute_Shader* _pCShader)
{
    if (false == m_isActive)
        return -1;

    if (1 >= m_Keyframes.size())
        return -1;

    _pCShader->Bind_SRV("g_KeyframeTexture", m_pSRVKeyframeDatas);

    switch (m_eModuleName)
    {
    case COLOR_KEYFRAME:
    {      
        return 12;
        break;
    }
    case SCALE_KEYFRAME:
    {
        return 13;
        break;
    }

    }

    return -1;
}


CKeyframe_Module* CKeyframe_Module::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const json& _jsonModuleInfo, _int _iNumInstance)
{
    CKeyframe_Module* pInstance = new CKeyframe_Module(_pDevice, _pContext);

    if (FAILED(pInstance->Initialize(_jsonModuleInfo, _iNumInstance)))
    {
        MSG_BOX("Failed to Created : CKeyframe_Module");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CEffect_Module* CKeyframe_Module::Clone()
{
    return new CKeyframe_Module(*this);
}

void CKeyframe_Module::Free()
{
    Safe_Release(m_pSRVKeyframeDatas);
    Safe_Release(m_pKeyframeDataTexture);

    m_KeyframeDatas.clear();
    m_Keyframes.clear();

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);

    __super::Free();
}

#ifdef _DEBUG

HRESULT CKeyframe_Module::Initialize(MODULE_NAME _eModuleName, _int _iNumInstance)
{
    m_eModuleName = _eModuleName;
    

    switch (m_eModuleName)
    {
    case COLOR_KEYFRAME:
    {
        m_isInit = false;
        m_strTypeName = "COLOR_KEYFRAME";
        m_eApplyTo = COLOR;
        break;
    }
    case SCALE_KEYFRAME:
    {
        m_isInit = false;
        m_strTypeName = "SCALE_KEYFRAME";
        m_eApplyTo = SCALE;
        break;
    }

    default:
        break;
    }
    
    m_Keyframes.push_back(0.f);
    m_KeyframeDatas.push_back(_float4(1.f, 1.f, 1.f, 1.f));

    m_KeyCurrentIndicies.resize(_iNumInstance, 0);

    return S_OK;
}

void CKeyframe_Module::Tool_Module_Update()
{
    __super::Tool_Module_Update();

    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    
    _int n = 0;
    static _int iNowIndex = 0;
    if (ImGui::BeginListBox("Keyframe List"))
    {

        for (auto& Item : m_Keyframes)
        {
            const _bool is_selected = (iNowIndex == n);
            if (ImGui::Selectable(to_string(m_Keyframes[n]).c_str(), is_selected))
            {
                iNowIndex = n;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
            ++n;
        }
        ImGui::EndListBox();
    }
    ImGui::PopItemFlag();

    if (m_Keyframes.size() > iNowIndex)
    {
        if (ImGui::DragFloat("Keyframe", &m_Keyframes[iNowIndex], 0.01f))
            m_isChanged = true;
        if (ImGui::DragFloat4("Keyframe Data", (_float*)&m_KeyframeDatas[iNowIndex], 0.01f))
            m_isChanged = true;

    }

    if (ImGui::Button("Add Keyframe"))
    {
        if (0 < m_Keyframes.size())
            m_Keyframes.push_back(m_Keyframes.back() + 1.f);
        else
            m_Keyframes.push_back(0.f);

        if (0 < m_KeyframeDatas.size())
            m_KeyframeDatas.push_back(m_KeyframeDatas.back());
        else
            m_KeyframeDatas.push_back(_float4(1.f, 1.f, 1.f, 1.f));
        m_isChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Module"))
    {
        if (m_Keyframes.size() > iNowIndex)
        {
            auto iter = m_Keyframes.begin() + iNowIndex;
            auto iter2 = m_KeyframeDatas.begin() + iNowIndex;

            m_Keyframes.erase(iter);
            m_KeyframeDatas.erase(iter2);
        }
        m_isChanged = true;
    }

    if (m_isChanged)
    {
        Safe_Release(m_pKeyframeDataTexture);
        Safe_Release(m_pSRVKeyframeDatas);
        _float4* pKeyFrameDatas = new _float4[128];

        for (_uint i = 0; i < (_uint)m_Keyframes.size() - 1; ++i)
        {
            _uint iFirst = clamp(_uint(m_Keyframes[i] * 128.f), 0U, 127U);
            _uint iSecond = clamp(_uint(m_Keyframes[i + 1] * 128.f), 0U, 127U);

            for (_uint j = iFirst; j < iSecond; ++j)
            {
                _float fCurTime = (_float)j / (_float)128.f;
                _float fRatio = (fCurTime - m_Keyframes[i]) / (m_Keyframes[i + 1] - m_Keyframes[i]);
                _float4 vKeyframe;

                XMStoreFloat4(&vKeyframe, XMVectorLerp(XMLoadFloat4(&m_KeyframeDatas[i]), XMLoadFloat4(&m_KeyframeDatas[i + 1]), fRatio));

                pKeyFrameDatas[j] = vKeyframe;
            }
        }

        D3D11_TEXTURE2D_DESC	TextureDesc{};

        TextureDesc.Width = 128;
        TextureDesc.Height = 16;
        TextureDesc.MipLevels = 1;
        TextureDesc.ArraySize = 1;
        TextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

        TextureDesc.SampleDesc.Quality = 0;
        TextureDesc.SampleDesc.Count = 1;

        TextureDesc.Usage = D3D11_USAGE_DEFAULT;
        TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        TextureDesc.CPUAccessFlags = 0;
        TextureDesc.MiscFlags = 0;

        _float4* pPixel = new _float4[128 * 16];
        ZeroMemory(pPixel, sizeof(_float4) * 128 * 16);

        for (_uint i = 0; i < 16; i++)
        {
            for (_uint j = 0; j < 128; j++)
            {
                _uint		iIndex = i * 128 + j;

                pPixel[iIndex] = pKeyFrameDatas[j];
            }
            pPixel[i * 128 + 127] = pPixel[i * 128 + 126];
        }

        D3D11_SUBRESOURCE_DATA		InitialData{};

        InitialData.pSysMem = pPixel;
        InitialData.SysMemPitch = 128 * sizeof(_float4);

        if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, &InitialData, &m_pKeyframeDataTexture)))
            MSG_BOX("실패");


        Safe_Delete_Array(pPixel);
        Safe_Delete_Array(pKeyFrameDatas);

        // D3D11_SHADER_RESOURCE_VIEW_DESC 설정 (기본적인 2D 텍스처)
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        // SRV 생성
        HRESULT hr = m_pDevice->CreateShaderResourceView(m_pKeyframeDataTexture, nullptr, &m_pSRVKeyframeDatas);
        if (FAILED(hr))
            MSG_BOX("실패");

    }

    if (ImGui::TreeNode("Image"))
    {
        ImVec2 imageSize(256, 32); // 이미지 크기 설정
        if (nullptr != m_pSRVKeyframeDatas)
        {
            ImGui::Image((ImTextureID)m_pSRVKeyframeDatas, imageSize);
        }
        ImGui::TreePop();
    }



}

HRESULT CKeyframe_Module::Save_Module(json& _jsonModuleInfo)
{
    if (FAILED(__super::Save_Module(_jsonModuleInfo)))
        return E_FAIL;

    for (_int i = 0; i < m_Keyframes.size(); ++i)
    {   
        json jsonKeyframe;
        jsonKeyframe["Time"] = m_Keyframes[i];
        
        _float4 vKeyframeData = m_KeyframeDatas[i];
        jsonKeyframe["Data"][0] = vKeyframeData.x;
        jsonKeyframe["Data"][1] = vKeyframeData.y;
        jsonKeyframe["Data"][2] = vKeyframeData.z;
        jsonKeyframe["Data"][3] = vKeyframeData.w;
    
        _jsonModuleInfo["Keyframe"].push_back(jsonKeyframe);
    }

    _jsonModuleInfo["Module"] = m_eModuleName;


    return S_OK;
}

CKeyframe_Module* CKeyframe_Module::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, MODULE_NAME _eModuleName, _int _iNumInstance)
{
    CKeyframe_Module* pInstance = new CKeyframe_Module(_pDevice, _pContext);

    if (FAILED(pInstance->Initialize(_eModuleName, _iNumInstance)))
    {
        MSG_BOX("Failed to Created : CKeyframe_Module");
        Safe_Release(pInstance);
    }

    return pInstance;
}


#endif