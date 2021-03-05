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

// Description : lz4 hc decompress wrapper


#include "CrySystem_precompiled.h"
#include <lz4.h>
#include "LZ4Decompressor.h"

bool CLZ4Decompressor::DecompressData(const char* pIn, char* pOut, const uint outputSize) const
{
    return LZ4_decompress_fast(pIn, pOut, outputSize) >= 0;
}

void CLZ4Decompressor::Release()
{
    delete this;
}