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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_CHUNKDATA_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_CHUNKDATA_H
#pragma once

struct CChunkData
{
    char* data;
    int size;

    CChunkData() { data = 0; size = 0; }
    ~CChunkData() { free(data); }

    template <class T>
    void Add(const T& object)
    {
        AddData(&object, sizeof(object));
    }
    void AddData(const void* pSrcData, int nSrcDataSize)
    {
        data = (char*)realloc(data, size + nSrcDataSize);
        memcpy(data + size, pSrcData, nSrcDataSize);
        size += nSrcDataSize;
    }
};


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_CHUNKDATA_H
