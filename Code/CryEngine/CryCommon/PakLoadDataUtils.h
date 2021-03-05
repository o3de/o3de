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
#pragma once

#include <ISystem.h>
#include <AzFramework/Archive/IArchive.h>

namespace PakLoadDataUtils
{
    template <class T>
    static bool LoadDataFromFile(T* data, size_t elems, AZ::IO::HandleType& fileHandle, int& nDataSize, EEndian eEndian, int* pSeek = 0)
    {
        auto ipak = GetISystem()->GetIPak();
        if (pSeek)
        {
            *pSeek = aznumeric_cast<int>(ipak->FTell(fileHandle));
        }

        if (ipak->FRead(data, elems, fileHandle, eEndian) != elems)
        {
            AZ_Assert(false, "Failed to read %zu elements", elems);
            return false;
        }
        nDataSize -= sizeof(T) * elems;
        AZ_Assert(nDataSize >= 0, "nDataSize must be equal or greater than 0");
        return true;
    }

    bool LoadDataFromFile_Seek(size_t elems, AZ::IO::HandleType& fileHandle, int& nDataSize, [[maybe_unused]] EEndian eEndian);

    template <class T>
    static bool LoadDataFromFile(T* data, size_t elems, uint8*& f, int& nDataSize, EEndian eEndian, [[maybe_unused]] int* pSeek = 0)
    {
        StepDataCopy(data, f, elems, eEndian);
        nDataSize -= elems * sizeof(T);
        AZ_Assert(nDataSize >= 0, "nDataSize must be equal or greater than 0");
        return (nDataSize >= 0);
    }

    bool LoadDataFromFile_Seek(size_t elems, uint8*& f, int& nDataSize, [[maybe_unused]] EEndian eEndian);

    void LoadDataFromFile_FixAlignment(AZ::IO::HandleType& fileHandle, int& nDataSize);

    void LoadDataFromFile_FixAlignment(uint8*& f, int& nDataSize);

} //namespace PakLoadDataUtils
