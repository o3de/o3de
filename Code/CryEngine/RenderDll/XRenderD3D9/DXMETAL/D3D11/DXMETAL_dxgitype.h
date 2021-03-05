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

//  Description: Contains portable definition of structs and enums to match
//               those in dxgitype.h in the DirectX SDK

#ifndef __DXGL_dxgitype_h__
#define __DXGL_dxgitype_h__

#include <AzDXGIFormat.h>

////////////////////////////////////////////////////////////////////////////
//  Defines
////////////////////////////////////////////////////////////////////////////

#define _FACDXGI    0x87a
#define MAKE_DXGI_HRESULT(code) MAKE_HRESULT(1, _FACDXGI, code)
#define MAKE_DXGI_STATUS(code)  MAKE_HRESULT(0, _FACDXGI, code)

#define DXGI_STATUS_OCCLUDED                    MAKE_DXGI_STATUS(1)
#define DXGI_STATUS_CLIPPED                     MAKE_DXGI_STATUS(2)
#define DXGI_STATUS_NO_REDIRECTION              MAKE_DXGI_STATUS(4)
#define DXGI_STATUS_NO_DESKTOP_ACCESS           MAKE_DXGI_STATUS(5)
#define DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE MAKE_DXGI_STATUS(6)
#define DXGI_STATUS_MODE_CHANGED                MAKE_DXGI_STATUS(7)
#define DXGI_STATUS_MODE_CHANGE_IN_PROGRESS     MAKE_DXGI_STATUS(8)


#define DXGI_ERROR_INVALID_CALL                 MAKE_DXGI_HRESULT(1)
#define DXGI_ERROR_NOT_FOUND                    MAKE_DXGI_HRESULT(2)
#define DXGI_ERROR_MORE_DATA                    MAKE_DXGI_HRESULT(3)
#define DXGI_ERROR_UNSUPPORTED                  MAKE_DXGI_HRESULT(4)
#define DXGI_ERROR_DEVICE_REMOVED               MAKE_DXGI_HRESULT(5)
#define DXGI_ERROR_DEVICE_HUNG                  MAKE_DXGI_HRESULT(6)
#define DXGI_ERROR_DEVICE_RESET                 MAKE_DXGI_HRESULT(7)
#define DXGI_ERROR_WAS_STILL_DRAWING            MAKE_DXGI_HRESULT(10)
#define DXGI_ERROR_FRAME_STATISTICS_DISJOINT    MAKE_DXGI_HRESULT(11)
#define DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE MAKE_DXGI_HRESULT(12)
#define DXGI_ERROR_DRIVER_INTERNAL_ERROR        MAKE_DXGI_HRESULT(32)
#define DXGI_ERROR_NONEXCLUSIVE                 MAKE_DXGI_HRESULT(33)
#define DXGI_ERROR_NOT_CURRENTLY_AVAILABLE      MAKE_DXGI_HRESULT(34)
#define DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED   MAKE_DXGI_HRESULT(35)
#define DXGI_ERROR_REMOTE_OUTOFMEMORY           MAKE_DXGI_HRESULT(36)


////////////////////////////////////////////////////////////////////////////
//  Structs
////////////////////////////////////////////////////////////////////////////

typedef struct DXGI_RGB
{
    float Red;
    float Green;
    float Blue;
} DXGI_RGB;

typedef struct DXGI_GAMMA_CONTROL
{
    DXGI_RGB Scale;
    DXGI_RGB Offset;
    DXGI_RGB GammaCurve[ 1025 ];
} DXGI_GAMMA_CONTROL;

typedef struct DXGI_GAMMA_CONTROL_CAPABILITIES
{
    BOOL ScaleAndOffsetSupported;
    float MaxConvertedValue;
    float MinConvertedValue;
    UINT NumGammaControlPoints;
    float ControlPointPositions[1025];
} DXGI_GAMMA_CONTROL_CAPABILITIES;

typedef struct DXGI_RATIONAL
{
    UINT Numerator;
    UINT Denominator;
} DXGI_RATIONAL;

typedef enum DXGI_MODE_SCANLINE_ORDER
{
    DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED        = 0,
    DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE        = 1,
    DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST  = 2,
    DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST  = 3
} DXGI_MODE_SCANLINE_ORDER;

typedef enum DXGI_MODE_SCALING
{
    DXGI_MODE_SCALING_UNSPECIFIED   = 0,
    DXGI_MODE_SCALING_CENTERED      = 1,
    DXGI_MODE_SCALING_STRETCHED     = 2
} DXGI_MODE_SCALING;

typedef enum DXGI_MODE_ROTATION
{
    DXGI_MODE_ROTATION_UNSPECIFIED  = 0,
    DXGI_MODE_ROTATION_IDENTITY     = 1,
    DXGI_MODE_ROTATION_ROTATE90     = 2,
    DXGI_MODE_ROTATION_ROTATE180    = 3,
    DXGI_MODE_ROTATION_ROTATE270    = 4
} DXGI_MODE_ROTATION;

typedef struct DXGI_MODE_DESC
{
    UINT Width;
    UINT Height;
    DXGI_RATIONAL RefreshRate;
    DXGI_FORMAT Format;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling;
} DXGI_MODE_DESC;

typedef struct DXGI_SAMPLE_DESC
{
    UINT Count;
    UINT Quality;
} DXGI_SAMPLE_DESC;

#endif // __DXGL_dxgitype_h__
