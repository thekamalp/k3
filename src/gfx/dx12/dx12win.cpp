// k3 graphics library
// windows win32 class
// Date: 10/10/2021

#include "k3dx12.h"

// ------------------------------------------------------------
// fence
k3fenceImpl::k3fenceImpl()
{
    _fence_value = 0;
    _cmd_q = NULL;
    _fence = NULL;
}

k3fenceImpl::~k3fenceImpl()
{
    if (_fence) {
        _fence->Release();
        _fence = NULL;
    }
}

k3fenceObj::k3fenceObj()
{
    _data = new k3fenceImpl;
}

k3fenceObj::~k3fenceObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3fenceImpl* k3fenceObj::getImpl()
{
    return _data;
}

const k3fenceImpl* k3fenceObj::getImpl() const
{
    return _data;
}

K3API uint64_t k3fenceObj::SetCpuFence()
{
    _data->_fence_value++;
    _data->_fence->Signal(_data->_fence_value);
    return _data->_fence_value;
}

K3API uint64_t k3fenceObj::SetGpuFence(k3gpuQueue queue)
{
    // TODO: implement other queues
    _data->_fence_value++;
    _data->_cmd_q->Signal(_data->_fence, _data->_fence_value);
    return _data->_fence_value;
}

K3API bool k3fenceObj::CheckFence(uint64_t value)
{
    uint64_t completed_value = _data->_fence->GetCompletedValue();
    return (completed_value >= value);
}


K3API void k3fenceObj::WaitCpuFence(uint64_t value)
{
    if (CheckFence(value)) return;

    HANDLE event = CreateEvent(NULL, false, false, NULL);
    if (event == NULL) {
        k3error::Handler("Could not create event", "k3fenceObj::WaitCpuFence");
        return;
    }
    _data->_fence->SetEventOnCompletion(value, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
}

K3API void k3fenceObj::WaitGpuFence(uint64_t value)
{
    _data->_cmd_q->Wait(_data->_fence, value);
}

// ------------------------------------------------------------
// resource
k3resourceImpl::k3resourceImpl()
{
    _resource_state = k3resourceState::COMMON;
    _dx12_resource = NULL;
    _pool = NULL;
    _width = 0;
    _height = 0;
    _depth = 0;
    _max_mip = 1;
    _samples = 1;
    _format = k3fmt::UNKNOWN;
}

k3resourceImpl::~k3resourceImpl()
{
    if (_dx12_resource) {
        _dx12_resource->Release();
        _dx12_resource = NULL;
    }
}

k3resourceObj::k3resourceObj()
{
    _data = new k3resourceImpl;
}

k3resourceObj::~k3resourceObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3resourceImpl* k3resourceObj::getImpl()
{
    return _data;
}

const k3resourceImpl* k3resourceObj::getImpl() const
{
    return _data;
}

K3API void k3resourceObj::getDesc(k3resourceDesc* desc)
{
    if (desc) {
        D3D12_RESOURCE_DESC dx12_desc = _data->_dx12_resource->GetDesc();
        desc->width = dx12_desc.Width;
        desc->height = dx12_desc.Height;
        desc->depth = dx12_desc.DepthOrArraySize;
        desc->mip_levels = dx12_desc.MipLevels;
        desc->format = k3win32Dx12WinImpl::ConertFromDXGIFormat(dx12_desc.Format);
        desc->num_samples = dx12_desc.SampleDesc.Count;
    }
}


// ------------------------------------------------------------
// surf
k3surfImpl::k3surfImpl()
{
    _resource = NULL;
    _srv_view_index = ~0;
    _srv_cpu_view.ptr = NULL;
    _srv_gpu_view.ptr = NULL;
    _uav_view_index = ~0;
    _uav_cpu_view.ptr = NULL;
    _uav_gpu_view.ptr = NULL;
    _rtv_cpu_view.ptr = NULL;
    _rtv_heap = NULL;
}

k3surfImpl::~k3surfImpl()
{
    if (_rtv_heap) {
        _rtv_heap->Release();
        _rtv_heap = NULL;
    }
}

k3surfObj::k3surfObj()
{
    _data = new k3surfImpl;
}

k3surfObj::~k3surfObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3surfImpl* k3surfObj::getImpl()
{
    return _data;
}

const k3surfImpl* k3surfObj::getImpl() const
{
    return _data;
}

K3API k3resource k3surfObj::GetResource()
{
    return _data->_resource;
}

K3API uint32_t k3surfObj::GetSRVViewIndex() const
{
    return _data->_srv_view_index;
}

K3API uint32_t k3surfObj::GetUAVViewIndex() const
{
    return _data->_uav_view_index;
}


// ------------------------------------------------------------
// sampler

k3samplerImpl::k3samplerImpl()
{
    _view_index = ~0;
    _cpu_view.ptr = NULL;
    _gpu_view.ptr = NULL;
}

k3samplerImpl::~k3samplerImpl()
{ }

k3samplerObj::k3samplerObj()
{
    _data = new k3samplerImpl;
}

k3samplerObj::~k3samplerObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3samplerImpl* k3samplerObj::getImpl()
{
    return _data;
}

const k3samplerImpl* k3samplerObj::getImpl() const
{
    return _data;
}

K3API uint32_t k3samplerObj::GetViewIndex() const
{
    return _data->_view_index;
}

// ------------------------------------------------------------
// buffer
k3bufferImpl::k3bufferImpl()
{
    _resource = NULL;
    _stride = 0;
    _view_index = ~0;
    _cpu_view.ptr = NULL;
    _gpu_view.ptr = NULL;
}

k3bufferImpl::~k3bufferImpl()
{ }

k3bufferObj::k3bufferObj()
{
    _data = new k3bufferImpl;
}

k3bufferObj::~k3bufferObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3bufferImpl* k3bufferObj::getImpl()
{
    return _data;
}

const k3bufferImpl* k3bufferObj::getImpl() const
{
    return _data;
}

K3API k3resource k3bufferObj::GetResource()
{
    return _data->_resource;
}

K3API uint32_t k3bufferObj::GetViewIndex() const
{
    return _data->_view_index;
}

// ------------------------------------------------------------
// blas
k3blasImpl::k3blasImpl()
{
    memset(&_geom, 0, sizeof(_geom));
    memset(&_size, 0, sizeof(_size));
    _ib = NULL;
    _vb = NULL;
    _blas = NULL;
    _create_scratch = NULL;
    _update_scratch = NULL;
}

k3blasImpl::~k3blasImpl()
{ }

k3blasObj::k3blasObj()
{
    _data = new k3blasImpl;
}

k3blasObj::~k3blasObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3blasImpl* k3blasObj::getImpl()
{
    return _data;
}

const k3blasImpl* k3blasObj::getImpl() const
{
    return _data;
}

K3API void k3blasObj::getSize(k3rtasSize* size)
{
    if (size) *size = _data->_size;
}

// ------------------------------------------------------------
// tlas
k3tlasImpl::k3tlasImpl()
{
    _num_instances = 0;
    _instances = NULL;
    _instance_upbuf = NULL;
    memset(&_size, 0, sizeof(_size));
    _tlas = NULL;
    _view_index = ~0;
    _create_scratch = NULL;
    _update_scratch = NULL;
}

k3tlasImpl::~k3tlasImpl()
{
    if (_instances) {
        delete[] _instances;
        _instances = NULL;
    }
}

k3tlasObj::k3tlasObj()
{
    _data = new k3tlasImpl;
}

k3tlasObj::~k3tlasObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3tlasImpl* k3tlasObj::getImpl()
{
    return _data;
}

const k3tlasImpl* k3tlasObj::getImpl() const
{
    return _data;
}

K3API uint32_t k3tlasObj::GetViewIndex() const
{
    return _data->_view_index;
}

K3API void k3tlasObj::UpdateInstance(uint64_t inst_id, k3tlasInstance* inst)
{
    D3D12_RAYTRACING_INSTANCE_DESC* dx12_inst = (D3D12_RAYTRACING_INSTANCE_DESC*)_data->_instance_upbuf->MapRange(inst_id * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    if (dx12_inst) {
        memcpy(dx12_inst->Transform, inst->transform, 12 * sizeof(float));
        dx12_inst->InstanceID = inst->id;
        dx12_inst->InstanceContributionToHitGroupIndex = inst->hit_group;
        dx12_inst->InstanceMask = inst->mask;
        dx12_inst->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;  // TODO: implement flags
        dx12_inst->AccelerationStructure = inst->blas->getImpl()->_blas->getImpl()->_dx12_resource->GetGPUVirtualAddress();
        _data->_instance_upbuf->UnmapRange(inst_id * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    }
}

K3API void k3tlasObj::UpdateTransform(uint64_t inst_id, float* xform)
{
    D3D12_RAYTRACING_INSTANCE_DESC* dx12_inst = (D3D12_RAYTRACING_INSTANCE_DESC*)_data->_instance_upbuf->MapRange(inst_id * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), 12 * sizeof(float));
    if (dx12_inst) {
        memcpy(dx12_inst->Transform, xform, 12 * sizeof(float));
        _data->_instance_upbuf->UnmapRange(inst_id * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), 12 * sizeof(float));
    }
}


// ------------------------------------------------------------
// cmdBuf
k3cmdBufImpl::k3cmdBufImpl()
{
    uint32_t i;
    _cmd_list = NULL;
    _cmd_list4 = NULL;
    _fence = NULL;
    for (i = 0; i < MAX_ALLOC; i++) {
        _cmd_alloc[i] = NULL;
        _cmd_alloc_fence[i] = 0;
    }
    _cur_alloc = 0;
    _cur_binding = NULL;
    _cur_viewport.x = 0;
    _cur_viewport.y = 0;
    _cur_viewport.width = 0;
    _cur_viewport.height = 0;
    _index_draw = false;
}

k3cmdBufImpl::~k3cmdBufImpl()
{
    uint32_t i;
    if (_cmd_list) {
        _cmd_list->Release();
        _cmd_list = NULL;
    }
    if (_cmd_list4) _cmd_list4 = NULL; // This is the same as _cmd_list if it exists
    for(i=0; i<MAX_ALLOC; i++) {
        if (_cmd_alloc[i]) {
            _cmd_alloc[i]->Release();
            _cmd_alloc[i] = NULL;
        }
    }
}

k3cmdBufObj::k3cmdBufObj()
{
    _data = new k3cmdBufImpl;
}

k3cmdBufObj::~k3cmdBufObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3cmdBufImpl* k3cmdBufObj::getImpl()
{
    return _data;
}

const k3cmdBufImpl* k3cmdBufObj::getImpl() const
{
    return _data;
}

K3API void k3cmdBufObj::Reset()
{
    // Find a free allocator
    HRESULT hr;
    //uint32_t a;
    //uint64_t smallest_fence = _data->_cmd_alloc_fence[0];
    //uint32_t smallest_alloc = 0;
    //for (a = 0; a < k3cmdBufImpl::MAX_ALLOC; a++) {
    //    if (_data->_fence->CheckFence(_data->_cmd_alloc_fence[a])) {
    //        // allocator found
    //        break;
    //    }
    //    if (_data->_cmd_alloc_fence[a] <= smallest_fence) {
    //        smallest_fence = _data->_cmd_alloc_fence[a];
    //        smallest_alloc = a;
    //    }
    //}
    //if (a == k3cmdBufImpl::MAX_ALLOC) {
    //    // no allocator found, so wait on teh earliest one
    //    _data->_fence->WaitCpuFence(smallest_fence);
    //    _data->_cur_alloc = smallest_alloc;
    //} else {
    //    _data->_cur_alloc = a;
    //}

    _data->_cur_alloc++;
    if (_data->_cur_alloc >= k3cmdBufImpl::MAX_ALLOC) _data->_cur_alloc = 0;
    _data->_fence->WaitCpuFence(_data->_cmd_alloc_fence[_data->_cur_alloc]);

    hr = _data->_cmd_alloc[_data->_cur_alloc]->Reset();
    if (hr != S_OK) {
        k3error::Handler("Could not reset command allocator", "k3cmdBufObj::Reset");
        return;
    }
    hr = _data->_cmd_list->Reset(_data->_cmd_alloc[_data->_cur_alloc], NULL);
    if (hr != S_OK) {
        k3error::Handler("Could not reset command list", "k3cmdBufObj::Reset");
    }
    ID3D12DescriptorHeap** shader_heap = _data->_gfx->getImpl()->_shader_heap;
    uint32_t num_heaps = (shader_heap[1]) ? 2 : 1;
    _data->_cmd_list->SetDescriptorHeaps(num_heaps, shader_heap);
}

K3API void k3cmdBufObj::Close()
{
    HRESULT hr = _data->_cmd_list->Close();
    if (hr != S_OK) {
        k3error::Handler("Could not close command list", "k3cmdBufObj::Close");
    }
    _data->_cur_binding = NULL;
    _data->_cur_viewport.x = 0;
    _data->_cur_viewport.y = 0;
    _data->_cur_viewport.width = 0;
    _data->_cur_viewport.height = 0;
}

K3API void k3cmdBufObj::TransitionResource(k3resource resource, k3resourceState new_state)
{
    k3resourceImpl* resource_impl = resource->getImpl();
    k3resourceState cur_state = resource_impl->_resource_state;
    if (cur_state == new_state) return;
    bool is_depth = (resource_impl->_format == k3fmt::D16_UNORM || resource_impl->_format == k3fmt::D24X8_UNORM || resource_impl->_format == k3fmt::D24_UNORM_S8_UINT ||
        resource_impl->_format == k3fmt::D32_FLOAT || resource_impl->_format == k3fmt::D32_FLOAT_S8X24_UINT);
    D3D12_RESOURCE_STATES dx12_cur_state = k3gfxImpl::ConvertToDx12ResourceState(cur_state, is_depth);
    D3D12_RESOURCE_STATES dx12_new_state = k3gfxImpl::ConvertToDx12ResourceState(new_state, is_depth);
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource_impl->_dx12_resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = dx12_cur_state;
    barrier.Transition.StateAfter = dx12_new_state;
    _data->_cmd_list->ResourceBarrier(1, &barrier);
    resource_impl->_resource_state = new_state;
}

K3API void k3cmdBufObj::ClearRenderTarget(k3surf surf, const float* color, const k3rect* rect)
{
    D3D12_RECT r;
    D3D12_RECT* rptr = NULL;
    UINT num_rect = 0;
    if (rect) {
        k3gfxImpl::ConvertToDx12Rect(&r, rect);
        rptr = &r;
        num_rect = 1;
    }
    k3surfImpl* surf_impl = surf->getImpl();
    if (surf_impl->_rtv_cpu_view.ptr == NULL) {
        k3error::Handler("Surface does not have render target view", "k3cmdBufObj::ClearRenderTarget");
        return;
    }
    _data->_cmd_list->ClearRenderTargetView(surf_impl->_rtv_cpu_view, color, num_rect, rptr);
}

K3API void k3cmdBufObj::ClearDepthTarget(k3surf surf, k3depthSelect clear, const float depth, uint8_t stencil, const k3rect* rect)
{
    D3D12_RECT r;
    D3D12_RECT* rptr = NULL;
    UINT num_rect = 0;
    D3D12_CLEAR_FLAGS dx12_clear = k3gfxImpl::ConvertToDx12ClearFlags(clear);
    if (rect) {
        k3gfxImpl::ConvertToDx12Rect(&r, rect);
        rptr = &r;
        num_rect = 1;
    }

    k3surfImpl* surf_impl = surf->getImpl();
    if (surf_impl->_rtv_cpu_view.ptr == NULL) {
        k3error::Handler("Surface does not have depth target view", "k3cmdBufObj::ClearDepthTarget");
        return;
    }
    _data->_cmd_list->ClearDepthStencilView(surf_impl->_rtv_cpu_view, dx12_clear, depth, stencil, num_rect, rptr);
}

K3API void k3cmdBufObj::UploadImage(k3uploadImage img, k3resource resource)
{
    D3D12_TEXTURE_COPY_LOCATION src, dst;
    src.pResource = img->getUploadImageImpl()->_resource;
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset = 0;
    src.PlacedFootprint.Footprint.Format = k3win32Dx12WinImpl::ConvertToDXGIFormat(img->GetFormat(), k3DxgiSurfaceType::COLOR);
    src.PlacedFootprint.Footprint.Width = img->GetWidth();
    src.PlacedFootprint.Footprint.Height = img->GetHeight();
    src.PlacedFootprint.Footprint.Depth = img->GetDepth();
    src.PlacedFootprint.Footprint.RowPitch = img->GetPitch();
    k3resourceImpl* resource_impl = resource->getImpl();
    dst.pResource = resource_impl->_dx12_resource;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;
    _data->_cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
}

K3API void k3cmdBufObj::DownloadImage(k3downloadImage img, k3resource resource)
{
    D3D12_TEXTURE_COPY_LOCATION src, dst;
    k3resourceImpl* resource_impl = resource->getImpl();
    src.pResource = resource_impl->_dx12_resource;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = 0;
    D3D12_RESOURCE_DESC dx12_desc = src.pResource->GetDesc();

    img->SetDimensions(dx12_desc.Width, dx12_desc.Height, dx12_desc.DepthOrArraySize, k3win32Dx12WinImpl::ConertFromDXGIFormat(dx12_desc.Format));
    img->MapForWrite();
    dst.pResource = img->getDownloadImageImpl()->_resource;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst.PlacedFootprint.Offset = 0;
    dst.PlacedFootprint.Footprint.Format = k3win32Dx12WinImpl::ConvertToDXGIFormat(img->GetFormat(), k3DxgiSurfaceType::COLOR);
    dst.PlacedFootprint.Footprint.Width = img->GetWidth();
    dst.PlacedFootprint.Footprint.Height = img->GetHeight();
    dst.PlacedFootprint.Footprint.Depth = img->GetDepth();
    dst.PlacedFootprint.Footprint.RowPitch = img->GetPitch();
    _data->_cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
    img->Unmap();
}

K3API void k3cmdBufObj::UploadBuffer(k3uploadBuffer buf, k3resource resource, uint64_t start)
{
    k3uploadBufferImpl* src_buffer_impl = buf->getImpl();
    k3resourceImpl* dst_resource_impl = resource->getImpl();
    uint64_t size = src_buffer_impl->_logical_size;
    if (dst_resource_impl->_width - start < size) size = dst_resource_impl->_width - start;
    _data->_cmd_list->CopyBufferRegion(dst_resource_impl->_dx12_resource, start, src_buffer_impl->_resource, 0, size);
}

K3API void k3cmdBufObj::UploadBufferSrcRange(k3uploadBuffer buf, k3resource resource, uint64_t src_start, uint64_t size, uint64_t dst_start)
{
    k3uploadBufferImpl* src_buffer_impl = buf->getImpl();
    k3resourceImpl* dst_resource_impl = resource->getImpl();
    uint64_t src_size = src_buffer_impl->_logical_size - src_start;
    if (size < src_size) src_size = size;
    if (dst_resource_impl->_width - dst_start < src_size) src_size = dst_resource_impl->_width - dst_start;
    _data->_cmd_list->CopyBufferRegion(dst_resource_impl->_dx12_resource, dst_start, src_buffer_impl->_resource, src_start, src_size);
}

K3API void k3cmdBufObj::GetCurrentViewport(k3rect* viewport)
{
    *viewport = _data->_cur_viewport;
}

K3API void k3cmdBufObj::SetGfxState(k3gfxState state)
{
    k3gfxStateImpl* state_impl = state->getImpl();
    _data->_cmd_list->SetPipelineState(state_impl->_state);
    if (_data->_cur_binding != state_impl->_binding) {
        _data->_cur_binding = state_impl->_binding;
        k3shaderBindingImpl* bind_impl = _data->_cur_binding->getImpl();
        _data->_cmd_list->SetGraphicsRootSignature(bind_impl->_binding);
    }
}

K3API void k3cmdBufObj::SetDrawPrim(k3drawPrimType draw_prim)
{
    D3D_PRIMITIVE_TOPOLOGY dx12_draw_prim = k3gfxImpl::ConvertToDx12DrawPrimType(draw_prim);
    _data->_cmd_list->IASetPrimitiveTopology(dx12_draw_prim);
}

K3API void k3cmdBufObj::SetViewToSurface(k3resource surface)
{
    k3resourceImpl* resource_impl = surface->getImpl();
    D3D12_VIEWPORT view_port;
    D3D12_RECT surface_size;
    
    surface_size.left = 0;
    surface_size.top = 0;
    surface_size.right = static_cast<LONG>(resource_impl->_width);
    surface_size.bottom = static_cast<LONG>(resource_impl->_height);
    
    view_port.TopLeftX = 0.0f;
    view_port.TopLeftY = 0.0f;
    view_port.Width = static_cast<float>(resource_impl->_width);
    view_port.Height = static_cast<float>(resource_impl->_height);
    view_port.MinDepth = 0.0f;
    view_port.MaxDepth = 1.0f;
    
    _data->_cmd_list->RSSetViewports(1, &view_port);
    _data->_cmd_list->RSSetScissorRects(1, &surface_size);
    _data->_cur_viewport.x = static_cast<uint32_t>(view_port.TopLeftX);
    _data->_cur_viewport.y = static_cast<uint32_t>(view_port.TopLeftY);
    _data->_cur_viewport.width = static_cast<uint32_t>(view_port.Width);
    _data->_cur_viewport.height = static_cast<uint32_t>(view_port.Height);
}

K3API void k3cmdBufObj::SetViewport(k3rect* rect)
{
    D3D12_VIEWPORT view_port;

    view_port.TopLeftX = static_cast<float>(rect->x);
    view_port.TopLeftY = static_cast<float>(rect->y);
    view_port.Width = static_cast<float>(rect->width);
    view_port.Height = static_cast<float>(rect->height);
    view_port.MinDepth = 0.0f;
    view_port.MaxDepth = 1.0f;

    _data->_cmd_list->RSSetViewports(1, &view_port);
    _data->_cur_viewport.x = static_cast<uint32_t>(view_port.TopLeftX);
    _data->_cur_viewport.y = static_cast<uint32_t>(view_port.TopLeftY);
    _data->_cur_viewport.width = static_cast<uint32_t>(view_port.Width);
    _data->_cur_viewport.height = static_cast<uint32_t>(view_port.Height);
}

K3API void k3cmdBufObj::SetScissor(k3rect* rect)
{
    D3D12_RECT scissor_size;

    scissor_size.left = rect->x;
    scissor_size.top = rect->y;
    scissor_size.right = static_cast<LONG>(rect->x + rect->width);
    scissor_size.bottom = static_cast<LONG>(rect->y + rect->height);

    _data->_cmd_list->RSSetScissorRects(1, &scissor_size);
}


K3API void k3cmdBufObj::SetRenderTargets(k3renderTargets* rt)
{
    if (rt) {
        uint32_t i;
        bool contiguous = true;
        D3D12_CPU_DESCRIPTOR_HANDLE dx12_rtv[K3_MAX_RENDER_TARGETS] = { NULL };
        D3D12_CPU_DESCRIPTOR_HANDLE* dx12_dsv = NULL;
        k3gfxImpl* gfxImpl = _data->_gfx->getImpl();
        UINT rtv_desc_size = gfxImpl->_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        k3surfImpl* surf_impl = NULL;
        for (i = 0; i < K3_MAX_RENDER_TARGETS; i++) {
            if (rt->render_targets[i] == NULL) break;
            surf_impl = rt->render_targets[i]->getImpl();
            dx12_rtv[i] = surf_impl->_rtv_cpu_view;
            if (dx12_rtv[i].ptr == NULL) {
                k3error::Handler("Surface is not a render target", "k3cmdBufObj::SetRenderTargets");
                break;
            } else {
                if (i > 0) {
                    if (dx12_rtv[i].ptr != dx12_rtv[i - 1].ptr + rtv_desc_size) contiguous = false;
                }
            }
        }
        if (rt->depth_target != NULL) {
            surf_impl = rt->depth_target->getImpl();
            dx12_dsv = &(surf_impl->_rtv_cpu_view);
            if (dx12_dsv->ptr == NULL) {
                k3error::Handler("Surface is not a render target", "k3cmdBufObj::SetRenderTargets");
                dx12_dsv = NULL;
            }
        }
        _data->_cmd_list->OMSetRenderTargets(i, dx12_rtv, contiguous, dx12_dsv);
    } else {
        _data->_cmd_list->OMSetRenderTargets(0, NULL, false, NULL);
    }
}

K3API void k3cmdBufObj::SetIndexBuffer(k3buffer index_buffer)
{
    if (index_buffer == NULL) {
        _data->_cmd_list->IASetIndexBuffer(NULL);
        _data->_index_draw = false;
        return;
    }
    k3bufferImpl* buffer_impl = index_buffer->getImpl();
    k3resourceImpl* resource_impl = buffer_impl->_resource->getImpl();
    if (resource_impl->_format != k3fmt::R16_UINT && resource_impl->_format != k3fmt::R32_UINT) {
        k3error::Handler("Illegal format for index buffer", "k3cmdBufObj::SetIndexBuffer");
        return;
    }
    D3D12_INDEX_BUFFER_VIEW dx12_ibv = { 0 };
    dx12_ibv.BufferLocation = resource_impl->_dx12_resource->GetGPUVirtualAddress();
    dx12_ibv.Format = k3win32Dx12WinImpl::ConvertToDXGIFormat(resource_impl->_format, k3DxgiSurfaceType::COLOR);
    dx12_ibv.SizeInBytes = (uint32_t)resource_impl->_width;
    _data->_cmd_list->IASetIndexBuffer(&dx12_ibv);
    _data->_index_draw = true;
}

K3API void k3cmdBufObj::SetVertexBuffer(uint32_t slot, k3buffer vertex_buffer)
{
    if (vertex_buffer == NULL) {
        _data->_cmd_list->IASetVertexBuffers(slot, 1, NULL);
        return;
    }
    k3bufferImpl* buffer_impl = vertex_buffer->getImpl();
    k3resourceImpl* resource_impl = buffer_impl->_resource->getImpl();
    if (buffer_impl->_stride == 0) {
        k3error::Handler("Illegal stride for vertex buffer", "k3cmdBufObj::SetVertexBuffer");
        return;
    }
    //if (resource_impl->_format != k3fmt::UNKNOWN) {
    //    k3error::Handler("Vertex buffer must have unknown format", "k3cmdBufObj::SetVertexBuffer");
    //    return;
    //}
    D3D12_VERTEX_BUFFER_VIEW dx12_vbv = { 0 };
    dx12_vbv.BufferLocation = resource_impl->_dx12_resource->GetGPUVirtualAddress();
    dx12_vbv.StrideInBytes = buffer_impl->_stride;
    dx12_vbv.SizeInBytes = (uint32_t)resource_impl->_width;
    _data->_cmd_list->IASetVertexBuffers(slot, 1, &dx12_vbv);
}

K3API void k3cmdBufObj::SetConstant(uint32_t index, uint32_t value, uint32_t offset)
{
    _data->_cmd_list->SetGraphicsRoot32BitConstant(index, value, offset);
}

K3API void k3cmdBufObj::SetConstants(uint32_t index, uint32_t num_constants, const void* values, uint32_t offset)
{
    _data->_cmd_list->SetGraphicsRoot32BitConstants(index, num_constants, values, offset);
}

K3API void k3cmdBufObj::SetConstantBuffer(uint32_t index, k3buffer constant_buffer)
{
    k3bufferImpl* buffer_impl = constant_buffer->getImpl();
    if (buffer_impl->_gpu_view.ptr == NULL) {
        k3error::Handler("Not view for constant buffer", "k3cmdBufObj::SetConstantBuffer");
        return;
    }
    _data->_cmd_list->SetGraphicsRootDescriptorTable(index, buffer_impl->_gpu_view);
}

K3API void k3cmdBufObj::SetShaderView(uint32_t index, k3surf surf)
{
    if (surf == NULL)  return;
    k3surfImpl* surf_impl = surf->getImpl();
    if (surf_impl->_srv_gpu_view.ptr == NULL) {
        k3error::Handler("Surface does not support shader resource view", "k3cmdBufObj::SetShaderView");
        return;
    }
    _data->_cmd_list->SetGraphicsRootDescriptorTable(index, surf_impl->_srv_gpu_view);
}

K3API void k3cmdBufObj::SetSampler(uint32_t index, k3sampler sampler)
{
    if (sampler == NULL) return;
    k3samplerImpl* sampler_impl = sampler->getImpl();
    _data->_cmd_list->SetGraphicsRootDescriptorTable(index, sampler_impl->_gpu_view);
}

K3API void k3cmdBufObj::SetBlendFactor(const float* blend_factor)
{
    if (blend_factor) {
        _data->_cmd_list->OMSetBlendFactor(blend_factor);
    }
}

K3API void k3cmdBufObj::SetStencilRef(uint8_t stencil_ref)
{
    _data->_cmd_list->OMSetStencilRef(stencil_ref);
}

K3API void k3cmdBufObj::BuildBlas(k3blas blas)
{
    k3blasImpl* blasImpl = blas->getImpl();
    if (blasImpl->_blas == NULL) {
        k3error::Handler("BLAS not allocated", "k3cmdBufObj::BuildBlas");
        return;
    }
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blas_desc = {};
    blas_desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    blas_desc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    blas_desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    blas_desc.Inputs.NumDescs = 1;
    blas_desc.Inputs.pGeometryDescs = &blasImpl->_geom;
    blas_desc.ScratchAccelerationStructureData = blasImpl->_create_scratch->getImpl()->_dx12_resource->GetGPUVirtualAddress();
    blas_desc.DestAccelerationStructureData = blasImpl->_blas->getImpl()->_dx12_resource->GetGPUVirtualAddress();
    _data->_cmd_list4->BuildRaytracingAccelerationStructure(&blas_desc, 0, NULL);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = blasImpl->_blas->getImpl()->_dx12_resource;
    _data->_cmd_list->ResourceBarrier(1, &barrier);
}

K3API void k3cmdBufObj::BuildTlas(k3tlas tlas)
{
    k3tlasImpl* tlasImpl = tlas->getImpl();
    if (tlasImpl->_tlas == NULL) {
        k3error::Handler("TLAS not allocated", "k3cmdBufObj::BuildTlas");
        return;
    }
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlas_desc = {};
    tlas_desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tlas_desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    tlas_desc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    tlas_desc.Inputs.NumDescs = tlasImpl->_num_instances;
    tlas_desc.Inputs.InstanceDescs = tlasImpl->_instance_upbuf->getImpl()->_resource->GetGPUVirtualAddress();
    tlas_desc.DestAccelerationStructureData = tlasImpl->_tlas->getImpl()->_dx12_resource->GetGPUVirtualAddress();
    tlas_desc.ScratchAccelerationStructureData = tlasImpl->_create_scratch->getImpl()->_dx12_resource->GetGPUVirtualAddress();
    _data->_cmd_list4->BuildRaytracingAccelerationStructure(&tlas_desc, 0, NULL);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = tlasImpl->_tlas->getImpl()->_dx12_resource;
    _data->_cmd_list->ResourceBarrier(1, &barrier);
}

wchar_t* createWideString(const char* str)
{
    size_t str_len = strlen(str) + 1;
    wchar_t* wide_str = new wchar_t[str_len];
    size_t converted_chars;
    mbstowcs_s(&converted_chars, wide_str, str_len, str, str_len);
    return wide_str;
}

K3API void k3cmdBufObj::UpdateRTStateTable(k3rtStateTable table, k3rtStateTableUpdate* desc)
{
    k3rtStateTableImpl* table_impl = table->getImpl();
    if (desc->start >= table_impl->_num_entries) {
        k3error::Handler("Start entry above entries in table", "k3cmdBufObj::UpdateRTStateTable");
        return;
    }
    if (desc->start + desc->num_entries > table_impl->_num_entries) {
        k3error::Handler("Ending entry above entries in table", "k3cmdBufObj::UpdateRTStateTable");
        return;
    }
    uint32_t entry_size = table_impl->getEntrySize();
    uint32_t start_offset = desc->start * entry_size;
    uint32_t copy_size = desc->num_entries * entry_size;
    uint8_t* entry_data = (uint8_t*)desc->copy_buffer->MapForWrite(copy_size);
    uint8_t* data;
    k3rtStateImpl* state_impl = desc->state->getImpl();
    ID3D12StateObjectProperties* rtso_props;
    state_impl->_state->QueryInterface(IID_PPV_ARGS(&rtso_props));
    uint32_t i, j;
    wchar_t* wide_str;
    k3tlasImpl* tlas_impl = NULL;
    k3surfImpl* surf_impl = NULL;
    k3bufferImpl* buffer_impl = NULL;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_view;
    for (i = 0; i < desc->num_entries; i++, entry_data += entry_size) {
        data = entry_data;
        wide_str = createWideString(desc->entries[i].shader);
        memcpy(data, rtso_props->GetShaderIdentifier(wide_str), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        delete[] wide_str;
        data += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        for (j = 0; j < table_impl->_num_args; j++) {
            if (j < desc->entries[i].num_args) {
                if (desc->entries[i].args[j].type == k3rtStateTableArgType::HANDLE) {
                    if (desc->entries[i].args[j].obj == NULL) {
                        gpu_view.ptr = NULL;
                    } else {
                        switch (desc->entries[i].args[j].obj->getObjType()) {
                        case k3objType::SURF:
                            surf_impl = ((k3surf)desc->entries[i].args[j].obj)->getImpl();
                            switch (desc->entries[i].args[j].bind_type) {
                            case k3shaderBindType::SRV:
                                gpu_view = surf_impl->_srv_gpu_view;
                                break;
                            case k3shaderBindType::UAV:
                                gpu_view = surf_impl->_uav_gpu_view;
                                break;
                            default:
                                gpu_view.ptr = NULL;
                                break;
                            }
                            break;
                        case k3objType::TLAS:
                            tlas_impl = ((k3tlas)desc->entries[i].args[j].obj)->getImpl();
                            gpu_view = tlas_impl->_gpu_view;
                            break;
                        case k3objType::BUFFER:
                            buffer_impl = ((k3buffer)desc->entries[i].args[j].obj)->getImpl();
                            gpu_view = buffer_impl->_gpu_view;
                            break;
                        default:
                            gpu_view.ptr = NULL;
                            break;
                        }
                        if (gpu_view.ptr == NULL) {
                            k3error::Handler("Resource does not have appropriate view", "k3cmdBufObj::UpdateRTStateTable");
                            return;
                        }
                    }
                    memcpy(data, &(gpu_view.ptr), 8);
                } else {
                    memcpy(data, desc->entries[i].args[j].c, 8);
                }
            } else {
                memset(data, 0, 8);
            }
            data += 8;
        }
    }
    desc->copy_buffer->Unmap();
    rtso_props->Release();
    
    _data->_cmd_list->CopyBufferRegion(table_impl->_resource->getImpl()->_dx12_resource, start_offset, desc->copy_buffer->getImpl()->_resource, 0, copy_size);
}

K3API void k3cmdBufObj::SetRTState(k3rtState state)
{
    k3rtStateImpl* state_impl = state->getImpl();
    k3shaderBinding binding = state_impl->_shader_bindings[0];
    if (binding->getType() == k3shaderBindingType::GLOBAL) {
        // if the first binding is a global binding (should be), then set it as the global root signature
        k3shaderBindingImpl* binding_impl = binding->getImpl();
        _data->_cmd_list->SetComputeRootSignature(binding_impl->_binding);
    }
    _data->_cmd_list4->SetPipelineState1(state_impl->_state);
}

K3API void k3cmdBufObj::RTDispatch(const k3rtDispatch* desc)
{
    const k3rtStateTableImpl* table_impl = desc->state_table->getImpl();
    D3D12_DISPATCH_RAYS_DESC dx12_desc = {};
    dx12_desc.Width = (desc->width) ? desc->width : 1;
    dx12_desc.Height = (desc->height) ? desc->height : 1;
    dx12_desc.Depth = (desc->depth) ? desc->depth : 1;
    D3D12_GPU_VIRTUAL_ADDRESS table_start = table_impl->_resource->getImpl()->_dx12_resource->GetGPUVirtualAddress();
    uint32_t entry_size = table_impl->getEntrySize();
    dx12_desc.RayGenerationShaderRecord.StartAddress = table_start + (desc->raygen_index * entry_size);
    dx12_desc.RayGenerationShaderRecord.SizeInBytes = desc->raygen_entries * entry_size;
    dx12_desc.MissShaderTable.StartAddress = table_start + (desc->miss_index * entry_size);
    dx12_desc.MissShaderTable.StrideInBytes = entry_size;
    dx12_desc.MissShaderTable.SizeInBytes = (desc->miss_entries * entry_size);
    dx12_desc.HitGroupTable.StartAddress = table_start + (desc->hit_group_index * entry_size);
    dx12_desc.HitGroupTable.StrideInBytes = entry_size;
    dx12_desc.HitGroupTable.SizeInBytes = (desc->hit_group_entries * entry_size);
    _data->_cmd_list4->DispatchRays(&dx12_desc);
}

K3API void k3cmdBufObj::Copy(k3resource dest, k3resource source)
{
    k3resourceImpl* dest_impl = dest->getImpl();
    k3resourceImpl* source_impl = source->getImpl();
    _data->_cmd_list->CopyResource(dest_impl->_dx12_resource, source_impl->_dx12_resource);
}

K3API void k3cmdBufObj::Draw(uint32_t vertex_count, uint32_t vertex_start, uint32_t instance_count, uint32_t instance_start, uint32_t index_start)
{
    if (_data->_index_draw) {
        _data->_cmd_list->DrawIndexedInstanced(vertex_count, instance_count, index_start, vertex_start, instance_start);
    } else {
        _data->_cmd_list->DrawInstanced(vertex_count, instance_count, vertex_start, instance_start);
    }
}

// ------------------------------------------------------------
// shader bindings
k3shaderBindingImpl::k3shaderBindingImpl()
{
    _binding = NULL;
    _type = k3shaderBindingType::GLOBAL;
}

k3shaderBindingImpl::~k3shaderBindingImpl()
{
    if (_binding) {
        _binding->Release();
        _binding = NULL;
    }
}

k3shaderBindingObj::k3shaderBindingObj()
{
    _data = new k3shaderBindingImpl;
}

k3shaderBindingObj::~k3shaderBindingObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3shaderBindingImpl* k3shaderBindingObj::getImpl()
{
    return _data;
}

const k3shaderBindingImpl* k3shaderBindingObj::getImpl() const
{
    return _data;
}

K3API k3shaderBindingType k3shaderBindingObj::getType() const
{
    return _data->_type;
}


// ------------------------------------------------------------
// shader
k3shaderImpl::k3shaderImpl()
{
    _byte_code.pShaderBytecode = NULL;
    _byte_code.BytecodeLength = 0;
}

k3shaderImpl::~k3shaderImpl()
{
    if (_byte_code.pShaderBytecode) {
        delete[] _byte_code.pShaderBytecode;
        _byte_code.pShaderBytecode = NULL;
    }
}

k3shaderObj::k3shaderObj()
{
    _data = new k3shaderImpl;
}

k3shaderObj::~k3shaderObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3shaderImpl* k3shaderObj::getImpl()
{
    return _data;
}

const k3shaderImpl* k3shaderObj::getImpl() const
{
    return _data;
}

// ------------------------------------------------------------
// rt state
k3rtStateImpl::k3rtStateImpl()
{
    _state = NULL;
    _shaders = NULL;
    _shader_bindings = NULL;
}

k3rtStateImpl::~k3rtStateImpl()
{
    if (_state) {
        _state->Release();
        _state = NULL;
    }
    if (_shaders) {
        delete[] _shaders;
        _shaders = NULL;
    }
    if (_shader_bindings) {
        delete[] _shader_bindings;
        _shader_bindings = NULL;
    }
}

k3rtStateObj::k3rtStateObj()
{
    _data = new k3rtStateImpl;
}

k3rtStateObj::~k3rtStateObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3rtStateImpl* k3rtStateObj::getImpl()
{
    return _data;
}

const k3rtStateImpl* k3rtStateObj::getImpl() const
{
    return _data;
}

// ------------------------------------------------------------
// rt state table
k3rtStateTableImpl::k3rtStateTableImpl()
{
    _resource = NULL;
    _num_entries = 0;
    _num_args = 0;
}

k3rtStateTableImpl::~k3rtStateTableImpl()
{ }

k3rtStateTableObj::k3rtStateTableObj()
{
    _data = new k3rtStateTableImpl;
}

k3rtStateTableObj::~k3rtStateTableObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3rtStateTableImpl* k3rtStateTableObj::getImpl()
{
    return _data;
}

const k3rtStateTableImpl* k3rtStateTableObj::getImpl() const
{
    return _data;
}

uint32_t k3rtStateTableImpl::getEntrySize() const
{
    uint32_t entry_size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + (8 * _num_args);;
    entry_size += D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1;
    entry_size &= ~(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1);
    return entry_size;
}


// ------------------------------------------------------------
// state
k3gfxStateImpl::k3gfxStateImpl()
{
    _state = NULL;
    _binding = NULL;
}

k3gfxStateImpl::~k3gfxStateImpl()
{
    if (_state) {
        _state->Release();
        _state = NULL;
    }
}

k3gfxStateObj::k3gfxStateObj()
{
    _data = new k3gfxStateImpl;
}

k3gfxStateObj::~k3gfxStateObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3gfxStateImpl* k3gfxStateObj::getImpl()
{
    return _data;
}

const k3gfxStateImpl* k3gfxStateObj::getImpl() const
{
    return _data;
}

// ------------------------------------------------------------
// k3memPool
k3memPoolImpl::k3memPoolImpl()
{
    _heap = NULL;
    _type = k3memType::GPU;
    _size = 0;
}

k3memPoolImpl::~k3memPoolImpl()
{
    if (_heap) {
        _heap->Release();
        _heap = NULL;
    }
}

k3memPoolObj::k3memPoolObj()
{
    _data = new k3memPoolImpl;
}

k3memPoolObj::~k3memPoolObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3memPoolImpl* k3memPoolObj::getImpl()
{
    return _data;
}

const k3memPoolImpl* k3memPoolObj::getImpl() const
{
    return _data;
}

K3API k3memType k3memPoolObj::getMemType()
{
    return _data->_type;
}

K3API uint64_t k3memPoolObj::getSize()
{
    return _data->_size;
}

// ------------------------------------------------------------
// upload image
k3uploadImageImpl::k3uploadImageImpl()
{
    _dev = NULL;
    _resource = NULL;
}

k3uploadImageImpl::~k3uploadImageImpl()
{
    if (_resource) {
        _resource->Release();
        _resource = NULL;
    }
    _dev = NULL;
}

k3uploadImageObj::k3uploadImageObj()
{
    _upload_data = new k3uploadImageImpl;
    _data->_pitch_pad = 256;
    _data->_slice_pitch_pad = 256;
}

k3uploadImageObj::~k3uploadImageObj()
{
    if (_upload_data) {
        delete _upload_data;
        _upload_data = NULL;
    }
}

k3uploadImageImpl* k3uploadImageObj::getUploadImageImpl()
{
    return _upload_data;
}

const k3uploadImageImpl* k3uploadImageObj::getUploadImageImpl() const
{
    return _upload_data;
}

K3API const void* k3uploadImageObj::MapForRead()
{
    HRESULT hr;
    void* ptr = NULL;
    if (_upload_data->_resource) {
        hr = _upload_data->_resource->Map(0, NULL, &ptr);
        if (hr != S_OK) {
            k3error::Handler("Could not map image", "k3uploadImageObj::MapForRead");
            return NULL;
        }
    }
    return ptr;
}

K3API void* k3uploadImageObj::MapForWrite()
{
    HRESULT hr;
    void* ptr = NULL;
    uint32_t size = _data->_depth * GetSlicePitch();
    if (_upload_data->_resource == NULL) _data->_size = 0;
    if (_data->_size < size) {
        if (_upload_data->_resource) _upload_data->_resource->Release();
        _data->_size = size;
        D3D12_HEAP_PROPERTIES heap_prop;
        heap_prop.Type = D3D12_HEAP_TYPE_UPLOAD;
        heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_prop.CreationNodeMask = 0;
        heap_prop.VisibleNodeMask = 0;
        D3D12_RESOURCE_DESC desc;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        desc.Width = _data->_size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        hr = _upload_data->_dev->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&_upload_data->_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create commited resource", "k3uploadImageObj::MapForWrite");
            return NULL;
        }
    }
    if (_upload_data->_resource) {
        hr = _upload_data->_resource->Map(0, NULL, &ptr);
        if (hr != S_OK) {
            k3error::Handler("Could not map image", "k3uploadImageObj::MapForWrite");
            return NULL;
        }
    }
    return ptr;
}

K3API void k3uploadImageObj::Unmap()
{
    if (_upload_data->_resource) {
        _upload_data->_resource->Unmap(0, NULL);
    }
}

K3API void k3uploadImageObj::GetDesc(k3resourceDesc* desc)
{
    if (desc) {
        desc->width = _data->_width;
        desc->height = _data->_height;
        desc->depth = _data->_depth;
        desc->format = _data->_fmt;
        desc->mip_levels = 1;
        desc->num_samples = 1;
        desc->mem_pool = NULL;
        desc->mem_offset = 0;
    }
}

// ------------------------------------------------------------
// download image
k3downloadImageImpl::k3downloadImageImpl()
{
    _dev = NULL;
    _resource = NULL;
}

k3downloadImageImpl::~k3downloadImageImpl()
{
    if (_resource) {
        _resource->Release();
        _resource = NULL;
    }
    _dev = NULL;
}

k3downloadImageObj::k3downloadImageObj()
{
    _download_data = new k3downloadImageImpl;
    _data->_pitch_pad = 256;
    _data->_slice_pitch_pad = 256;
}

k3downloadImageObj::~k3downloadImageObj()
{
    if (_download_data) {
        delete _download_data;
        _download_data = NULL;
    }
}

k3downloadImageImpl* k3downloadImageObj::getDownloadImageImpl()
{
    return _download_data;
}

const k3downloadImageImpl* k3downloadImageObj::getDownloadImageImpl() const
{
    return _download_data;
}

K3API const void* k3downloadImageObj::MapForRead()
{
    HRESULT hr;
    void* ptr = NULL;
    if (_download_data->_resource) {
        hr = _download_data->_resource->Map(0, NULL, &ptr);
        if (hr != S_OK) {
            k3error::Handler("Could not map image", "k3downloadImageObj::MapForRead");
            return NULL;
        }
    }
    return ptr;
}

K3API void* k3downloadImageObj::MapForWrite()
{
    HRESULT hr;
    void* ptr = NULL;
    uint32_t size = _data->_depth * GetSlicePitch();
    if (_download_data->_resource == NULL) _data->_size = 0;
    if (_data->_size < size) {
        if (_download_data->_resource) _download_data->_resource->Release();
        _data->_size = size;
        D3D12_HEAP_PROPERTIES heap_prop;
        heap_prop.Type = D3D12_HEAP_TYPE_READBACK;
        heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_prop.CreationNodeMask = 0;
        heap_prop.VisibleNodeMask = 0;
        D3D12_RESOURCE_DESC desc;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        desc.Width = _data->_size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        hr = _download_data->_dev->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&_download_data->_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create commited resource", "k3downloadImageObj::MapForWrite");
            return NULL;
        }
    }
    if (_download_data->_resource) {
        hr = _download_data->_resource->Map(0, NULL, &ptr);
        if (hr != S_OK) {
            k3error::Handler("Could not map image", "k3downloadImageObj::MapForWrite");
            return NULL;
        }
    }
    return ptr;
}

K3API void k3downloadImageObj::Unmap()
{
    if (_download_data->_resource) {
        _download_data->_resource->Unmap(0, NULL);
    }
}

K3API void k3downloadImageObj::GetDesc(k3resourceDesc* desc)
{
    if (desc) {
        desc->width = _data->_width;
        desc->height = _data->_height;
        desc->depth = _data->_depth;
        desc->format = _data->_fmt;
        desc->mip_levels = 1;
        desc->num_samples = 1;
        desc->mem_pool = NULL;
        desc->mem_offset = 0;
    }
}

// ------------------------------------------------------------
// upload buffer
k3uploadBufferImpl::k3uploadBufferImpl()
{
    _logical_size = 0;
    _size = 0;
    _dev = NULL;
    _resource = NULL;
}

k3uploadBufferImpl::~k3uploadBufferImpl()
{
    if (_resource) {
        _resource->Release();
        _resource = NULL;
    }
    _dev = NULL;
}

k3uploadBufferObj::k3uploadBufferObj()
{
    _data = new k3uploadBufferImpl;
}

k3uploadBufferObj::~k3uploadBufferObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3uploadBufferImpl* k3uploadBufferObj::getImpl()
{
    return _data;
}

const k3uploadBufferImpl* k3uploadBufferObj::getImpl() const
{
    return _data;
}

K3API void* k3uploadBufferObj::MapForWrite(uint64_t size)
{
    HRESULT hr;
    void* ptr = NULL;
    if (_data->_resource == NULL) _data->_size = 0;
    if (_data->_size < size) {
        if (_data->_resource) _data->_resource->Release();
        _data->_size = size;
        D3D12_HEAP_PROPERTIES heap_prop;
        heap_prop.Type = D3D12_HEAP_TYPE_UPLOAD;
        heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_prop.CreationNodeMask = 0;
        heap_prop.VisibleNodeMask = 0;
        D3D12_RESOURCE_DESC desc;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        desc.Width = _data->_size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        hr = _data->_dev->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&_data->_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create commited resource", "k3uploadBufferObj::MapForWrite");
            return NULL;
        }
    }
    if (_data->_resource) {
        _data->_logical_size = size;
        hr = _data->_resource->Map(0, NULL, &ptr);
        if (hr != S_OK) {
            k3error::Handler("Could not map image", "k3uploadBufferObj::MapForWrite");
            return NULL;
        }
    }
    return ptr;
}

K3API void k3uploadBufferObj::Unmap()
{
    if (_data->_resource) {
        _data->_resource->Unmap(0, NULL);
    }
}

K3API void* k3uploadBufferObj::MapRange(uint64_t offset, uint64_t size)
{
    HRESULT hr;
    void* ptr = NULL;
    if (_data->_resource) {
        if (offset + size >= _data->_logical_size) {
            k3error::Handler("Mapping to exceeds buffer size", "k3uploadBufferObj::MapRange");
            return NULL;
        }
        D3D12_RANGE range;
        range.Begin = offset;
        range.End = size;
        hr = _data->_resource->Map(0, &range, &ptr);
        ptr = ((uint8_t*)ptr) + offset;
        if (hr != S_OK) {
            k3error::Handler("Could not map image", "k3uploadBufferObj::MapRange");
            return NULL;
        }
    }
    return ptr;

}
K3API void k3uploadBufferObj::UnmapRange(uint64_t offset, uint64_t size)
{
    if (_data->_resource) {
        if (offset + size >= _data->_logical_size) {
            k3error::Handler("Mapping to exceeds buffer size", "k3uploadBufferObj::UnmapRange");
            return;
        }
        D3D12_RANGE range;
        range.Begin = offset;
        range.End = size;
        _data->_resource->Unmap(0, &range);
    }
}

// ------------------------------------------------------------
// gfx
uint32_t k3gfxImpl::_num_gfx = 0;
IDXGIFactory4* k3gfxImpl::_factory = NULL;
#ifdef _DEBUG
ID3D12Debug1* k3gfxImpl::_debug_controller = NULL;
#endif
IDXGIAdapter1* k3gfxImpl::_adapter = NULL;
char k3gfxImpl::_adapter_name[128] = { 0 };

void k3gfxImpl::ConvertToDx12Rect(D3D12_RECT* dst, const k3rect* src)
{
    dst->left = src->x;
    dst->top = src->y;
    dst->right = src->x + src->width;
    dst->bottom = src->y + src->height;
}

D3D12_CLEAR_FLAGS k3gfxImpl::ConvertToDx12ClearFlags(k3depthSelect clear)
{
    D3D12_CLEAR_FLAGS dx12_clear;
    switch (clear) {
    case k3depthSelect::DEPTH:         dx12_clear = D3D12_CLEAR_FLAG_DEPTH; break;
    case k3depthSelect::STENCIL:       dx12_clear = D3D12_CLEAR_FLAG_STENCIL; break;
    case k3depthSelect::DEPTH_STENCIL: dx12_clear = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL; break;
    default: dx12_clear = D3D12_CLEAR_FLAG_DEPTH; break;
    }
    return dx12_clear;
}

D3D12_RESOURCE_STATES k3gfxImpl::ConvertToDx12ResourceState(k3resourceState state, bool is_depth)
{
    D3D12_RESOURCE_STATES dx12_state = D3D12_RESOURCE_STATE_COMMON;
    switch (state) {
    case k3resourceState::COMMON: dx12_state = D3D12_RESOURCE_STATE_COMMON; break;
    case k3resourceState::SHADER_BUFFER: dx12_state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; break;
    case k3resourceState::INDEX_BUFFER: dx12_state = D3D12_RESOURCE_STATE_INDEX_BUFFER; break;
    case k3resourceState::RENDER_TARGET: dx12_state = (is_depth) ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_RENDER_TARGET; break;
    case k3resourceState::UAV: dx12_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS; break;
    case k3resourceState::DEPTH_WRITE: dx12_state = D3D12_RESOURCE_STATE_DEPTH_WRITE; break;
    case k3resourceState::DEPTH_READ: dx12_state = D3D12_RESOURCE_STATE_DEPTH_READ; break;
    case k3resourceState::FRONT_END_SHADER_RESOURCE: dx12_state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; break;
    case k3resourceState::PIXEL_SHADER_RESOURCE: dx12_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; break;
    case k3resourceState::SHADER_RESOURCE: dx12_state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; break;
    case k3resourceState::STREAM_OUT: dx12_state = D3D12_RESOURCE_STATE_STREAM_OUT; break;
    case k3resourceState::COPY_DEST: dx12_state = D3D12_RESOURCE_STATE_COPY_DEST; break;
    case k3resourceState::COPY_SOURCE: dx12_state = D3D12_RESOURCE_STATE_COPY_SOURCE; break;
    case k3resourceState::RESOLVE_DEST: dx12_state = D3D12_RESOURCE_STATE_RESOLVE_DEST; break;
    case k3resourceState::RESOLVE_SOURCE: dx12_state = D3D12_RESOURCE_STATE_RESOLVE_SOURCE; break;
    case k3resourceState::RT_ACCEL_STRUCT: dx12_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE; break;
    default:
        k3error::Handler("Unknown resource state", "k3gfxImpl::ConvertToDx12ResourceState");
        break;
    }
    return dx12_state;
}

D3D12_ROOT_PARAMETER_TYPE k3gfxImpl::ConvertToDx12RootParameterType(k3bindingType bind_type)
{
    D3D12_ROOT_PARAMETER_TYPE dx12_bind_type = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

    switch (bind_type) {
    case k3bindingType::CONSTANT:       dx12_bind_type = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; break;
    case k3bindingType::CBV:            dx12_bind_type = D3D12_ROOT_PARAMETER_TYPE_CBV; break;
    case k3bindingType::SRV:            dx12_bind_type = D3D12_ROOT_PARAMETER_TYPE_SRV; break;
    case k3bindingType::UAV:            dx12_bind_type = D3D12_ROOT_PARAMETER_TYPE_UAV; break;
    case k3bindingType::VIEW_SET:       dx12_bind_type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; break;
    case k3bindingType::VIEW_SET_TABLE: dx12_bind_type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; break;
    }

    return dx12_bind_type;
}

D3D12_BLEND k3gfxImpl::ConvertToDx12Blend(k3blend blend)
{
    D3D12_BLEND dx12_blend = D3D12_BLEND_ZERO;
    switch (blend) {
    case k3blend::ZERO: dx12_blend = D3D12_BLEND_ZERO; break;
    case k3blend::ONE: dx12_blend = D3D12_BLEND_ONE; break;
    case k3blend::SRC_COLOR: dx12_blend = D3D12_BLEND_SRC_COLOR; break;
    case k3blend::INV_SRC_COLOR: dx12_blend = D3D12_BLEND_INV_SRC_COLOR; break;
    case k3blend::SRC_ALPHA: dx12_blend = D3D12_BLEND_SRC_ALPHA; break;
    case k3blend::INV_SRC_ALPHA: dx12_blend = D3D12_BLEND_INV_SRC_ALPHA; break;
    case k3blend::DEST_ALPHA: dx12_blend = D3D12_BLEND_DEST_ALPHA; break;
    case k3blend::INV_DEST_ALPHA: dx12_blend = D3D12_BLEND_INV_DEST_ALPHA; break;
    case k3blend::DEST_COLOR: dx12_blend = D3D12_BLEND_DEST_COLOR; break;
    case k3blend::INV_DEST_COLOR: dx12_blend = D3D12_BLEND_INV_DEST_COLOR; break;
    case k3blend::SRC_ALPHA_SAT: dx12_blend = D3D12_BLEND_SRC_ALPHA_SAT; break;
    case k3blend::BLEND_FACTOR: dx12_blend = D3D12_BLEND_BLEND_FACTOR; break;
    case k3blend::INV_BLEND_FACTOR: dx12_blend = D3D12_BLEND_INV_BLEND_FACTOR; break;
    case k3blend::SRC1_COLOR: dx12_blend = D3D12_BLEND_SRC1_COLOR; break;
    case k3blend::INV_SRC1_COLOR: dx12_blend = D3D12_BLEND_INV_SRC1_COLOR; break;
    case k3blend::SRC1_ALPHA: dx12_blend = D3D12_BLEND_SRC1_ALPHA; break;
    case k3blend::INV_SRC1_ALPHA: dx12_blend = D3D12_BLEND_INV_SRC1_ALPHA; break;
    }
    return dx12_blend;
}

D3D12_BLEND_OP k3gfxImpl::ConvertToDx12BlendOp(k3blendOp blend_op)
{
    D3D12_BLEND_OP dx12_blend_op = D3D12_BLEND_OP_ADD;
    switch (blend_op) {
    case k3blendOp::ADD: dx12_blend_op = D3D12_BLEND_OP_ADD; break;
    case k3blendOp::SUBTRACT: dx12_blend_op = D3D12_BLEND_OP_SUBTRACT; break;
    case k3blendOp::REV_SUBTRACT: dx12_blend_op = D3D12_BLEND_OP_REV_SUBTRACT; break;
    case k3blendOp::MIN: dx12_blend_op = D3D12_BLEND_OP_MIN; break;
    case k3blendOp::MAX: dx12_blend_op = D3D12_BLEND_OP_MAX; break;
    }
    return dx12_blend_op;
}

D3D12_LOGIC_OP k3gfxImpl::ConvertToDx12Rop(k3rop rop)
{
    D3D12_LOGIC_OP dx12_rop = D3D12_LOGIC_OP_CLEAR;
    switch (rop) {
    case k3rop::CLEAR: dx12_rop = D3D12_LOGIC_OP_CLEAR; break;
    case k3rop::SET: dx12_rop = D3D12_LOGIC_OP_SET; break;
    case k3rop::COPY: dx12_rop = D3D12_LOGIC_OP_COPY; break;
    case k3rop::COPY_INVERTED: dx12_rop = D3D12_LOGIC_OP_COPY_INVERTED; break;
    case k3rop::NOOP: dx12_rop = D3D12_LOGIC_OP_NOOP; break;
    case k3rop::INVERT: dx12_rop = D3D12_LOGIC_OP_INVERT; break;
    case k3rop::AND: dx12_rop = D3D12_LOGIC_OP_AND; break;
    case k3rop::NAND: dx12_rop = D3D12_LOGIC_OP_NAND; break;
    case k3rop::OR: dx12_rop = D3D12_LOGIC_OP_OR; break;
    case k3rop::NOR: dx12_rop = D3D12_LOGIC_OP_NOR; break;
    case k3rop::XOR: dx12_rop = D3D12_LOGIC_OP_XOR; break;
    case k3rop::EQUIV: dx12_rop = D3D12_LOGIC_OP_EQUIV; break;
    case k3rop::AND_REVERSE: dx12_rop = D3D12_LOGIC_OP_AND_REVERSE; break;
    case k3rop::AND_INVERTED: dx12_rop = D3D12_LOGIC_OP_AND_INVERTED; break;
    case k3rop::OR_REVERSE: dx12_rop = D3D12_LOGIC_OP_OR_REVERSE; break;
    case k3rop::OR_INVERTED: dx12_rop = D3D12_LOGIC_OP_OR_INVERTED; break;
    }
    return dx12_rop;
}

D3D12_FILL_MODE k3gfxImpl::ConvertToDx12FillMode(k3fill fill_mode)
{
    D3D12_FILL_MODE dx12_fill_mode = D3D12_FILL_MODE_SOLID;
    switch (fill_mode) {
    case k3fill::WIREFRAME: dx12_fill_mode = D3D12_FILL_MODE_WIREFRAME; break;
    case k3fill::SOLID: dx12_fill_mode = D3D12_FILL_MODE_SOLID; break;
    }
    return dx12_fill_mode;
}

D3D12_CULL_MODE k3gfxImpl::ConvertToDx12CullMode(k3cull cull_mode)
{
    D3D12_CULL_MODE dx12_cull_mode = D3D12_CULL_MODE_NONE;
    switch (cull_mode) {
    case k3cull::NONE: dx12_cull_mode = D3D12_CULL_MODE_NONE; break;
    case k3cull::FRONT: dx12_cull_mode = D3D12_CULL_MODE_FRONT; break;
    case k3cull::BACK: dx12_cull_mode = D3D12_CULL_MODE_BACK; break;
    }
    return dx12_cull_mode;
}

D3D12_CONSERVATIVE_RASTERIZATION_MODE k3gfxImpl::ConvertToDx12ConservativeRast(bool enable)
{
    D3D12_CONSERVATIVE_RASTERIZATION_MODE dx12_cons_rast = (enable) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    return dx12_cons_rast;
}

D3D12_COMPARISON_FUNC k3gfxImpl::ConvertToDx12TestFunc(k3testFunc test)
{
    D3D12_COMPARISON_FUNC dx12_test = D3D12_COMPARISON_FUNC_ALWAYS;
    switch (test) {
    case k3testFunc::NEVER: dx12_test = D3D12_COMPARISON_FUNC_NEVER; break;
    case k3testFunc::LESS: dx12_test = D3D12_COMPARISON_FUNC_LESS; break;
    case k3testFunc::EQUAL: dx12_test = D3D12_COMPARISON_FUNC_EQUAL; break;
    case k3testFunc::LESS_EQUAL: dx12_test = D3D12_COMPARISON_FUNC_LESS_EQUAL; break;
    case k3testFunc::GREATER: dx12_test = D3D12_COMPARISON_FUNC_GREATER; break;
    case k3testFunc::NOT_EQUAL: dx12_test = D3D12_COMPARISON_FUNC_NOT_EQUAL; break;
    case k3testFunc::GREATER_EQUAL: dx12_test = D3D12_COMPARISON_FUNC_GREATER_EQUAL; break;
    case k3testFunc::ALWAYS: dx12_test = D3D12_COMPARISON_FUNC_ALWAYS; break;
    }
    return dx12_test;
}

D3D12_DEPTH_WRITE_MASK k3gfxImpl::ConvertToDx12DepthWrite(bool write_enable)
{
    D3D12_DEPTH_WRITE_MASK dx12_write_mask = (write_enable) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    return dx12_write_mask;
}

D3D12_STENCIL_OP k3gfxImpl::ConvertToDx12StencilOp(k3stencilOp op)
{
    D3D12_STENCIL_OP dx12_op = D3D12_STENCIL_OP_KEEP;
    switch (op) {
    case k3stencilOp::KEEP: dx12_op = D3D12_STENCIL_OP_KEEP; break;
    case k3stencilOp::ZERO: dx12_op = D3D12_STENCIL_OP_ZERO; break;
    case k3stencilOp::REPLACE: dx12_op = D3D12_STENCIL_OP_REPLACE; break;
    case k3stencilOp::INCR_SAT: dx12_op = D3D12_STENCIL_OP_INCR_SAT; break;
    case k3stencilOp::DECR_SAT: dx12_op = D3D12_STENCIL_OP_DECR_SAT; break;
    case k3stencilOp::INVERT: dx12_op = D3D12_STENCIL_OP_INVERT; break;
    case k3stencilOp::INCR: dx12_op = D3D12_STENCIL_OP_INCR; break;
    case k3stencilOp::DECR: dx12_op = D3D12_STENCIL_OP_DECR; break;
    }
    return dx12_op;
}

D3D12_INPUT_CLASSIFICATION k3gfxImpl::ConvertToDx12InputType(k3inputType input_type)
{
    D3D12_INPUT_CLASSIFICATION dx12_input_type = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    switch (input_type) {
    case k3inputType::VERTEX: dx12_input_type = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; break;
    case k3inputType::INSTANCE: dx12_input_type = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA; break;
    }
    return dx12_input_type;
}

D3D12_INDEX_BUFFER_STRIP_CUT_VALUE k3gfxImpl::ConvertToDx12StripCut(k3stripCut cut)
{
    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE dx12_cut_value = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    switch (cut) {
    case k3stripCut::NONE: dx12_cut_value = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; break;
    case k3stripCut::CUT_FFFF: dx12_cut_value = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF; break;
    case k3stripCut::CUT_FFFF_FFFF: dx12_cut_value = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF; break;
    }
    return dx12_cut_value;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE k3gfxImpl::ConvertToDx12PrimType(k3primType prim)
{
    D3D12_PRIMITIVE_TOPOLOGY_TYPE dx12_prim_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    switch (prim) {
    case k3primType::UNDEFINED: dx12_prim_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED; break;
    case k3primType::POINT: dx12_prim_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT; break;
    case k3primType::LINE: dx12_prim_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; break;
    case k3primType::TRIANGLE: dx12_prim_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
    case k3primType::PATCH: dx12_prim_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH; break;
    }
    return dx12_prim_type;
}

D3D_PRIMITIVE_TOPOLOGY k3gfxImpl::ConvertToDx12DrawPrimType(k3drawPrimType draw_prim)
{
    D3D_PRIMITIVE_TOPOLOGY dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    switch (draw_prim) {
    case k3drawPrimType::UNDEFINED: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED; break;
    case k3drawPrimType::POINTLIST: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
    case k3drawPrimType::LINELIST: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
    case k3drawPrimType::LINESTRIP: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
    case k3drawPrimType::TRIANGLELIST: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
    case k3drawPrimType::TRIANGLESTRIP: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
    case k3drawPrimType::LINELIST_ADJ: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ; break;
    case k3drawPrimType::LINESTRIP_ADJ: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ; break;
    case k3drawPrimType::TRIANGLELIST_ADJ: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ; break;
    case k3drawPrimType::TRIANGLESTRIP_ADJ: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_1: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_2: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_3: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_4: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_5: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_6: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_7: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_8: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_9: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_10: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_11: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_12: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_13: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_14: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_15: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_16: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_17: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_18: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_19: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_20: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_21: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_22: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_23: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_24: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_25: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_26: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_27: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_28: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_29: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_30: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_31: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST; break;
    case k3drawPrimType::CONTROL_POINT_PATCHLIST_32: dx12_draw_prim = D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST; break;
    }
    return dx12_draw_prim;
}

D3D12_HEAP_TYPE k3gfxImpl::ConvertToDx12MemType(k3memType mem_type)
{
    D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT;
    switch (mem_type) {
    case k3memType::GPU: heap_type = D3D12_HEAP_TYPE_DEFAULT; break;
    case k3memType::UPLOAD: heap_type = D3D12_HEAP_TYPE_UPLOAD; break;
    case k3memType::READBACK: heap_type = D3D12_HEAP_TYPE_READBACK; break;
    }
    return heap_type;
}

D3D12_DESCRIPTOR_RANGE_TYPE k3gfxImpl::ConvertToDx12ShaderBindType(k3shaderBindType type)
{
    D3D12_DESCRIPTOR_RANGE_TYPE dx12_desc_range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    switch (type) {
    case k3shaderBindType::SRV: dx12_desc_range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
    case k3shaderBindType::CBV: dx12_desc_range_type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; break;
    case k3shaderBindType::UAV: dx12_desc_range_type = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; break;
    case k3shaderBindType::SAMPLER: dx12_desc_range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; break;
    }
    return dx12_desc_range_type;
}

D3D12_FILTER k3gfxImpl::ConvertToDx12Fitler(k3texFilter filter)
{
    D3D12_FILTER dx12_filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    switch (filter) {
    case k3texFilter::MIN_MAG_MIP_POINT: dx12_filter = D3D12_FILTER_MIN_MAG_MIP_POINT; break;
    case k3texFilter::MIN_MAG_POINT_MIP_LINEAR: dx12_filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR; break;
    case k3texFilter::MIN_POINT_MAG_LINEAR_MIP_POINT: dx12_filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
    case k3texFilter::MIN_POINT_MAG_MIP_LINEAR: dx12_filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR; break;
    case k3texFilter::MIN_LINEAR_MAG_MIP_POINT: dx12_filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT; break;
    case k3texFilter::MIN_LINEAR_MAG_POINT_MIP_LINEAR: dx12_filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
    case k3texFilter::MIN_MAG_LINEAR_MIP_POINT: dx12_filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT; break;
    case k3texFilter::MIN_MAG_MIP_LINEAR: dx12_filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; break;
    case k3texFilter::ANISOTROPIC: dx12_filter = D3D12_FILTER_ANISOTROPIC; break;
    case k3texFilter::COMPARISON_MIN_MAG_MIP_POINT: dx12_filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT; break;
    case k3texFilter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR: dx12_filter = D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR; break;
    case k3texFilter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT: dx12_filter = D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
    case k3texFilter::COMPARISON_MIN_POINT_MAG_MIP_LINEAR: dx12_filter = D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR; break;
    case k3texFilter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT: dx12_filter = D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT; break;
    case k3texFilter::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR: dx12_filter = D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
    case k3texFilter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT: dx12_filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; break;
    case k3texFilter::COMPARISON_MIN_MAG_MIP_LINEAR: dx12_filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; break;
    case k3texFilter::COMPARISON_ANISOTROPIC: dx12_filter = D3D12_FILTER_COMPARISON_ANISOTROPIC; break;
    case k3texFilter::MINIMUM_MIN_MAG_MIP_POINT: dx12_filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT; break;
    case k3texFilter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR: dx12_filter = D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR; break;
    case k3texFilter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT: dx12_filter = D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
    case k3texFilter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR: dx12_filter = D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR; break;
    case k3texFilter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT: dx12_filter = D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT; break;
    case k3texFilter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR: dx12_filter = D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
    case k3texFilter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT: dx12_filter = D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT; break;
    case k3texFilter::MINIMUM_MIN_MAG_MIP_LINEAR: dx12_filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR; break;
    case k3texFilter::MINIMUM_ANISOTROPIC: dx12_filter = D3D12_FILTER_MINIMUM_ANISOTROPIC; break;
    case k3texFilter::MAXIMUM_MIN_MAG_MIP_POINT: dx12_filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT; break;
    case k3texFilter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR: dx12_filter = D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR; break;
    case k3texFilter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT: dx12_filter = D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
    case k3texFilter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR: dx12_filter = D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR; break;
    case k3texFilter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT: dx12_filter = D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT; break;
    case k3texFilter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR: dx12_filter = D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
    case k3texFilter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT: dx12_filter = D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT; break;
    case k3texFilter::MAXIMUM_MIN_MAG_MIP_LINEAR: dx12_filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR; break;
    case k3texFilter::MAXIMUM_ANISOTROPIC: dx12_filter = D3D12_FILTER_MAXIMUM_ANISOTROPIC; break;
    }
    return dx12_filter;
}
D3D12_TEXTURE_ADDRESS_MODE k3gfxImpl::ConvertToDx12AddrMode(k3texAddr addr_mode)
{
    D3D12_TEXTURE_ADDRESS_MODE dx12_addr_mode = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    switch (addr_mode) {
    case k3texAddr::WRAP: dx12_addr_mode = D3D12_TEXTURE_ADDRESS_MODE_WRAP; break;
    case k3texAddr::CLAMP: dx12_addr_mode = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; break;
    case k3texAddr::MIRROR: dx12_addr_mode = D3D12_TEXTURE_ADDRESS_MODE_MIRROR; break;
    case k3texAddr::MIRROR_ONCE: dx12_addr_mode = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE; break;
    }
    return dx12_addr_mode;
}

D3D12_HIT_GROUP_TYPE k3gfxImpl::ConvertToDx12HitGroupType(k3rtHitGroupType hit_group_type)
{
    D3D12_HIT_GROUP_TYPE dx12_hit_group_type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    switch (hit_group_type) {
    case k3rtHitGroupType::TRIANGLES: dx12_hit_group_type = D3D12_HIT_GROUP_TYPE_TRIANGLES; break;
    case k3rtHitGroupType::PROCEDURAL: dx12_hit_group_type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE; break;
    }
    return dx12_hit_group_type;
}


k3winObj::k3winObj()
{
    _data = new k3win32Dx12WinImpl;
}

K3API k3surf k3winObj::GetBackBuffer()
{
    k3win32Dx12WinImpl* d = (k3win32Dx12WinImpl*)_data;
    UINT b = d->_swap_chain->GetCurrentBackBufferIndex();
    return d->_surf[b];
}

K3API void k3winObj::SetFullscreen(bool fullscreen)
{
    if (fullscreen == _data->_is_fullscreen) return;
    k3win32Dx12WinImpl* d = (k3win32Dx12WinImpl*)_data;

    HRESULT hr = d->_swap_chain->SetFullscreenState(fullscreen, NULL);
    if (hr != S_OK) {
        k3error::Handler("Could not change fullscreen state", "k3winObj::SetFullscreen");
        return;
    }

    if(fullscreen) {
        SetWindowLong(d->_hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
        SetWindowPos(d->_hwnd, NULL, 0, 0, d->_width, d->_height, SWP_NOZORDER);
    } else {
        uint32_t full_width, full_height;
        d->GetFullWindowSize(&full_width, &full_height);
        SetWindowLong(d->_hwnd, GWL_STYLE, k3win32WinImpl::_windowed_style | WS_VISIBLE);
        SetWindowPos(d->_hwnd, NULL, d->_x_pos, d->_y_pos, full_width, full_height, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    }
    d->_is_fullscreen = fullscreen;
}

K3API void k3winObj::SetVsyncInterval(uint32_t interval)
{
    _data->_vsync_interval = interval;
}

K3API void k3winObj::SwapBuffer()
{
    k3win32Dx12WinImpl* d = (k3win32Dx12WinImpl*)_data;
    d->_swap_chain->Present(d->_vsync_interval, 0);
}


k3win32Dx12WinImpl::k3win32Dx12WinImpl()
{
    _rtv_heap = NULL;
    _swap_chain = NULL;
}

k3win32Dx12WinImpl::~k3win32Dx12WinImpl()
{
    if (_rtv_heap) {
        _rtv_heap->Release();
        _rtv_heap = NULL;
    }
    if (_swap_chain) {
        _swap_chain->Release();
        _swap_chain = NULL;
    }
}

DXGI_FORMAT k3win32Dx12WinImpl::ConvertToDXGIFormat(k3fmt fmt, k3DxgiSurfaceType surf_type)
{
    DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN;

    switch (fmt) {
    case k3fmt::RGBA8_UNORM: dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
    case k3fmt::BGRA8_UNORM: dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM; break;
    //case k3fmt::RGBX8_UNORM: dxgi_format = DXGI_FORMAT_
    case k3fmt::BGRX8_UNORM: dxgi_format = DXGI_FORMAT_B8G8R8X8_UNORM; break;
    case k3fmt::RGBA16_UNORM: dxgi_format = DXGI_FORMAT_R16G16B16A16_UNORM; break;
    case k3fmt::RGBA16_UINT: dxgi_format = DXGI_FORMAT_R16G16B16A16_UINT; break;
    case k3fmt::RGBA16_FLOAT: dxgi_format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
        //case k3fmt::RGBA32_UNORM: dxgi_format = DXGI_FORMAT_R32G32B32A32_UNORM; break;
    case k3fmt::RGBA32_UINT: dxgi_format = DXGI_FORMAT_R32G32B32A32_UINT; break;
    case k3fmt::RGBA32_FLOAT: dxgi_format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
    case k3fmt::RGB10A2_UNORM: dxgi_format = DXGI_FORMAT_R10G10B10A2_UNORM; break;
    case k3fmt::BGR5A1_UNORM: dxgi_format = DXGI_FORMAT_B5G5R5A1_UNORM; break;

        //case k3fmt::RGB8_UNORM: dxgi_format = DXGI_FORMAT_
        //case k3fmt::BGR8_UNORM: dxgi_format = DXGI_FORMAT_B8G8R8_UNORM; break;
    case k3fmt::RGB32_UINT: dxgi_format = DXGI_FORMAT_R32G32B32_UINT; break;
    case k3fmt::RGB32_FLOAT: dxgi_format = DXGI_FORMAT_R32G32B32_FLOAT; break;
    case k3fmt::B5G6R5_UNORM: dxgi_format = DXGI_FORMAT_B5G6R5_UNORM; break;

    case k3fmt::RG8_UNORM: dxgi_format = DXGI_FORMAT_R8G8_UNORM; break;
    case k3fmt::RG16_UNORM: dxgi_format = DXGI_FORMAT_R16G16_UNORM; break;
    case k3fmt::RG16_UINT: dxgi_format = DXGI_FORMAT_R16G16_UINT; break;
    case k3fmt::RG16_FLOAT: dxgi_format = DXGI_FORMAT_R16G16_FLOAT; break;
        //case k3fmt::RG32_UNORM: dxgi_format = DXGI_FORMAT_R32G32
    case k3fmt::RG32_UINT: dxgi_format = DXGI_FORMAT_R32G32_UINT; break;
    case k3fmt::RG32_FLOAT: dxgi_format = DXGI_FORMAT_R32G32_FLOAT; break;

    case k3fmt::R8_UNORM: dxgi_format = DXGI_FORMAT_R8_UNORM; break;
    case k3fmt::A8_UNORM: dxgi_format = DXGI_FORMAT_A8_UNORM; break;
    case k3fmt::R16_UNORM: dxgi_format = DXGI_FORMAT_R16_UNORM; break;
    case k3fmt::R16_UINT: dxgi_format = DXGI_FORMAT_R16_UINT; break;
    case k3fmt::R16_FLOAT: dxgi_format = DXGI_FORMAT_R16_FLOAT; break;
    case k3fmt::R32_UINT: dxgi_format = DXGI_FORMAT_R32_UINT; break;
        //case k3fmt::R32_UNORM: dxgi_format = DXGI_FORMAT_
    case k3fmt::R32_FLOAT: dxgi_format = DXGI_FORMAT_R32_FLOAT; break;

    case k3fmt::BC1_UNORM: dxgi_format = DXGI_FORMAT_BC1_UNORM; break;
    case k3fmt::BC2_UNORM: dxgi_format = DXGI_FORMAT_BC2_UNORM; break;
    case k3fmt::BC3_UNORM: dxgi_format = DXGI_FORMAT_BC3_UNORM; break;
    case k3fmt::BC4_UNORM: dxgi_format = DXGI_FORMAT_BC4_UNORM; break;
    case k3fmt::BC5_UNORM: dxgi_format = DXGI_FORMAT_BC5_UNORM; break;
        //case k3fmt::BC6_UNORM: dxgi_format = DXGI_FORMAT_BC6_UNORM; break;
    case k3fmt::BC7_UNORM: dxgi_format = DXGI_FORMAT_BC7_UNORM; break;

    case k3fmt::D16_UNORM:
        switch (surf_type) {
        case k3DxgiSurfaceType::TYPELESS: dxgi_format = DXGI_FORMAT_R16_TYPELESS; break;
        case k3DxgiSurfaceType::COLOR:    dxgi_format = DXGI_FORMAT_R16_UNORM; break;
        case k3DxgiSurfaceType::DEPTH:    dxgi_format = DXGI_FORMAT_D16_UNORM; break;
        }
        break;
    case k3fmt::D24X8_UNORM:
        switch (surf_type) {
        case k3DxgiSurfaceType::TYPELESS: dxgi_format = DXGI_FORMAT_R24G8_TYPELESS; break;
        case k3DxgiSurfaceType::COLOR:    dxgi_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
        case k3DxgiSurfaceType::DEPTH:    dxgi_format = DXGI_FORMAT_D24_UNORM_S8_UINT; break;
        }
        break;
    case k3fmt::D24_UNORM_S8_UINT:
        switch (surf_type) {
        case k3DxgiSurfaceType::TYPELESS: dxgi_format = DXGI_FORMAT_R24G8_TYPELESS; break;
        case k3DxgiSurfaceType::COLOR:    dxgi_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
        case k3DxgiSurfaceType::DEPTH:    dxgi_format = DXGI_FORMAT_D24_UNORM_S8_UINT; break;
        }
        break;
    case k3fmt::D32_FLOAT:
        switch (surf_type) {
        case k3DxgiSurfaceType::TYPELESS: dxgi_format = DXGI_FORMAT_R32_TYPELESS; break;
        case k3DxgiSurfaceType::COLOR:    dxgi_format = DXGI_FORMAT_R32_FLOAT; break;
        case k3DxgiSurfaceType::DEPTH:    dxgi_format = DXGI_FORMAT_D32_FLOAT; break;
        }
        break;
    case k3fmt::D32_FLOAT_S8X24_UINT:
        switch (surf_type) {
        case k3DxgiSurfaceType::TYPELESS: dxgi_format = DXGI_FORMAT_R32G8X24_TYPELESS; break;
        case k3DxgiSurfaceType::COLOR:    dxgi_format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; break;
        case k3DxgiSurfaceType::DEPTH:    dxgi_format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT; break;
        }
        break;
    }

    return dxgi_format;
}

k3fmt k3win32Dx12WinImpl::ConertFromDXGIFormat(DXGI_FORMAT fmt)
{
    k3fmt out_fmt = k3fmt::UNKNOWN;
    switch (fmt) {
    case DXGI_FORMAT_R8G8B8A8_UNORM: out_fmt = k3fmt::RGBA8_UNORM; break;
    case DXGI_FORMAT_B8G8R8A8_UNORM: out_fmt = k3fmt::BGRA8_UNORM; break;
    case DXGI_FORMAT_B8G8R8X8_UNORM: out_fmt = k3fmt::BGRX8_UNORM; break;
    case DXGI_FORMAT_R16G16B16A16_UNORM: out_fmt = k3fmt::RGBA16_UNORM; break;
    case DXGI_FORMAT_R16G16B16A16_UINT: out_fmt = k3fmt::RGBA16_UINT; break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT: out_fmt = k3fmt::RGBA16_FLOAT; break;
    case DXGI_FORMAT_R32G32B32A32_UINT: out_fmt = k3fmt::RGBA32_UINT; break;
    case DXGI_FORMAT_R32G32B32A32_FLOAT: out_fmt = k3fmt::RGBA32_FLOAT; break;
    case DXGI_FORMAT_R10G10B10A2_UNORM: out_fmt = k3fmt::RGB10A2_UNORM; break;
    case DXGI_FORMAT_B5G5R5A1_UNORM: out_fmt = k3fmt::BGR5A1_UNORM; break;
    case DXGI_FORMAT_R32G32B32_UINT: out_fmt = k3fmt::RGB32_UINT; break;
    case DXGI_FORMAT_R32G32B32_FLOAT: out_fmt = k3fmt::RGB32_FLOAT; break;
    case DXGI_FORMAT_B5G6R5_UNORM: out_fmt = k3fmt::B5G6R5_UNORM; break;
    case DXGI_FORMAT_R8G8_UNORM: out_fmt = k3fmt::RG8_UNORM; break;
    case DXGI_FORMAT_R16G16_UNORM: out_fmt = k3fmt::RG16_UNORM; break;
    case DXGI_FORMAT_R16G16_UINT: out_fmt = k3fmt::RG16_UINT; break;
    case DXGI_FORMAT_R16G16_FLOAT: out_fmt = k3fmt::RG16_FLOAT; break;
    case DXGI_FORMAT_R32G32_UINT: out_fmt = k3fmt::RG32_UINT; break;
    case DXGI_FORMAT_R32G32_FLOAT: out_fmt = k3fmt::RG32_FLOAT; break;
    case DXGI_FORMAT_R8_UNORM: out_fmt = k3fmt::R8_UNORM; break;
    case DXGI_FORMAT_A8_UNORM: out_fmt = k3fmt::A8_UNORM; break;
    case DXGI_FORMAT_R16_UNORM: out_fmt = k3fmt::R16_UNORM; break;
    case DXGI_FORMAT_R16_UINT: out_fmt = k3fmt::R16_UINT; break;
    case DXGI_FORMAT_R16_FLOAT: out_fmt = k3fmt::R16_FLOAT; break;
    case DXGI_FORMAT_R32_UINT: out_fmt = k3fmt::R32_UINT; break;
    case DXGI_FORMAT_R32_FLOAT: out_fmt = k3fmt::R32_FLOAT; break;
    case DXGI_FORMAT_BC1_UNORM: out_fmt = k3fmt::BC1_UNORM; break;
    case DXGI_FORMAT_BC2_UNORM: out_fmt = k3fmt::BC2_UNORM; break;
    case DXGI_FORMAT_BC3_UNORM: out_fmt = k3fmt::BC3_UNORM; break;
    case DXGI_FORMAT_BC4_UNORM: out_fmt = k3fmt::BC4_UNORM; break;
    case DXGI_FORMAT_BC5_UNORM: out_fmt = k3fmt::BC5_UNORM; break;
    case DXGI_FORMAT_BC7_UNORM: out_fmt = k3fmt::BC7_UNORM; break;
    case DXGI_FORMAT_D16_UNORM: out_fmt = k3fmt::D16_UNORM; break;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS: out_fmt = k3fmt::D24X8_UNORM; break;
    case DXGI_FORMAT_D24_UNORM_S8_UINT: out_fmt = k3fmt::D24_UNORM_S8_UINT; break;
    case DXGI_FORMAT_D32_FLOAT: out_fmt = k3fmt::D32_FLOAT; break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: out_fmt = k3fmt::D32_FLOAT_S8X24_UINT; break;
    }

    return out_fmt;
}


void k3win32Dx12WinImpl::ResizeBackBuffer()
{
    DXGI_FORMAT dxgi_fmt = ConvertToDXGIFormat(_color_fmt, k3DxgiSurfaceType::COLOR);
    UINT swap_chain_flags = 0;
    HRESULT hr;
    uint32_t n;
    
    k3gfxImpl* gfxImpl = gfx->getImpl();
    if (_swap_chain) {
        gfx->WaitGpuIdle();
        for (n = 0; n < BACK_BUFFERS; n++) {
            if (_surf[n] != NULL) {
                k3surfImpl* surfImpl = _surf[n]->getImpl();
                k3resourceImpl* resourceImpl = surfImpl->_resource->getImpl();
                resourceImpl->_dx12_resource->Release();
                resourceImpl->_dx12_resource = NULL;
            }
        }
        _swap_chain->ResizeBuffers(BACK_BUFFERS, _width, _height, dxgi_fmt, swap_chain_flags);
    } else {
        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;
        ZeroMemory(&swap_chain_desc, sizeof(DXGI_SWAP_CHAIN_DESC1));
        swap_chain_desc.BufferCount = BACK_BUFFERS;
        swap_chain_desc.Width = _width;
        swap_chain_desc.Height = _height;
        swap_chain_desc.Format = dxgi_fmt;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swap_chain_desc.Scaling = DXGI_SCALING_NONE;
        swap_chain_desc.Stereo = false;
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_desc;
        ZeroMemory(&swap_chain_fullscreen_desc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
        swap_chain_fullscreen_desc.Windowed = !_is_fullscreen;

        IDXGISwapChain1* new_swap_chain;
        hr = k3gfxImpl::_factory->CreateSwapChainForHwnd(gfxImpl->_cmd_q, _hwnd, &swap_chain_desc, &swap_chain_fullscreen_desc, NULL, &new_swap_chain);
        if (hr != S_OK) {
            k3error::Handler("Could not create swap chain", "k3win32Dx12WinImpl::ResizeBackBuffer");
            return;
        }
        hr = new_swap_chain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&new_swap_chain);
        new_swap_chain->Release();
        if (hr != S_OK) {
            k3error::Handler("Could not query swap chain interface", "k3win32Dx12WinImpl::ResizeBackBuffer");
            return;
        }
        _swap_chain = (IDXGISwapChain3*)new_swap_chain;
    }

    // create RTV heap
    if (_rtv_heap == NULL) {
        D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
        ZeroMemory(&rtv_heap_desc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
        rtv_heap_desc.NumDescriptors = BACK_BUFFERS;
        rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtv_heap_desc.NodeMask = 0;
        hr = gfxImpl->_dev->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&_rtv_heap));
        if (hr != S_OK) {
            k3error::Handler("Could not create RTV heap", "k3win32Dx12WinImpl::ResizeBackBuffer");
            return;
        }
    }
    
    UINT rtv_desc_size = gfxImpl->_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle;// (_rtv_heap->GetCPUDescriptorHandleForHeapStart());
    rtv_handle = _rtv_heap->GetCPUDescriptorHandleForHeapStart();
    for (n = 0; n < BACK_BUFFERS; n++) {
        if (_surf[n] == NULL) _surf[n] = new k3surfObj;
        k3surfImpl* surfImpl = _surf[n]->getImpl();
        if (surfImpl->_resource == NULL) surfImpl->_resource = new k3resourceObj;
        k3resourceImpl* resourceImpl = surfImpl->_resource->getImpl();
        hr = _swap_chain->GetBuffer(n, IID_PPV_ARGS(&resourceImpl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not get swap chain back buffer", "k3win32Dx12WinImpl::ResizeBackBuffer");
            return;
        }
        gfxImpl->_dev->CreateRenderTargetView(resourceImpl->_dx12_resource, NULL, rtv_handle);
        resourceImpl->_width = _width;
        resourceImpl->_height = _height;
        resourceImpl->_depth = 1;
        resourceImpl->_max_mip = 0;
        resourceImpl->_samples = 1;
        resourceImpl->_format = _color_fmt;
        resourceImpl->_resource_state = k3resourceState::COMMON;
        surfImpl->_rtv_cpu_view = rtv_handle;
        rtv_handle.ptr +=  rtv_desc_size;
    }
}

k3gfxImpl::k3gfxImpl()
{
    _dev = NULL;
    _dev5 = NULL;
    _dxr_tier = 0;
#ifdef _DEBUG
    _dbg_dev = NULL;
#endif
    _cmd_q = NULL;
    _cpu_fence = NULL;
    _shader_heap[0] = NULL;
    _shader_heap[1] = NULL;
    _font_vs = NULL;
    _font_ps = NULL;
}

k3gfxObj::k3gfxObj()
{
    _data = new k3gfxImpl;
    k3gfxImpl::_num_gfx++;
}

k3gfxObj::~k3gfxObj()
{
    if (_data) {
        if (_data->_shader_heap[0]) _data->_shader_heap[0]->Release();
        if (_data->_shader_heap[1]) _data->_shader_heap[1]->Release();
        if (_data->_cmd_q) _data->_cmd_q->Release();
#ifdef _DEBUG
        if (_data->_dbg_dev) _data->_dbg_dev->Release();
#endif
        if (_data->_dev) _data->_dev->Release();
        if (_data->_dev5) _data->_dev5->Release();
        delete _data;
        _data = NULL;
    }
    if (k3gfxImpl::_num_gfx) k3gfxImpl::_num_gfx--;
    if (k3gfxImpl::_num_gfx == 0) {
        if (k3gfxImpl::_factory) {
            k3gfxImpl::_factory->Release();
            k3gfxImpl::_factory = NULL;
        }
#ifdef _DEBUG
        if (k3gfxImpl::_debug_controller) {
            k3gfxImpl::_debug_controller->Release();
            k3gfxImpl::_debug_controller = NULL;
        }
#endif
        if (k3gfxImpl::_adapter) {
            k3gfxImpl::_adapter->Release();
            k3gfxImpl::_adapter = NULL;
        }
    }
}

k3gfxImpl* k3gfxObj::getImpl()
{
    return _data;
}

const k3gfxImpl* k3gfxObj::getImpl() const
{
    return _data;
}

void k3gfxObj::GetFontShaders(k3shader& vs, k3shader& ps)
{
    if (_data->_font_vs == NULL) {
        const char* vs_code =
            "cbuffer font_cbuffer : register(b0) {\n"
            "   unsigned int4 text[4];\n"
            "   float4 xform[64];\n"
            "   float4 fg_color;\n"
            "   float4 bg_color;\n"
            "   float4 char_scale[128];\n"
            "}\n"
            "struct VS_OUTPUT {\n"
            "    float4 pos : SV_POSITION;\n"
            "    float2 texcoord : TEXCOORD0;\n"
            "};\n"
            "VS_OUTPUT main( unsigned int vert : SV_VertexID, unsigned int instance : SV_InstanceID ) {\n"
            "   VS_OUTPUT o;\n"
            "   float2 i_position;\n"
            "   if(vert==0)      i_position = float2(0.0, 0.0);\n"
            "   else if(vert==1) i_position = float2(1.0, 0.0);\n"
            "   else if(vert==2) i_position = float2(0.0, 1.0);\n"
            "   else if(vert==3) i_position = float2(1.0, 1.0);\n"
            "   float4 this_xform = xform[instance];\n"
            "   o.pos.xy = (i_position * this_xform.xy) + this_xform.zw;\n"
            "   o.pos.zw = float2( 0.0, 1.0 );\n"
            "   unsigned int4 this_char16 = text[instance >> 4];\n"
            "   unsigned int2 this_char8 = (((instance >> 3) & 1) == 0) ? this_char16.xy : this_char16.zw;\n"
            "   unsigned int  this_char4 = (((instance >> 2) & 1) == 0) ? this_char8.x   : this_char8.y;\n"
            "   unsigned int  this_char = (this_char4 >> ((instance & 3) << 3)) & 0xff;\n"
            "   float4 cscale = char_scale[this_char];\n"
            "   o.texcoord.xy = i_position*cscale.xy + cscale.zw;\n"
            "   return o;\n"
            "}\n";
        _data->_font_vs = CompileShaderFromString(vs_code, k3shaderType::VERTEX_SHADER);
    }
    if (_data->_font_ps == NULL) {
        const char* ps_code = 
            "cbuffer font_cbuffer : register(b0) {\n"
            "   unsigned int4 text[4];\n"
            "   float4 xform[64];\n"
            "   float4 fg_color;\n"
            "   float4 bg_color;\n"
            "   float4 char_scale[128];\n"
            "}\n"
            "Texture2D<float4> glyphs : register(t0);\n"
            "SamplerState sampleLinear : register(s0);\n"
            "struct PS_INPUT {\n"
            "   float4 pos : SV_POSITION;\n"
            "   float2 texcoord : TEXCOORD0;\n"
            "};\n"
            "float4 main( PS_INPUT i ) : SV_TARGET {\n"
            "   float blend = glyphs.Sample( sampleLinear, i.texcoord ).r;\n"
            "   return fg_color*blend + bg_color*(1.0-blend);\n"
            "}\n";
        _data->_font_ps = CompileShaderFromString(ps_code, k3shaderType::PIXEL_SHADER);
    }
    vs = _data->_font_vs;
    ps = _data->_font_ps;
}

k3gfx k3gfxObj::Create(uint32_t num_views, uint32_t num_samplers)
{
    // Create the factory, and debug controller if needed
    UINT dxgi_factory_flags = 0;
    HRESULT hr;

    k3gfx gfx = new k3gfxObj;

#ifdef _DEBUG
    if (k3gfxImpl::_debug_controller == NULL) {
        ID3D12Debug* dc;
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dc));
        if (hr != S_OK) {
            k3error::Handler("Could not get dx12 debug interface", "k3gfxObj::Create");
            return NULL;
        }
        hr = dc->QueryInterface(IID_PPV_ARGS(&k3gfxImpl::_debug_controller));
        if (hr != S_OK) {
            k3error::Handler("Could not get dx12 query debug interface", "k3gfxObj::Create");
            return NULL;
        }
        k3gfxImpl::_debug_controller->EnableDebugLayer();
        k3gfxImpl::_debug_controller->SetEnableGPUBasedValidation(false);
        dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        dc->Release();
        dc = NULL;
    }
#endif
    if (k3gfxImpl::_factory == NULL) {
        hr = CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&k3gfxImpl::_factory));
        if (hr != S_OK) {
            k3error::Handler("Could not create dxgi factory", "k3gfxObj::Create");
            return NULL;
        }
    }

    // find the right adapter
    DXGI_ADAPTER_DESC1 desc;
    UINT adapter_index;
    bool adapter_found = false;
    if (k3gfxImpl::_adapter == NULL) {
        for (adapter_index = 0; k3gfxImpl::_factory->EnumAdapters1(adapter_index, &k3gfxImpl::_adapter) != DXGI_ERROR_NOT_FOUND; adapter_index++) {
            k3gfxImpl::_adapter->GetDesc1(&desc);

            // Dont select software renderer
            if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {

                hr = D3D12CreateDevice(k3gfxImpl::_adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), NULL);
                if (hr == S_FALSE) {
                    size_t bytes_converted;
                    wcstombs_s(&bytes_converted, k3gfxImpl::_adapter_name, desc.Description, 128);
                    adapter_found = true;
                    break;
                }
            }

            // if this adpater didn't work, release it
            k3gfxImpl::_adapter->Release();
        }
    }

    // if nothing found, error out
    if (!adapter_found) {
        k3error::Handler("Could not find dx12 device", "k3gfxObj::Create");
        return NULL;
    }

    // Create a dxr device, and copy the handle to the standard device
    hr = D3D12CreateDevice(k3gfxImpl::_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&(gfx->_data->_dev)));
    gfx->_data->_dev->QueryInterface(IID_PPV_ARGS(&(gfx->_data->_dev5)));

    // Create the device, and debug device if needed
    if (hr != S_OK) {
        k3error::Handler("Could not create d3d12 device", "k3gfxObj::Create");
        return NULL;
    }

    // Check DXR tier
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 opt5 = { 0 };
    hr = gfx->_data->_dev5->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &opt5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
    if (hr != S_OK) {
        k3error::Handler("Could not get feature support", "k3gfxObj::Create");
        return NULL;
    }
    switch (opt5.RaytracingTier) {
    case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: gfx->_data->_dxr_tier = 0; break;
    case D3D12_RAYTRACING_TIER_1_0: gfx->_data->_dxr_tier = 10; break;
    case D3D12_RAYTRACING_TIER_1_1: gfx->_data->_dxr_tier = 11; break;
    }

#ifdef _DEBUG
    hr = gfx->_data->_dev->QueryInterface(&(gfx->_data->_dbg_dev));
    if (hr != S_OK) {
        k3error::Handler("Could not create d3d12 debug device", "k3gfxObj::Create");
        return NULL;
    }
#endif

    // Create command queueu and allocator
    D3D12_COMMAND_QUEUE_DESC cmd_q_desc = { };
    cmd_q_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmd_q_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = gfx->_data->_dev->CreateCommandQueue(&cmd_q_desc, IID_PPV_ARGS(&(gfx->_data->_cmd_q)));
    if (hr != S_OK) {
        k3error::Handler("Could not create command queue", "k3gfxObj::Create");
        return NULL;
    }
    gfx->_data->_cpu_fence = gfx->CreateFence();

    D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc;
    if (num_views > 0) {
        desc_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc_heap_desc.NumDescriptors = num_views;
        desc_heap_desc.NodeMask = 0;
        desc_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = gfx->_data->_dev->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(&gfx->_data->_shader_heap[0]));
        if (hr != S_OK) {
            k3error::Handler("Could not create descriptor heap", "k3gfxObj::Create");
            return NULL;
        }
    }
    if (num_samplers > 0) {
        desc_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc_heap_desc.NumDescriptors = num_samplers;
        hr = gfx->_data->_dev->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(&gfx->_data->_shader_heap[1]));
        if (hr != S_OK) {
            k3error::Handler("Could not create sampler heap", "k3gfxObj::Create");
            return NULL;
        }
    }

    return gfx;
}

K3API const char* k3gfxObj::AdapterName()
{
    return k3gfxImpl::_adapter_name;
}

K3API uint32_t k3gfxObj::GetRayTracingSupport()
{
    return _data->_dxr_tier;
}

K3API k3fence k3gfxObj::CreateFence()
{
    k3fence fence = new k3fenceObj;
    k3fenceImpl* fenceImpl = fence->getImpl();
    HRESULT hr;
    hr = _data->_dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&(fenceImpl->_fence));
    if (hr != S_OK) {
        k3error::Handler("Could not create fence", "k3gfxObj::CreateFence");
        return NULL;
    }
    fenceImpl->_cmd_q = _data->_cmd_q;
    return fence;
}

K3API void k3gfxObj::WaitGpuIdle()
{
    k3fence fence = _data->_cpu_fence;
    uint64_t fence_value = fence->SetGpuFence(k3gpuQueue::GRAPHICS);
    fence->WaitCpuFence(fence_value);
}

K3API k3cmdBuf k3gfxObj::CreateCmdBuf()
{
    k3cmdBuf cmd = new k3cmdBufObj;
    k3cmdBufImpl* cmd_impl = cmd->getImpl();
    HRESULT hr;
    uint32_t n;
    for (n = 0; n < k3cmdBufImpl::MAX_ALLOC; n++) {
        hr = _data->_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&(cmd_impl->_cmd_alloc[n])));
        if (hr != S_OK) {
            k3error::Handler("Could not create command allocator", "k3gfxObj::CreateCmdBuf");
            return NULL;
        }
    }
    if (_data->_dev5) {
        // if we have a dx12.1 device, we cann create command lst 4, which is needed for DXR
        hr = _data->_dev5->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_impl->_cmd_alloc[0], NULL, IID_PPV_ARGS(&cmd_impl->_cmd_list4));
        cmd_impl->_cmd_list = cmd_impl->_cmd_list4;
    } else {
        hr = _data->_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_impl->_cmd_alloc[0], NULL, IID_PPV_ARGS(&cmd_impl->_cmd_list));
    }
    if (hr != S_OK) {
        k3error::Handler("Could not create command list", "k3gfxObj::CreateCmdBuf");
        return NULL;
    }
    cmd_impl->_fence = CreateFence();
    cmd_impl->_gfx = this;
    cmd->Close();

    return cmd;
}

K3API void k3gfxObj::SubmitCmdBuf(k3cmdBuf cmd)
{
    k3cmdBufImpl* cmd_impl = cmd->getImpl();
    ID3D12CommandList* cmd_lists[] = { cmd_impl->_cmd_list };
    _data->_cmd_q->ExecuteCommandLists(1, cmd_lists);
    cmd_impl->_cmd_alloc_fence[cmd_impl->_cur_alloc] = cmd_impl->_fence->SetGpuFence(k3gpuQueue::GRAPHICS);
}

K3API k3shaderBinding k3gfxObj::CreateShaderBinding(uint32_t num_params, k3bindingParam* params, uint32_t num_samplers, k3samplerDesc* samplers)
{
    return CreateTypedShaderBinding(num_params, params, num_samplers, samplers, k3shaderBindingType::GLOBAL);
}

K3API k3shaderBinding k3gfxObj::CreateTypedShaderBinding(uint32_t num_params, k3bindingParam* params, uint32_t num_samplers, k3samplerDesc* samplers, k3shaderBindingType sh_bind_type)
{
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc;
    D3D12_ROOT_PARAMETER1* dx12_params = (num_params) ? new D3D12_ROOT_PARAMETER1[num_params] : NULL;
    D3D12_STATIC_SAMPLER_DESC* dx12_samplers = (num_samplers) ? new D3D12_STATIC_SAMPLER_DESC[num_samplers] : NULL;
    desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    desc.Desc_1_1.NumParameters = num_params;
    desc.Desc_1_1.pParameters = dx12_params;
    desc.Desc_1_1.NumStaticSamplers = num_samplers;
    desc.Desc_1_1.pStaticSamplers = dx12_samplers;
    if (sh_bind_type == k3shaderBindingType::GLOBAL) {
        desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    } else {
        desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    }
    D3D12_DESCRIPTOR_RANGE1 dx12_desc_range[128];
    uint32_t r = 0;
    uint32_t i, j;
    for (i = 0; i < num_params; i++) {
        dx12_params[i].ParameterType = k3gfxImpl::ConvertToDx12RootParameterType(params[i].type);
        dx12_params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        switch (params[i].type) {
        case k3bindingType::CONSTANT:
            dx12_params[i].Constants.ShaderRegister = params[i].constant.reg;
            dx12_params[i].Constants.RegisterSpace = params[i].constant.space;
            dx12_params[i].Constants.Num32BitValues = params[i].constant.num_const;
            break;
        case k3bindingType::CBV:
        case k3bindingType::SRV:
        case k3bindingType::UAV:
            dx12_params[i].Descriptor.ShaderRegister = params[i].view_desc.reg;
            dx12_params[i].Descriptor.RegisterSpace = params[i].view_desc.space;
            dx12_params[i].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
            break;
        case k3bindingType::VIEW_SET:
            dx12_params[i].DescriptorTable.NumDescriptorRanges = 1;
            dx12_params[i].DescriptorTable.pDescriptorRanges = &dx12_desc_range[r];
            dx12_desc_range[r].NumDescriptors = params[i].view_set_desc.num_views;
            dx12_desc_range[r].RangeType = k3gfxImpl::ConvertToDx12ShaderBindType(params[i].view_set_desc.type);
            dx12_desc_range[r].BaseShaderRegister = params[i].view_set_desc.reg;
            dx12_desc_range[r].RegisterSpace = params[i].view_set_desc.space;
            dx12_desc_range[r].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;// params[i].view_set_desc.offset;
            dx12_desc_range[r].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            r++;
            break;
        case k3bindingType::VIEW_SET_TABLE:
            dx12_params[i].DescriptorTable.NumDescriptorRanges = params[i].view_set_table_desc.num_view_sets;
            dx12_params[i].DescriptorTable.pDescriptorRanges = &dx12_desc_range[r];
            for (j = 0; j < params[i].view_set_table_desc.num_view_sets; j++) {
                dx12_desc_range[r].NumDescriptors = params[i].view_set_table_desc.view_sets[j].num_views;
                dx12_desc_range[r].RangeType = k3gfxImpl::ConvertToDx12ShaderBindType(params[i].view_set_table_desc.view_sets[j].type);
                dx12_desc_range[r].BaseShaderRegister = params[i].view_set_table_desc.view_sets[j].reg;
                dx12_desc_range[r].RegisterSpace = params[i].view_set_table_desc.view_sets[j].space;
                dx12_desc_range[r].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;// params[i].view_set_table_desc.view_sets[j].offset;
                dx12_desc_range[r].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
                r++;
            }
            break;
        }
    }
    for (i = 0; i < num_samplers; i++) {
        dx12_samplers[i].Filter = k3gfxImpl::ConvertToDx12Fitler(samplers[i].filter);
        dx12_samplers[i].AddressU = k3gfxImpl::ConvertToDx12AddrMode(samplers[i].addr_u);
        dx12_samplers[i].AddressV = k3gfxImpl::ConvertToDx12AddrMode(samplers[i].addr_v);
        dx12_samplers[i].AddressW = k3gfxImpl::ConvertToDx12AddrMode(samplers[i].addr_w);
        dx12_samplers[i].MipLODBias = samplers[i].mip_load_bias;
        dx12_samplers[i].MaxAnisotropy = samplers[i].max_anisotropy;
        dx12_samplers[i].ComparisonFunc = k3gfxImpl::ConvertToDx12TestFunc(samplers[i].compare_func);
        dx12_samplers[i].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        dx12_samplers[i].MinLOD = samplers[i].min_lod;
        dx12_samplers[i].MaxLOD = samplers[i].max_lod;
        dx12_samplers[i].ShaderRegister = i;
        dx12_samplers[i].RegisterSpace = 0;
        dx12_samplers[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }
    ID3DBlob* serial_sig;
    ID3DBlob* error;
    HRESULT hr;
    hr = D3D12SerializeVersionedRootSignature(&desc, &serial_sig, &error);
    if(dx12_params) delete[] dx12_params;
    if (dx12_samplers) delete[] dx12_samplers;
    if (hr != S_OK) {
        k3error::Handler("Could not serialize root signature", "k3gfxObj::CreateShaderBinding");
        return NULL;
    }
    k3shaderBinding binding = new k3shaderBindingObj;
    k3shaderBindingImpl* binding_impl = binding->getImpl();
    hr = _data->_dev->CreateRootSignature(0, serial_sig->GetBufferPointer(), serial_sig->GetBufferSize(), IID_PPV_ARGS(&(binding_impl->_binding)));
    if (hr != S_OK) {
        k3error::Handler("Could not create root signature", "k3gfxObj::CreateShaderBinding");
        return NULL;
    }
    binding_impl->_type = sh_bind_type;
    return binding;
}

K3API k3shader k3gfxObj::CreateShaderFromCompiledFile(const char* file_name)
{
    FILE* fh;
    fopen_s(&fh, file_name, "rb");
    if (fh == NULL) {
        k3error::Handler("Could not load file", "k3gfxObj::CreateShaderFromCompiledFile");
        return NULL;
    }
    fseek(fh, 0, SEEK_END);
    uint32_t size = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    k3shader shader = new k3shaderObj;
    k3shaderImpl* shader_impl = shader->getImpl();
    void* byte_code = new uint8_t[size];
    fread(byte_code, 1, size, fh);
    shader_impl->_byte_code.pShaderBytecode = byte_code;
    shader_impl->_byte_code.BytecodeLength = size;
    fclose(fh);
    return shader;
}

K3API k3shader k3gfxObj::CompileShaderFromString(const char* code, k3shaderType shader_type)
{
    size_t code_len = strlen(code);
    const char* target = "";
    HRESULT hr;
    ID3DBlob* byte_code;
    ID3DBlob* error_code;
    switch (shader_type) {
    case k3shaderType::VERTEX_SHADER:   target = "vs_5_0"; break;
    case k3shaderType::PIXEL_SHADER:    target = "ps_5_0"; break;
    case k3shaderType::GEOMETRY_SHADER: target = "gs_5_0"; break;
    case k3shaderType::HULL_SHADER:     target = "hs_5_0"; break;
    case k3shaderType::DOMAIN_SHADER:   target = "ds_5_0"; break;
    case k3shaderType::LIBRARY:         target = "lib_6_3"; break;
    }
    hr = D3DCompile(code, code_len, NULL, NULL, NULL, "main", target, 0, 0, &byte_code, &error_code);
    if (hr != S_OK) {
        k3error::Handler((const char*)error_code->GetBufferPointer(), "k3gfxObj::CompileShaderFromString");
        if(byte_code) byte_code->Release();
        error_code->Release();
        return NULL;
    }
    k3shader shader = new k3shaderObj;
    k3shaderImpl* shader_impl = shader->getImpl();
    void* code_copy = new uint8_t[byte_code->GetBufferSize()];
    shader_impl->_byte_code.pShaderBytecode = code_copy;
    memcpy(code_copy, byte_code->GetBufferPointer(), byte_code->GetBufferSize());
    shader_impl->_byte_code.BytecodeLength = byte_code->GetBufferSize();
    byte_code->Release();
    if(error_code) error_code->Release();
    return shader;
}

K3API k3shader k3gfxObj::CompileShaderFromFile(const char* file_name, k3shaderType shader_type)
{
    FILE* file_handle;
    fopen_s(&file_handle, file_name, "rb");
    if (file_handle == NULL) {
        k3error::Handler("File not found", "k3gfxObj::CompileShaderFromFile");
        return NULL;
    }
    fseek(file_handle, 0, SEEK_END);
    uint32_t size = ftell(file_handle);
    fseek(file_handle, 0, SEEK_SET);
    char* code = new char[size + 1];
    fread(code, 1, size, file_handle);
    code[size] = '\0';
    fclose(file_handle);
    k3shader shader = CompileShaderFromString(code, shader_type);
    delete[] code;
    return shader;
}

K3API k3rtState k3gfxObj::CreateRTState(uint32_t num_elements, const k3rtStateDesc* desc)
{
    if (num_elements == 0) {
        k3error::Handler("No elements for RT state", "k3gfxObj::CreateRTState");
        return NULL;
    }

    uint32_t i, j;
    uint32_t num_shaders = 0;
    uint32_t num_shader_bindings = 0;
    D3D12_STATE_SUBOBJECT* subobj = new D3D12_STATE_SUBOBJECT[num_elements];

    D3D12_DXIL_LIBRARY_DESC* dx12_shader_desc;
    D3D12_HIT_GROUP_DESC* dx12_hit_group_desc;
    D3D12_LOCAL_ROOT_SIGNATURE* dx12_local_root_sig;
    D3D12_GLOBAL_ROOT_SIGNATURE* dx12_global_root_sig;
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* dx12_export_assoc;
    D3D12_RAYTRACING_SHADER_CONFIG* dx12_shader_config;
    D3D12_RAYTRACING_PIPELINE_CONFIG* dx12_pipe_config;
    for (i = 0; i < num_elements; i++) {
        switch (desc[i].type) {
        case k3rtStateType::SHADER:
            num_shaders++;
            dx12_shader_desc = new D3D12_DXIL_LIBRARY_DESC;
            dx12_shader_desc->DXILLibrary = desc[i].shader.obj->getImpl()->_byte_code;
            dx12_shader_desc->NumExports = desc[i].shader.num_entries;
            dx12_shader_desc->pExports = (dx12_shader_desc->NumExports) ? new D3D12_EXPORT_DESC[dx12_shader_desc->NumExports] : NULL;
            for (j = 0; j < dx12_shader_desc->NumExports; j++) {
                dx12_shader_desc->pExports[j].Name = createWideString(desc[i].shader.entries[j]);
                dx12_shader_desc->pExports[j].ExportToRename = NULL;
                dx12_shader_desc->pExports[j].Flags = D3D12_EXPORT_FLAG_NONE;
            }
            subobj[i].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            subobj[i].pDesc = dx12_shader_desc;
            break;
        case k3rtStateType::HIT_GROUP:
            dx12_hit_group_desc = new D3D12_HIT_GROUP_DESC;
            dx12_hit_group_desc->Type = k3gfxImpl::ConvertToDx12HitGroupType(desc[i].elem.hit_group.type);
            dx12_hit_group_desc->HitGroupExport = (desc[i].elem.hit_group.name) ? createWideString(desc[i].elem.hit_group.name) : NULL;
            dx12_hit_group_desc->AnyHitShaderImport = (desc[i].elem.hit_group.any_hit_shader) ? createWideString(desc[i].elem.hit_group.any_hit_shader) : NULL;
            dx12_hit_group_desc->ClosestHitShaderImport = (desc[i].elem.hit_group.closest_hit_shader) ? createWideString(desc[i].elem.hit_group.closest_hit_shader) : NULL;
            dx12_hit_group_desc->IntersectionShaderImport = (desc[i].elem.hit_group.intersection_shader) ? createWideString(desc[i].elem.hit_group.intersection_shader) : NULL;
            subobj[i].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            subobj[i].pDesc = dx12_hit_group_desc;
            break;
        case k3rtStateType::SHADER_BINDING:
            num_shader_bindings++;
            if (desc[i].shader_binding->getType() == k3shaderBindingType::LOCAL) {
                dx12_local_root_sig = new D3D12_LOCAL_ROOT_SIGNATURE;
                dx12_local_root_sig->pLocalRootSignature = desc[i].shader_binding->getImpl()->_binding;
                subobj[i].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
                subobj[i].pDesc = dx12_local_root_sig;
            } else {
                dx12_global_root_sig = new D3D12_GLOBAL_ROOT_SIGNATURE;
                dx12_global_root_sig->pGlobalRootSignature = desc[i].shader_binding->getImpl()->_binding;
                subobj[i].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
                subobj[i].pDesc = dx12_global_root_sig;
            }
            break;
        case k3rtStateType::EXPORT_ASSOCIATION:
            dx12_export_assoc = new D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            dx12_export_assoc->NumExports = desc[i].elem.export_association.num_exports;
            if (dx12_export_assoc->NumExports) {
                dx12_export_assoc->pExports = new const wchar_t* [dx12_export_assoc->NumExports];
                for (j = 0; j < dx12_export_assoc->NumExports; j++) {
                    dx12_export_assoc->pExports[j] = createWideString(desc[i].elem.export_association.export_names[j]);
                }
                dx12_export_assoc->pSubobjectToAssociate = &(subobj[desc[i].elem.export_association.association_index]);
            } else {
                dx12_export_assoc->pExports = NULL;
                dx12_export_assoc->pSubobjectToAssociate = NULL;
            }
            subobj[i].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            subobj[i].pDesc = dx12_export_assoc;
            break;
        case k3rtStateType::SHADER_CONFIG:
            dx12_shader_config = new D3D12_RAYTRACING_SHADER_CONFIG;
            dx12_shader_config->MaxAttributeSizeInBytes = desc[i].elem.shader_config.attrib_size;
            dx12_shader_config->MaxPayloadSizeInBytes = desc[i].elem.shader_config.payload_size;
            subobj[i].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            subobj[i].pDesc = dx12_shader_config;
            break;
        case k3rtStateType::PIPELINE_CONFIG:
            dx12_pipe_config = new D3D12_RAYTRACING_PIPELINE_CONFIG;
            dx12_pipe_config->MaxTraceRecursionDepth = desc[i].elem.pipeline_config.max_recursion;
            subobj[i].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
            subobj[i].pDesc = dx12_pipe_config;
            break;
        }
    }

    k3rtState rt_state = new k3rtStateObj;
    k3rtStateImpl* rt_state_impl = rt_state->getImpl();
    D3D12_STATE_OBJECT_DESC dx12_state_obj_desc = {};
    dx12_state_obj_desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    dx12_state_obj_desc.NumSubobjects = num_elements;
    dx12_state_obj_desc.pSubobjects = subobj;
    HRESULT hr = _data->_dev5->CreateStateObject(&dx12_state_obj_desc, IID_PPV_ARGS(&(rt_state_impl->_state)));
    // Do error checking after we free up all temporary memory

    uint32_t sh = 0, shb = 0;
    rt_state_impl->_shaders = (num_shaders) ? new k3shader[num_shaders] : NULL;
    rt_state_impl->_shader_bindings = (num_shader_bindings) ? new k3shaderBinding[num_shader_bindings] : NULL;
    for (i = 0; i < num_elements; i++) {
        switch (desc[i].type) {
        case k3rtStateType::SHADER:
            // Keep a reference to the shader
            rt_state_impl->_shaders[sh] = desc[i].shader.obj;
            sh++;
            dx12_shader_desc = (D3D12_DXIL_LIBRARY_DESC*)subobj[i].pDesc;
            if (dx12_shader_desc->pExports) {
                for (j = 0; j < dx12_shader_desc->NumExports; j++) {
                    delete[] dx12_shader_desc->pExports[j].Name;
                }
                delete[] dx12_shader_desc->pExports;
            }
            break;
        case k3rtStateType::HIT_GROUP:
            dx12_hit_group_desc = (D3D12_HIT_GROUP_DESC*)subobj[i].pDesc;
            if (dx12_hit_group_desc->HitGroupExport) delete[] dx12_hit_group_desc->HitGroupExport;
            if (dx12_hit_group_desc->AnyHitShaderImport) delete[] dx12_hit_group_desc->AnyHitShaderImport;
            if (dx12_hit_group_desc->ClosestHitShaderImport) delete[] dx12_hit_group_desc->ClosestHitShaderImport;
            if (dx12_hit_group_desc->IntersectionShaderImport) delete[] dx12_hit_group_desc->IntersectionShaderImport;
            break;
        case k3rtStateType::SHADER_BINDING:
            // Keep a reference to the shader bindings
            if (desc[i].shader_binding->getType() == k3shaderBindingType::GLOBAL) {
                // Put the global shader binding in the first slot
                rt_state_impl->_shader_bindings[shb] = rt_state_impl->_shader_bindings[0];
                rt_state_impl->_shader_bindings[0] = desc[i].shader_binding;
            } else {
                rt_state_impl->_shader_bindings[shb] = desc[i].shader_binding;
            }
            shb++;
            break;
        case k3rtStateType::EXPORT_ASSOCIATION:
            dx12_export_assoc = (D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*)subobj[i].pDesc;
            if (dx12_export_assoc->NumExports) {
                for (j = 0; j < dx12_export_assoc->NumExports; j++) {
                    delete[] dx12_export_assoc->pExports[j];
                }
                delete[] dx12_export_assoc->pExports;
            }
            break;
        }
        delete subobj[i].pDesc;
    }

    delete[] subobj;

    if (hr != S_OK) {
        k3error::Handler("Could not create RT State object", "k3gfxObj::CreateRTState");
        return NULL;
    }

    return rt_state;
}


K3API k3gfxState k3gfxObj::CreateGfxState(const k3gfxStateDesc* desc)
{
    uint32_t i;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC dx12_desc = { 0 };
    dx12_desc.pRootSignature = desc->shader_binding->getImpl()->_binding;
    k3shader shader;
    shader = desc->vertex_shader;   if(shader != NULL) dx12_desc.VS = shader->getImpl()->_byte_code;
    shader = desc->pixel_shader;    if(shader != NULL) dx12_desc.PS = shader->getImpl()->_byte_code;
    shader = desc->domain_shader;   if(shader != NULL) dx12_desc.DS = shader->getImpl()->_byte_code;
    shader = desc->hull_shader;     if(shader != NULL) dx12_desc.HS = shader->getImpl()->_byte_code;
    shader = desc->geometry_shader; if(shader != NULL) dx12_desc.GS = shader->getImpl()->_byte_code;
    dx12_desc.BlendState.AlphaToCoverageEnable = desc->blend_state.alpha_to_mask;
    dx12_desc.BlendState.IndependentBlendEnable = desc->blend_state.independent_blend;
    for (i = 0; i < K3_MAX_RENDER_TARGETS; i++) {
        dx12_desc.BlendState.RenderTarget[i].BlendEnable = desc->blend_state.blend_op[i].blend_enable;
        dx12_desc.BlendState.RenderTarget[i].LogicOpEnable = desc->blend_state.blend_op[i].rop_enable;
        dx12_desc.BlendState.RenderTarget[i].SrcBlend = k3gfxImpl::ConvertToDx12Blend(desc->blend_state.blend_op[i].src_blend);
        dx12_desc.BlendState.RenderTarget[i].DestBlend = k3gfxImpl::ConvertToDx12Blend(desc->blend_state.blend_op[i].dst_blend);
        dx12_desc.BlendState.RenderTarget[i].BlendOp = k3gfxImpl::ConvertToDx12BlendOp(desc->blend_state.blend_op[i].blend_op);
        dx12_desc.BlendState.RenderTarget[i].SrcBlendAlpha = k3gfxImpl::ConvertToDx12Blend(desc->blend_state.blend_op[i].alpha_src_blend);
        dx12_desc.BlendState.RenderTarget[i].DestBlendAlpha = k3gfxImpl::ConvertToDx12Blend(desc->blend_state.blend_op[i].alpha_dst_blend);
        dx12_desc.BlendState.RenderTarget[i].BlendOpAlpha = k3gfxImpl::ConvertToDx12BlendOp(desc->blend_state.blend_op[i].alpha_blend_op);
        dx12_desc.BlendState.RenderTarget[i].LogicOp = k3gfxImpl::ConvertToDx12Rop(desc->blend_state.blend_op[i].rop);
        dx12_desc.BlendState.RenderTarget[i].RenderTargetWriteMask = desc->blend_state.blend_op[i].rt_write_mask;
    }
    dx12_desc.SampleMask = desc->sample_mask;
    dx12_desc.RasterizerState.FillMode = k3gfxImpl::ConvertToDx12FillMode(desc->rast_state.fill_mode);
    dx12_desc.RasterizerState.CullMode = k3gfxImpl::ConvertToDx12CullMode(desc->rast_state.cull_mode);
    dx12_desc.RasterizerState.FrontCounterClockwise = desc->rast_state.front_counter_clockwise;
    dx12_desc.RasterizerState.DepthBias = desc->rast_state.depth_bias;
    dx12_desc.RasterizerState.DepthBiasClamp = desc->rast_state.depth_bias_clamp;
    dx12_desc.RasterizerState.SlopeScaledDepthBias = desc->rast_state.slope_scale_depth_bias;
    dx12_desc.RasterizerState.DepthClipEnable = desc->rast_state.depth_clip_enable;
    dx12_desc.RasterizerState.MultisampleEnable = desc->rast_state.msaa_enable;
    dx12_desc.RasterizerState.AntialiasedLineEnable = desc->rast_state.aa_line_enable;
    dx12_desc.RasterizerState.ForcedSampleCount = 0;
    dx12_desc.RasterizerState.ConservativeRaster = k3gfxImpl::ConvertToDx12ConservativeRast(desc->rast_state.conservative_rast_enable);
    dx12_desc.DepthStencilState.DepthEnable = desc->depth_state.depth_enable;
    dx12_desc.DepthStencilState.DepthWriteMask = k3gfxImpl::ConvertToDx12DepthWrite(desc->depth_state.depth_write_enable);
    dx12_desc.DepthStencilState.DepthFunc = k3gfxImpl::ConvertToDx12TestFunc(desc->depth_state.depth_test);
    dx12_desc.DepthStencilState.StencilEnable = desc->depth_state.stencil_enable;
    dx12_desc.DepthStencilState.StencilReadMask = desc->depth_state.stencil_read_mask;
    dx12_desc.DepthStencilState.StencilWriteMask = desc->depth_state.stencil_write_mask;
    dx12_desc.DepthStencilState.FrontFace.StencilFailOp = k3gfxImpl::ConvertToDx12StencilOp(desc->depth_state.front.fail_op);
    dx12_desc.DepthStencilState.FrontFace.StencilDepthFailOp = k3gfxImpl::ConvertToDx12StencilOp(desc->depth_state.front.z_fail_op);
    dx12_desc.DepthStencilState.FrontFace.StencilPassOp = k3gfxImpl::ConvertToDx12StencilOp(desc->depth_state.front.pass_op);
    dx12_desc.DepthStencilState.FrontFace.StencilFunc = k3gfxImpl::ConvertToDx12TestFunc(desc->depth_state.front.stencil_test);
    dx12_desc.DepthStencilState.BackFace.StencilFailOp = k3gfxImpl::ConvertToDx12StencilOp(desc->depth_state.back.fail_op);
    dx12_desc.DepthStencilState.BackFace.StencilDepthFailOp = k3gfxImpl::ConvertToDx12StencilOp(desc->depth_state.back.z_fail_op);
    dx12_desc.DepthStencilState.BackFace.StencilPassOp = k3gfxImpl::ConvertToDx12StencilOp(desc->depth_state.back.pass_op);
    dx12_desc.DepthStencilState.BackFace.StencilFunc = k3gfxImpl::ConvertToDx12TestFunc(desc->depth_state.back.stencil_test);
    dx12_desc.InputLayout.NumElements = desc->num_input_elements;
    if (desc->num_input_elements > 0) {
        D3D12_INPUT_ELEMENT_DESC* in_layout = new D3D12_INPUT_ELEMENT_DESC[desc->num_input_elements];
        for (i = 0; i < desc->num_input_elements; i++) {
            in_layout[i].SemanticName = desc->input_elements[i].name;
            in_layout[i].SemanticIndex = desc->input_elements[i].index;
            in_layout[i].Format = k3win32Dx12WinImpl::ConvertToDXGIFormat(desc->input_elements[i].format, k3DxgiSurfaceType::COLOR);
            in_layout[i].InputSlot = desc->input_elements[i].slot;
            in_layout[i].AlignedByteOffset = desc->input_elements[i].offset;
            in_layout[i].InputSlotClass = k3gfxImpl::ConvertToDx12InputType(desc->input_elements[i].in_type);
            in_layout[i].InstanceDataStepRate = desc->input_elements[i].instance_step;
        }
        dx12_desc.InputLayout.pInputElementDescs = in_layout;
    } else {
        dx12_desc.InputLayout.pInputElementDescs = NULL;
    }
    dx12_desc.IBStripCutValue = k3gfxImpl::ConvertToDx12StripCut(desc->cut_index);
    dx12_desc.PrimitiveTopologyType = k3gfxImpl::ConvertToDx12PrimType(desc->prim_type);
    dx12_desc.NumRenderTargets = desc->num_render_targets;
    for (i = 0; i < desc->num_render_targets; i++) {
        dx12_desc.RTVFormats[i] = k3win32Dx12WinImpl::ConvertToDXGIFormat(desc->rtv_format[i], k3DxgiSurfaceType::COLOR);
    }
    dx12_desc.DSVFormat = k3win32Dx12WinImpl::ConvertToDXGIFormat(desc->dsv_format, k3DxgiSurfaceType::DEPTH);
    dx12_desc.SampleDesc.Count = desc->msaa_samples;
    dx12_desc.SampleDesc.Quality = 0;

    HRESULT hr;
    k3gfxState state = new k3gfxStateObj;
    k3gfxStateImpl* state_impl = state->getImpl();
    hr = _data->_dev->CreateGraphicsPipelineState(&dx12_desc, IID_PPV_ARGS(&(state_impl->_state)));
    if (dx12_desc.InputLayout.pInputElementDescs) {
        delete[] dx12_desc.InputLayout.pInputElementDescs;
    }
    if (hr != S_OK) {
        k3error::Handler("Could not create gfx pipeline state", "k3gfxObj::CreateGfxState");
        return NULL;
    }
    state_impl->_binding = desc->shader_binding;
    return state;
}

K3API k3rtStateTable k3gfxObj::CreateRTStateTable(const k3rtStateTableDesc* desc)
{
    HRESULT hr;
    k3rtStateTable state_table = new k3rtStateTableObj;
    k3rtStateTableImpl* state_table_impl = state_table->getImpl();
    state_table_impl->_resource = new k3resourceObj;
    k3resourceImpl* resource_impl = state_table_impl->_resource->getImpl();
    state_table_impl->_num_args = desc->num_args;
    state_table_impl->_num_entries = desc->num_entries;
    uint32_t entry_size = state_table_impl->getEntrySize();
    uint32_t table_size = state_table_impl->_num_entries * entry_size;
    table_size = (table_size + 0xFF) & ~0xFF;  // pad it out to 256B

    D3D12_RESOURCE_DESC dx12_resource_desc = {};
    dx12_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    dx12_resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    dx12_resource_desc.Width = table_size;
    dx12_resource_desc.Height = 1;
    dx12_resource_desc.DepthOrArraySize = 1;
    dx12_resource_desc.MipLevels = 1;
    dx12_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    dx12_resource_desc.SampleDesc.Count = 1;
    dx12_resource_desc.SampleDesc.Quality = 0;
    dx12_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    dx12_resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    if (desc->mem_pool == NULL) {
        // no mem pool defined, so create a committed resource
        D3D12_HEAP_PROPERTIES dx12_heap_prop;
        dx12_heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
        dx12_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        dx12_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        dx12_heap_prop.CreationNodeMask = 0;
        dx12_heap_prop.VisibleNodeMask = 0;
        D3D12_HEAP_FLAGS dx12_heap_flags = D3D12_HEAP_FLAG_NONE;
        hr = _data->_dev->CreateCommittedResource(&dx12_heap_prop, dx12_heap_flags, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&resource_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create committed resource", "k3gfxObj::CreateRTStateTable");
            return NULL;
        }
    } else {
        // mem pool specified, so create placed resource
        const k3memPoolImpl* mem_pool_impl = desc->mem_pool->getImpl();
        hr = _data->_dev->CreatePlacedResource(mem_pool_impl->_heap, desc->mem_offset, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&resource_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create placed resource", "k3gfxObj::CreateRTStateTable");
            return NULL;
        }
        resource_impl->_pool = desc->mem_pool;
    }
    return state_table;
}


K3API k3memPool k3gfxObj::CreateMemPool(uint64_t size, k3memType mem_type, uint32_t flag)
{
    D3D12_HEAP_DESC heap_desc;
    heap_desc.Properties.Type = k3gfxImpl::ConvertToDx12MemType(mem_type);
    heap_desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_desc.Properties.CreationNodeMask = 0;
    heap_desc.Properties.VisibleNodeMask = 0;
    heap_desc.SizeInBytes = size;
    heap_desc.Alignment = (flag & K3_MEM_FLAG_MSAA) ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    heap_desc.Flags = D3D12_HEAP_FLAG_NONE;
    k3memPool mem_pool = new k3memPoolObj;
    k3memPoolImpl* mem_impl = mem_pool->getImpl();
    HRESULT hr = _data->_dev->CreateHeap(&heap_desc, IID_PPV_ARGS(&(mem_impl->_heap)));
    if (hr != S_OK) {
        k3error::Handler("Could not create heap", "k3gfxObj::CreateMemPool");
        return NULL;
    }
    mem_impl->_size = size;
    mem_impl->_type = mem_type;
    return mem_pool;
}

K3API k3uploadImage k3gfxObj::CreateUploadImage()
{
    k3uploadImage img = new k3uploadImageObj;
    k3uploadImageImpl* imgImpl = img->getUploadImageImpl();
    imgImpl->_dev = _data->_dev;
    return img;
}

K3API k3downloadImage k3gfxObj::CreateDownloadImage()
{
    k3downloadImage img = new k3downloadImageObj;
    k3downloadImageImpl* imgImpl = img->getDownloadImageImpl();
    imgImpl->_dev = _data->_dev;
    return img;
}

K3API k3uploadBuffer k3gfxObj::CreateUploadBuffer()
{
    k3uploadBuffer buf = new k3uploadBufferObj;
    k3uploadBufferImpl* buf_impl = buf->getImpl();
    buf_impl->_dev = _data->_dev;
    return buf;
}

K3API k3surf k3gfxObj::CreateSurface(k3resourceDesc* rdesc, k3viewDesc* rtv_desc, k3viewDesc* srv_desc, k3viewDesc* uav_desc)
{
    HRESULT hr;
    k3resource resource = new k3resourceObj;
    k3resourceImpl* resource_impl = resource->getImpl();
    bool is_array = false;
    bool is_cube = false;
    float clear_value = 0.0f;
    if (rtv_desc) {
        is_array = is_array || rtv_desc->is_array;
        is_cube = is_cube || rtv_desc->is_cube;
        clear_value = rtv_desc->clear_value;
    }
    if (srv_desc) {
        is_array = is_array || srv_desc->is_array;
        is_cube = is_cube || srv_desc->is_cube;
    }
    if (uav_desc) {
        is_array = is_array || uav_desc->is_array;
        is_cube = is_cube || uav_desc->is_cube;
    }
    bool is_depth = (rdesc->format == k3fmt::D16_UNORM || rdesc->format == k3fmt::D24X8_UNORM || rdesc->format == k3fmt::D24_UNORM_S8_UINT ||
        rdesc->format == k3fmt::D32_FLOAT || rdesc->format == k3fmt::D32_FLOAT_S8X24_UINT);
    D3D12_RESOURCE_DESC dx12_resource_desc = {};
    dx12_resource_desc.Dimension = (rdesc->depth > 1 && !is_array && !is_cube) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D :
        ((rdesc->height > 1) ? D3D12_RESOURCE_DIMENSION_TEXTURE2D : D3D12_RESOURCE_DIMENSION_TEXTURE1D);
    dx12_resource_desc.Alignment = (rdesc->num_samples > 1) ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    dx12_resource_desc.Width = rdesc->width;
    dx12_resource_desc.Height = rdesc->height;
    dx12_resource_desc.DepthOrArraySize = rdesc->depth;
    dx12_resource_desc.MipLevels = rdesc->mip_levels;
    dx12_resource_desc.Format = k3win32Dx12WinImpl::ConvertToDXGIFormat(rdesc->format, k3DxgiSurfaceType::DEPTH);
    dx12_resource_desc.SampleDesc.Count = rdesc->num_samples;
    dx12_resource_desc.SampleDesc.Quality = 0;
    dx12_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dx12_resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_CLEAR_VALUE dx12_clear;
    dx12_clear.Format = dx12_resource_desc.Format;
    dx12_clear.Color[0] = clear_value;
    dx12_clear.Color[1] = clear_value;
    dx12_clear.Color[2] = clear_value;
    dx12_clear.Color[3] = 1.0f;
    if (is_depth) {
        dx12_clear.DepthStencil.Depth = clear_value;
        dx12_clear.DepthStencil.Stencil = 0;
    }
    D3D12_CLEAR_VALUE* dx12_clear_ptr = NULL;
    if (rtv_desc) {
        dx12_resource_desc.Flags |= ((is_depth) ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        dx12_clear_ptr = &dx12_clear;
    }
    if (uav_desc) dx12_resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    else if (srv_desc == NULL) dx12_resource_desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    k3resourceState init_state = k3resourceState::COMMON;
    D3D12_RESOURCE_STATES dx_init_state = k3gfxImpl::ConvertToDx12ResourceState(init_state, is_depth);
    if (rdesc->mem_pool == NULL) {
        // no mem pool defined, so create a committed resource
        D3D12_HEAP_PROPERTIES dx12_heap_prop;
        dx12_heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
        dx12_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        dx12_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        dx12_heap_prop.CreationNodeMask = 0;
        dx12_heap_prop.VisibleNodeMask = 0;
        D3D12_HEAP_FLAGS dx12_heap_flags = D3D12_HEAP_FLAG_NONE;
        hr = _data->_dev->CreateCommittedResource(&dx12_heap_prop, dx12_heap_flags, &dx12_resource_desc, dx_init_state, dx12_clear_ptr, IID_PPV_ARGS(&resource_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create committed resource", "k3gfxObj::CreateSurface");
            return NULL;
        }
    } else {
        // mem pool specified, so create placed resource
        const k3memPoolImpl* mem_pool_impl = rdesc->mem_pool->getImpl();
        hr = _data->_dev->CreatePlacedResource(mem_pool_impl->_heap, rdesc->mem_offset, &dx12_resource_desc, dx_init_state, dx12_clear_ptr, IID_PPV_ARGS(&resource_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create placed resource", "k3gfxObj::CreateSurface");
            return NULL;
        }
        resource_impl->_pool = rdesc->mem_pool;
    }
    resource_impl->_width = rdesc->width;
    resource_impl->_height = rdesc->height;
    resource_impl->_depth = rdesc->depth;
    resource_impl->_format = rdesc->format;
    resource_impl->_max_mip = rdesc->mip_levels;
    resource_impl->_resource_state = init_state;
    resource_impl->_samples = rdesc->num_samples;

    k3surf surf = CreateSurfaceAlias(resource, rtv_desc, srv_desc, uav_desc);
    return surf;
}

K3API k3surf k3gfxObj::CreateSurfaceAlias(k3resource resource, k3viewDesc* rtv_desc, k3viewDesc* srv_desc, k3viewDesc* uav_desc)
{
    k3surf surf = new k3surfObj;
    k3surfImpl* surf_impl = surf->getImpl();
    surf_impl->_resource = resource;
    k3resourceImpl* resource_impl = resource->getImpl();
    bool is_depth = (resource_impl->_format == k3fmt::D16_UNORM || resource_impl->_format == k3fmt::D24X8_UNORM || resource_impl->_format == k3fmt::D24_UNORM_S8_UINT ||
        resource_impl->_format == k3fmt::D32_FLOAT || resource_impl->_format == k3fmt::D32_FLOAT_S8X24_UINT);
    if (rtv_desc) {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = (is_depth) ? D3D12_DESCRIPTOR_HEAP_TYPE_DSV : D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = 1;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HRESULT hr = _data->_dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&surf_impl->_rtv_heap));
        if (hr != S_OK) {
            k3error::Handler("Could not create descriptor heap", "k3gfxObj::CreateSurfaceAlias");
            return NULL;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE dx12_desc_handle(surf_impl->_rtv_heap->GetCPUDescriptorHandleForHeapStart());
        uint32_t max_array_size = resource_impl->_depth - rtv_desc->array_start;
        uint32_t array_size = (rtv_desc->array_size > max_array_size) ? max_array_size : rtv_desc->array_size;
        if (is_depth) {
            D3D12_DEPTH_STENCIL_VIEW_DESC dx12_dsv_desc = { };
            dx12_dsv_desc.Format = k3win32Dx12WinImpl::ConvertToDXGIFormat(resource_impl->_format, k3DxgiSurfaceType::DEPTH);
            if (resource_impl->_samples > 1) {
                if (resource_impl->_depth > 1) {
                    dx12_dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    dx12_dsv_desc.Texture2DMSArray.FirstArraySlice = rtv_desc->array_start;
                    dx12_dsv_desc.Texture2DMSArray.ArraySize = array_size;
                } else {
                    dx12_dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
                }
            } else {
                if (resource_impl->_depth > 1) {
                    dx12_dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                    dx12_dsv_desc.Texture2DArray.FirstArraySlice = rtv_desc->array_start;
                    dx12_dsv_desc.Texture2DArray.ArraySize = array_size;
                    dx12_dsv_desc.Texture2DArray.MipSlice = rtv_desc->mip_start;
                } else if (resource_impl->_height > 1) {
                    dx12_dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                    dx12_dsv_desc.Texture2D.MipSlice = rtv_desc->mip_start;
                } else {
                    dx12_dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
                    dx12_dsv_desc.Texture1D.MipSlice = rtv_desc->mip_start;
                }
            }
            switch (rtv_desc->read_only) {
            case k3depthSelect::NONE: dx12_dsv_desc.Flags = D3D12_DSV_FLAG_NONE; break;
            case k3depthSelect::DEPTH: dx12_dsv_desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH; break;
            case k3depthSelect::STENCIL: dx12_dsv_desc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL; break;
            case k3depthSelect::DEPTH_STENCIL: dx12_dsv_desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL; break;
            }
            _data->_dev->CreateDepthStencilView(resource_impl->_dx12_resource, &dx12_dsv_desc, dx12_desc_handle);
            surf_impl->_rtv_cpu_view = dx12_desc_handle;
        } else {
            D3D12_RENDER_TARGET_VIEW_DESC dx12_rtv_desc = { };
            dx12_rtv_desc.Format = k3win32Dx12WinImpl::ConvertToDXGIFormat(resource_impl->_format, k3DxgiSurfaceType::TYPELESS);
            if (resource_impl->_samples > 1) {
                if (resource_impl->_depth > 1) {
                    dx12_rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    dx12_rtv_desc.Texture2DMSArray.FirstArraySlice = rtv_desc->array_start;
                    dx12_rtv_desc.Texture2DMSArray.ArraySize = array_size;
                } else {
                    dx12_rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                }
            } else {
                if (resource_impl->_depth > 1) {
                    dx12_rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                    dx12_rtv_desc.Texture2DArray.FirstArraySlice = rtv_desc->array_start;
                    dx12_rtv_desc.Texture2DArray.ArraySize = array_size;
                    dx12_rtv_desc.Texture2DArray.MipSlice = rtv_desc->mip_start;
                    dx12_rtv_desc.Texture2DArray.PlaneSlice = 0;
                } else if (resource_impl->_height > 1) {
                    dx12_rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                    dx12_rtv_desc.Texture2D.MipSlice = rtv_desc->mip_start;
                    dx12_rtv_desc.Texture2D.PlaneSlice = 0;
                } else {
                    dx12_rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                    dx12_rtv_desc.Texture1D.MipSlice = rtv_desc->mip_start;
                }
            }
            _data->_dev->CreateRenderTargetView(resource_impl->_dx12_resource, &dx12_rtv_desc, dx12_desc_handle);
            surf_impl->_rtv_cpu_view = dx12_desc_handle;
        }
    }
    UINT desc_size = _data->_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (srv_desc) {
        D3D12_CPU_DESCRIPTOR_HANDLE dx12_desc_handle(_data->_shader_heap[0]->GetCPUDescriptorHandleForHeapStart());
        D3D12_GPU_DESCRIPTOR_HANDLE dx12_gpu_desc_handle(_data->_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
        dx12_desc_handle.ptr += (srv_desc->view_index * desc_size);
        dx12_gpu_desc_handle.ptr += (srv_desc->view_index * desc_size);
        uint32_t max_array_size = resource_impl->_depth - srv_desc->array_start;
        uint32_t array_size = (srv_desc->array_size > max_array_size) ? max_array_size : srv_desc->array_size;
        D3D12_SHADER_RESOURCE_VIEW_DESC dx12_srv_desc = { };
        dx12_srv_desc.Format = k3win32Dx12WinImpl::ConvertToDXGIFormat(resource_impl->_format, k3DxgiSurfaceType::TYPELESS);
        dx12_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        if (resource_impl->_samples > 1) {
            if (resource_impl->_depth > 1) {
                dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                dx12_srv_desc.Texture2DMSArray.FirstArraySlice = srv_desc->array_start;
                dx12_srv_desc.Texture2DMSArray.ArraySize = array_size;
            } else {
                dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
        } else {
            if (srv_desc->is_cube) {
                if (resource_impl->_depth > 6) {
                    dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                    dx12_srv_desc.TextureCubeArray.MostDetailedMip = srv_desc->mip_start;
                    dx12_srv_desc.TextureCubeArray.MipLevels = -1;
                    dx12_srv_desc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
                    dx12_srv_desc.TextureCubeArray.First2DArrayFace = srv_desc->array_start;
                    dx12_srv_desc.TextureCubeArray.NumCubes = array_size / 6;
                } else {
                    dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    dx12_srv_desc.TextureCube.MostDetailedMip = srv_desc->mip_start;
                    dx12_srv_desc.TextureCube.MipLevels = -1;
                    dx12_srv_desc.TextureCube.ResourceMinLODClamp = 0.0f;
                }
            } else {
                if (resource_impl->_depth > 1) {
                    if (srv_desc->is_array) {
                        dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        dx12_srv_desc.Texture2DArray.MostDetailedMip = srv_desc->mip_start;
                        dx12_srv_desc.Texture2DArray.MipLevels = -1;
                        dx12_srv_desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
                        dx12_srv_desc.Texture2DArray.PlaneSlice = 0;
                        dx12_srv_desc.Texture2DArray.FirstArraySlice = srv_desc->array_start;
                        dx12_srv_desc.Texture2DArray.ArraySize = array_size;
                    } else {
                        dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                        dx12_srv_desc.Texture3D.MostDetailedMip = srv_desc->mip_start;
                        dx12_srv_desc.Texture3D.MipLevels = -1;
                        dx12_srv_desc.Texture3D.ResourceMinLODClamp = 0.0f;
                    }
                } else if (resource_impl->_height > 1) {
                    if (srv_desc->is_array) {
                        dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                        dx12_srv_desc.Texture1DArray.MostDetailedMip = srv_desc->mip_start;
                        dx12_srv_desc.Texture1DArray.MipLevels = -1;
                        dx12_srv_desc.Texture1DArray.ResourceMinLODClamp = 0.0f;
                        dx12_srv_desc.Texture1DArray.FirstArraySlice = srv_desc->array_start;
                        dx12_srv_desc.Texture1DArray.ArraySize = array_size;
                    } else {
                        dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        dx12_srv_desc.Texture2D.MostDetailedMip = srv_desc->mip_start;
                        dx12_srv_desc.Texture2D.MipLevels = -1;
                        dx12_srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
                        dx12_srv_desc.Texture2D.PlaneSlice = 0;
                    }
                } else {
                    dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                    dx12_srv_desc.Texture1D.MostDetailedMip = srv_desc->mip_start;
                    dx12_srv_desc.Texture1D.MipLevels = -1;
                    dx12_srv_desc.Texture1D.ResourceMinLODClamp = 0.0f;
                }
            }
        }
        _data->_dev->CreateShaderResourceView(resource_impl->_dx12_resource, &dx12_srv_desc, dx12_desc_handle);
        surf_impl->_srv_view_index = srv_desc->view_index;
        surf_impl->_srv_cpu_view = dx12_desc_handle;
        surf_impl->_srv_gpu_view = dx12_gpu_desc_handle;
    }
    if (uav_desc) {
        D3D12_CPU_DESCRIPTOR_HANDLE dx12_desc_handle(_data->_shader_heap[0]->GetCPUDescriptorHandleForHeapStart());
        D3D12_GPU_DESCRIPTOR_HANDLE dx12_gpu_desc_handle(_data->_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
        dx12_desc_handle.ptr += (uav_desc->view_index * desc_size);
        dx12_gpu_desc_handle.ptr += (uav_desc->view_index * desc_size);
        uint32_t max_array_size = resource_impl->_depth - uav_desc->array_start;
        uint32_t array_size = (uav_desc->array_size > max_array_size) ? max_array_size : uav_desc->array_size;
        D3D12_UNORDERED_ACCESS_VIEW_DESC dx12_uav_desc = {};
        dx12_uav_desc.Format = k3win32Dx12WinImpl::ConvertToDXGIFormat(resource_impl->_format, k3DxgiSurfaceType::TYPELESS);
        if (resource_impl->_depth > 1 && !uav_desc->is_array && !uav_desc->is_cube) {
            dx12_uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            dx12_uav_desc.Texture3D.MipSlice = uav_desc->mip_start;
            dx12_uav_desc.Texture3D.FirstWSlice = uav_desc->array_start;
            dx12_uav_desc.Texture3D.WSize = array_size;
        } else {
            if (resource_impl->_depth > 1) {
                dx12_uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                dx12_uav_desc.Texture2DArray.MipSlice = uav_desc->mip_start;
                dx12_uav_desc.Texture2DArray.FirstArraySlice = uav_desc->array_start;
                dx12_uav_desc.Texture2DArray.ArraySize = array_size;
                dx12_uav_desc.Texture2DArray.PlaneSlice = 0;
            } else {
                if (resource_impl->_height > 1) {
                    dx12_uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    dx12_uav_desc.Texture2D.MipSlice = uav_desc->mip_start;
                    dx12_uav_desc.Texture2D.PlaneSlice = 0;
                } else {
                    dx12_uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                    dx12_uav_desc.Texture1D.MipSlice = uav_desc->mip_start;
                }
            }
        }
        _data->_dev->CreateUnorderedAccessView(resource_impl->_dx12_resource, NULL, &dx12_uav_desc, dx12_desc_handle);
        surf_impl->_uav_view_index = uav_desc->view_index;
        surf_impl->_uav_cpu_view = dx12_desc_handle;
        surf_impl->_uav_gpu_view = dx12_gpu_desc_handle;
    }
    return surf;
}

K3API k3sampler k3gfxObj::CreateSampler(const k3samplerDesc* sdesc)
{
    k3sampler sampler = new k3samplerObj;
    k3samplerImpl* sampler_impl = sampler->getImpl();
    D3D12_DESCRIPTOR_HEAP_TYPE dx12_heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    ID3D12DescriptorHeap* dx12_desc_heap = _data->_shader_heap[1];
    if (dx12_desc_heap == NULL) {
        k3error::Handler("Sampler heap not created", "k3gfxObj::CreateSampler");
        return NULL;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE dx12_desc_handle(dx12_desc_heap->GetCPUDescriptorHandleForHeapStart());
    D3D12_GPU_DESCRIPTOR_HANDLE dx12_gpu_desc_handle(dx12_desc_heap->GetGPUDescriptorHandleForHeapStart());
    UINT desc_size = _data->_dev->GetDescriptorHandleIncrementSize(dx12_heap_type);
    dx12_desc_handle.ptr += sdesc->sampler_index * desc_size;
    dx12_gpu_desc_handle.ptr += sdesc->sampler_index * desc_size;
    D3D12_SAMPLER_DESC dx12_sdesc = {};
    dx12_sdesc.Filter = k3gfxImpl::ConvertToDx12Fitler(sdesc->filter);
    dx12_sdesc.AddressU = k3gfxImpl::ConvertToDx12AddrMode(sdesc->addr_u);
    dx12_sdesc.AddressV = k3gfxImpl::ConvertToDx12AddrMode(sdesc->addr_v);
    dx12_sdesc.AddressW = k3gfxImpl::ConvertToDx12AddrMode(sdesc->addr_w);
    dx12_sdesc.ComparisonFunc = k3gfxImpl::ConvertToDx12TestFunc(sdesc->compare_func);
    dx12_sdesc.BorderColor[0] = sdesc->border_color[0];
    dx12_sdesc.BorderColor[1] = sdesc->border_color[1];
    dx12_sdesc.BorderColor[2] = sdesc->border_color[2];
    dx12_sdesc.BorderColor[3] = sdesc->border_color[3];
    dx12_sdesc.MaxAnisotropy = sdesc->max_anisotropy;
    dx12_sdesc.MipLODBias = sdesc->mip_load_bias;
    dx12_sdesc.MinLOD = sdesc->min_lod;
    dx12_sdesc.MaxLOD = sdesc->max_lod;
    _data->_dev->CreateSampler(&dx12_sdesc, dx12_desc_handle);
    sampler_impl->_view_index = sdesc->sampler_index;
    sampler_impl->_cpu_view = dx12_desc_handle;
    sampler_impl->_gpu_view = dx12_gpu_desc_handle;
    return sampler;
}

uint32_t pad256(uint32_t size)
{
    uint32_t mask = ~((uint32_t)0xFF);
    return (size + 0xFF) & mask;
}

uint64_t pad256(uint64_t size)
{
    uint64_t mask = ~((uint64_t)0xFF);
    return (size + 0xFF) & mask;
}

K3API k3buffer k3gfxObj::CreateBuffer(const k3bufferDesc* bdesc)
{
    HRESULT hr;
    k3buffer buffer = new k3bufferObj;
    k3bufferImpl* buffer_impl = buffer->getImpl();
    buffer_impl->_resource = new k3resourceObj;
    k3resourceImpl* resource_impl = buffer_impl->_resource->getImpl();
    uint32_t padded_size = bdesc->size;
    bool constant_buffer = (bdesc->format == k3fmt::UNKNOWN && bdesc->stride == 0);
    bool shader_visible = constant_buffer || bdesc->shader_resource;
    if (shader_visible) padded_size = pad256(padded_size);
    D3D12_RESOURCE_DESC dx12_resource_desc = {};
    dx12_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    dx12_resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    dx12_resource_desc.Width = padded_size;
    dx12_resource_desc.Height = 1;
    dx12_resource_desc.DepthOrArraySize = 1;
    dx12_resource_desc.MipLevels = 1;
    dx12_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    dx12_resource_desc.SampleDesc.Count = 1;
    dx12_resource_desc.SampleDesc.Quality = 0;
    dx12_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    dx12_resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (bdesc->mem_pool == NULL) {
        // no mem pool defined, so create a committed resource
        D3D12_HEAP_PROPERTIES dx12_heap_prop;
        dx12_heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
        dx12_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        dx12_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        dx12_heap_prop.CreationNodeMask = 0;
        dx12_heap_prop.VisibleNodeMask = 0;
        D3D12_HEAP_FLAGS dx12_heap_flags = D3D12_HEAP_FLAG_NONE;
        hr = _data->_dev->CreateCommittedResource(&dx12_heap_prop, dx12_heap_flags, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&resource_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create committed resource", "k3gfxObj::CreateSurface");
            return NULL;
        }
    } else {
        // mem pool specified, so create placed resource
        const k3memPoolImpl* mem_pool_impl = bdesc->mem_pool->getImpl();
        hr = _data->_dev->CreatePlacedResource(mem_pool_impl->_heap, bdesc->mem_offset, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&resource_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create placed resource", "k3gfxObj::CreateSurface");
            return NULL;
        }
        resource_impl->_pool = bdesc->mem_pool;
    }
    resource_impl->_width = padded_size;
    resource_impl->_height = 1;
    resource_impl->_depth = 1;
    resource_impl->_format = bdesc->format;
    resource_impl->_max_mip = 1;
    resource_impl->_resource_state = k3resourceState::COMMON;
    resource_impl->_samples = 1;
    if (shader_visible) {
        // create constant buffer view
        UINT desc_size = _data->_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE cbv_handle(_data->_shader_heap[0]->GetCPUDescriptorHandleForHeapStart());
        D3D12_GPU_DESCRIPTOR_HANDLE cbv_gpu_handle(_data->_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
        cbv_handle.ptr += bdesc->view_index * desc_size;
        cbv_gpu_handle.ptr += bdesc->view_index * desc_size;
        if (constant_buffer) {
            D3D12_CONSTANT_BUFFER_VIEW_DESC dx12_cbv_desc = {};
            dx12_cbv_desc.BufferLocation = resource_impl->_dx12_resource->GetGPUVirtualAddress();
            dx12_cbv_desc.SizeInBytes = padded_size;
            _data->_dev->CreateConstantBufferView(&dx12_cbv_desc, cbv_handle);
            buffer_impl->_view_index = bdesc->view_index;
            buffer_impl->_cpu_view = cbv_handle;
            buffer_impl->_gpu_view = cbv_gpu_handle;
        } else {
            uint32_t stride = (bdesc->stride) ? bdesc->stride : 1; // todo: make this a fuction of format size
            D3D12_SHADER_RESOURCE_VIEW_DESC dx12_srv_desc = {};
            dx12_srv_desc.Format = DXGI_FORMAT_UNKNOWN;
            dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            dx12_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            dx12_srv_desc.Buffer.NumElements = bdesc->size / stride;
            dx12_srv_desc.Buffer.StructureByteStride = stride;
            dx12_srv_desc.Buffer.FirstElement = 0;
            dx12_srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            _data->_dev->CreateShaderResourceView(resource_impl->_dx12_resource, &dx12_srv_desc, cbv_handle);
            buffer_impl->_view_index = bdesc->view_index;
            buffer_impl->_cpu_view = cbv_handle;
            buffer_impl->_gpu_view = cbv_gpu_handle;
        }
    }
    buffer_impl->_stride = bdesc->stride;
    return buffer;
}

K3API k3blas k3gfxObj::CreateBlas(const k3blasCreateDesc* bldesc)
{
    if (bldesc->vb == NULL) {
        k3error::Handler("No vertex buffer for BLAS creation", "k3gfxObj::CreateBlas");
        return NULL;
    }
    const k3bufferImpl* vb_impl = bldesc->vb->getImpl();
    const k3resourceImpl* vb_res_impl = vb_impl->_resource->getImpl();
    switch (vb_res_impl->_format) {
    // legal formats
    case k3fmt::RG32_FLOAT:
    case k3fmt::RGB32_FLOAT:
    case k3fmt::RG16_FLOAT:
    case k3fmt::RGBA16_FLOAT:
    // TODO: add format support for SNORM
    //case k3fmt::RG16_SNORM:
    //case k3fmt::RGBA16_SNORM:
        break;
    case k3fmt::RG16_UNORM:
    case k3fmt::RGBA16_UNORM:
    case k3fmt::RGB10A2_UNORM:
    case k3fmt::RG8_UNORM:
    case k3fmt::RGBA8_UNORM:
    // TODO: add format support for SNORM
    //case k3fmt::RG8_SNORM:
    //case k3fmt::RGBA8_SNORM:
        if (_data->_dxr_tier >= 11) break;
    default:
        k3error::Handler("Illegal vertex format", "k3gfxObj::CreateBlas");
        return NULL;
    }

    HRESULT hr;
    k3blas blas = new k3blasObj;
    k3blasImpl* blas_impl = blas->getImpl();

    D3D12_RAYTRACING_GEOMETRY_DESC* geom = &blas_impl->_geom;
    geom->Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geom->Triangles.VertexBuffer.StartAddress = vb_res_impl->_dx12_resource->GetGPUVirtualAddress();
    geom->Triangles.VertexBuffer.StrideInBytes = vb_impl->_stride;
    geom->Triangles.VertexCount = (uint32_t)(vb_res_impl->_width / vb_impl->_stride);
    geom->Triangles.VertexFormat = k3win32Dx12WinImpl::ConvertToDXGIFormat(vb_res_impl->_format, k3DxgiSurfaceType::COLOR);
    if (bldesc->ib != NULL) {
        const k3bufferImpl* ib_impl = bldesc->ib->getImpl();
        const k3resourceImpl* ib_res_impl = ib_impl->_resource->getImpl();
        uint32_t index_stride = ((ib_res_impl->_format == k3fmt::R16_UINT) ? 2 : 4);
        geom->Triangles.IndexBuffer = ib_res_impl->_dx12_resource->GetGPUVirtualAddress() + 3 * index_stride * bldesc->start_prim;
        geom->Triangles.IndexFormat = k3win32Dx12WinImpl::ConvertToDXGIFormat(ib_res_impl->_format, k3DxgiSurfaceType::COLOR);
        if (bldesc->num_prims) {
            geom->Triangles.IndexCount = 3 * bldesc->num_prims;
        } else {
            geom->Triangles.IndexCount = (uint32_t)(ib_res_impl->_width / index_stride);
        }
    } else {
        geom->Triangles.IndexBuffer = NULL;
        geom->Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
        geom->Triangles.IndexCount = 0;
        geom->Triangles.VertexBuffer.StartAddress += 3 * vb_impl->_stride * bldesc->start_prim;
        if (bldesc->num_prims) {
            geom->Triangles.VertexCount = 3 * bldesc->num_prims;
        }
    }
    // TODO: add support for transform
    geom->Triangles.Transform3x4 = 0;
    // TODO: add support for other kinds of geometry (transparent, etc)
    geom->Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    blas_impl->_ib = bldesc->ib;
    blas_impl->_vb = bldesc->vb;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS as_inputs = {};
    as_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    as_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    as_inputs.pGeometryDescs = geom;
    as_inputs.NumDescs = 1;
    as_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO as_build_info = {};
    _data->_dev5->GetRaytracingAccelerationStructurePrebuildInfo(&as_inputs, &as_build_info);
    blas_impl->_size.rtas_size = pad256(as_build_info.ResultDataMaxSizeInBytes);
    blas_impl->_size.create_size = pad256(as_build_info.ScratchDataSizeInBytes);
    blas_impl->_size.update_size = pad256(as_build_info.UpdateScratchDataSizeInBytes);

    // These need to be allocated in a separate call AllocBlas
    // if alloc falg is false
    // this is needed to create as a placed resource instead of a committed one
    blas_impl->_blas = NULL;
    blas_impl->_create_scratch = NULL;
    blas_impl->_update_scratch = NULL;

    if (bldesc->alloc) {
        // Allocates data structures in committed memory
        blas_impl->_blas = new k3resourceObj;
        k3resourceImpl* res_impl = blas_impl->_blas->getImpl();

        D3D12_RESOURCE_DESC dx12_resource_desc = {};
        dx12_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        dx12_resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        dx12_resource_desc.Width = blas_impl->_size.rtas_size;
        dx12_resource_desc.Height = 1;
        dx12_resource_desc.DepthOrArraySize = 1;
        dx12_resource_desc.MipLevels = 1;
        dx12_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        dx12_resource_desc.SampleDesc.Count = 1;
        dx12_resource_desc.SampleDesc.Quality = 0;
        dx12_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        dx12_resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES dx12_heap_prop;
        dx12_heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
        dx12_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        dx12_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        dx12_heap_prop.CreationNodeMask = 0;
        dx12_heap_prop.VisibleNodeMask = 0;
        D3D12_HEAP_FLAGS dx12_heap_flags = D3D12_HEAP_FLAG_NONE;
        hr = _data->_dev->CreateCommittedResource(&dx12_heap_prop, dx12_heap_flags, &dx12_resource_desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create BLAS resource", "k3gfxObj::CreateBlas");
            return NULL;
        }
        res_impl->_width = blas_impl->_size.rtas_size;
        res_impl->_height = 1;
        res_impl->_depth = 1;
        res_impl->_format = vb_res_impl->_format;
        res_impl->_max_mip = 1;
        res_impl->_resource_state = k3resourceState::RT_ACCEL_STRUCT;
        res_impl->_samples = 1;

        if (blas_impl->_size.create_size) {
            blas_impl->_create_scratch = new k3resourceObj;
            res_impl = blas_impl->_create_scratch->getImpl();
            dx12_resource_desc.Width = blas_impl->_size.create_size;
            hr = _data->_dev->CreateCommittedResource(&dx12_heap_prop, dx12_heap_flags, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
            if (hr != S_OK) {
                k3error::Handler("Could not create create_scatch resource", "k3gfxObj::CreateBlas");
                return NULL;
            }
            res_impl->_width = blas_impl->_size.create_size;
            res_impl->_height = 1;
            res_impl->_depth = 1;
            res_impl->_format = k3fmt::UNKNOWN;
            res_impl->_max_mip = 1;
            res_impl->_resource_state = k3resourceState::COMMON;
            res_impl->_samples = 1;
        }

        if (blas_impl->_size.update_size) {
            blas_impl->_update_scratch = new k3resourceObj;
            res_impl = blas_impl->_update_scratch->getImpl();
            dx12_resource_desc.Width = blas_impl->_size.update_size;
            hr = _data->_dev->CreateCommittedResource(&dx12_heap_prop, dx12_heap_flags, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
            if (hr != S_OK) {
                k3error::Handler("Could not create update_scratch resource", "k3gfxObj::CreateBlas");
                return NULL;
            }
            res_impl->_width = blas_impl->_size.update_size;
            res_impl->_height = 1;
            res_impl->_depth = 1;
            res_impl->_format = k3fmt::UNKNOWN;
            res_impl->_max_mip = 1;
            res_impl->_resource_state = k3resourceState::COMMON;
            res_impl->_samples = 1;
        }
    }

    return blas;
}

K3API void k3gfxObj::AllocBlas(k3blas bl, const k3rtasAllocDesc* rtadesc)
{
    HRESULT hr;
    k3blasImpl* blas_impl = bl->getImpl();
    const k3bufferImpl* vb_impl = blas_impl->_vb->getImpl();
    const k3resourceImpl* vb_res_impl = vb_impl->_resource->getImpl();

    // Allocates data structures in placed memory
    blas_impl->_blas = new k3resourceObj;
    k3resourceImpl* res_impl = blas_impl->_blas->getImpl();

    D3D12_RESOURCE_DESC dx12_resource_desc = {};
    dx12_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    dx12_resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    dx12_resource_desc.Width = blas_impl->_size.rtas_size;
    dx12_resource_desc.Height = 1;
    dx12_resource_desc.DepthOrArraySize = 1;
    dx12_resource_desc.MipLevels = 1;
    dx12_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    dx12_resource_desc.SampleDesc.Count = 1;
    dx12_resource_desc.SampleDesc.Quality = 0;
    dx12_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    dx12_resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hr = _data->_dev->CreatePlacedResource(rtadesc->rtas_mem_pool->getImpl()->_heap, rtadesc->rtas_mem_offset, &dx12_resource_desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
    if (hr != S_OK) {
        k3error::Handler("Could not allocate BLAS resource", "k3gfxObj::AllocBlas");
        return;
    }
    res_impl->_width = blas_impl->_size.rtas_size;
    res_impl->_height = 1;
    res_impl->_depth = 1;
    res_impl->_format = vb_res_impl->_format;
    res_impl->_max_mip = 1;
    res_impl->_resource_state = k3resourceState::RT_ACCEL_STRUCT;
    res_impl->_samples = 1;

    if (blas_impl->_size.create_size) {
        blas_impl->_create_scratch = new k3resourceObj;
        res_impl = blas_impl->_create_scratch->getImpl();
        dx12_resource_desc.Width = blas_impl->_size.create_size;
        hr = _data->_dev->CreatePlacedResource(rtadesc->create_mem_pool->getImpl()->_heap, rtadesc->create_mem_offset, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not allocate create_scatch resource", "k3gfxObj::AllocBlas");
            return;
        }
        res_impl->_width = blas_impl->_size.create_size;
        res_impl->_height = 1;
        res_impl->_depth = 1;
        res_impl->_format = k3fmt::UNKNOWN;
        res_impl->_max_mip = 1;
        res_impl->_resource_state = k3resourceState::COMMON;
        res_impl->_samples = 1;
    }

    if (blas_impl->_size.update_size) {
        blas_impl->_update_scratch = new k3resourceObj;
        res_impl = blas_impl->_update_scratch->getImpl();
        dx12_resource_desc.Width = blas_impl->_size.update_size;
        hr = _data->_dev->CreatePlacedResource(rtadesc->update_mem_pool->getImpl()->_heap, rtadesc->update_mem_offset, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not allocate update_scratch resource", "k3gfxObj::AllocBlas");
            return;
        }
        res_impl->_width = blas_impl->_size.update_size;
        res_impl->_height = 1;
        res_impl->_depth = 1;
        res_impl->_format = k3fmt::UNKNOWN;
        res_impl->_max_mip = 1;
        res_impl->_resource_state = k3resourceState::COMMON;
        res_impl->_samples = 1;
    }
}

K3API k3tlas k3gfxObj::CreateTlas(const k3tlasCreateDesc* tldesc)
{
    HRESULT hr;
    k3tlas tlas = new k3tlasObj;
    k3tlasImpl* tlas_impl = tlas->getImpl();

    // Create a copy of the instance information
    tlas_impl->_num_instances = tldesc->num_instances;
    tlas_impl->_instances = new k3tlasInstance[tlas_impl->_num_instances];
    uint32_t i;
    for (i = 0; i < tlas_impl->_num_instances; i++) {
        memcpy(tlas_impl->_instances[i].transform, tldesc->instances[i].transform, 12 * sizeof(float));
        tlas_impl->_instances[i].id = tldesc->instances[i].id;
        tlas_impl->_instances[i].hit_group = tldesc->instances[i].hit_group;
        tlas_impl->_instances[i].mask = tldesc->instances[i].mask;
        tlas_impl->_instances[i].flags = tldesc->instances[i].flags;
        tlas_impl->_instances[i].blas = tldesc->instances[i].blas;
    }

    // Create upload buffer for TLAS instances, and copy instance information
    tlas_impl->_instance_upbuf = CreateUploadBuffer();
    D3D12_RAYTRACING_INSTANCE_DESC* inst = (D3D12_RAYTRACING_INSTANCE_DESC*)tlas_impl->_instance_upbuf->MapForWrite(tlas_impl->_num_instances * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    for (i = 0; i < tlas_impl->_num_instances; i++) {
        memcpy(inst[i].Transform, tlas_impl->_instances[i].transform, 12 * sizeof(float));
        inst[i].InstanceID = tlas_impl->_instances[i].id;
        inst[i].InstanceContributionToHitGroupIndex = tlas_impl->_instances[i].hit_group;
        inst[i].InstanceMask = tlas_impl->_instances[i].mask;
        inst[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;  // TODO: implement flags
        inst[i].AccelerationStructure = tlas_impl->_instances[i].blas->getImpl()->_blas->getImpl()->_dx12_resource->GetGPUVirtualAddress();
    }
    tlas_impl->_instance_upbuf->Unmap();

    // Get size of acceleration structure
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS as_inputs = {};
    as_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    as_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    as_inputs.NumDescs = tlas_impl->_num_instances;
    as_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO as_build_info = {};
    _data->_dev5->GetRaytracingAccelerationStructurePrebuildInfo(&as_inputs, &as_build_info);
    tlas_impl->_size.rtas_size = pad256(as_build_info.ResultDataMaxSizeInBytes);
    tlas_impl->_size.create_size = pad256(as_build_info.ScratchDataSizeInBytes);
    tlas_impl->_size.update_size = pad256(as_build_info.UpdateScratchDataSizeInBytes);

    // These need to be allocated in a separate call AllocTlas
    // if alloc flag is false
    // this is needed to create as a placed resource instead of a committed one
    tlas_impl->_tlas = NULL;
    tlas_impl->_create_scratch = NULL;
    tlas_impl->_update_scratch = NULL;

    if (tldesc->alloc) {
        // Allocate data structures in committed memory
        tlas_impl->_tlas = new k3resourceObj;
        k3resourceImpl* res_impl = tlas_impl->_tlas->getImpl();

        D3D12_RESOURCE_DESC dx12_resource_desc = {};
        dx12_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        dx12_resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        dx12_resource_desc.Width = tlas_impl->_size.rtas_size;
        dx12_resource_desc.Height = 1;
        dx12_resource_desc.DepthOrArraySize = 1;
        dx12_resource_desc.MipLevels = 1;
        dx12_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        dx12_resource_desc.SampleDesc.Count = 1;
        dx12_resource_desc.SampleDesc.Quality = 0;
        dx12_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        dx12_resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES dx12_heap_prop;
        dx12_heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
        dx12_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        dx12_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        dx12_heap_prop.CreationNodeMask = 0;
        dx12_heap_prop.VisibleNodeMask = 0;
        D3D12_HEAP_FLAGS dx12_heap_flags = D3D12_HEAP_FLAG_NONE;
        hr = _data->_dev->CreateCommittedResource(&dx12_heap_prop, dx12_heap_flags, &dx12_resource_desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not create TLAS resource", "k3gfxObj::CreateTlas");
            return NULL;
        }
        res_impl->_width = tlas_impl->_size.rtas_size;
        res_impl->_height = 1;
        res_impl->_depth = 1;
        res_impl->_format = k3fmt::UNKNOWN;
        res_impl->_max_mip = 1;
        res_impl->_resource_state = k3resourceState::RT_ACCEL_STRUCT;
        res_impl->_samples = 1;

        if (tlas_impl->_size.create_size) {
            tlas_impl->_create_scratch = new k3resourceObj;
            res_impl = tlas_impl->_create_scratch->getImpl();
            dx12_resource_desc.Width = tlas_impl->_size.create_size;
            hr = _data->_dev->CreateCommittedResource(&dx12_heap_prop, dx12_heap_flags, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
            if (hr != S_OK) {
                k3error::Handler("Could not create create_scatch resource", "k3gfxObj::CreateTlas");
                return NULL;
            }
            res_impl->_width = tlas_impl->_size.create_size;
            res_impl->_height = 1;
            res_impl->_depth = 1;
            res_impl->_format = k3fmt::UNKNOWN;
            res_impl->_max_mip = 1;
            res_impl->_resource_state = k3resourceState::COMMON;
            res_impl->_samples = 1;
        }

        if (tlas_impl->_size.update_size) {
            tlas_impl->_update_scratch = new k3resourceObj;
            res_impl = tlas_impl->_update_scratch->getImpl();
            dx12_resource_desc.Width = tlas_impl->_size.update_size;
            hr = _data->_dev->CreateCommittedResource(&dx12_heap_prop, dx12_heap_flags, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
            if (hr != S_OK) {
                k3error::Handler("Could not create update_scratch resource", "k3gfxObj::CreateTlas");
                return NULL;
            }
            res_impl->_width = tlas_impl->_size.update_size;
            res_impl->_height = 1;
            res_impl->_depth = 1;
            res_impl->_format = k3fmt::UNKNOWN;
            res_impl->_max_mip = 1;
            res_impl->_resource_state = k3resourceState::COMMON;
            res_impl->_samples = 1;
        }

        UINT desc_size = _data->_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE dx12_desc_handle(_data->_shader_heap[0]->GetCPUDescriptorHandleForHeapStart());
        D3D12_GPU_DESCRIPTOR_HANDLE dx12_gpu_desc_handle(_data->_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
        dx12_desc_handle.ptr += (tldesc->view_index * desc_size);
        dx12_gpu_desc_handle.ptr += (tldesc->view_index * desc_size);
        D3D12_SHADER_RESOURCE_VIEW_DESC dx12_srv_desc = {};
        dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        dx12_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        dx12_srv_desc.RaytracingAccelerationStructure.Location = tlas_impl->_tlas->getImpl()->_dx12_resource->GetGPUVirtualAddress();
        _data->_dev->CreateShaderResourceView(NULL, &dx12_srv_desc, dx12_desc_handle);
        tlas_impl->_cpu_view = dx12_desc_handle;
        tlas_impl->_gpu_view = dx12_gpu_desc_handle;
    }
    tlas_impl->_view_index = tldesc->view_index;

    return tlas;
}

K3API void k3gfxObj::AllocTlas(k3tlas tl, const k3rtasAllocDesc* rtadesc)
{
    HRESULT hr;
    k3tlasImpl* tlas_impl = tl->getImpl();

    // Allocates data structures in placed memory
    tlas_impl->_tlas = new k3resourceObj;
    k3resourceImpl* res_impl = tlas_impl->_tlas->getImpl();

    D3D12_RESOURCE_DESC dx12_resource_desc = {};
    dx12_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    dx12_resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    dx12_resource_desc.Width = tlas_impl->_size.rtas_size;
    dx12_resource_desc.Height = 1;
    dx12_resource_desc.DepthOrArraySize = 1;
    dx12_resource_desc.MipLevels = 1;
    dx12_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    dx12_resource_desc.SampleDesc.Count = 1;
    dx12_resource_desc.SampleDesc.Quality = 0;
    dx12_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    dx12_resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hr = _data->_dev->CreatePlacedResource(rtadesc->rtas_mem_pool->getImpl()->_heap, rtadesc->rtas_mem_offset, &dx12_resource_desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
    if (hr != S_OK) {
        k3error::Handler("Could not allocate BLAS resource", "k3gfxObj::AllocBlas");
        return;
    }
    res_impl->_width = tlas_impl->_size.rtas_size;
    res_impl->_height = 1;
    res_impl->_depth = 1;
    res_impl->_format = k3fmt::UNKNOWN;
    res_impl->_max_mip = 1;
    res_impl->_resource_state = k3resourceState::RT_ACCEL_STRUCT;
    res_impl->_samples = 1;

    if (tlas_impl->_size.create_size) {
        tlas_impl->_create_scratch = new k3resourceObj;
        res_impl = tlas_impl->_create_scratch->getImpl();
        dx12_resource_desc.Width = tlas_impl->_size.create_size;
        hr = _data->_dev->CreatePlacedResource(rtadesc->create_mem_pool->getImpl()->_heap, rtadesc->create_mem_offset, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not allocate create_scatch resource", "k3gfxObj::AllocBlas");
            return;
        }
        res_impl->_width = tlas_impl->_size.create_size;
        res_impl->_height = 1;
        res_impl->_depth = 1;
        res_impl->_format = k3fmt::UNKNOWN;
        res_impl->_max_mip = 1;
        res_impl->_resource_state = k3resourceState::COMMON;
        res_impl->_samples = 1;
    }

    if (tlas_impl->_size.update_size) {
        tlas_impl->_update_scratch = new k3resourceObj;
        res_impl = tlas_impl->_update_scratch->getImpl();
        dx12_resource_desc.Width = tlas_impl->_size.update_size;
        hr = _data->_dev->CreatePlacedResource(rtadesc->update_mem_pool->getImpl()->_heap, rtadesc->update_mem_offset, &dx12_resource_desc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&res_impl->_dx12_resource));
        if (hr != S_OK) {
            k3error::Handler("Could not allocate update_scratch resource", "k3gfxObj::AllocBlas");
            return;
        }
        res_impl->_width = tlas_impl->_size.update_size;
        res_impl->_height = 1;
        res_impl->_depth = 1;
        res_impl->_format = k3fmt::UNKNOWN;
        res_impl->_max_mip = 1;
        res_impl->_resource_state = k3resourceState::COMMON;
        res_impl->_samples = 1;
    }

    UINT desc_size = _data->_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE dx12_desc_handle(_data->_shader_heap[0]->GetCPUDescriptorHandleForHeapStart());
    D3D12_GPU_DESCRIPTOR_HANDLE dx12_gpu_desc_handle(_data->_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
    dx12_desc_handle.ptr += (tlas_impl->_view_index * desc_size);
    dx12_gpu_desc_handle.ptr += (tlas_impl->_view_index * desc_size);
    D3D12_SHADER_RESOURCE_VIEW_DESC dx12_srv_desc = {};
    dx12_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    dx12_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    dx12_srv_desc.RaytracingAccelerationStructure.Location = tlas_impl->_tlas->getImpl()->_dx12_resource->GetGPUVirtualAddress();
    _data->_dev->CreateShaderResourceView(NULL, &dx12_srv_desc, dx12_desc_handle);
    tlas_impl->_cpu_view = dx12_desc_handle;
    tlas_impl->_gpu_view = dx12_gpu_desc_handle;
}

