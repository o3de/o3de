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

#ifndef CRYINCLUDE_CRYCOMMON_IMAGEEXTENSIONHELPER_INFO_H
#define CRYINCLUDE_CRYCOMMON_IMAGEEXTENSIONHELPER_INFO_H
#pragma once

#include "CryString.h"
#include "TypeInfo_decl.h"
#include "ImageExtensionHelper.h"

// Crytek specific image extensions
//
// usually added to the end of DDS files


STRUCT_INFO_BEGIN(CImageExtensionHelper::DDS_PIXELFORMAT)
STRUCT_VAR_INFO(dwSize, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwFlags, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwFourCC, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwRGBBitCount, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwRBitMask, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwGBitMask, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwBBitMask, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwABitMask, TYPE_INFO(DWORD))
STRUCT_INFO_END(CImageExtensionHelper::DDS_PIXELFORMAT)

STRUCT_INFO_BEGIN(CImageExtensionHelper::DDS_HEADER_DXT10)
STRUCT_VAR_INFO(dxgiFormat, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(resourceDimension, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(miscFlag, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(arraySize, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(reserved, TYPE_INFO(DWORD))
STRUCT_INFO_END(CImageExtensionHelper::DDS_HEADER_DXT10)

STRUCT_INFO_BEGIN(CImageExtensionHelper::DDS_HEADER)
STRUCT_VAR_INFO(dwSize, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwHeaderFlags, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwHeight, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwWidth, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwPitchOrLinearSize, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwDepth, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwMipMapCount, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwAlphaBitDepth, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwReserved1, TYPE_ARRAY(10, TYPE_INFO(DWORD)))
STRUCT_VAR_INFO(ddspf, TYPE_INFO(CImageExtensionHelper::DDS_PIXELFORMAT))
STRUCT_VAR_INFO(dwSurfaceFlags, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(dwCubemapFlags, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(bNumPersistentMips, TYPE_INFO(BYTE))
STRUCT_VAR_INFO(bReserved2, TYPE_ARRAY(7, TYPE_INFO(BYTE)))
STRUCT_VAR_INFO(dwTextureStage, TYPE_INFO(DWORD))
STRUCT_INFO_END(CImageExtensionHelper::DDS_HEADER)

STRUCT_INFO_BEGIN(CImageExtensionHelper::DDS_FILE_DESC)
STRUCT_VAR_INFO(dwMagic, TYPE_INFO(DWORD))
STRUCT_VAR_INFO(header, TYPE_INFO(CImageExtensionHelper::DDS_HEADER))
STRUCT_INFO_END(CImageExtensionHelper::DDS_FILE_DESC)

#endif // CRYINCLUDE_CRYCOMMON_IMAGEEXTENSIONHELPER_INFO_H
