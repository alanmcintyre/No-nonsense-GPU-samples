#include <stdio.h>
#include <tchar.h>
#include <D3D11.h>
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

    IDXGIFactory1* factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)(&factory));

    UINT i = 0; 
    IDXGIAdapter1* adapter; 
    std::vector<ID3D11Device*> devices;
    std::vector<ID3D11DeviceContext*> contexts;
    while(factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND) 
    { 
		// Display the device description.
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter->GetDesc1(&adapterDesc);
		wprintf(L"Adapter %d: %s", i, adapterDesc.Description);

		if (DXGI_ADAPTER_FLAG_SOFTWARE == adapterDesc.Flags)
		{
			wprintf(L" (software adapter)\n");
		}
		else if (DXGI_ADAPTER_FLAG_NONE == adapterDesc.Flags)
		{
			// For hardware adapters, create a device and context.

			wprintf(L" (hardware adapter)\n");

			// Create a D3D11 device and immediate context on each adapter.
			D3D_FEATURE_LEVEL featureLevel;
			ID3D11Device* device = nullptr;
			ID3D11DeviceContext* context = nullptr;
			// Note that, when providing an adapter, you must use 
			// D3D_DRIVER_TYPE_UNKNOWN as specified in the D3D11CreateDevice
			// documentation.
			hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 
				NULL, NULL, 0, D3D11_SDK_VERSION, &device, 
				&featureLevel, &context);
			if (FAILED(hr))
			{
				printf("D3D11CreateDevice failed with return code %x\n", hr);
				return hr;
			}

			devices.push_back(device);
			contexts.push_back(context);
		}
		else
		{
			wprintf(L" (unknown adapter type %d)\n", (int) adapterDesc.Flags);
		}


		// Because EnumAdapters1 increments the adapter's reference count,
		// we need to release it now that we're done with it.
		adapter->Release();
		adapter = nullptr;

	    ++i; 
    }

	factory->Release();
	factory = nullptr;

	for (auto d : devices)
	{
		d->Release();
	}
	for (auto c : contexts)
	{
		c->Release();
	}

    return 0;
}
