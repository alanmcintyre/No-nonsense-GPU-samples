#include <stdio.h>
#include <tchar.h>
#include <D3DX11.h>
#include <D3Dcompiler.h>
#include <vector>

// TODO: This sample is not careful to clean up resources before exiting if 
// something fails.  If you use it for something important, it's up to you 
// to include proper error checks and cleanup code.

int _tmain(int /*argc*/, _TCHAR* /*argv[]*/)
{
    // GROUP_SIZE_X defined in kernel.hlsl must match the 
    // groupSize declared here.
    size_t const groupSize = 512;
    size_t const numGroups = 16;
    size_t const dimension = numGroups*groupSize;

    // Create a D3D11 device and immediate context. 
    // TODO: The code below uses the default video adapter, with the
    // default set of feature levels.  Please see the MSDN docs if 
    // you wish to control which adapter and feature level are used.
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
        NULL, NULL, 0, D3D11_SDK_VERSION, &device, 
        &featureLevel, &context);
    if (FAILED(hr))
    {
        printf("D3D11CreateDevice failed with return code %x\n", hr);
        return hr;
    }

    // Create system memory and fill it with our initial data.  Note that
    // these data structures aren't really necessary , it's just a demonstration
    // of how you can take existing data structures you might have and copy
    // their data to/from GPU computations.
    std::vector<float> x(dimension);
    std::vector<float> y(dimension);
    std::vector<float> z(dimension);
    float const a = 2.0f;
    for (size_t i = 0; i < dimension; ++ i)
    {
        x[i] = static_cast<float>(i);
        y[i] = 100 - static_cast<float>(i);
    }

    // Create structured buffers for the "x" and "y" vectors.
    D3D11_BUFFER_DESC inputBufferDesc;
    inputBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    // The buffers are read-only by the GPU, writeable by the CPU.
    // TODO: If you will never again upate the data in a GPU buffer,
    // you might want to look at using a D3D11_SUBRESOURCE_DATA here to
    // provide the initialization data instead of doing the mapping 
    // and copying that happens below.
    inputBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    inputBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    inputBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    inputBufferDesc.StructureByteStride = sizeof(float);
    inputBufferDesc.ByteWidth = sizeof(float) * dimension;
    ID3D11Buffer* xBuffer = nullptr;
    hr = device->CreateBuffer(&inputBufferDesc, NULL, &xBuffer);
    if (FAILED(hr))
    {
        printf("CreateBuffer failed for x buffer with return code %x\n", hr);
        return hr;
    }
    // We can re-use inputBufferDesc here because the layout and usage of the x
    // and y buffers is exactly the same.
    ID3D11Buffer* yBuffer = nullptr;
    hr = device->CreateBuffer(&inputBufferDesc, NULL, &yBuffer);
    if (FAILED(hr))
    {
        printf("CreateBuffer failed for x buffer with return code %x\n", hr);
        return hr;
    }

    // Create shader resource views for the "x" and "y" buffers.
    // TODO: You can optionally provide a D3D11_SHADER_RESOURCE_VIEW_DESC
    // as the second parameter if you need to use only part of the buffer
    // inside the compute shader.
    ID3D11ShaderResourceView* xSRV = nullptr;
    hr = device->CreateShaderResourceView(xBuffer, NULL, &xSRV);
    if (FAILED(hr))
    {
        printf("CreateShaderResourceView failed for x buffer with return code %x\n", hr);
        return hr;
    }

    ID3D11ShaderResourceView* ySRV = nullptr;
    hr = device->CreateShaderResourceView(yBuffer, NULL, &ySRV);
    if (FAILED(hr))
    {
        printf("CreateShaderResourceView failed for y buffer with return code %x\n", hr);
        return hr;
    }

    // Create a structured buffer for the "z" vector.  This buffer needs to be 
    // writeable by the GPU, so we can't create it with CPU read/write access.
    D3D11_BUFFER_DESC outputBufferDesc;
    outputBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS 
        | D3D11_BIND_SHADER_RESOURCE;
    outputBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    outputBufferDesc.CPUAccessFlags = 0;
    outputBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    outputBufferDesc.StructureByteStride = sizeof(float);
    outputBufferDesc.ByteWidth = sizeof(float) * dimension;
    ID3D11Buffer* zBuffer = nullptr;
    hr = device->CreateBuffer(&outputBufferDesc, NULL, &zBuffer);
    if (FAILED(hr))
    {
        printf("CreateBuffer failed for z buffer with return code %x\n", hr);
        return hr;
    }

    // Create an unordered access view for the "z" vector.  
    D3D11_UNORDERED_ACCESS_VIEW_DESC outputUAVDesc;
    outputUAVDesc.Buffer.FirstElement = 0;        
    outputUAVDesc.Buffer.Flags = 0;            
    outputUAVDesc.Buffer.NumElements = dimension;
    outputUAVDesc.Format = DXGI_FORMAT_UNKNOWN;    
    outputUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;   
    ID3D11UnorderedAccessView* zBufferUAV;
    hr = device->CreateUnorderedAccessView(zBuffer, 
        &outputUAVDesc, &zBufferUAV);
    if (FAILED(hr))
    {
        printf("CreateUnorderedAccessView failed for z buffer with return code %x\n", hr);
        return hr;
    }

    // Create a staging buffer, which will be used to copy back from zBuffer.
    D3D11_BUFFER_DESC stagingBufferDesc;
    stagingBufferDesc.BindFlags = 0;
    stagingBufferDesc.Usage = D3D11_USAGE_STAGING;  
    stagingBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    stagingBufferDesc.StructureByteStride = sizeof(float);
    stagingBufferDesc.ByteWidth = sizeof(float) * dimension;
    ID3D11Buffer* stagingBuffer;
    hr = device->CreateBuffer(&stagingBufferDesc, NULL, &stagingBuffer);
    if (FAILED(hr))
    {
        printf("CreateBuffer failed for staging buffer with return code %x\n", hr);
        return hr;
    }

    // Create a constant buffer (this buffer is used to pass the constant 
    // value 'a' to the kernel as cbuffer Constants).
    D3D11_BUFFER_DESC cbDesc;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;  
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    // Even though the constant buffer only has one float, DX expects
    // ByteWidth to be a multiple of 4 floats (i.e., one 128-bit register).
    cbDesc.ByteWidth = sizeof(float)*4;
    ID3D11Buffer* constantBuffer = nullptr;
    hr = device->CreateBuffer( &cbDesc, NULL, &constantBuffer);
    if (FAILED(hr))
    {
        printf("CreateBuffer failed for constant buffer with return code %x\n", hr);
        return hr;
    }

    // Map the constant buffer and set the constant value 'a'.
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    float* constants = reinterpret_cast<float*>(mappedResource.pData);
    constants[0] = a;
    constants = nullptr;
    context->Unmap(constantBuffer, 0);

    // Map the x buffer and copy our data into it.
    context->Map(xBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    float* xvalues = reinterpret_cast<float*>(mappedResource.pData);
    memcpy(xvalues, &x[0], sizeof(float)*x.size());
    xvalues = nullptr;
    context->Unmap(xBuffer, 0);

    // Map the y buffer and copy our data into it.
    context->Map(yBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    float* yvalues = reinterpret_cast<float*>(mappedResource.pData);
    memcpy(yvalues, &y[0], sizeof(float)*y.size());
    yvalues = nullptr;
    context->Unmap(yBuffer, 0);

    // Compile the compute shader into a blob.
    ID3DBlob* errorBlob = nullptr;
    ID3DBlob* shaderBlob = nullptr;
    hr = D3DX11CompileFromFile(L"kernel.hlsl", NULL, NULL, "saxpy", "cs_4_0",
        D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, &shaderBlob, &errorBlob, NULL);
    if (FAILED(hr))
    {
        // Print out the error message if there is one.
        if (errorBlob)
        {
            char const* message = (char*)errorBlob->GetBufferPointer();
            printf("kernel.hlsl failed to compile; error message:\n");
            printf("%s\n", message);
            errorBlob->Release();
        }
        return hr;
    }

    // Create a shader object from the compiled blob.
    ID3D11ComputeShader* computeShader;
    hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), 
        shaderBlob->GetBufferSize(), NULL, &computeShader);
    if (FAILED(hr))
    {
        printf("CreateComputeShader failed with return code %x\n", hr);
        return hr;
    }

    // Make the shader active.
    context->CSSetShader(computeShader, NULL, 0);

    // Attach the z buffer to the output via its unordered access view.
    UINT initCounts = 0xFFFFFFFF;
    context->CSSetUnorderedAccessViews(0, 1, &zBufferUAV, &initCounts);

    // Attach the input buffers via their shader resource views.
    context->CSSetShaderResources(0, 1, &xSRV);
    context->CSSetShaderResources(1, 1, &ySRV);

    // Attach the constant buffer
    context->CSSetConstantBuffers(0, 1, &constantBuffer);

    // Execute the shader, in 'numGroups' groups of 'groupSize' threads each.
    context->Dispatch(numGroups, 1, 1);

    // Copy the z buffer to the staging buffer so that we can 
    // retrieve the data for accesss by the CPU.
    context->CopyResource(stagingBuffer, zBuffer);

    // Map the staging buffer for reading.
    context->Map(stagingBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
    float* zData = reinterpret_cast<float*>(mappedResource.pData);
    memcpy(&z[0], zData, sizeof(float)*z.size());
    zData = nullptr;
    context->Unmap(stagingBuffer, 0);

    // Now compare the GPU results against expected values.
    bool resultOK = true;
    for (size_t i = 0; i < x.size(); ++ i)
    {
        // NOTE: This comparison assumes the GPU produces *exactly* the 
        // same result as the CPU.  In general, this will not be the case
        // with floating-point calculations.
        float const expected = a*x[i] + y[i];
        if (z[i] != expected)
        {
            printf("Unexpected result at position %lu: expected %.7e, got %.7e\n",
                i, expected, z[i]);
            resultOK = false;
        }
    }

    if (!resultOK)
    {
        printf("GPU results differed from the CPU results.\n");
        OutputDebugStringA("GPU results differed from the CPU results.\n");
        return 1;
    }

    printf("GPU output matched the CPU results.\n");
    OutputDebugStringA("GPU output matched the CPU results.\n");

    // Disconnect everything from the pipeline.
    ID3D11UnorderedAccessView* nullUAV = nullptr;
    context->CSSetUnorderedAccessViews( 0, 1, &nullUAV, &initCounts);
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->CSSetShaderResources(0, 1, &nullSRV);
    context->CSSetShaderResources(1, 1, &nullSRV);
    ID3D11Buffer* nullBuffer = nullptr;
    context->CSSetConstantBuffers(0, 1, &nullBuffer);

    // Release resources.  Again, note that none of the error checks above
    // release resources that have been allocated up to this point, so the 
    // sample doesn't clean up after itself correctly unless everything succeeds.
    computeShader->Release();
    shaderBlob->Release();
    constantBuffer->Release();
    stagingBuffer->Release();
    zBufferUAV->Release();
    zBuffer->Release();
    xSRV->Release();
    xBuffer->Release();
    ySRV->Release();
    yBuffer->Release();
    context->Release();
    device->Release();

    return 0;
}
