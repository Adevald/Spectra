#include "sdf_engine.h"

#include "ui.cpp"

void InitOrResizeUAVTexture2D(ID3D12Device *D, texture *Tex, 
                              int Width, int Height, 
                              DXGI_FORMAT Format,
                              descriptor_arena *Arena)
{
    if (!Tex->Handle)
    {
        *Tex = InitTexture2D(D, Width, Height, Format,
                             D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, Tex, Arena);
    }
    else
    {
        descriptor PrevUAV = Tex->UAV;
        
        Tex->Handle->Release();
        *Tex = InitTexture2D(D, Width, Height, Format,
                             D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        Tex->UAV = PrevUAV;
        D->CreateUnorderedAccessView(Tex->Handle, 0, 0, 
                                     Tex->UAV.CPUHandle);
    }
}

void 
engine::InitOrResizeWindowDependentResources()
{
    ID3D12Device *D = Context.Device;
    
    InitOrResizeUAVTexture2D(D, &NoisyLightTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &PositionTex, Width, Height, 
                             DXGI_FORMAT_R32G32B32A32_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &NormalTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &AlbedoTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &EmissionTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &RayDirTex, Width, Height, 
                             DXGI_FORMAT_R32G32B32A32_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &DisocclusionTex, Width, Height, 
                             DXGI_FORMAT_R8_UINT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &PositionHistTex, Width, Height, 
                             DXGI_FORMAT_R32G32B32A32_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &NormalHistTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &LightHistTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &LumMomentTex, Width, Height, 
                             DXGI_FORMAT_R16G16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &LumMomentHistTex, Width, Height, 
                             DXGI_FORMAT_R16G16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &VarianceTex, Width, Height, 
                             DXGI_FORMAT_R32_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &NextVarianceTex, Width, Height, 
                             DXGI_FORMAT_R32_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &GBufferTex, Width, Height, 
                             DXGI_FORMAT_R32G32B32A32_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &PrevPixelIdTex, Width, Height, 
                             DXGI_FORMAT_R32G32_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &IntegratedLightTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &TempTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &TaaOutputTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &TaaHistTex, Width, Height, 
                             DXGI_FORMAT_R16G16B16A16_FLOAT, 
                             &DescriptorArena);
    
    InitOrResizeUAVTexture2D(D, &OutputTex, Width, Height, 
                             DXGI_FORMAT_R8G8B8A8_UNORM, 
                             &DescriptorArena);
}

internal b32
VerifyComputeShader(char *Filename, char *EntryPoint, ID3DBlob **ErrorBlob_Out)
{
    file File = ReadTextFile(Filename);
    if (!File.Data)
    {
        Win32MessageBox("Shader Load Failure", MB_OK|MB_ICONERROR, 
                        "Failed to load shader %s", Filename);
        return false;
    }
    
    b32 HasError = 0;
    ID3DBlob *Blob = CompileShader(File, Filename, EntryPoint, "cs_5_1", &HasError);
    free(File.Data);
    
    if (HasError)
    {
        *ErrorBlob_Out = Blob;
        return false;
    }
    
    Blob->Release();
    return true;
}

void 
frame_data::BeginProfile(ID3D12GraphicsCommandList *CmdList, char *Identifer)
{
    ASSERT(!IsInSession);
    IsInSession = true;
    
    int Index = CurrQueryIndex++;
    CmdList->EndQuery(QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, Index);
    
    Sessions[CurrSessionIndex].Name = strdup(Identifer);
    Sessions[CurrSessionIndex].BeginIndex = Index;
}

void 
frame_data::EndProfile(ID3D12GraphicsCommandList *CmdList)
{
    ASSERT(IsInSession);
    
    int Index = CurrQueryIndex++;
    CmdList->EndQuery(QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, Index);
    
    Sessions[CurrSessionIndex].EndIndex = Index;
    CurrSessionIndex += 1;
    
    IsInSession = false;
}

void 
frame_data::ReadbackCounters()
{
    if (CurrQueryIndex == 0) return;
    
    D3D12_RANGE ReadRange = {0, sizeof(u64) * CurrQueryIndex};
    
    u64 *Counters = 0;
    DXOP(CounterBuffer->Map(0, &ReadRange, (void **)&Counters));
    
    D3D12_RANGE WriteRange = {};
    CounterBuffer->Unmap(0, &WriteRange);
    
    // record results
    printf("PROFILE RESULTS ------------------------------\n");
    for (int SessionI = 0; SessionI < CurrSessionIndex; ++SessionI)
    {
        profile_session *Session = Sessions + SessionI;
        
        u64 BeginCounter = Counters[Session->BeginIndex];
        u64 EndCounter = Counters[Session->EndIndex];
        f32 Miliseconds = 1000.0f * f32(EndCounter - BeginCounter) / f32(TimestampFrequency);
        
        printf("%s: %.2fms\n", Session->Name, Miliseconds);
    }
    printf("----------------------------------------------\n");
    
    for (int SessionI = 0; SessionI < CurrSessionIndex; ++SessionI)
    {
        profile_session *Session = Sessions + SessionI;
        free(Session->Name);
    }
    CurrQueryIndex = 0;
    CurrSessionIndex = 0;
}

void
engine::UpdateAndRender(HWND Window, int ClientWidth, int ClientHeight,
                        input *Input, b32 NeedsReload, f32 dT)
{
    //NOTE(chen): if window is minimized, don't run at all
    if (ClientWidth == 0 || ClientHeight == 0) return;
    
    if (!IsInitialized)
    {
        ID3D12Debug *Debug = 0;
        DXOP(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug)));
        Debug->EnableDebugLayer();
        
        ID3D12Device *D = 0;
        DXOP(D3D12CreateDevice(0, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&D)));
        
        Context = InitGPUContext(D, BACKBUFFER_COUNT);
        
        SwapChain = CreateSwapChain(Context.CmdQueue, Window, BACKBUFFER_COUNT);
        Width = ClientWidth;
        Height = ClientHeight;
        
        DescriptorArena = InitDescriptorArena(D, 100, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        RTVArena = InitDescriptorArena(D, 100, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        for (int I = 0; I < BACKBUFFER_COUNT; ++I)
        {
            ID3D12Resource *BackBuffer = 0;
            SwapChain->GetBuffer(I, IID_PPV_ARGS(&BackBuffer));
            BackBufferTexs[I] = WrapTexture(BackBuffer, D3D12_RESOURCE_STATE_PRESENT);
            AssignRTV(D, &BackBufferTexs[I], &RTVArena);
        }
        
        for (int FrameI = 0; FrameI < ARRAY_COUNT(FrameData); ++FrameI)
        {
            frame_data *Frame = FrameData + FrameI;
            DXOP(Context.CmdQueue->GetTimestampFrequency(&Frame->TimestampFrequency));
            Frame->QueryCount = 1000;
            
            D3D12_QUERY_HEAP_DESC QueryHeapDesc = {};
            QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            QueryHeapDesc.Count = Frame->QueryCount;
            DXOP(D->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&Frame->QueryHeap)));
            
            Frame->CounterBuffer = InitBuffer(D, sizeof(u64) * Frame->QueryCount, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE);
        }
        
        UISystem = InitUISystem(&Context, &DescriptorArena);
        
        PrimaryPSO = InitComputePSO(D, "../code/primary.hlsl", "main");
        PathTracePSO = InitComputePSO(D, "../code/pathtrace.hlsl", "main");
        ComputeDisocclusionPSO = InitComputePSO(D, "../code/compute_disocclusion.hlsl", "main");
        TemporalFilterPSO = InitComputePSO(D, "../code/temporal_filter.hlsl", "main");
        CalcVariancePSO = InitComputePSO(D, "../code/calc_variance.hlsl", "main");
        PackGBufferPSO = InitComputePSO(D, "../code/pack_gbuffer.hlsl", "main");
        CorrelateHistoryPSO = InitComputePSO(D, "../code/correlate_history.hlsl", "main");
        SpatialFilterPSO = InitComputePSO(D, "../code/spatial_filter.hlsl", "main");
        ApplyPrimaryShadingPSO = InitComputePSO(D, "../code/apply_primary_shading.hlsl", "main");
        TaaPSO = InitComputePSO(D, "../code/taa.hlsl", "main");
        ToneMapPSO = InitComputePSO(D, "../code/tonemap.hlsl", "main");
        
        for (int I = 0; I < 64; ++I)
        {
            char BlueNoiseFilename[256];
            snprintf(BlueNoiseFilename, sizeof(BlueNoiseFilename),
                     "../data/textures/HDR_RGBA_%d.png", I);
            BlueNoiseTexs[I] = Context.LoadTexture2D(BlueNoiseFilename, 
                                                     DXGI_FORMAT_R8G8B8A8_UNORM,
                                                     D3D12_RESOURCE_FLAG_NONE,
                                                     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            AssignSRV(D, BlueNoiseTexs+I, &DescriptorArena);
        }
        
        InitOrResizeWindowDependentResources();
        
        Camera.P = {0.0f, 0.0f, -4.0f};
        Camera.Orientation = Quaternion();
        
        for (u64 I = 0; I < ARRAY_COUNT(HaltonSequence); ++I)
        {
            HaltonSequence[I].X = RadicalInverse(I+1, 2);
            HaltonSequence[I].Y = RadicalInverse(I+1, 3);
        }
        
        IsInitialized = true;
    }
    Time += dT;
    
    if (ClientWidth != Width || ClientHeight != Height)
    {
        Context.FlushFramesInFlight();
        
        Width = ClientWidth;
        Height = ClientHeight;
        
        UISystem.BuildUIVB(&Context, Width, Height);
        InitOrResizeWindowDependentResources();
        
        for (int BI = 0; BI < BACKBUFFER_COUNT; ++BI)
        {
            BackBufferTexs[BI].Handle->Release();
        }
        
        DXOP(SwapChain->ResizeBuffers(BACKBUFFER_COUNT, 0, 0,
                                      DXGI_FORMAT_R8G8B8A8_UNORM, 0));
        
        for (int BI = 0; BI < BACKBUFFER_COUNT; ++BI)
        {
            ID3D12Resource *BackBuffer = 0;
            SwapChain->GetBuffer(BI, IID_PPV_ARGS(&BackBuffer));
            
            descriptor PrevRTV = BackBufferTexs[BI].RTV;
            BackBufferTexs[BI] = WrapTexture(BackBuffer, D3D12_RESOURCE_STATE_PRESENT);
            BackBufferTexs[BI].RTV = PrevRTV;
            Context.Device->CreateRenderTargetView(BackBufferTexs[BI].Handle, 
                                                   0, BackBufferTexs[BI].RTV.CPUHandle);
        }
    }
    
    if (NeedsReload)
    {
        Context.FlushFramesInFlight();
        
        ID3D12Device *D = Context.Device;
        
        b32 ShadersAreValid = true;
        ID3DBlob *ErrorBlob = 0;
        if (ShadersAreValid) ShadersAreValid = VerifyComputeShader("../code/primary.hlsl", "main", &ErrorBlob);
        if (ShadersAreValid) ShadersAreValid = VerifyComputeShader("../code/pathtrace.hlsl", "main", &ErrorBlob);
        if (ShadersAreValid) ShadersAreValid = VerifyComputeShader("../code/apply_primary_shading.hlsl", "main", &ErrorBlob);
        
        if (ErrorBlob)
        {
            UISystem.SetErrorMessage((char *)ErrorBlob->GetBufferPointer());
            ErrorBlob->Release();
            UISystem.BuildUIVB(&Context, Width, Height);
        }
        else
        {
            UISystem.SetErrorMessage("");
            UISystem.BuildUIVB(&Context, Width, Height);
        }
        
        if (ShadersAreValid)
        {
            PrimaryPSO.Release();
            PathTracePSO.Release();
            ApplyPrimaryShadingPSO.Release();
            
            PrimaryPSO = InitComputePSO(D, "../code/primary.hlsl", "main");
            PathTracePSO = InitComputePSO(D, "../code/pathtrace.hlsl", "main");
            ApplyPrimaryShadingPSO = InitComputePSO(D, "../code/apply_primary_shading.hlsl", "main");
        }
        
        FrameIndex = 0;
    }
    
    // camera orientation
    if (Input->MouseDown)
    {
        v2 MousedP = {f32(Input->MousedP.X) / f32(Width), f32(Input->MousedP.Y) / f32(Height)};
        quaternion XRot = Quaternion(YAxis(), MousedP.X * Pi32);
        Camera.Orientation = XRot * Camera.Orientation;
        
        v3 LocalXAxis = Rotate(XAxis(), Camera.Orientation);
        quaternion YRot = Quaternion(LocalXAxis, MousedP.Y * Pi32);
        Camera.Orientation = YRot * Camera.Orientation;
    }
    
    // camera translation
    v3 dP = {};
    if (Input->Keys['W']) dP.Z += 1.0f;
    if (Input->Keys['S']) dP.Z -= 1.0f;
    if (Input->Keys['A']) dP.X -= 1.0f;
    if (Input->Keys['D']) dP.X += 1.0f;
    dP = Normalize(dP);
    dP = Rotate(dP, Camera.Orientation);
    f32 Speed = 5.0f;
    Camera.P += Speed * dP * dT;
    
    UINT CurrBackbufferIndex = SwapChain->GetCurrentBackBufferIndex();
    frame_data *Frame = FrameData + CurrBackbufferIndex;
    
    Context.Reset(CurrBackbufferIndex);
    ID3D12GraphicsCommandList *CmdList = Context.CmdList;
    ID3D12CommandQueue *CmdQueue = Context.CmdQueue;
    
    int ThreadGroupCountX = (Width-1)/32 + 1;
    int ThreadGroupCountY = (Height-1)/32 + 1;
    
    v3 CamOffset = Rotate(ZAxis(), Camera.Orientation);
    v3 CamAt = Camera.P + CamOffset;
    v2 PixelOffset = V2(-0.5f) + HaltonSequence[FrameIndex % ARRAY_COUNT(HaltonSequence)];
    
    if (Input->Keys['N']) SwitchViewThreshold -= 10;
    if (Input->Keys['M']) SwitchViewThreshold += 10;
    if (SwitchViewThreshold < 0) SwitchViewThreshold = 0;
    if (SwitchViewThreshold >= Width) SwitchViewThreshold = Width-1;
    
    // primary rays
    Frame->BeginProfile(CmdList, "Primary rays");
    {
        CmdList->SetPipelineState(PrimaryPSO.Handle);
        CmdList->SetComputeRootSignature(PrimaryPSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, AlbedoTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, EmissionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, RayDirTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(5, 1, &Width, 0);
        CmdList->SetComputeRoot32BitConstants(5, 1, &Height, 1);
        CmdList->SetComputeRoot32BitConstants(5, 1, &FrameIndex, 2);
        CmdList->SetComputeRoot32BitConstants(5, 3, &Camera.P, 4);
        CmdList->SetComputeRoot32BitConstants(5, 3, &CamAt, 8);
        CmdList->SetComputeRoot32BitConstants(5, 1, &Time, 11);
        CmdList->SetComputeRoot32BitConstants(5, 2, &PixelOffset, 12);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&PositionTex);
        Context.UAVBarrier(&NormalTex);
        Context.UAVBarrier(&AlbedoTex);
        Context.UAVBarrier(&EmissionTex);
        Context.UAVBarrier(&RayDirTex);
        Context.FlushBarriers();
    }
    Frame->EndProfile(CmdList);
    
    // correlate history pixels
    Frame->BeginProfile(CmdList, "Correlate history pixels");
    {
        quaternion PrevCameraInvOrientation = Conjugate(PrevCamera.Orientation);
        
        CmdList->SetPipelineState(CorrelateHistoryPSO.Handle);
        CmdList->SetComputeRootSignature(CorrelateHistoryPSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, PrevPixelIdTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(2, 3, &PrevCamera.P, 0);
        CmdList->SetComputeRoot32BitConstants(2, 4, &PrevCameraInvOrientation, 4);
        CmdList->SetComputeRoot32BitConstants(2, 1, &Width, 8);
        CmdList->SetComputeRoot32BitConstants(2, 1, &Height, 9);
        CmdList->SetComputeRoot32BitConstants(2, 1, &FrameIndex, 10);
        CmdList->SetComputeRoot32BitConstants(2, 2, &PixelOffset, 12);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&PrevPixelIdTex);
        Context.FlushBarriers();
    }
    Frame->EndProfile(CmdList);
    
    // compute disocclusion
    Frame->BeginProfile(CmdList, "Compute disocclusion");
    {
        CmdList->SetPipelineState(ComputeDisocclusionPSO.Handle);
        CmdList->SetComputeRootSignature(ComputeDisocclusionPSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, PositionHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, NormalHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, PrevPixelIdTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(5, DisocclusionTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(6, 1, &FrameIndex, 0);
        CmdList->SetComputeRoot32BitConstants(6, 3, &Camera.P, 1);
        CmdList->SetComputeRoot32BitConstants(6, 1, &Width, 4);
        CmdList->SetComputeRoot32BitConstants(6, 1, &Height, 5);
        
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&DisocclusionTex);
        Context.FlushBarriers();
    }
    Frame->EndProfile(CmdList);
    
    // path tracing
    Frame->BeginProfile(CmdList, "Path tracing");
    {
        CmdList->SetPipelineState(PathTracePSO.Handle);
        CmdList->SetComputeRootSignature(PathTracePSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, NoisyLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, AlbedoTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, EmissionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(5, RayDirTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(6, DisocclusionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(7, BlueNoiseTexs[0].SRV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(8, 1, &Width, 0);
        CmdList->SetComputeRoot32BitConstants(8, 1, &Height, 1);
        CmdList->SetComputeRoot32BitConstants(8, 1, &FrameIndex, 2);
        CmdList->SetComputeRoot32BitConstants(8, 3, &Camera.P, 4);
        CmdList->SetComputeRoot32BitConstants(8, 3, &CamAt, 8);
        CmdList->SetComputeRoot32BitConstants(8, 1, &Time, 11);
        CmdList->SetComputeRoot32BitConstants(8, 2, &PixelOffset, 12);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&NoisyLightTex);
        Context.UAVBarrier(&PositionTex);
        Context.UAVBarrier(&NormalTex);
        Context.UAVBarrier(&AlbedoTex);
        Context.UAVBarrier(&EmissionTex);
        Context.UAVBarrier(&RayDirTex);
        Context.FlushBarriers();
    }
    Frame->EndProfile(CmdList);
    
    // pack gbuffer
    Frame->BeginProfile(CmdList, "Pack gbuffer");
    {
        quaternion CameraInvOrientation = Conjugate(Camera.Orientation);
        
        CmdList->SetPipelineState(PackGBufferPSO.Handle);
        CmdList->SetComputeRootSignature(PackGBufferPSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, GBufferTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(3, 3, &Camera.P, 0);
        CmdList->SetComputeRoot32BitConstants(3, 4, &CameraInvOrientation, 4);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&GBufferTex);
        Context.FlushBarriers();
    }
    Frame->EndProfile(CmdList);
    
    // temporal filter
    Frame->BeginProfile(CmdList, "Temporal Filter");
    {
        quaternion CurrCameraInvOrientation = Conjugate(Camera.Orientation);
        
        CmdList->SetPipelineState(TemporalFilterPSO.Handle);
        CmdList->SetComputeRootSignature(TemporalFilterPSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, NoisyLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, LightHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(5, PositionHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(6, NormalHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(7, LumMomentHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(8, LumMomentTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(9, PrevPixelIdTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(10, 1, &FrameIndex, 0);
        CmdList->SetComputeRoot32BitConstants(10, 3, &Camera.P, 1);
        CmdList->SetComputeRoot32BitConstants(10, 4, &CurrCameraInvOrientation, 4);
        CmdList->SetComputeRoot32BitConstants(10, 1, &Width, 8);
        CmdList->SetComputeRoot32BitConstants(10, 1, &Height, 9);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&IntegratedLightTex);
        Context.UAVBarrier(&LumMomentTex);
        Context.FlushBarriers();
        
        Context.CopyResourceBarriered(&LightHistTex, &IntegratedLightTex);
        Context.CopyResourceBarriered(&LumMomentHistTex, &LumMomentTex);
        Context.CopyResourceBarriered(&PositionHistTex, &PositionTex);
        Context.CopyResourceBarriered(&NormalHistTex, &NormalTex);
    }
    Frame->EndProfile(CmdList);
    
    //NOTE(chen): record temporal filtered only result to help debug spatial filter
    {
        Context.CopyResourceBarriered(&NoisyLightTex, &IntegratedLightTex);
    }
    
    // calc variance
    Frame->BeginProfile(CmdList, "Calc variance");
    {
        CmdList->SetPipelineState(CalcVariancePSO.Handle);
        CmdList->SetComputeRootSignature(CalcVariancePSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, LumMomentTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, VarianceTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, GBufferTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(4, 3, &Camera.P, 0);
        
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&VarianceTex);
        Context.FlushBarriers();
    }
    Frame->EndProfile(CmdList);
    
    // spatial filter
    Frame->BeginProfile(CmdList, "Spatial filter");
    {
        int Iterations = 6;
        // this gaurantees the end result ends up in IntegratedNoisyLightTex
        ASSERT(Iterations % 2 == 0); 
        
        texture *PingPongs[2] = {&IntegratedLightTex, &TempTex};
        texture *Variances[2] = {&VarianceTex, &NextVarianceTex};
        
        for (int Depth = 0; Depth < Iterations; ++Depth)
        {
            int FilterStride = 1 << Depth;
            
            CmdList->SetPipelineState(SpatialFilterPSO.Handle);
            CmdList->SetComputeRootSignature(SpatialFilterPSO.RootSignature);
            CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
            CmdList->SetComputeRootDescriptorTable(0, PingPongs[0]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(1, PingPongs[1]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(2, GBufferTex.UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(3, Variances[0]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(4, Variances[1]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(5, LightHistTex.UAV.GPUHandle);
            CmdList->SetComputeRoot32BitConstants(6, 3, &Camera.P, 0);
            CmdList->SetComputeRoot32BitConstants(6, 1, &Depth, 3);
            CmdList->SetComputeRoot32BitConstants(6, 1, &FilterStride, 4);
            CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
            
            Context.UAVBarrier(PingPongs[1]);
            Context.UAVBarrier(Variances[1]);
            Context.FlushBarriers();
            
            // swap
            texture *Temp = PingPongs[1];
            PingPongs[1] = PingPongs[0];
            PingPongs[0] = Temp;
            
            // swap
            texture *VarTemp = Variances[1];
            Variances[1] = Variances[0];
            Variances[0] = VarTemp;
        }
    }
    Frame->EndProfile(CmdList);
    
    // apply primary shading
    Frame->BeginProfile(CmdList, "Primary shading");
    {
        CmdList->SetPipelineState(ApplyPrimaryShadingPSO.Handle);
        CmdList->SetComputeRootSignature(ApplyPrimaryShadingPSO.RootSignature);
        CmdList->SetComputeRootDescriptorTable(0, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, AlbedoTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, EmissionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, RayDirTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(5, 1, &Time, 0);
        CmdList->SetComputeRoot32BitConstants(5, 3, &Camera.P, 1);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&IntegratedLightTex);
        Context.FlushBarriers();
    }
    Frame->EndProfile(CmdList);
    
    // apply primary shading on noisy input for comparison
    {
        CmdList->SetPipelineState(ApplyPrimaryShadingPSO.Handle);
        CmdList->SetComputeRootSignature(ApplyPrimaryShadingPSO.RootSignature);
        CmdList->SetComputeRootDescriptorTable(0, NoisyLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, AlbedoTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, EmissionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, RayDirTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(5, 1, &Time, 0);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&IntegratedLightTex);
        Context.FlushBarriers();
    }
    
    // TAA
    Frame->BeginProfile(CmdList, "TAA");
    {
        CmdList->SetPipelineState(TaaPSO.Handle);
        CmdList->SetComputeRootSignature(TaaPSO.RootSignature);
        CmdList->SetComputeRootDescriptorTable(0, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, TaaHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, TaaOutputTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, PrevPixelIdTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(4, 1, &FrameIndex, 0);
        CmdList->SetComputeRoot32BitConstants(4, 1, &Width, 1);
        CmdList->SetComputeRoot32BitConstants(4, 1, &Height, 2);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&TaaOutputTex);
        Context.FlushBarriers();
        
        Context.CopyResourceBarriered(&TaaHistTex, &TaaOutputTex);
    }
    Frame->EndProfile(CmdList);
    
    Context.UAVBarrier(&LightHistTex);
    Context.FlushBarriers();
    
    // tonemap
    {
        CmdList->SetPipelineState(ToneMapPSO.Handle);
        CmdList->SetComputeRootSignature(ToneMapPSO.RootSignature);
        CmdList->SetComputeRootDescriptorTable(0, TaaOutputTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, OutputTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, NoisyLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, BlueNoiseTexs[0].SRV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(4, 1, &SwitchViewThreshold, 0);
        CmdList->SetComputeRoot32BitConstants(4, 1, &FrameIndex, 1);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&OutputTex);
        Context.FlushBarriers();
    }
    
    UINT BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    texture *BackBufferTex = BackBufferTexs + BackBufferIndex;
    Context.CopyResourceBarriered(BackBufferTex, &OutputTex);
    
    Context.TransitionBarrier(BackBufferTex, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.FlushBarriers();
    
    UISystem.DrawMessage(&Context, *BackBufferTex);
    
    Context.TransitionBarrier(BackBufferTex, D3D12_RESOURCE_STATE_PRESENT);
    Context.FlushBarriers();
    
    CmdList->ResolveQueryData(Frame->QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP,
                              0, Frame->CurrQueryIndex, Frame->CounterBuffer, 0);
    
    DXOP(CmdList->Close());
    
    ID3D12CommandList *CmdLists[] = {CmdList};
    CmdQueue->ExecuteCommandLists(1, CmdLists);
    
    SwapChain->Present(1, 0);
    
    UINT NextBackbufferIndex = SwapChain->GetCurrentBackBufferIndex();
    Context.WaitForGpu(NextBackbufferIndex);
    
    FrameData[NextBackbufferIndex].ReadbackCounters();
    
    FrameIndex += 1;
    PrevCamera = Camera;
}