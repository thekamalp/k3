// k3 graphics library
// internal header file for dx12 api
#pragma once

#include "k3win32.h"

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

enum class k3DxgiSurfaceType {
    TYPELESS,
    COLOR,
    DEPTH
};

class k3win32Dx12WinImpl : public k3win32WinImpl
{
public:
    k3win32Dx12WinImpl();
    virtual ~k3win32Dx12WinImpl();
    virtual void ResizeBackBuffer();
    static DXGI_FORMAT ConvertToDXGIFormat(k3fmt fmt, k3DxgiSurfaceType surf_type);
    static k3fmt ConertFromDXGIFormat(DXGI_FORMAT fmt);

    IDXGISwapChain3* _swap_chain;
    ID3D12DescriptorHeap* _rtv_heap;
    k3surf _surf[BACK_BUFFERS];
};

class k3fenceImpl
{
public:
    k3fenceImpl();
    virtual ~k3fenceImpl();
    uint64_t _fence_value;
    ID3D12CommandQueue* _cmd_q;
    ID3D12Fence* _fence;
};

class k3resourceImpl
{
public:
    k3resourceImpl();
    virtual ~k3resourceImpl();
    k3resourceState _resource_state;
    ID3D12Resource* _dx12_resource;
    k3memPool _pool;
    uint64_t _width;
    uint32_t _height;
    uint32_t _depth;
    uint32_t _max_mip;
    uint32_t _samples;
    k3fmt _format;
};

class k3surfImpl
{
public:
    k3surfImpl();
    virtual ~k3surfImpl();
    k3resource _resource;
    uint32_t _srv_view_index;
    D3D12_CPU_DESCRIPTOR_HANDLE _srv_cpu_view;
    D3D12_GPU_DESCRIPTOR_HANDLE _srv_gpu_view;
    uint32_t _uav_view_index;
    D3D12_CPU_DESCRIPTOR_HANDLE _uav_cpu_view;
    D3D12_GPU_DESCRIPTOR_HANDLE _uav_gpu_view;
    // can be used for rtv or dsv, depending on format
    D3D12_CPU_DESCRIPTOR_HANDLE _rtv_cpu_view;
    ID3D12DescriptorHeap* _rtv_heap;
};

class k3samplerImpl
{
public:
    k3samplerImpl();
    virtual ~k3samplerImpl();
    uint32_t _view_index;
    D3D12_CPU_DESCRIPTOR_HANDLE _cpu_view;
    D3D12_GPU_DESCRIPTOR_HANDLE _gpu_view;
};

class k3bufferImpl
{
public:
    k3bufferImpl();
    virtual ~k3bufferImpl();
    k3resource _resource;
    uint32_t _stride;
    uint32_t _view_index;
    D3D12_CPU_DESCRIPTOR_HANDLE _cpu_view;
    D3D12_GPU_DESCRIPTOR_HANDLE _gpu_view;
};

class k3blasImpl
{
public:
    k3blasImpl();
    virtual ~k3blasImpl();
    D3D12_RAYTRACING_GEOMETRY_DESC _geom;
    k3buffer _ib;
    k3buffer _vb;
    k3rtasSize _size;
    k3resource _blas;
    k3resource _create_scratch;
    k3resource _update_scratch;
};

class k3tlasImpl
{
public:
    k3tlasImpl();
    virtual ~k3tlasImpl();
    uint32_t _num_instances;
    k3tlasInstance* _instances;
    k3uploadBuffer _instance_upbuf;
    k3rtasSize _size;
    k3resource _tlas;
    k3resource _create_scratch;
    k3resource _update_scratch;
    uint32_t _view_index;
    D3D12_CPU_DESCRIPTOR_HANDLE _cpu_view;
    D3D12_GPU_DESCRIPTOR_HANDLE _gpu_view;
};

class k3cmdBufImpl
{
public:
    static const uint32_t MAX_ALLOC = 4;
    k3cmdBufImpl();
    virtual ~k3cmdBufImpl();
    ID3D12GraphicsCommandList* _cmd_list;
    ID3D12GraphicsCommandList4* _cmd_list4;
    k3fence _fence;
    ID3D12CommandAllocator* _cmd_alloc[MAX_ALLOC];
    uint64_t _cmd_alloc_fence[MAX_ALLOC];
    uint32_t _cur_alloc;
    k3shaderBinding _cur_binding;
    k3rect _cur_viewport;
    k3gfx _gfx;
    bool _index_draw;
};

class k3shaderBindingImpl
{
public:
    k3shaderBindingImpl();
    virtual ~k3shaderBindingImpl();
    ID3D12RootSignature* _binding;
    k3shaderBindingType _type;
};

class k3shaderImpl
{
public:
    k3shaderImpl();
    virtual ~k3shaderImpl();
    D3D12_SHADER_BYTECODE _byte_code;
};

class k3rtStateImpl
{
public:
    k3rtStateImpl();
    virtual ~k3rtStateImpl();
    ID3D12StateObject* _state;
    k3shader* _shaders;
    k3shaderBinding* _shader_bindings;
};

class k3rtStateTableImpl
{
public:
    k3rtStateTableImpl();
    virtual ~k3rtStateTableImpl();
    uint32_t getEntrySize() const;
    k3resource _resource;
    uint32_t _num_entries;
    uint32_t _num_args;   // number of 8 byte arguments per entry
};

class k3gfxStateImpl
{
public:
    k3gfxStateImpl();
    virtual ~k3gfxStateImpl();
    ID3D12PipelineState* _state;
    k3shaderBinding _binding;
};

class k3memPoolImpl
{
public:
    k3memPoolImpl();
    virtual ~k3memPoolImpl();
    ID3D12Heap* _heap;
    k3memType _type;
    uint64_t _size;
};

class k3uploadImageImpl
{
public:
    k3uploadImageImpl();
    virtual ~k3uploadImageImpl();
    ID3D12Device* _dev;
    ID3D12Resource* _resource;
};

class k3uploadBufferImpl
{
public:
    k3uploadBufferImpl();
    virtual ~k3uploadBufferImpl();
    uint64_t _logical_size;
    uint64_t _size;
    ID3D12Device* _dev;
    ID3D12Resource* _resource;
};

class k3gfxImpl
{
public:
    k3gfxImpl();
    static void ConvertToDx12Rect(D3D12_RECT* dst, const k3rect* src);
    static D3D12_CLEAR_FLAGS ConvertToDx12ClearFlags(k3depthSelect clear);
    static D3D12_RESOURCE_STATES ConvertToDx12ResourceState(k3resourceState state, bool is_depth);
    static D3D12_ROOT_PARAMETER_TYPE ConvertToDx12RootParameterType(k3bindingType bind_type);
    static D3D12_BLEND ConvertToDx12Blend(k3blend blend);
    static D3D12_BLEND_OP ConvertToDx12BlendOp(k3blendOp blend_op);
    static D3D12_LOGIC_OP ConvertToDx12Rop(k3rop rop);
    static D3D12_FILL_MODE ConvertToDx12FillMode(k3fill fill_mode);
    static D3D12_CULL_MODE ConvertToDx12CullMode(k3cull cull_mode);
    static D3D12_CONSERVATIVE_RASTERIZATION_MODE ConvertToDx12ConservativeRast(bool enable);
    static D3D12_COMPARISON_FUNC ConvertToDx12TestFunc(k3testFunc test);
    static D3D12_DEPTH_WRITE_MASK ConvertToDx12DepthWrite(bool write_enable);
    static D3D12_STENCIL_OP ConvertToDx12StencilOp(k3stencilOp op);
    static D3D12_INPUT_CLASSIFICATION ConvertToDx12InputType(k3inputType input_type);
    static D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ConvertToDx12StripCut(k3stripCut cut);
    static D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertToDx12PrimType(k3primType prim);
    static D3D12_PRIMITIVE_TOPOLOGY ConvertToDx12DrawPrimType(k3drawPrimType draw_prim);
    static D3D12_HEAP_TYPE ConvertToDx12MemType(k3memType mem_type);
    static D3D12_DESCRIPTOR_RANGE_TYPE ConvertToDx12ShaderBindType(k3shaderBindType type);
    static D3D12_FILTER ConvertToDx12Fitler(k3texFilter filter);
    static D3D12_TEXTURE_ADDRESS_MODE ConvertToDx12AddrMode(k3texAddr addr_mode);
    static D3D12_HIT_GROUP_TYPE ConvertToDx12HitGroupType(k3rtHitGroupType hit_group_type);
    static uint32_t _num_gfx;
    static IDXGIFactory4* _factory;
#ifdef _DEBUG
    static ID3D12Debug1* _debug_controller;
#endif
    static IDXGIAdapter1* _adapter;
    static char _adapter_name[128];
    ID3D12Device* _dev;
    ID3D12Device5* _dev5;
    uint32_t _dxr_tier;
#ifdef _DEBUG
    ID3D12DebugDevice* _dbg_dev;
#endif
    ID3D12CommandQueue* _cmd_q;
    k3fence _cpu_fence;
    ID3D12DescriptorHeap* _shader_heap[2];  // first for CBV/SRV/UAV, second for sampers
    k3shader _font_vs;
    k3shader _font_ps;
};