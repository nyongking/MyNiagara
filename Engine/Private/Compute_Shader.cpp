#include "Compute_Shader.h"

CCompute_Shader::CCompute_Shader(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
    : CComponent(_pDevice, _pContext)
{
}

CCompute_Shader::CCompute_Shader(const CCompute_Shader& _Prototype)
    : CComponent(_Prototype)
    //, m_ComputeShaders(_Prototype.m_ComputeShaders)
    , m_pEffect(_Prototype.m_pEffect)
    , m_iNumLayouts(_Prototype.m_iNumLayouts)
    //, m_pEventQuery(_Prototype.m_pEventQuery)
{
    //for (auto& Pair : m_ComputeShaders)
    //    Safe_AddRef(Pair.second);

    Safe_AddRef(m_pEffect);
   // Safe_AddRef(m_pEventQuery);
}

HRESULT CCompute_Shader::Initialize_Prototype(const _tchar* _pShaderFilePath)
{
    _uint iHlslFlag = 0;

#if defined(DEBUG) || defined(_DEBUG)
    iHlslFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
//
//    ID3DBlob* pShaderBlob = nullptr;
//    ID3DBlob* pErrorBlob = nullptr;
//
//    for (_uint i = 0; i < _iCount; ++i)
//    {
//        
//        HRESULT hr = D3DCompileFromFile(
//            _pShaderFilePath, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, _szEntry[i],
//            "cs_5_0", iHlslFlag, 0, &pShaderBlob, &pErrorBlob);
//
//        if (FAILED(hr))
//            return E_FAIL;
//
//        ID3D11ComputeShader* pCShader = { nullptr };
//        if (FAILED(m_pDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(),
//            pShaderBlob->GetBufferSize(), NULL, &pCShader)))
//            return E_FAIL;
//
//        if (nullptr != pCShader)
//            m_ComputeShaders.emplace(_szEntry[i], pCShader);
//    } 


    ID3DBlob* pBlobBuffer = nullptr;
    /* 셰이더 컴파일 */
    if (FAILED(D3DX11CompileEffectFromFile(_pShaderFilePath, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, iHlslFlag, 0, m_pDevice, &m_pEffect, &pBlobBuffer)))
    {
        return E_FAIL;
    }

    ID3DX11EffectTechnique* pTechnique = m_pEffect->GetTechniqueByIndex(0);
    if (nullptr == pTechnique)
    {
        return E_FAIL;
    }

    D3DX11_TECHNIQUE_DESC TechniqueDesc = {};
    pTechnique->GetDesc(&TechniqueDesc);
    m_iNumLayouts = TechniqueDesc.Passes;


    //D3D11_QUERY_DESC queryDesc = {};
    //queryDesc.Query = D3D11_QUERY_EVENT;
    //queryDesc.MiscFlags = 0;
    //m_pDevice->CreateQuery(&queryDesc, &m_pEventQuery);

    return S_OK;
}

HRESULT CCompute_Shader::Initialize(void* _pArg)
{


    return S_OK;
}

HRESULT CCompute_Shader::Bind_RawValue(const _char* _pConstantName, const void* _pData, _uint _iLength)
{
    ID3DX11EffectVariable* pVariable = m_pEffect->GetVariableByName(_pConstantName);
    if (nullptr == pVariable)
        return E_FAIL;


    return pVariable->SetRawValue(_pData, 0, _iLength);
}

HRESULT CCompute_Shader::Bind_Matrix(const _char* _pConstantName, const _float4x4* _pMatrix)
{
    ID3DX11EffectVariable* pVariable = m_pEffect->GetVariableByName(_pConstantName);
    if (nullptr == pVariable)
        return E_FAIL;

    ID3DX11EffectMatrixVariable* pMatrixVariable = pVariable->AsMatrix();
    if (nullptr == pMatrixVariable)
        return E_FAIL;

    return pMatrixVariable->SetMatrix(reinterpret_cast<const _float*>(_pMatrix));
}

HRESULT CCompute_Shader::Bind_SRV(const _char* _pConstantName, ID3D11ShaderResourceView* _pSRV)
{
    ID3DX11EffectVariable* pVariable = m_pEffect->GetVariableByName(_pConstantName);
    if (nullptr == pVariable)
        return E_FAIL;

    ID3DX11EffectShaderResourceVariable* pSRVVariable = pVariable->AsShaderResource();
    if (nullptr == pSRVVariable)
        return E_FAIL;

    return pSRVVariable->SetResource(_pSRV);
}

HRESULT CCompute_Shader::Set_SRVs(ID3D11ShaderResourceView** _ppSRVs, _uint _iCount, _uint _iStartSlot)
{
    if (nullptr == _ppSRVs)
    {
        ID3D11ShaderResourceView* nullSRV[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        m_pContext->CSSetShaderResources(0, 6, nullSRV);
        return S_OK;
    }

    else
    {
        m_pContext->CSSetShaderResources(_iStartSlot, _iCount, _ppSRVs);
    }

    return S_OK;
}

HRESULT CCompute_Shader::Set_UAVs(ID3D11UnorderedAccessView** _ppUAVs, _uint _iCount, _uint _iStartSlot)
{
    if (nullptr == _ppUAVs)
    {
        ID3D11UnorderedAccessView* nullSUV[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        m_pContext->CSSetUnorderedAccessViews(0, 6, nullSUV, nullptr);
        return S_OK;
    }

    else
    {
        m_pContext->CSSetUnorderedAccessViews(_iStartSlot, _iCount, _ppUAVs, &_iCount);
    }

    return S_OK;
}

HRESULT CCompute_Shader::Begin(_uint _iPassIndex)
{
    if (_iPassIndex >= m_iNumLayouts)
        return E_FAIL;

    ID3DX11EffectPass* pPass = m_pEffect->GetTechniqueByIndex(0)->GetPassByIndex(_iPassIndex);
    if (pPass == nullptr)
        return E_FAIL;
    pPass->Apply(0, m_pContext);
    m_pContext->IASetInputLayout(nullptr);

    return S_OK;
}

HRESULT CCompute_Shader::Compute(_uint _iThreadGroupCountX, _uint _iThreadGroupCountY, _uint _iThreadGroupCountZ)
{    

    m_pContext->Dispatch(_iThreadGroupCountX, _iThreadGroupCountY, _iThreadGroupCountZ);



    return S_OK;
}

HRESULT CCompute_Shader::End_Compute()
{
   /* while (S_FALSE == m_pContext->GetData(m_pEventQuery, nullptr, 0, 0)) {
    
    }
    m_pContext->End(m_pEventQuery);*/

    ID3D11ShaderResourceView* nullSRV[6] =
    {
        0
    };
    ID3D11UnorderedAccessView* nullUAV[6] = {
        0
    };
    m_pContext->CSSetUnorderedAccessViews(0, 6, nullUAV, NULL);
    m_pContext->CSSetShaderResources(0, 6, nullSRV);

    m_pContext->CSSetShader(nullptr, nullptr, 0);


    return S_OK;
}

CCompute_Shader* CCompute_Shader::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext, const _tchar* _pShaderFilePath)
{
    CCompute_Shader* pInstance = new CCompute_Shader(_pDevice, _pContext);

    if (FAILED(pInstance->Initialize_Prototype(_pShaderFilePath)))
    {
        MSG_BOX("Failed to Created : CCompute_Shader");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CComponent* CCompute_Shader::Clone(void* _pArg)
{
    CCompute_Shader* pInstance = new CCompute_Shader(*this);

    if (FAILED(pInstance->Initialize(_pArg)))
    {
        MSG_BOX("Failed to Cloned : CCompute_Shader");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CCompute_Shader::Free()
{
    Safe_Release(m_pEffect);
    //Safe_Release(m_pEventQuery);

    __super::Free();
}
