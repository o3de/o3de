/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>

#if !defined(AZ_DX12_REFCOUNTED)
    #error "Incorrect include of DX12_Windows.h, please include DX12.h instead of this header"
#endif

#include <d3d12.h>
#include <dxgi1_6.h>

AZ_PUSH_DISABLE_WARNING(4265, "-Wunknown-warning-option") // class has virtual functions, but its non-trivial destructor is not virtual; 
#include <wrl.h>
AZ_POP_DISABLE_WARNING

#include <d3dx12.h>
#include <d3dcommon.h>

// This define is enabled if LY_PIX_ENABLED is enabled during configure. You can use LY_PIX_PATH to point where pix is downloaded.
// Enabling this define will allow the runtime code to add PIX markers which will help with pix and renderdoc gpu captures
#ifdef USE_PIX
    #include <WinPixEventRuntime/pix3.h>
#else
    // Undefine these for now, should consider a more elegant solution.
    #define PIXBeginEvent(...)
    #define PIXEndEvent(...)
#endif //USE_PIX

// This define controls whether ID3D12PipelineLibrary instances are used to de-duplicate
// pipeline states. This feature was added in the Windows Anniversary Update, so if you
// have an older version of windows this will need to be disabled.
#define AZ_DX12_USE_PIPELINE_LIBRARY

// Enabling this define will force every scope into its own command list that
// is explicitly flushed through the GPU before the next scope is processed.
// Use it to debug TDR's when you need to know which scope is causing the problem.
//#define AZ_DX12_FORCE_FLUSH_SCOPES


// This define is enabled if Aftermath SDK is downloaded and the path is hooked up to Env var ATOM_AFTERMATH_PATH.
// Enabling this define will allow AfterMath SDK to do a dump in case of GPU crash/TDR.
// The dump is outputted in the same folder as the executable.
// Windows DX12 back-end will also try to output the last executing pass if that information is available.
#if defined(USE_NSIGHT_AFTERMATH)
#include <RHI/NsightAftermathGpuCrashTracker_Windows.h>
#endif

// This define controls whether DXR ray tracing support is available on the platform.
#define AZ_DX12_DXR_SUPPORT

// This define is used to initialize the D3D12_ROOT_SIGNATURE_DESC::Flags property.
#define AZ_DX12_ROOT_SIGNATURE_FLAGS D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT

using ID3D12CommandAllocatorX = ID3D12CommandAllocator;
using ID3D12CommandQueueX = ID3D12CommandQueue;
using ID3D12DeviceX = ID3D12Device5;
using ID3D12PipelineLibraryX = ID3D12PipelineLibrary1;
using ID3D12PipelineStateX = ID3D12PipelineState;
using ID3D12GraphicsCommandListX = ID3D12GraphicsCommandList4;

using IDXGIAdapterX = IDXGIAdapter3;
using IDXGIFactoryX = IDXGIFactory6;
using IDXGISwapChainX = IDXGISwapChain4;
using DXGI_SWAP_CHAIN_DESCX = DXGI_SWAP_CHAIN_DESC1;

#define DX12_TEXTURE_DATA_PITCH_ALIGNMENT D3D12_TEXTURE_DATA_PITCH_ALIGNMENT

// To allow for the using's to overlap, please use the base DirectX types here instead of the remapped types

AZ_DX12_REFCOUNTED(ID3D12CommandAllocator)
AZ_DX12_REFCOUNTED(ID3D12CommandQueue)
AZ_DX12_REFCOUNTED(ID3D12CommandSignature)
AZ_DX12_REFCOUNTED(ID3D12Device)
AZ_DX12_REFCOUNTED(ID3D12Device5)
AZ_DX12_REFCOUNTED(ID3D12Fence)
AZ_DX12_REFCOUNTED(ID3D12GraphicsCommandList)
AZ_DX12_REFCOUNTED(ID3D12GraphicsCommandList1)
AZ_DX12_REFCOUNTED(ID3D12Heap)
AZ_DX12_REFCOUNTED(ID3D12Object)
AZ_DX12_REFCOUNTED(ID3D12PipelineState)
AZ_DX12_REFCOUNTED(ID3D12PipelineLibrary)
AZ_DX12_REFCOUNTED(ID3D12PipelineLibrary1)
AZ_DX12_REFCOUNTED(ID3D12QueryHeap)
AZ_DX12_REFCOUNTED(ID3D12Resource)
AZ_DX12_REFCOUNTED(ID3D12RootSignature)
AZ_DX12_REFCOUNTED(ID3D12StateObject)

AZ_DX12_REFCOUNTED(IDXGIAdapter)
AZ_DX12_REFCOUNTED(IDXGIAdapter1)
AZ_DX12_REFCOUNTED(IDXGIAdapter2)
AZ_DX12_REFCOUNTED(IDXGIAdapter3)

AZ_DX12_REFCOUNTED(IDXGIFactory)
AZ_DX12_REFCOUNTED(IDXGIFactory1)
AZ_DX12_REFCOUNTED(IDXGIFactory2)
AZ_DX12_REFCOUNTED(IDXGIFactory3)
AZ_DX12_REFCOUNTED(IDXGIFactory4)
AZ_DX12_REFCOUNTED(IDXGIFactory5)
AZ_DX12_REFCOUNTED(IDXGIFactory6)
AZ_DX12_REFCOUNTED(IDXGIFactory7)
AZ_DX12_REFCOUNTED(IDXGISwapChain)
AZ_DX12_REFCOUNTED(IDXGISwapChain1)
AZ_DX12_REFCOUNTED(IDXGISwapChain2)
AZ_DX12_REFCOUNTED(IDXGISwapChain3)
AZ_DX12_REFCOUNTED(IDXGISwapChain4)


