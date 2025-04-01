#pragma once

#include "IVoidRenderContext.h"
#ifdef VOID_STAR_D3D12_BACKEND
#include <SDL_video.h>

#include "D3D12MemAlloc.h"
#include "D3D12_PipelineState.h"
#include "D3D12_VoidImage.h"
#include "DescriptorAllocator.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <wrl.h>

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <queue>
// D3D12 extension library.
//#include <d3dx12.h>

using Microsoft::WRL::ComPtr;

class VoidGraphicsCommandList_D3D12;

class VoidRenderContext_D3D12 : public IVoidRenderContext
{
public:
    VoidRenderContext_D3D12() = default;
    ~VoidRenderContext_D3D12() override = default;

    // startup and teardown
    void init(VoidExtent2D windowExtent, SDL_Window* windowHandle) override;
    void shutdown() override;

    void window_resized(VoidExtent2D newSize) override;

    void build_swapchain(VoidExtent2D size);

    // frame
    void present() override;

    // resource state
    void transition_resource(std::shared_ptr<VoidResource> resource, VoidResourceState oldState, VoidResourceState newState) override;

    // shader binding
    void set_vertex_shader(std::shared_ptr<VoidShader> shader) override;
    void set_geometry_shader(std::shared_ptr<VoidShader> shader) override;
    void set_pixel_shader(std::shared_ptr<VoidShader> shader) override;
    void set_mesh_shader(std::shared_ptr<VoidShader> shader) override;
    void set_amp_shader(std::shared_ptr<VoidShader> shader) override;
    void set_shader_root_path(std::string path) override;

    // compute dispatch
    void dispatch_mesh(uint32_t x, uint32_t y, uint32_t z) override;

    // resource creation
    std::shared_ptr<VoidStructuredBuffer> create_structured(size_t size, bool dynamic = false) override;
    std::shared_ptr<VoidSampler> create_sampler(VoidSamplerFilter filter, VoidSamplerAddressMode uAddress, VoidSamplerAddressMode vAddress, VoidSamplerAddressMode wAddress, float minLOD, float maxLOD) override;
    std::shared_ptr<VoidImage> create_image(VoidExtent3D size, VoidImageFormat format, VoidResourceFlags flags = VoidResourceFlags::None, VoidResourceState initialState = VoidResourceState::ShaderRead) override;
    std::shared_ptr<VoidIndexBuffer> create_index_buffer(uint32_t* pIndices, size_t count) override;
    std::shared_ptr<VoidShader> create_shader(std::string path, VoidShaderStage stage) override;

    // view creation
    void register_render_target_view(VoidImage* target) override;
    void register_depth_stencil_view(VoidImage* target) override;
    void register_image_view(VoidImage* image) override;
    void register_resource_view(VoidStructuredBuffer* buffer) override;
    void register_resource_view(VoidStructuredBuffer* buffer, size_t numElements, size_t elementSize) override;
    void register_constant_view(VoidStructuredBuffer* buffer) override;

    // buffer copy
    void copy_image(void* data, VoidImage* image) override;
    void copy_buffer(VoidStructuredBuffer* src, VoidStructuredBuffer* dst) override;

    // resource release
    //void release_image(std::shared_ptr<VoidImage> image) override;
    //void release_index_buffer(std::shared_ptr<VoidIndexBuffer> buffer) override;
    void release_sampler(VoidSampler* sampler) override;
    void release_shader(VoidShader* shader) override;
    void release_resource(VoidResource* resource) override;
    //void release_structured(std::shared_ptr<VoidStructuredBuffer> buffer) override;

    // draw
    void draw_indexed(uint32_t indexCount, uint32_t firstIndex) override;
    void draw_instanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) override;

    // command lists
    std::shared_ptr<IVoidGraphicsCommandList> allocate_graphics_list() override;
    void submit_graphics_list(std::shared_ptr<IVoidGraphicsCommandList> list) override;
    void submit_graphics_list(ID3D12GraphicsCommandList* list);

    // operations
    void blit_to_render_target(std::shared_ptr<VoidImage> src, std::shared_ptr<VoidImage> dest) override;
    void clear_depth(float value, VoidImage* image) override;
    void clear_image(float rgba[4], VoidImage* image) override;

    // immediate
    void immediate_submit(std::function<void(ID3D12GraphicsCommandList*)> func);

    // render state
    void set_cull_mode(VoidCullMode mode) override;
    void set_depth_test_enabled(bool enabled) override;
    void set_num_render_targets(uint32_t count) override;
    void set_primitive(VoidTopologyType type) override;
    void set_input_primitive(VoidInputTopologyType type) override;
    void set_stencil_test_enabled(bool enabled) override;
    void set_draw_extent(VoidExtent2D extent) override;
    void set_fill_mode(VoidFillMode mode) override;
    void set_depth_clip(bool enabled) override;

    // binding
    void bind_index_buffer(std::shared_ptr<VoidIndexBuffer> buffer) override;
    void bind_render_target(uint16_t slot, std::shared_ptr<VoidImage> target) override;
    void bind_depth_target(std::shared_ptr<VoidImage> target) override;
    void bind_resource(uint16_t slot, std::shared_ptr<VoidImage> image) override;
    void bind_resource(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer) override;
    void bind_sampler(uint16_t slot, std::shared_ptr<VoidSampler> sampler) override;
    //void bind_depth_target(uint16_t slot, bool array, uint32_t layer) override;
    void bind_constant(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer) override;
    std::shared_ptr<VoidImage> get_back_buffer() override;

    // util
    uint32_t get_current_frame_index();
    VoidExtent2D get_current_draw_extent() override;
    /*std::shared_ptr<VoidSampler> get_default_linear_sampler() override;
    std::shared_ptr<VoidSampler> get_default_nearest_sampler() override;*/
    uint8_t get_frame_index() override;
    ID3D12GraphicsCommandList* get_command_list_raw();
    Void3DMemoryStats get_memory_usage_stats() override;

private:
    static bool check_result(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::exception();
        }

        return true;
    }

    ComPtr<IDXGIAdapter4> get_adapter(bool useWarp);
    bool init_device_and_resources();
    void transition_resource(ID3D12GraphicsCommandList* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
    uint64_t signal_fence(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, uint64_t& fenceValue);
    void wait_for_fence_value(ID3D12Fence* fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
    void begin_new_frame();
    ID3D12GraphicsCommandList6* get_command_list();
    ID3D12GraphicsCommandList6* get_prologue_list();
    void free_descriptors(D3D12_VoidResource* resource);
    void pre_draw();
    void init_blit_pso();

    static constexpr uint32_t _maxCpuBufferDesc = 100000;
    static constexpr uint32_t _maxCpuSamplerDesc = 1000;
    static constexpr uint32_t _maxCpuRtvDesc = 1000;
    static constexpr uint32_t _maxCpuDsvDesc = 1000;
    static constexpr D3D_FEATURE_LEVEL _desiredFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    SDL_Window* _window = nullptr;
    bool _needResize = false;
    VoidExtent2D _needResizeTo;
    VoidExtent2D _windowExtent;
    ComPtr<IDXGIFactory6> _dxgiFactory;
    ComPtr<ID3D12Device10> _device;
    ComPtr<ID3D12CommandQueue> _commandQueue;
    ComPtr<IDXGISwapChain4> _swapChain;
    std::shared_ptr<D3D12_VoidImage> _backBuffers[_numFrames];
    ComPtr<ID3D12GraphicsCommandList6> _graphicsCommandList;
    ComPtr<ID3D12GraphicsCommandList6> _prologueCommandList;
    ComPtr<ID3D12CommandAllocator> _commandAllocator[_numFrames];
    ComPtr<ID3D12CommandAllocator> _prologueAllocator[_numFrames];
    ComPtr<ID3D12CommandAllocator> _immediateCmdAllocator;
    ComPtr<IDXGIAdapter4> _adapter;
    D3D12MA::Allocator* _allocator = nullptr;
    ComPtr<ID3D12Fence> _fenceDirect;
    ComPtr<ID3D12Fence> _fenceCopy;
    ComPtr<ID3D12Fence> _immediateFence;
    ComPtr<ID3D12CommandQueue> _directCommandQueue;
    ComPtr<ID3D12CommandQueue> _directUploadQueue;
    ComPtr<ID3D12CommandQueue> _copyCommandQueue;
    std::unique_ptr<D3D12_PipelineStateObjectCache> _blitPsoCache;
    ComPtr<ID3D12RootSignature> _blitRootSignature;
    std::shared_ptr<D3D12_VoidShader> _blitVertShader;
    std::shared_ptr<D3D12_VoidShader> _blitPixelShader;
    D3D12_PipelineCacheDesc _blitStateDesc;
    uint64_t _currentFrame = 0;
    uint64_t _fenceValue = 0;
    uint64_t _immediateFenceValue = 0;
    uint64_t _frameFenceValues[_numFrames] = {};
    HANDLE _fenceEvent = nullptr;
    HANDLE _immediateFenceEvent = nullptr;
    UINT _currentBackBufferIndex = 0;
    std::string _shaderRootPath = "";
    std::queue<std::shared_ptr<void>> _resourceLocks[_numFrames];
    std::queue<std::shared_ptr<IVoidGraphicsCommandList>> _listLocks[_numFrames];
    std::mutex _listQueueLock;
    std::mutex _commandAllocatorLock;
    moodycamel::ConcurrentQueue<VoidGraphicsCommandList_D3D12*> _freeRenderLists;
    moodycamel::ConcurrentQueue<VoidGraphicsCommandList_D3D12*> _freeImmediateLists;

    CpuDescriptorAllocator<_maxCpuBufferDesc>* _cpuBufferDescAllocator = nullptr;
    CpuDescriptorAllocator<_maxCpuSamplerDesc>* _cpuSamplerDescAllocator = nullptr;
    CpuDescriptorAllocator<_maxCpuRtvDesc>* _cpuRtvDescAllocator = nullptr;
    CpuDescriptorAllocator<_maxCpuDsvDesc>* _cpuDsvDescAllocator = nullptr;

    D3D12_PipelineState* _pipelineState = nullptr;

    /*std::shared_ptr<D3D12_VoidSampler> _defaultNearestSampler = nullptr;
    std::shared_ptr<D3D12_VoidSampler> _defaultLinearSampler = nullptr;*/

#ifdef VOID_USE_IMGUI
    ComPtr<ID3D12DescriptorHeap> _imguiDescHeap;
#endif
};

#endif