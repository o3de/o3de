/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_EDITOR_UTIL_IMAGEASC_H
#define CRYINCLUDE_EDITOR_UTIL_IMAGEASC_H
#pragma once

#include "Util/Image.h"

class SANDBOX_API CImageASC
{
public:
    bool Load(const QString& fileName, CFloatImage& outImage);
    bool Save(const QString& fileName, const CFloatImage& image);
};


#endif // CRYINCLUDE_EDITOR_UTIL_IMAGEASC_H
