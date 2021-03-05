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

#ifndef CRYINCLUDE_CRYSYSTEM_ZLIBCOMPRESSOR_H
#define CRYINCLUDE_CRYSYSTEM_ZLIBCOMPRESSOR_H
#pragma once


#include "IZLibCompressor.h"

class CZLibCompressor
    : public IZLibCompressor
{
protected:
    virtual ~CZLibCompressor();

public:
    virtual IZLibDeflateStream* CreateDeflateStream(int inLevel, EZLibMethod inMethod, int inWindowBits, int inMemLevel, EZLibStrategy inStrategy, EZLibFlush inFlushMethod);
    virtual void                                        Release();

    virtual void                                        MD5Init(SMD5Context* pIOCtx);
    virtual void                                        MD5Update(SMD5Context* pIOCtx, const char* pInBuff, unsigned int len);
    virtual void                                        MD5Final(SMD5Context * pIOCtx, char outDigest[16]);
};

#endif // CRYINCLUDE_CRYSYSTEM_ZLIBCOMPRESSOR_H
