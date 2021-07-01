/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_IMAGETIF_H
#define CRYINCLUDE_EDITOR_UTIL_IMAGETIF_H
#pragma once

#include "Util/Image.h"

class SANDBOX_API CImageTIF
{
public:
    bool Load(const QString& fileName, CImageEx& outImage);
    bool Load(const QString& fileName, CFloatImage& outImage);
    bool SaveRAW(const QString& fileName, const void* pData, int width, int height, int bytesPerChannel, int numChannels, bool bFloat, const char* preset);
    static const char* GetPreset(const QString& fileName);
};


#endif // CRYINCLUDE_EDITOR_UTIL_IMAGETIF_H
