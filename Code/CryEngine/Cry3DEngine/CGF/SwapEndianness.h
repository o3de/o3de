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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_SWAPENDIANNESS_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_SWAPENDIANNESS_H
#pragma once



inline void SwapEndians_(void* pData, size_t nCount, size_t nSizeCheck)
{
    // Primitive type.
    switch (nSizeCheck)
    {
    case 1:
        break;
    case 2:
    {
        while (nCount--)
        {
            uint16& i = *((uint16*&)pData)++;
            i = ((i >> 8) + (i << 8)) & 0xFFFF;
        }
        break;
    }
    case 4:
    {
        while (nCount--)
        {
            uint32& i = *((uint32*&)pData)++;
            i = (i >> 24) + ((i >> 8) & 0xFF00) + ((i & 0xFF00) << 8) + (i << 24);
        }
        break;
    }
    case 8:
    {
        while (nCount--)
        {
            uint64& i = *((uint64*&)pData)++;
            i = (i >> 56) + ((i >> 40) & 0xFF00) + ((i >> 24) & 0xFF0000) + ((i >> 8) & 0xFF000000)
                + ((i & 0xFF000000) << 8) + ((i & 0xFF0000) << 24) + ((i & 0xFF00) << 40) + (i << 56);
        }
        break;
    }
    default:
        assert(0);
    }
}


template<class T>
void SwapEndianness(T* t, std::size_t count)
{
    SwapEndians_(t, count, sizeof(T));
}


template<class T>
void SwapEndianness(T& t)
{
    SwapEndianness(&t, 1);
}


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_SWAPENDIANNESS_H
