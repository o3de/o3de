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

#pragma once

#include <IShader.h> // <> required for Interfuscator

// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(IShader_info_h)
#elif !defined(LINUX) && !defined(APPLE)
#define ISHADER_INFO_H_TRAIT_DEFINE_ETEX_INFO 1
#endif

#if ISHADER_INFO_H_TRAIT_DEFINE_ETEX_INFO
ENUM_INFO_BEGIN(ETEX_Format)
ENUM_ELEM_INFO(, eTF_Unknown)
ENUM_ELEM_INFO(, eTF_R8G8B8A8S)
ENUM_ELEM_INFO(, eTF_R8G8B8A8)
ENUM_ELEM_INFO(, eTF_A8)
ENUM_ELEM_INFO(, eTF_R8)
ENUM_ELEM_INFO(, eTF_R8S)
ENUM_ELEM_INFO(, eTF_R16)
ENUM_ELEM_INFO(, eTF_R16F)
ENUM_ELEM_INFO(, eTF_R32F)
ENUM_ELEM_INFO(, eTF_R8G8)
ENUM_ELEM_INFO(, eTF_R8G8S)
ENUM_ELEM_INFO(, eTF_R16G16)
ENUM_ELEM_INFO(, eTF_R16G16S)
ENUM_ELEM_INFO(, eTF_R16G16F)
ENUM_ELEM_INFO(, eTF_R11G11B10F)
ENUM_ELEM_INFO(, eTF_R10G10B10A2)
ENUM_ELEM_INFO(, eTF_R16G16B16A16)
ENUM_ELEM_INFO(, eTF_R16G16B16A16S)
ENUM_ELEM_INFO(, eTF_R16G16B16A16F)
ENUM_ELEM_INFO(, eTF_R32G32B32A32F)
ENUM_ELEM_INFO(, eTF_CTX1)
ENUM_ELEM_INFO(, eTF_BC1)
ENUM_ELEM_INFO(, eTF_BC2)
ENUM_ELEM_INFO(, eTF_BC3)
ENUM_ELEM_INFO(, eTF_BC4U)
ENUM_ELEM_INFO(, eTF_BC4S)
ENUM_ELEM_INFO(, eTF_BC5U)
ENUM_ELEM_INFO(, eTF_BC5S)
ENUM_ELEM_INFO(, eTF_BC6UH)
ENUM_ELEM_INFO(, eTF_BC6SH)
ENUM_ELEM_INFO(, eTF_BC7)
ENUM_ELEM_INFO(, eTF_R9G9B9E5)
ENUM_ELEM_INFO(, eTF_D16)
ENUM_ELEM_INFO(, eTF_D24S8)
ENUM_ELEM_INFO(, eTF_D32F)
ENUM_ELEM_INFO(, eTF_D32FS8)
ENUM_ELEM_INFO(, eTF_B5G6R5)
ENUM_ELEM_INFO(, eTF_B5G5R5)
ENUM_ELEM_INFO(, eTF_B4G4R4A4)
ENUM_ELEM_INFO(, eTF_EAC_R11)
ENUM_ELEM_INFO(, eTF_EAC_RG11)
ENUM_ELEM_INFO(, eTF_ETC2)
ENUM_ELEM_INFO(, eTF_ETC2A)
ENUM_ELEM_INFO(, eTF_A8L8)
ENUM_ELEM_INFO(, eTF_L8)
ENUM_ELEM_INFO(, eTF_L8V8U8)
ENUM_ELEM_INFO(, eTF_B8G8R8)
ENUM_ELEM_INFO(, eTF_L8V8U8X8)
ENUM_ELEM_INFO(, eTF_B8G8R8X8)
ENUM_ELEM_INFO(, eTF_B8G8R8A8)
ENUM_INFO_END(ETEX_Format)
#endif
