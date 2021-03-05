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

// Description : Provides the interface for the lz4 hc decompress wrapper

#ifndef CRYINCLUDE_CRYCOMMON_ILZ4DECOMPRESSOR_H
#define CRYINCLUDE_CRYCOMMON_ILZ4DECOMPRESSOR_H
#pragma once


struct ILZ4Decompressor
{
protected:
    virtual ~ILZ4Decompressor() {};     // use Release()

public:
    virtual bool DecompressData(const char* pIn, char* pOut, const uint outputSize) const = 0;

    virtual void Release() = 0;
};

#endif // CRYINCLUDE_CRYCOMMON_ILZ4DECOMPRESSOR_H
