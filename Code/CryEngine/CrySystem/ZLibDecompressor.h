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

// Description : zlib inflate wrapper


#ifndef CRYINCLUDE_CRYSYSTEM_ZLIBDECOMPRESSOR_H
#define CRYINCLUDE_CRYSYSTEM_ZLIBDECOMPRESSOR_H
#pragma once


#include "IZlibDecompressor.h"

class CZLibDecompressor
    : public IZLibDecompressor
{
public:
    virtual IZLibInflateStream* CreateInflateStream();
    virtual void Release();

private:
    virtual ~CZLibDecompressor() {}
};

#endif // CRYINCLUDE_CRYSYSTEM_ZLIBDECOMPRESSOR_H
