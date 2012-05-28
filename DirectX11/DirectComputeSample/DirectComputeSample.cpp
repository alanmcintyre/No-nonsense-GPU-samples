#include <stdio.h>
#include <tchar.h>
#include <D3DX11.h>
#include <D3Dcompiler.h>
#include <cassert>

int _tmain(int argc, _TCHAR* argv[])
{
    // Create a D3D11 device and immediate context.  
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device *pD3DDevice = nullptr;
    ID3D11DeviceContext *pD3DContext = nullptr;
    HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
        NULL, NULL, 0, D3D11_SDK_VERSION, &pD3DDevice, 
        &featureLevel, &pD3DContext);
    assert(SUCCEEDED(hr));

    // Create a structured buffer of 4-vectors
    struct Vector4
    {
        float x, y, z, w;
    };
    UINT W = 640;
    UINT H = 480;
    D3D11_BUFFER_DESC sbDesc;
    sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS 
                      | D3D11_BIND_SHADER_RESOURCE;
    sbDesc.Usage = D3D11_USAGE_DEFAULT;
    sbDesc.CPUAccessFlags = 0;
    sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    sbDesc.StructureByteStride = sizeof(Vector4);
    sbDesc.ByteWidth = sizeof(Vector4) * W * H;
    ID3D11Buffer *pStructuredBuffer = nullptr;
    hr = pD3DDevice->CreateBuffer(&sbDesc, NULL, &pStructuredBuffer);
    assert(SUCCEEDED(hr));

    // Create an unordered access view for the structured buffer
    D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
    sbUAVDesc.Buffer.FirstElement = 0;        
    sbUAVDesc.Buffer.Flags = 0;            
    sbUAVDesc.Buffer.NumElements = W * H;
    sbUAVDesc.Format = DXGI_FORMAT_UNKNOWN;    
    sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;   
    ID3D11UnorderedAccessView *pStructuredBufferUAV;
    hr = pD3DDevice->CreateUnorderedAccessView(pStructuredBuffer, 
        &sbUAVDesc, &pStructuredBufferUAV );
    assert(SUCCEEDED(hr));

    // Create a staging buffer to actually copy data to-from the GPU buffer. 
    D3D11_BUFFER_DESC stagingBufferDesc;
    stagingBufferDesc.BindFlags = 0;
    stagingBufferDesc.Usage = D3D11_USAGE_STAGING;  
    stagingBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED ;
    stagingBufferDesc.StructureByteStride = sizeof(Vector4);
    stagingBufferDesc.ByteWidth = sizeof(Vector4) * W * H;
    ID3D11Buffer *pStagingBuffer;
    hr = pD3DDevice->CreateBuffer(&stagingBufferDesc, NULL, &pStagingBuffer);
    assert(SUCCEEDED(hr));

    // Create a constant buffer 
    D3D11_BUFFER_DESC cbDesc;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;  
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.ByteWidth = sizeof(Vector4);
    ID3D11Buffer *pConstantBuffer = nullptr;
    hr = pD3DDevice->CreateBuffer( &cbDesc, NULL, &pConstantBuffer );
    assert(SUCCEEDED(hr));

    // must use D3D11_MAP_WRITE_DISCARD
    // http://msdn.microsoft.com/en-us/library/bb205318(VS.85).aspx
    D3D11_MAPPED_SUBRESOURCE stagingWriteResource;
    pD3DContext->Map(pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, 
        &stagingWriteResource );
    unsigned int *data = (unsigned int *)(stagingWriteResource.pData);
    for (int i = 0; i < 4; i ++) 
    {
        data[i] = 50+i;
    }
    data = nullptr;
    pD3DContext->Unmap(pConstantBuffer, 0);

    // Compile the kernel
    ID3DBlob* pErrorBlob = nullptr;
    ID3DBlob *pBlob = nullptr;
    hr = D3DX11CompileFromFile(L"kernel.hlsl", NULL, NULL, "main", "cs_4_0",
        D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, &pBlob, &pErrorBlob, NULL);
    if (FAILED(hr))
    {
        // Print out the error message if there is one.
        if (pErrorBlob)
        {
            char const* message = (char*)pErrorBlob->GetBufferPointer();
            printf("%s\n", message);
            pErrorBlob->Release();
        }
        assert(false);
    }

    // Create a shader object from the compiled blob
    ID3D11ComputeShader* pComputeShader;
    hr = pD3DDevice->CreateComputeShader(pBlob->GetBufferPointer(), 
        pBlob->GetBufferSize(), NULL, &pComputeShader);
    assert(SUCCEEDED(hr));

    D3D11_QUERY_DESC pQueryDesc;
    pQueryDesc.Query = D3D11_QUERY_EVENT;
    pQueryDesc.MiscFlags = 0;
    ID3D11Query *pEventQuery;
    pD3DDevice->CreateQuery(&pQueryDesc, &pEventQuery);

    pD3DContext->End(pEventQuery);
    while( pD3DContext->GetData(pEventQuery, NULL, 0, 0) == S_FALSE ) 
    {
        // spin until event is finished
    } 

    pD3DContext->CopyResource( pStructuredBuffer, pStagingBuffer );

    pD3DContext->End( pEventQuery );
    while( pD3DContext->GetData( pEventQuery, NULL, 0, 0 ) == S_FALSE )
    {
        // spin until event is finished
    } 

    pEventQuery->Release();

    // now make the compute shader active
    pD3DContext->CSSetShader( pComputeShader, NULL, 0 );
    // To bind the UAV to the computer shader, we use the code: 
    // http://msdn.microsoft.com/en-us/library/dd445761.aspx
    UINT initCounts = 0;
    pD3DContext->CSSetUnorderedAccessViews( 0, 1, &pStructuredBufferUAV, &initCounts );
    pD3DContext->CSSetConstantBuffers( 0 ,1,  &pConstantBuffer );
    pD3DContext->CSSetConstantBuffers( 1 ,1,  &pConstantBuffer );

    // now dispatch ("run") the compute shader, with a set of 16x16 groups.
    pD3DContext->Dispatch( 16, 16, 1 );

    pD3DContext->CopyResource( pStagingBuffer, pStructuredBuffer );
    // http://msdn.microsoft.com/en-us/library/bb173512(VS.85).aspx D3D10 
    // had mappable buffers, but D3D11 moves this to context function
    // http://www.slideshare.net/repii/your-game-needs-direct3d-11-so-get-started-now
    D3D11_MAPPED_SUBRESOURCE stagingReadResource;
    pD3DContext->Map( pStagingBuffer, 0, D3D11_MAP_READ, 0, &stagingReadResource);
    unsigned int *stagingData = (unsigned int *)(stagingReadResource.pData);
    int offset = 31*4;
    printf(" %d %d %d %d\n", stagingData[offset+0], stagingData[offset+1], stagingData[offset+2], stagingData[offset+3] );
    pD3DContext->Unmap( pStagingBuffer, 0);

    // D3D11 on D3D10 hW: only a single UAV can be bound to a pipeline at once. 
    // set to NULL to unbind
    ID3D11UnorderedAccessView *pNullUAV = NULL;
    pD3DContext->CSSetUnorderedAccessViews( 0, 1, &pNullUAV, &initCounts );

    return 0;
}
