#include "pch.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_sdl2.h"
#include "VoidGraphicsCommandList_D3D12.h"
#ifdef VOID_STAR_D3D12_BACKEND
#include <fstream>
#include <SDL_syswm.h>
#include "D3D12_Translator.h"
#include "D3D12_VoidBuffers.h"
#include "D3D12_VoidSampler.h"
#include "D3D12_VoidShader.h"
#include "VoidRenderContext_D3D12.h"
#include "d3dx12.h"

void VoidRenderContext_D3D12::init(VoidExtent2D windowExtent, SDL_Window* windowHandle)
{
	wchar_t buffer[500];
	GetCurrentDirectory(500, buffer);

	std::wstring wrootPath(buffer);
	std::string rootPath;
	{
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wrootPath[0], (int)wrootPath.size(), nullptr, 0, nullptr,
		                                      nullptr);
		rootPath = std::string(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wrootPath[0], (int)wrootPath.size(), &rootPath[0], size_needed, nullptr,
		                    nullptr);
	}

	rootPath = rootPath + "\\";

	set_shader_root_path(rootPath);

	_window = windowHandle;
	_windowExtent = windowExtent;

#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugInterface;
	check_result(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

	_adapter = get_adapter(false);

	if (_adapter == nullptr)
		throw std::runtime_error("failed to find valid GPU");

	if (!init_device_and_resources())
	{
		throw std::runtime_error("failed to init D3D12 context");
	}

	HRESULT hr = get_command_list()->Close();
	hr = get_prologue_list()->Close();

#ifdef VOID_USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForD3D(_window);
	ImGui_ImplDX12_Init(_device.Get(), _numFrames, DXGI_FORMAT_R8G8B8A8_UNORM, _imguiDescHeap.Get(),
	                    _imguiDescHeap->GetCPUDescriptorHandleForHeapStart(),
	                    _imguiDescHeap->GetGPUDescriptorHandleForHeapStart());
#endif

	begin_new_frame();

	_pipelineState->frame_ended(get_command_list());
}

void VoidRenderContext_D3D12::shutdown()
{
	for (int i = 0; i < _numFrames; i++)
	{
		_frameFenceValues[i] = signal_fence(_directCommandQueue.Get(), _fenceDirect.Get(), _fenceValue);

		wait_for_fence_value(_fenceDirect.Get(), _frameFenceValues[i], _fenceEvent);
	}


	auto prologueAllocator = _prologueAllocator[get_current_frame_index()];
	prologueAllocator->Reset();

	auto prologueList = get_prologue_list();
	prologueList->Reset(prologueAllocator.Get(), nullptr);
	_prologueCommandList = nullptr;


	// empty all resource locks

	for (auto& locks : _resourceLocks)
	{
		locks = std::queue<std::shared_ptr<void>>();
	}

	for (auto& locks : _listLocks)
	{
		locks = std::queue<std::shared_ptr<IVoidGraphicsCommandList>>();
	}

	delete _pipelineState;

	D3D12_PipelineState::shutdown_static();

	_allocator->Release();
	_dxgiFactory->Release();
	// todo
}

void VoidRenderContext_D3D12::window_resized(VoidExtent2D newSize)
{
	_needResizeTo = newSize;
	_needResize = true;
}

void VoidRenderContext_D3D12::build_swapchain(VoidExtent2D size)
{
	if (!_swapChain)
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = _numFrames;
		swapChainDesc.Width = static_cast<UINT>(size.width);
		swapChainDesc.Height = static_cast<UINT>(size.height);
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(_window, &wmInfo);
		HWND hwnd = wmInfo.info.win.window;

		ComPtr<IDXGISwapChain1> tempSwapChain;
		check_result(_dxgiFactory->CreateSwapChainForHwnd(_directCommandQueue.Get(), hwnd, &swapChainDesc, nullptr,
		                                                  nullptr, &tempSwapChain));

		check_result(tempSwapChain.As(&_swapChain));

		_dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	}
	else
	{
		// wait for all frames in flight to be done
		for (int i = 0; i < _numFrames; i++)
		{
			// Schedule a Signal command in the GPU queue.
			UINT64 fenceValue = _frameFenceValues[i];
			if (SUCCEEDED(_directCommandQueue->Signal(_fenceDirect.Get(), fenceValue)))
			{
				// Wait until the Signal has been processed.
				if (SUCCEEDED(_fenceDirect->SetEventOnCompletion(fenceValue, _fenceEvent)))
				{
					std::ignore = WaitForSingleObjectEx(_fenceEvent, INFINITE, FALSE);

					// Increment the fence value for the current frame.
					_frameFenceValues[i]++;
				}
			}
		}

		for (auto& buffer : _backBuffers)
		{
			buffer->get_d3d12_resource()->Release();
			free_descriptors(buffer.get());
		}

		check_result(_swapChain->ResizeBuffers(_numFrames, size.width, size.height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

		// hack, clear all locks
		for (int i = 0; i < _numFrames; i++)
		{
			while (!_resourceLocks[i].empty())
			{
				auto& lock = _resourceLocks[i].front();
				lock.reset();
				_resourceLocks[i].pop();
			}
		}
	}
	for (uint32_t i = 0; i < _numFrames; i++)
	{
		ID3D12Resource* backBuffer;
		check_result(_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		if (_backBuffers[i] != nullptr)
		{
			free_descriptors(_backBuffers[i].get());
		}

		_backBuffers[i] = std::make_shared<D3D12_VoidImage>(size.width, size.height, 1, nullptr, backBuffer,
		                                                    DXGI_FORMAT_R8G8B8A8_UNORM, nullptr);
		_backBuffers[i]->_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		_backBuffers[i]->_d3d12Resource->SetName(i == 0 ? L"BackBuffer0" : L"BackBuffer1");

		register_render_target_view(_backBuffers[i].get());
	}
}

void VoidRenderContext_D3D12::present()
{
	auto backBuffer = _backBuffers[get_current_frame_index()];

#ifdef VOID_USE_IMGUI
	// todo wtf is this? got to be a nicer way
	_pipelineState->bind_render_target(0, _backBuffers[get_current_frame_index()]);
	_pipelineState->set_num_rt(1);
	_pipelineState->bind_pso_if_invalid(get_command_list(), true);
	_pipelineState->bind_shader_resources(get_command_list());
	_pipelineState->ensure_resource_states(get_command_list());
	//ImGui::ShowDemoWindow();
	ImGui::Render();
	get_command_list()->SetDescriptorHeaps(1, _imguiDescHeap.GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), get_command_list());
#endif

	VOID_ASSERT_STATE_SET(backBuffer);
	if (VOID_GET_RESOURCE_STATE(backBuffer) != D3D12_RESOURCE_STATE_PRESENT)
		transition_resource(get_command_list(), backBuffer->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(backBuffer),
		                    D3D12_RESOURCE_STATE_PRESENT);

	get_command_list()->Close();

	ID3D12CommandList* const commandLists[] = {get_command_list()};

	_directCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	UINT syncInterval = _vSyncEnabled ? 1 : 0;
	UINT presentFlags = 0;

	_swapChain->Present(syncInterval, presentFlags);
	_frameFenceValues[get_current_frame_index()] = signal_fence(_directCommandQueue.Get(), _fenceDirect.Get(),
	                                                            _fenceValue);

	wait_for_fence_value(_fenceDirect.Get(), _frameFenceValues[get_current_frame_index()], _fenceEvent);

	begin_new_frame();

	_pipelineState->frame_ended(get_command_list());
}

void VoidRenderContext_D3D12::transition_resource(std::shared_ptr<VoidResource> resource, VoidResourceState oldState,
                                                  VoidResourceState newState)
{
	auto r = std::dynamic_pointer_cast<D3D12_VoidResource>(resource);

	auto desiredState = D3D12_Translator::resource_state(newState);
	auto currentState = D3D12_Translator::resource_state(oldState);

	if (r->_currentState != desiredState)
	{
		transition_resource(get_command_list(), r->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(r), desiredState);
		r->_currentState = desiredState;
	}
}

void VoidRenderContext_D3D12::set_vertex_shader(std::shared_ptr<VoidShader> shader)
{
	auto vs = reinterpret_cast<D3D12_VoidShader*>(shader.get());
	_pipelineState->bind_vertex_shader(vs);
}

void VoidRenderContext_D3D12::set_geometry_shader(std::shared_ptr<VoidShader> shader)
{
	auto gs = reinterpret_cast<D3D12_VoidShader*>(shader.get());
	_pipelineState->bind_geo_shader(gs);
}

void VoidRenderContext_D3D12::set_pixel_shader(std::shared_ptr<VoidShader> shader)
{
	auto ps = reinterpret_cast<D3D12_VoidShader*>(shader.get());
	_pipelineState->bind_pixel_shader(ps);
}

void VoidRenderContext_D3D12::set_mesh_shader(std::shared_ptr<VoidShader> shader)
{
	auto ms = reinterpret_cast<D3D12_VoidShader*>(shader.get());
	_pipelineState->bind_mesh_shader(ms);
}

void VoidRenderContext_D3D12::set_amp_shader(std::shared_ptr<VoidShader> shader)
{
	auto as = reinterpret_cast<D3D12_VoidShader*>(shader.get());
	_pipelineState->bind_amp_shader(as);
}

void VoidRenderContext_D3D12::set_shader_root_path(std::string path)
{
	_shaderRootPath = path;
}

void VoidRenderContext_D3D12::dispatch_mesh(uint32_t x, uint32_t y, uint32_t z)
{
	pre_draw();

	get_command_list()->DispatchMesh(x, y, z);
}

std::shared_ptr<VoidStructuredBuffer> VoidRenderContext_D3D12::create_structured(size_t size, bool dynamic)
{
	auto heapType = dynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

	D3D12MA::Allocation* srvAllocation;
	ID3D12Resource* srvResource;

	// align all buffer sizes to 256
	auto alignedSize = (size + 255) & ~255;

	D3D12_RESOURCE_DESC srvDesc = {};
	srvDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	srvDesc.Alignment = 0;
	srvDesc.Width = alignedSize;
	srvDesc.Height = 1;
	srvDesc.DepthOrArraySize = 1;
	srvDesc.MipLevels = 1;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.SampleDesc.Count = 1;
	srvDesc.SampleDesc.Quality = 0;
	srvDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	srvDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12MA::ALLOCATION_DESC srvAllocDesc = {};
	srvAllocDesc.HeapType = heapType;

	HRESULT hr = _allocator->CreateResource(&srvAllocDesc, &srvDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
	                                        &srvAllocation, IID_PPV_ARGS(&srvResource));

	if (FAILED(hr))
	{
		auto reason = _device->GetDeviceRemovedReason();
		throw std::runtime_error("failed to allocate buffer");
	}

	auto res = std::make_shared<D3D12_VoidStructuredBuffer>(srvAllocation, srvResource, alignedSize, heapType, this);
	res->_currentState = D3D12_RESOURCE_STATE_COMMON;
	res->_resourceDesc = srvDesc;

	return res;
}

std::shared_ptr<VoidSampler> VoidRenderContext_D3D12::create_sampler(VoidSamplerFilter filter,
                                                                     VoidSamplerAddressMode uAddress,
                                                                     VoidSamplerAddressMode vAddress,
                                                                     VoidSamplerAddressMode wAddress, float minLOD,
                                                                     float maxLOD)
{
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_Translator::sampler_filter(filter);
	samplerDesc.AddressU = D3D12_Translator::address_mode(uAddress);
	samplerDesc.AddressV = D3D12_Translator::address_mode(vAddress);
	samplerDesc.AddressW = D3D12_Translator::address_mode(wAddress);
	samplerDesc.MipLODBias = 0.f; // todo
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.BorderColor[0] = 1.f;
	samplerDesc.BorderColor[1] = 1.f;
	samplerDesc.BorderColor[2] = 1.f;
	samplerDesc.BorderColor[3] = 1.f;
	samplerDesc.MinLOD = minLOD;
	samplerDesc.MaxLOD = maxLOD;

	auto desc = _cpuSamplerDescAllocator->allocate();

	_device->CreateSampler(&samplerDesc, desc);

	return std::make_shared<D3D12_VoidSampler>(desc, this);
}

std::shared_ptr<VoidImage> VoidRenderContext_D3D12::create_image(VoidExtent3D size, VoidImageFormat format,
                                                                 VoidResourceFlags flags,
                                                                 VoidResourceState initialState)
{
	// create texture resource

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = static_cast<UINT64>(size.width);
	textureDesc.Height = static_cast<UINT>(size.height);
	textureDesc.DepthOrArraySize = static_cast<UINT16>(size.depth);
	textureDesc.MipLevels = 1;
	textureDesc.Format = D3D12_Translator::format(format);
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_Translator::resource_flags(flags);

	D3D12MA::Allocation* textureAllocation;
	ID3D12Resource* textureResource;

	D3D12MA::ALLOCATION_DESC allocDesc = {};
	allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE* clearValue = nullptr;

	D3D12_CLEAR_VALUE clearValDesc = {};

	if ((flags & VoidResourceFlags::DepthStencil) == DepthStencil)
	{
		clearValDesc.Format = D3D12_Translator::dsv_type_map(D3D12_Translator::format(format));
		clearValDesc.DepthStencil.Depth = 1.f;
		clearValDesc.DepthStencil.Stencil = 0;

		clearValue = &clearValDesc;
	}
	else if((flags & VoidResourceFlags::RenderTarget) == RenderTarget)
	{
		clearValDesc.Format = textureDesc.Format;
		clearValDesc.Color[0] = 0.f;
		clearValDesc.Color[1] = 0.f;
		clearValDesc.Color[2] = 0.f;
		clearValDesc.Color[3] = 0.f;

		clearValue = &clearValDesc;
	}

	HRESULT hr = _allocator->CreateResource(&allocDesc, &textureDesc, D3D12_Translator::resource_state(initialState),
	                                        clearValue, &textureAllocation, IID_PPV_ARGS(&textureResource));

	if (FAILED(hr))
	{
		throw std::runtime_error("failed to create texture resource");
	}

	auto response = std::make_shared<D3D12_VoidImage>(size.width, size.height, size.depth, textureAllocation,
	                                                  textureResource, D3D12_Translator::format(format), this);
	response->_currentState = D3D12_Translator::resource_state(initialState);
	response->_resourceDesc = textureDesc;

	return response;
}

std::shared_ptr<VoidIndexBuffer> VoidRenderContext_D3D12::create_index_buffer(uint32_t* pIndices, size_t count)
{
	auto bufferSize = sizeof(uint32_t) * count;

	D3D12MA::Allocation* indexBufferAllocation;
	ID3D12Resource* indexBuffer;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	bufferDesc.Width = bufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12MA::ALLOCATION_DESC allocDesc = {};
	allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	check_result(_allocator->CreateResource(&allocDesc, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
	                                        &indexBufferAllocation, IID_PPV_ARGS(&indexBuffer)));

	D3D12MA::Allocation* uploadBufferAllocation;
	ID3D12Resource* uploadBuffer;

	D3D12_RESOURCE_DESC uploadBufferDesc = {};
	uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadBufferDesc.Alignment = 0;
	uploadBufferDesc.Width = bufferSize;
	uploadBufferDesc.Height = 1;
	uploadBufferDesc.DepthOrArraySize = 1;
	uploadBufferDesc.MipLevels = 1;
	uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadBufferDesc.SampleDesc.Count = 1;
	uploadBufferDesc.SampleDesc.Quality = 0;
	uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12MA::ALLOCATION_DESC uploadAllocDesc = {};
	uploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

	check_result(_allocator->CreateResource(&uploadAllocDesc, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
	                                        nullptr, &uploadBufferAllocation, IID_PPV_ARGS(&uploadBuffer)));

	void* mappedData = nullptr;
	check_result(uploadBuffer->Map(0, nullptr, &mappedData));
	memcpy(mappedData, pIndices, bufferSize);
	uploadBuffer->Unmap(0, nullptr);

	immediate_submit([&](ID3D12GraphicsCommandList* cmd)
	{
		cmd->CopyBufferRegion(indexBuffer, 0, uploadBuffer, 0, bufferSize);

		transition_resource(cmd, indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	});

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	uploadBufferAllocation->Release();

	return std::make_shared<D3D12_VoidIndexBuffer>(pIndices, count, indexBufferAllocation, indexBufferView, indexBuffer,
	                                               this);
}

std::shared_ptr<VoidShader> VoidRenderContext_D3D12::create_shader(std::string path, VoidShaderStage stage)
{
	path = _shaderRootPath + path;

	std::wstring wpath;
	{
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.size()), nullptr, 0);
		wpath = std::wstring(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.size()), &wpath[0], size_needed);
	}

	std::ifstream shaderFile(wpath, std::ios::binary);
	if (!shaderFile)
	{
		throw std::runtime_error("failed to load shader");
	}

	std::vector<char> rawFile((std::istreambuf_iterator(shaderFile)), std::istreambuf_iterator<char>());

	CD3DX12_HEAP_PROPERTIES def(D3D12_HEAP_TYPE_DEFAULT);

	ComPtr<ID3DBlob> shader;

	check_result(D3DCreateBlob(rawFile.size(), &shader));
	memcpy(shader->GetBufferPointer(), rawFile.data(), rawFile.size());

	return std::make_shared<D3D12_VoidShader>(rawFile, shader, stage);
}

void VoidRenderContext_D3D12::register_render_target_view(VoidImage* target)
{
	auto rt = reinterpret_cast<D3D12_VoidImage*>(target);

	if (rt->get_depth() < 2)
	{
		auto rtv = _cpuRtvDescAllocator->allocate();

		_device->CreateRenderTargetView(rt->get_d3d12_resource(), nullptr, rtv);

		rt->_rtvDescriptor = rtv;
	}
	else
	{
		auto rtv = _cpuRtvDescAllocator->allocate();

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = rt->get_d3d12_format();
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.ArraySize = rt->get_depth();
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;

		_device->CreateRenderTargetView(rt->get_d3d12_resource(), &rtvDesc, rtv);

		rt->_rtvDescriptor = rtv;
	}
}

void VoidRenderContext_D3D12::register_depth_stencil_view(VoidImage* target)
{
	auto ds = reinterpret_cast<D3D12_VoidImage*>(target);

	if (ds->get_depth() < 2)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = D3D12_Translator::dsv_type_map(ds->get_d3d12_format());
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D.MipSlice = 0;

		auto dsvHandle = _cpuDsvDescAllocator->allocate();

		_device->CreateDepthStencilView(ds->get_d3d12_resource(), &dsvDesc, dsvHandle);

		ds->_dsvDescriptor = dsvHandle;
	}
	else
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = D3D12_Translator::dsv_type_map(ds->get_d3d12_format());
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.ArraySize = ds->get_depth();
		dsvDesc.Texture2DArray.FirstArraySlice = 0;

		auto dsv = _cpuDsvDescAllocator->allocate();

		_device->CreateDepthStencilView(ds->get_d3d12_resource(), &dsvDesc, dsv);

		ds->_dsvDescriptor = dsv;
	}
}

void VoidRenderContext_D3D12::register_image_view(VoidImage* image)
{
	auto i = reinterpret_cast<D3D12_VoidImage*>(image);

	auto& desc = i->_resourceDesc;

	if (i->get_depth() < 2)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = D3D12_Translator::srv_type_map(desc.Format);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

		auto srv = _cpuBufferDescAllocator->allocate();

		_device->CreateShaderResourceView(i->get_d3d12_resource(), &srvDesc, srv);

		i->_srvDescriptor = srv;
	}
	else
	{
		// create srv array

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = D3D12_Translator::srv_type_map(desc.Format);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		auto srv = _cpuBufferDescAllocator->allocate();

		_device->CreateShaderResourceView(i->get_d3d12_resource(), &srvDesc, srv);

		i->_srvDescriptor = srv;
	}
}

void VoidRenderContext_D3D12::register_resource_view(VoidStructuredBuffer* buffer)
{
	auto b = reinterpret_cast<D3D12_VoidStructuredBuffer*>(buffer);

	auto& desc = b->_resourceDesc;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescView = {};
	srvDescView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescView.Format = DXGI_FORMAT_UNKNOWN;
	srvDescView.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDescView.Buffer.FirstElement = 0;
	srvDescView.Buffer.NumElements = 1;
	srvDescView.Buffer.StructureByteStride = static_cast<UINT>(desc.Width);
	srvDescView.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	auto srv = _cpuBufferDescAllocator->allocate();

	_device->CreateShaderResourceView(b->get_d3d12_resource(), &srvDescView, srv);

	b->_srvDescriptor = srv;
}

void VoidRenderContext_D3D12::register_resource_view(VoidStructuredBuffer* buffer, size_t numElements,
                                                     size_t elementSize)
{
	auto b = reinterpret_cast<D3D12_VoidStructuredBuffer*>(buffer);

	auto& desc = b->_resourceDesc;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescView = {};
	srvDescView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescView.Format = DXGI_FORMAT_UNKNOWN;
	srvDescView.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDescView.Buffer.FirstElement = 0;
	srvDescView.Buffer.NumElements = static_cast<UINT>(numElements);
	srvDescView.Buffer.StructureByteStride = static_cast<UINT>(elementSize);
	srvDescView.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	auto srv = _cpuBufferDescAllocator->allocate();

	_device->CreateShaderResourceView(b->get_d3d12_resource(), &srvDescView, srv);

	b->_srvDescriptor = srv;
}

void VoidRenderContext_D3D12::register_constant_view(VoidStructuredBuffer* buffer)
{
	auto b = reinterpret_cast<D3D12_VoidStructuredBuffer*>(buffer);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = b->get_d3d12_resource()->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(b->get_size());

	auto cbvHandle = _cpuBufferDescAllocator->allocate();
	_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

	b->_cbvDescriptor = cbvHandle;
}

void VoidRenderContext_D3D12::copy_image(void* data, VoidImage* image)
{
	auto i = reinterpret_cast<D3D12_VoidImage*>(image);

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(i->get_d3d12_resource(), 0, 1);

	D3D12_RESOURCE_DESC uploadBufferDesc = {};
	uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadBufferDesc.Alignment = 0;
	uploadBufferDesc.Width = uploadBufferSize;
	uploadBufferDesc.Height = 1;
	uploadBufferDesc.DepthOrArraySize = 1;
	uploadBufferDesc.MipLevels = 1;
	uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadBufferDesc.SampleDesc.Count = 1;
	uploadBufferDesc.SampleDesc.Quality = 0;
	uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12MA::Allocation* uploadAllocation;
	ID3D12Resource* uploadBuffer;

	D3D12MA::ALLOCATION_DESC uploadAllocDesc = {};
	uploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

	HRESULT hr = _allocator->CreateResource(&uploadAllocDesc, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
	                                        nullptr, &uploadAllocation, IID_PPV_ARGS(&uploadBuffer));

	if (FAILED(hr))
	{
		throw std::runtime_error("failed to create upload buffer");
	}

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = data;
	textureData.RowPitch = i->get_width() * D3D12_Translator::num_channels(i->get_d3d12_format());
	textureData.SlicePitch = textureData.RowPitch * i->get_height();

	VOID_ASSERT_STATE_SET(i);

	immediate_submit([&](ID3D12GraphicsCommandList* cmd)
	{
		auto oldState = VOID_GET_RESOURCE_STATE(i);

		if (oldState != D3D12_RESOURCE_STATE_COPY_DEST)
			transition_resource(cmd, i->get_d3d12_resource(), oldState, D3D12_RESOURCE_STATE_COPY_DEST);

		UpdateSubresources(cmd, i->get_d3d12_resource(), uploadBuffer, 0, 0, 1, &textureData);

		transition_resource(cmd, i->get_d3d12_resource(), D3D12_RESOURCE_STATE_COPY_DEST, oldState);
	});

	uploadBuffer->Release();
	uploadAllocation->Release();
}

void VoidRenderContext_D3D12::copy_buffer(VoidStructuredBuffer* src, VoidStructuredBuffer* dst)
{
	auto vSrc = reinterpret_cast<D3D12_VoidStructuredBuffer*>(src);
	auto vDst = reinterpret_cast<D3D12_VoidStructuredBuffer*>(dst);

	VOID_ASSERT_STATE_SET(vSrc);
	VOID_ASSERT_STATE_SET(vDst);

	immediate_submit([&](ID3D12GraphicsCommandList* cmd)
	{
		transition_resource(cmd, vSrc->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(vSrc),
		                    D3D12_RESOURCE_STATE_COPY_SOURCE);
		transition_resource(cmd, vDst->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(vDst),
		                    D3D12_RESOURCE_STATE_COPY_DEST);

		cmd->CopyBufferRegion(vDst->get_d3d12_resource(), 0, vSrc->get_d3d12_resource(), 0, vSrc->get_size());
	});

	vSrc->_currentState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	vDst->_currentState = D3D12_RESOURCE_STATE_COPY_DEST;
}

//void VoidRenderContext_D3D12::release_image(std::shared_ptr<VoidImage> image)
//{
//	auto i = reinterpret_cast<D3D12_VoidImage*>(image.get());
//	free_descriptors(i);
//	i->get_d3d12_resource()->Release();
//	i->get_d3d12_allocation()->Release();
//}
//
//void VoidRenderContext_D3D12::release_index_buffer(std::shared_ptr<VoidIndexBuffer> buffer)
//{
//	auto b = reinterpret_cast<D3D12_VoidIndexBuffer*>(buffer.get());
//	b->get_d3d12_resource()->Release();
//	b->get_d3d12_allocation()->Release();
//}

void VoidRenderContext_D3D12::release_sampler(VoidSampler* sampler)
{
	auto s = reinterpret_cast<D3D12_VoidSampler*>(sampler);
	_cpuSamplerDescAllocator->free(s->get_d3d12_handle());
}

void VoidRenderContext_D3D12::release_shader(VoidShader* shader)
{
	auto s = reinterpret_cast<D3D12_VoidShader*>(shader);
	s->get_d3d12_blob()->Release();
}

void VoidRenderContext_D3D12::release_resource(VoidResource* resource)
{
	auto r = reinterpret_cast<D3D12_VoidResource*>(resource);
	free_descriptors(r);
	r->get_d3d12_resource()->Release();
	r->get_d3d12_allocation()->Release();
}

//void VoidRenderContext_D3D12::release_structured(std::shared_ptr<VoidStructuredBuffer> buffer)
//{
//	auto b = reinterpret_cast<D3D12_VoidStructuredBuffer*>(buffer.get());
//	free_descriptors(b);
//	b->get_d3d12_resource()->Release();
//	b->get_d3d12_allocation()->Release();
//}

void VoidRenderContext_D3D12::draw_indexed(uint32_t indexCount, uint32_t firstIndex)
{
	pre_draw();

	get_command_list()->DrawIndexedInstanced(indexCount, 1, firstIndex, 0, 0);
}

void VoidRenderContext_D3D12::draw_instanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation,
                                             uint32_t startInstanceLocation)
{
	pre_draw();
	get_command_list()->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
}

std::shared_ptr<IVoidGraphicsCommandList> VoidRenderContext_D3D12::allocate_graphics_list()
{
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12CommandAllocator> allocator;
	
	check_result(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
	// todo handle reset on new frame?
	//commandAllocator->Reset();
	_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	return std::make_shared<VoidGraphicsCommandList_D3D12>(commandList, allocator, _device.Get(), &_windowExtent);
}

void VoidRenderContext_D3D12::submit_graphics_list(std::shared_ptr<IVoidGraphicsCommandList> list)
{
	{
		std::unique_lock lock{_listQueueLock};
		_listLocks[get_current_frame_index()].push(list);
	}

	auto d3dList = reinterpret_cast<VoidGraphicsCommandList_D3D12*>(list.get());

	submit_graphics_list(d3dList->_commandList.Get());
}

void VoidRenderContext_D3D12::submit_graphics_list(ID3D12GraphicsCommandList* list)
{
	list->Close();

	ID3D12CommandList* const commandLists[] = {list};

	_directCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void VoidRenderContext_D3D12::blit_to_render_target(std::shared_ptr<VoidImage> src, std::shared_ptr<VoidImage> dest)
{
	_resourceLocks[get_current_frame_index()].push(src);
	_resourceLocks[get_current_frame_index()].push(dest);

	auto srcImage = reinterpret_cast<D3D12_VoidImage*>(src.get());
	auto dstImage = reinterpret_cast<D3D12_VoidImage*>(dest.get());

	//VOID_ASSERT(dstImage->get_d3d12_format() == DXGI_FORMAT_R8G8B8A8_UNORM, "blit destination must be R8G8B8A8_UNORM");
	VOID_ASSERT_STATE_SET(srcImage);
	VOID_ASSERT_STATE_SET(dstImage);

	// we are binding a special PSO, normal pipeline state will need to be rebound

	auto cmd = get_command_list();

	cmd->SetGraphicsRootSignature(_blitRootSignature.Get());

	_blitStateDesc.renderTargets[0] = dstImage;

	auto pso = _blitPsoCache->get_state(_blitStateDesc);

	cmd->SetPipelineState(pso);

	auto srvDesc = _pipelineState->allocate_buffer_desc();

	auto srcSrv = srcImage->get_d3d12_srv();
	auto dstRtv = dstImage->get_d3d12_rtv();

	auto srvCpu = srvDesc.first;
	auto srvGpu = srvDesc.second;

	UINT one = 1;

	_device->CopyDescriptors(1, &srvCpu, &one, 1, &srcSrv, &one, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (srcImage->_currentState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	{
		transition_resource(cmd, srcImage->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(srcImage),
		                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		srcImage->_currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}

	if (dstImage->_currentState != D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		transition_resource(cmd, dstImage->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(dstImage),
		                    D3D12_RESOURCE_STATE_RENDER_TARGET);
		dstImage->_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	cmd->SetGraphicsRootDescriptorTable(0, srvGpu);
	cmd->OMSetRenderTargets(1, &dstRtv, FALSE, nullptr);

	D3D12_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(dstImage->get_width());
	vp.Height = static_cast<float>(dstImage->get_height());
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	cmd->RSSetViewports(1, &vp);

	D3D12_RECT scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(dstImage->get_width());
	scissorRect.bottom = static_cast<LONG>(dstImage->get_height());

	cmd->RSSetScissorRects(1, &scissorRect);

	cmd->DrawInstanced(4, 1, 0, 0);

	_pipelineState->force_bind_state(cmd);
}

void VoidRenderContext_D3D12::clear_depth(float value, VoidImage* image)
{
	auto cmd = get_command_list();

	auto ds = reinterpret_cast<D3D12_VoidImage*>(image);

	VOID_ASSERT(ds->_dsvDescriptor.has_value(), "can't clear, no dsv");
	VOID_ASSERT_STATE_SET(ds);

	if (ds->_currentState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
	{
		transition_resource(cmd, ds->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(ds),
		                    D3D12_RESOURCE_STATE_DEPTH_WRITE);
		ds->_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	cmd->ClearDepthStencilView(ds->get_d3d12_dsv(), D3D12_CLEAR_FLAG_DEPTH, value, 0, 0, nullptr);
}

void VoidRenderContext_D3D12::clear_image(float rgba[4], VoidImage* image)
{
	auto cmd = get_command_list();

	auto rtv = reinterpret_cast<D3D12_VoidImage*>(image);

	VOID_ASSERT(rtv->_rtvDescriptor.has_value(), "can't clear, no rtv");
	VOID_ASSERT_STATE_SET(rtv);

	if (rtv->_currentState != D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		transition_resource(cmd, rtv->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(rtv),
		                    D3D12_RESOURCE_STATE_RENDER_TARGET);
		rtv->_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	cmd->ClearRenderTargetView(rtv->get_d3d12_rtv(), rgba, 0, nullptr);
}

void VoidRenderContext_D3D12::immediate_submit(std::function<void(ID3D12GraphicsCommandList*)> func)
{
	//ID3D12GraphicsCommandList* cmd;

	//HRESULT hr = _immediateCmdAllocator->Reset();
	//if (FAILED(hr))
	//{
	//	throw std::runtime_error("failed to reset");
	//}

	//hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _immediateCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmd));
	//if(FAILED(hr))
	//{
	//	throw std::runtime_error("failed to create list");
	//}

	//// why do i have to close this? exception is thrown if we don't.
	//cmd->Close();

	//hr = cmd->Reset(_immediateCmdAllocator.Get(), nullptr);
	//if(FAILED(hr))
	//{
	//	throw std::runtime_error("failed to reset cmd");
	//}

	auto list = allocate_graphics_list();

	auto d3dlist = reinterpret_cast<VoidGraphicsCommandList_D3D12*>(list.get());

	func(d3dlist->_commandList.Get());

	auto hr = d3dlist->_commandList->Close();
	if (FAILED(hr))
	{
		throw std::runtime_error("failed to close cmd");
	}

	ID3D12CommandList* ppCommandLists[] = {d3dlist->_commandList.Get()};
	_directUploadQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	_immediateFenceValue++;
	hr = _directUploadQueue->Signal(_immediateFence.Get(), _immediateFenceValue);
	if (FAILED(hr))
	{
		throw std::runtime_error("fence failed");
	}

	if (_immediateFence->GetCompletedValue() < _immediateFenceValue)
	{
		hr = _immediateFence->SetEventOnCompletion(_immediateFenceValue, _immediateFenceEvent);
		if (FAILED(hr))
		{
			throw std::runtime_error("failed to set fence event");
		}

		WaitForSingleObject(_immediateFenceEvent, INFINITE);
	}

	//cmd->Release();
}

void VoidRenderContext_D3D12::set_cull_mode(VoidCullMode mode)
{
	_pipelineState->set_cull_mode(D3D12_Translator::cull_mode(mode));
}

void VoidRenderContext_D3D12::set_depth_test_enabled(bool enabled)
{
	_pipelineState->set_enable_depth(enabled);
}

void VoidRenderContext_D3D12::set_num_render_targets(uint32_t count)
{
	_pipelineState->set_num_rt(count);
}

void VoidRenderContext_D3D12::set_primitive(VoidTopologyType type)
{
	_pipelineState->set_topology(D3D12_Translator::topology(type));
}

void VoidRenderContext_D3D12::set_input_primitive(VoidInputTopologyType type)
{
	_pipelineState->set_input_topology(D3D12_Translator::input_topology(type));
}

void VoidRenderContext_D3D12::set_stencil_test_enabled(bool enabled)
{
	_pipelineState->set_enable_stencil(enabled);
}

void VoidRenderContext_D3D12::set_draw_extent(VoidExtent2D extent)
{
	_pipelineState->set_draw_extent(extent);
}

void VoidRenderContext_D3D12::set_fill_mode(VoidFillMode mode)
{
	_pipelineState->set_fill_mode(D3D12_Translator::fill_mode(mode));
}

void VoidRenderContext_D3D12::set_depth_clip(bool enabled)
{
	_pipelineState->set_depth_clip(enabled);
}

void VoidRenderContext_D3D12::bind_index_buffer(std::shared_ptr<VoidIndexBuffer> buffer)
{
	_resourceLocks[get_current_frame_index()].push(buffer);
	auto b = std::dynamic_pointer_cast<D3D12_VoidIndexBuffer>(buffer);
	_pipelineState->bind_index_buffer(b);
}

void VoidRenderContext_D3D12::bind_render_target(uint16_t slot, std::shared_ptr<VoidImage> target)
{
	_resourceLocks[get_current_frame_index()].push(target);
	auto rt = std::dynamic_pointer_cast<D3D12_VoidImage>(target);
	_pipelineState->bind_render_target(slot, rt);
}

void VoidRenderContext_D3D12::bind_depth_target(std::shared_ptr<VoidImage> target)
{
	_resourceLocks[get_current_frame_index()].push(target);
	auto rt = std::dynamic_pointer_cast<D3D12_VoidImage>(target);
	_pipelineState->bind_depth_target(rt);
}

void VoidRenderContext_D3D12::bind_resource(uint16_t slot, std::shared_ptr<VoidImage> image)
{
	_resourceLocks[get_current_frame_index()].push(image);
	auto i = std::dynamic_pointer_cast<D3D12_VoidImage>(image);
	_pipelineState->bind_resource(slot, i);
}

void VoidRenderContext_D3D12::bind_resource(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer)
{
	_resourceLocks[get_current_frame_index()].push(buffer);
	auto b = std::dynamic_pointer_cast<D3D12_VoidStructuredBuffer>(buffer);
	_pipelineState->bind_resource(slot, b);
}

void VoidRenderContext_D3D12::bind_sampler(uint16_t slot, std::shared_ptr<VoidSampler> sampler)
{
	_resourceLocks[get_current_frame_index()].push(sampler);
	auto s = std::dynamic_pointer_cast<D3D12_VoidSampler>(sampler);
	_pipelineState->bind_resource(slot, s);
}

//void VoidRenderContext_D3D12::bind_depth_target(uint16_t slot, bool array, uint32_t layer)
//{
//	_pipelineState->bind_depth_target(slot);
//}

void VoidRenderContext_D3D12::bind_constant(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer)
{
	_resourceLocks[get_current_frame_index()].push(buffer);
	auto b = std::dynamic_pointer_cast<D3D12_VoidStructuredBuffer>(buffer);
	_pipelineState->bind_constant(slot, b);
}

std::shared_ptr<VoidImage> VoidRenderContext_D3D12::get_back_buffer()
{
	return _backBuffers[get_current_frame_index()];
}

uint32_t VoidRenderContext_D3D12::get_current_frame_index()
{
	return _swapChain->GetCurrentBackBufferIndex();
}

VoidExtent2D VoidRenderContext_D3D12::get_current_draw_extent()
{
	return _windowExtent;
}

//std::shared_ptr<VoidSampler> VoidRenderContext_D3D12::get_default_linear_sampler()
//{
//	return _defaultLinearSampler;
//}
//
//std::shared_ptr<VoidSampler> VoidRenderContext_D3D12::get_default_nearest_sampler()
//{
//	return _defaultNearestSampler;
//}

uint8_t VoidRenderContext_D3D12::get_frame_index()
{
	return static_cast<uint8_t>(_swapChain->GetCurrentBackBufferIndex());
}

ID3D12GraphicsCommandList* VoidRenderContext_D3D12::get_command_list_raw()
{
	return get_command_list();
}

Void3DMemoryStats VoidRenderContext_D3D12::get_memory_usage_stats()
{
	D3D12MA::Budget budget;
	_allocator->GetBudget(&budget, nullptr);

	auto stats = Void3DMemoryStats();
	stats.available = budget.BudgetBytes;
	stats.used = budget.UsageBytes;

	return stats;
}


//void VoidRenderContext_D3D12::transition_resource(VoidRenderTarget* target, VoidImageState newState)
//{
//	auto d3d12Target = reinterpret_cast<D3D12_VoidRenderTarget*>(target);
//	transition_resource(get_command_list(), d3d12Target->get_d3d12_resource(), d3d12Target->_currentState, D3D12_Translator::resource_state(newState));
//	d3d12Target->_currentState = D3D12_Translator::resource_state(newState);
//}
//
//void VoidRenderContext_D3D12::transition_resource(VoidBufferImage* target, VoidImageState newState)
//{
//	auto d3d12resource = reinterpret_cast<D3D12_VoidResource*>(target);
//	transition_resource(get_command_list(), d3d12resource->get_d3d12_resource(), d3d12resource->_currentState, D3D12_Translator::resource_state(newState));
//	d3d12resource->_currentState = D3D12_Translator::resource_state(newState);
//}
//
//void VoidRenderContext_D3D12::transition_resource(VoidImage* image, VoidImageState newState)
//{
//	auto d3d12Image = reinterpret_cast<D3D12_VoidImage*>(image);
//	transition_resource(get_command_list(), d3d12Image->get_d3d12_resource(), d3d12Image->_currentState, D3D12_Translator::resource_state(newState));
//	d3d12Image->_currentState = D3D12_Translator::resource_state(newState);
//}

ComPtr<IDXGIAdapter4> VoidRenderContext_D3D12::get_adapter(bool useWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#ifdef _DEBUG
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	check_result(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (useWarp)
	{
		check_result(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		check_result(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		ComPtr<IDXGIFactory6> factory6;
		dxgiFactory.As(&factory6);

		HRESULT hr;

		if (factory6)
			_dxgiFactory = factory6;

		for (UINT testAdapterIndex = 0; ; ++testAdapterIndex)
		{
			ComPtr<IDXGIAdapter1> testAdapter;

			if (factory6)
			{
				ComPtr<IDXGIAdapter> baseAdapter;
				hr = factory6->EnumAdapterByGpuPreference(testAdapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				                                          IID_PPV_ARGS(&baseAdapter));
				if (SUCCEEDED(hr))
				{
					baseAdapter.As(&testAdapter);
				}
			}
			else
			{
				throw std::runtime_error("need dxgi 6");
				hr = dxgiFactory->EnumAdapters1(testAdapterIndex, &testAdapter);
			}

			if (FAILED(hr))
				break;

			DXGI_ADAPTER_DESC1 testDesc;
			if (!check_result(testAdapter->GetDesc1(&testDesc)))
			{
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(testAdapter.Get(), _desiredFeatureLevel, _uuidof(ID3D12Device), nullptr)))
			{
				const bool isSoftware = (testDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
				const SIZE_T memSize = testDesc.DedicatedVideoMemory / 1048576;
				auto wstr = std::wstring(testDesc.Description);
				//_log.information(fmt::format("found device {} with {}mb memory", std::string(wstr.begin(), wstr.end()), memSize));

				if (!dxgiAdapter1)
				{
					dxgiAdapter1 = std::move(testAdapter);
				}
			}
		}
	}

	if (dxgiAdapter1)
	{
		dxgiAdapter1.As(&dxgiAdapter4);
	}

	return dxgiAdapter4;
}

bool VoidRenderContext_D3D12::init_device_and_resources()
{
	ComPtr<ID3D12Device2> device;
	check_result(D3D12CreateDevice(_adapter.Get(), _desiredFeatureLevel, IID_PPV_ARGS(&device)));

	check_result(device->QueryInterface(IID_PPV_ARGS(&_device)));

#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(_device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	}
#endif

	// init D3D12MA
	{
		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = _device.Get();
		allocatorDesc.pAdapter = _adapter.Get();
		allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
		allocatorDesc.PreferredBlockSize = 0;

		if (FAILED(D3D12MA::CreateAllocator(&allocatorDesc, &_allocator)))
		{
			return false;
		}
	}

	// init queues
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		check_result(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_directCommandQueue)));
		_directCommandQueue->SetName(L"Direct Queue"); // don't care if this fails

		check_result(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_directUploadQueue)));
		_directUploadQueue->SetName(L"Direct Queue"); // don't care if this fails

		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

		check_result(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_copyCommandQueue)));
		_copyCommandQueue->SetName(L"Copy Queue");

		check_result(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fenceCopy)));
		_fenceCopy->SetName(L"fenceCopy");

		check_result(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_immediateFence)));

		check_result(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fenceDirect)));
		_fenceDirect->SetName(L"fenceDirect");

		_fenceEvent = CreateEventA(nullptr, FALSE, FALSE, "fenceEvent");
	}

	// init descriptors
	{
		_cpuBufferDescAllocator = new CpuDescriptorAllocator<_maxCpuBufferDesc>();
		_cpuRtvDescAllocator = new CpuDescriptorAllocator<_maxCpuRtvDesc>();
		_cpuSamplerDescAllocator = new CpuDescriptorAllocator<_maxCpuSamplerDesc>();
		_cpuDsvDescAllocator = new CpuDescriptorAllocator<_maxCpuDsvDesc>();

		_cpuBufferDescAllocator->init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		_cpuRtvDescAllocator->init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cpuSamplerDescAllocator->init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		_cpuDsvDescAllocator->init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

#ifdef VOID_USE_IMGUI
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		check_result(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_imguiDescHeap)));
#endif
	}

	// init allocators
	{
		check_result(
			_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_immediateCmdAllocator)));

		for (uint8_t i = 0; i < _numFrames; i++)
		{
			check_result(
				_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator[i])));
			check_result(
				_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_prologueAllocator[i])));
		}
	}

	// init swap chain
	{
		build_swapchain(_windowExtent);
	}

	// default samplers
	{
		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MipLODBias = 0.f; // todo
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.BorderColor[0] = 1.f;
		samplerDesc.BorderColor[1] = 1.f;
		samplerDesc.BorderColor[2] = 1.f;
		samplerDesc.BorderColor[3] = 1.f;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

		/*auto sampler = _cpuSamplerDescAllocator->allocate();

		_device->CreateSampler(&samplerDesc, sampler);

		_defaultLinearSampler = std::make_shared<D3D12_VoidSampler>(sampler, this);

		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

		sampler = _cpuSamplerDescAllocator->allocate();

		_device->CreateSampler(&samplerDesc, sampler);

		_defaultNearestSampler = std::make_shared<D3D12_VoidSampler>(sampler, this);*/
	}

	// init pipeline state
	D3D12_PipelineState::init(_device.Get());
	_pipelineState = new D3D12_PipelineState(_device.Get());
	_pipelineState->init(_windowExtent);

	init_blit_pso();

	return true;
}

void VoidRenderContext_D3D12::transition_resource(ID3D12GraphicsCommandList* cmd, ID3D12Resource* resource,
                                                  D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmd->ResourceBarrier(1, &barrier);
}

uint64_t VoidRenderContext_D3D12::signal_fence(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence,
                                               uint64_t& fenceValue)
{
	auto fenceValueForSignal = ++fenceValue;
	check_result(commandQueue->Signal(fence, fenceValueForSignal));

	return fenceValueForSignal;
}

void VoidRenderContext_D3D12::wait_for_fence_value(ID3D12Fence* fence, uint64_t fenceValue, HANDLE fenceEvent,
                                                   std::chrono::milliseconds duration)
{
	if (fence->GetCompletedValue() < fenceValue)
	{
		check_result(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
	}
}

void VoidRenderContext_D3D12::begin_new_frame()
{
	// empty the resource locks from last frame
	auto& locks = _resourceLocks[get_current_frame_index()];
	while (!locks.empty())
	{
		auto& lock = locks.front();
		lock.reset();
		locks.pop();
	}

	auto& listLocks = _listLocks[get_current_frame_index()];
	while (!listLocks.empty())
	{
		auto& lock = listLocks.front();
		lock.reset();
		listLocks.pop();
	}

	auto commandAllocator = _commandAllocator[get_current_frame_index()];

	commandAllocator->Reset();

	auto commandList = get_command_list();

	commandList->Reset(commandAllocator.Get(), nullptr);

	auto prologueAllocator = _prologueAllocator[get_current_frame_index()];
	prologueAllocator->Reset();

	auto prologueList = get_prologue_list();

	prologueList->Reset(prologueAllocator.Get(), nullptr);

	if (_needResize)
	{
		build_swapchain(_needResizeTo);
		_needResize = false;
		_windowExtent = _needResizeTo;
		std::cout << "CPU buffer descs used: " << _cpuBufferDescAllocator->get_used() << "\n";
	}

	auto backBuffer = _backBuffers[get_current_frame_index()];

#ifdef VOID_USE_IMGUI
	ImGui_ImplSDL2_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();
	//ImGui::ShowDemoWindow();
#endif

	transition_resource(get_prologue_list(), backBuffer->get_d3d12_resource(), D3D12_RESOURCE_STATE_PRESENT,
	                    D3D12_RESOURCE_STATE_RENDER_TARGET);
	submit_graphics_list(prologueList);
}

ID3D12GraphicsCommandList6* VoidRenderContext_D3D12::get_command_list()
{
	if (_graphicsCommandList == nullptr)
	{
		auto commandAllocator = _commandAllocator[get_current_frame_index()];
		commandAllocator->Reset();
		_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
		                           IID_PPV_ARGS(&_graphicsCommandList));
	}

	return _graphicsCommandList.Get();
}

ID3D12GraphicsCommandList6* VoidRenderContext_D3D12::get_prologue_list()
{
	if (_prologueCommandList == nullptr)
	{
		auto commandAllocator = _prologueAllocator[get_current_frame_index()];
		commandAllocator->Reset();
		_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
		                           IID_PPV_ARGS(&_prologueCommandList));
	}

	return _prologueCommandList.Get();
}

void VoidRenderContext_D3D12::free_descriptors(D3D12_VoidResource* resource)
{
	if (resource->_srvDescriptor.has_value())
		_cpuBufferDescAllocator->free(resource->_srvDescriptor.value());

	if (resource->_rtvDescriptor.has_value())
		_cpuRtvDescAllocator->free(resource->_rtvDescriptor.value());

	if (resource->_dsvDescriptor.has_value())
		_cpuDsvDescAllocator->free(resource->_dsvDescriptor.value());

	if (resource->_cbvDescriptor.has_value())
		_cpuBufferDescAllocator->free(resource->_cbvDescriptor.value());
}

void VoidRenderContext_D3D12::pre_draw()
{
	_pipelineState->bind_pso_if_invalid(get_command_list());
	_pipelineState->bind_shader_resources(get_command_list());
	_pipelineState->ensure_resource_states(get_command_list());
}

void VoidRenderContext_D3D12::init_blit_pso()
{
	D3D12_DESCRIPTOR_RANGE1 descRange = {};
	descRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	descRange.NumDescriptors = 1;
	descRange.RegisterSpace = 0;
	descRange.BaseShaderRegister = 0;
	descRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	D3D12_ROOT_PARAMETER1 rootParameter = {};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameter.DescriptorTable.pDescriptorRanges = &descRange;
	rootParameter.DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC staticSampler = {};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.MipLODBias = 0.0f;
	staticSampler.MaxAnisotropy = 1;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSampler.MinLOD = 0.0f;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0; // s0
	staticSampler.RegisterSpace = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootDesc.Desc_1_1.NumParameters = 1;
	rootDesc.Desc_1_1.pParameters = &rootParameter;
	rootDesc.Desc_1_1.NumStaticSamplers = 1;
	rootDesc.Desc_1_1.pStaticSamplers = &staticSampler;
	rootDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* serializedRootSig = nullptr;
	ID3DBlob* errorBlob = nullptr;
	check_result(D3D12SerializeVersionedRootSignature(&rootDesc, &serializedRootSig, &errorBlob));


	check_result(_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
	                                          serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&_blitRootSignature)));

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

	auto vertPath = _shaderRootPath + "BlitVert.cso";

	std::ifstream shaderFile(vertPath, std::ios::binary);
	if (!shaderFile)
	{
		throw std::runtime_error("failed to load shader");
	}

	std::vector<char> rawFile((std::istreambuf_iterator(shaderFile)), std::istreambuf_iterator<char>());

	check_result(D3DCreateBlob(rawFile.size(), &vertexShader));
	memcpy(vertexShader->GetBufferPointer(), rawFile.data(), rawFile.size());

	shaderFile.close();

	_blitVertShader = std::make_shared<D3D12_VoidShader>(rawFile, vertexShader, VoidShaderStage::Vertex);

	shaderFile = std::ifstream(_shaderRootPath + "BlitPixel.cso", std::ios::binary);
	if (!shaderFile)
	{
		throw std::runtime_error("failed to load shader");
	}

	rawFile = std::vector<char>(std::istreambuf_iterator(shaderFile), std::istreambuf_iterator<char>());

	check_result(D3DCreateBlob(rawFile.size(), &pixelShader));
	memcpy(pixelShader->GetBufferPointer(), rawFile.data(), rawFile.size());

	shaderFile.close();

	_blitPixelShader = std::make_shared<D3D12_VoidShader>(rawFile, pixelShader, VoidShaderStage::Pixel);

	/*D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = _blitRootSignature.Get();
	psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	check_result(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_blitPso)));*/

	_blitStateDesc = {};
	_blitStateDesc.cullMode = D3D12_CULL_MODE_NONE;
	_blitStateDesc.rootSig = _blitRootSignature.Get();
	_blitStateDesc.vs = _blitVertShader.get();
	_blitStateDesc.ps = _blitPixelShader.get();
	_blitStateDesc.depthEnable = FALSE;
	_blitStateDesc.stencilEnable = FALSE;
	_blitStateDesc.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	_blitStateDesc.numRenderTargets = 1;
	_blitStateDesc.renderTargets = std::vector<D3D12_VoidImage*>(1);

	_blitPsoCache = std::make_unique<D3D12_PipelineStateObjectCache>(_device.Get());
}


#endif
