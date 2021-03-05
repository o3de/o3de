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

#include "CrySystem_precompiled.h"
#include "System.h"
#include "CryZlib.h"

bool CSystem::CompressDataBlock(const void* input, size_t inputSize, void* output, size_t& outputSize, int level)
{
    uLongf destLen = outputSize;
    Bytef* dest = static_cast<Bytef*>(output);
    uLong sourceLen = inputSize;
    const Bytef* source = static_cast<const Bytef*>(input);
    bool ok = Z_OK == compress2(dest, &destLen, source, sourceLen, level);
    outputSize = destLen;
    return ok;
}

bool CSystem::DecompressDataBlock(const void* input, size_t inputSize, void* output, size_t& outputSize)
{
    uLongf destLen = outputSize;
    Bytef* dest = static_cast<Bytef*>(output);
    uLong sourceLen = inputSize;
    const Bytef* source = static_cast<const Bytef*>(input);
    bool ok = Z_OK == uncompress(dest, &destLen, source, sourceLen);
    outputSize = destLen;
    return ok;
}
