/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Contains portable definition of structs and enums to match
//               those in DXGI.h in the DirectX SDK

#ifndef __DXGL_DXGI_h__
#define __DXGL_DXGI_h__

#include "DXGL_dxgitype.h"


////////////////////////////////////////////////////////////////////////////
//  Defines
////////////////////////////////////////////////////////////////////////////

#define DXGI_MAX_SWAP_CHAIN_BUFFERS        (16)
#define DXGI_PRESENT_TEST               0x00000001UL
#define DXGI_PRESENT_DO_NOT_SEQUENCE    0x00000002UL
#define DXGI_PRESENT_RESTART            0x00000004UL

#define DXGI_USAGE_SHADER_INPUT             (1L << (0 + 4))
#define DXGI_USAGE_RENDER_TARGET_OUTPUT     (1L << (1 + 4))
#define DXGI_USAGE_BACK_BUFFER              (1L << (2 + 4))
#define DXGI_USAGE_SHARED                   (1L << (3 + 4))
#define DXGI_USAGE_READ_ONLY                (1L << (4 + 4))
#define DXGI_USAGE_DISCARD_ON_PRESENT       (1L << (5 + 4))
#define DXGI_USAGE_UNORDERED_ACCESS         (1L << (6 + 4))

#define DXGI_ENUM_MODES_INTERLACED  (1UL)

#define DXGI_ENUM_MODES_SCALING (2UL)

#define DXGI_MWA_NO_WINDOW_CHANGES      (1 << 0)
#define DXGI_MWA_NO_ALT_ENTER           (1 << 1)
#define DXGI_MWA_NO_PRINT_SCREEN        (1 << 2)
#define DXGI_MWA_VALID                  (0x7)



////////////////////////////////////////////////////////////////////////////
//  Enums
////////////////////////////////////////////////////////////////////////////

typedef
    enum DXGI_RESIDENCY
{
    DXGI_RESIDENCY_FULLY_RESIDENT   = 1,
    DXGI_RESIDENCY_RESIDENT_IN_SHARED_MEMORY    = 2,
    DXGI_RESIDENCY_EVICTED_TO_DISK  = 3
}   DXGI_RESIDENCY;

typedef
    enum DXGI_SWAP_EFFECT
{
    DXGI_SWAP_EFFECT_DISCARD    = 0,
    DXGI_SWAP_EFFECT_SEQUENTIAL = 1
}   DXGI_SWAP_EFFECT;

typedef
    enum DXGI_SWAP_CHAIN_FLAG
{
    DXGI_SWAP_CHAIN_FLAG_NONPREROTATED  = 1,
    DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH  = 2,
    DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE = 4
}   DXGI_SWAP_CHAIN_FLAG;

typedef
    enum DXGI_ADAPTER_FLAG
{
    DXGI_ADAPTER_FLAG_NONE  = 0,
    DXGI_ADAPTER_FLAG_REMOTE    = 1,
    DXGI_ADAPTER_FLAG_FORCE_DWORD   = 0xffffffff
}   DXGI_ADAPTER_FLAG;

typedef UINT DXGI_USAGE;


////////////////////////////////////////////////////////////////////////////
//  Structs
////////////////////////////////////////////////////////////////////////////

typedef struct DXGI_FRAME_STATISTICS
{
    UINT PresentCount;
    UINT PresentRefreshCount;
    UINT SyncRefreshCount;
    LARGE_INTEGER SyncQPCTime;
    LARGE_INTEGER SyncGPUTime;
}   DXGI_FRAME_STATISTICS;

typedef struct DXGI_MAPPED_RECT
{
    INT Pitch;
    BYTE* pBits;
}   DXGI_MAPPED_RECT;

typedef struct DXGI_ADAPTER_DESC
{
    WCHAR Description[ 128 ];
    UINT VendorId;
    UINT DeviceId;
    UINT SubSysId;
    UINT Revision;
    SIZE_T DedicatedVideoMemory;
    SIZE_T DedicatedSystemMemory;
    SIZE_T SharedSystemMemory;
    LUID AdapterLuid;
}   DXGI_ADAPTER_DESC;

typedef struct DXGI_OUTPUT_DESC
{
    WCHAR DeviceName[ 32 ];
    RECT DesktopCoordinates;
    BOOL AttachedToDesktop;
    DXGI_MODE_ROTATION Rotation;
    HMONITOR Monitor;
}   DXGI_OUTPUT_DESC;

typedef struct DXGI_SHARED_RESOURCE
{
    HANDLE Handle;
}   DXGI_SHARED_RESOURCE;

typedef struct DXGI_SURFACE_DESC
{
    UINT Width;
    UINT Height;
    DXGI_FORMAT Format;
    DXGI_SAMPLE_DESC SampleDesc;
}   DXGI_SURFACE_DESC;

typedef struct DXGI_SWAP_CHAIN_DESC
{
    DXGI_MODE_DESC BufferDesc;
    DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage;
    UINT BufferCount;
    HWND OutputWindow;
    BOOL Windowed;
    DXGI_SWAP_EFFECT SwapEffect;
    UINT Flags;
}   DXGI_SWAP_CHAIN_DESC;

typedef struct DXGI_ADAPTER_DESC1
{
    WCHAR Description[ 128 ];
    UINT VendorId;
    UINT DeviceId;
    UINT SubSysId;
    UINT Revision;
    SIZE_T DedicatedVideoMemory;
    SIZE_T DedicatedSystemMemory;
    SIZE_T SharedSystemMemory;
    LUID AdapterLuid;
    UINT Flags;
}   DXGI_ADAPTER_DESC1;

typedef struct DXGI_DISPLAY_COLOR_SPACE
{
    FLOAT PrimaryCoordinates[ 8 ][ 2 ];
    FLOAT WhitePoints[ 16 ][ 2 ];
}   DXGI_DISPLAY_COLOR_SPACE;


typedef UINT DXGI_USAGE;


////////////////////////////////////////////////////////////////////////////
//  Forward declaration of unused interfaces
////////////////////////////////////////////////////////////////////////////

//struct IDXGIObject;                   // Typedef as CCryDXGLGIObject
struct IDXGIDeviceSubObject;
struct IDXGIResource;
struct IDXGIKeyedMutex;
struct IDXGISurface;
struct IDXGISurface1;
//struct IDXGIAdapter;              // Typedef as CCryDXGLGIAdapter
//struct IDXGIOutput;                   // Typedef as CCryDXGLGIOutput
//struct IDXGISwapChain;            // Typedef as CCryDXGLSwapChain
//struct IDXGIFactory;              // Typedef as CCryDXGLGIFactory
//struct IDXGIDevice;                   // Typedef as CCryDXGLDevice
//struct IDXGIFactory1;             // Typedef as CCryDXGLGIFactory
//struct IDXGIAdapter1;             // Typedef as CCryDXGLGIAdapter
//struct IDXGIDevice1;              // Typedef as CCryDXGLDevice

#endif //__DXGL_DXGI_h__