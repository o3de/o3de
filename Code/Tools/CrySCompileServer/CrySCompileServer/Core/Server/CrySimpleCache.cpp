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

#include "CrySimpleCache.hpp"
#include "CrySimpleServer.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/time.h>
#include <AzCore/std/algorithm.h>

enum EFileEntryHeaderFlags
{
    EFEHF_NONE                  =   (0 << 0),
    EFEHF_REFERENCE     =   (1 << 0),
};

#pragma pack(push, 1)
struct SFileEntryHeader
{
    char     signature[4]; // entry signature.
    uint32_t dataSize;  // Size of entry data.
    uint32_t flags;     // Flags
    uint8_t  hash[16];  // Hash code for the data.
};
#pragma pack(pop)

static const int MAX_DATA_SIZE = 1024 * 1024;

CCrySimpleCache& CCrySimpleCache::Instance()
{
    static CCrySimpleCache g_Cache;
    return g_Cache;
}

void CCrySimpleCache::Init()
{
    CCrySimpleMutexAutoLock Lock(m_Mutex);
    m_CachingEnabled    =   false;
    m_Hit       =   0;
    m_Miss  =   0;
}

std::string CCrySimpleCache::CreateFileName(const tdHash& rHash) const
{
    std::string Name;
    Name    =   CSTLHelper::Hash2String(rHash);
    char Tmp[4] = "012";
    Tmp[0]  =   Name.c_str()[0];
    Tmp[1]  =   Name.c_str()[1];
    Tmp[2]  =   Name.c_str()[2];

    return SEnviropment::Instance().m_CachePath + Tmp + "/" + Name;
}


bool CCrySimpleCache::Find(const tdHash& rHash, tdDataVector& rData)
{
    if (!m_CachingEnabled)
    {
        return false;
    }

    CCrySimpleMutexAutoLock Lock(m_Mutex);
    tdEntries::iterator it = m_Entries.find(rHash);
    if (it != m_Entries.end())
    {
        tdData::iterator dataIt = m_Data.find(it->second);
        if (dataIt == m_Data.end())
        {
            m_Miss++;
            return false;
        }
        m_Hit++;
        rData = dataIt->second;
        return true;
    }
    m_Miss++;
    return false;
}

void CCrySimpleCache::Add(const tdHash& rHash, const tdDataVector& rData)
{
    if (!m_CachingEnabled)
    {
        return;
    }
    if (rData.size() > 0)
    {
        SFileEntryHeader hdr;
        memcpy(hdr.signature, "SHDR", 4);
        hdr.dataSize = (uint32_t)rData.size();
        hdr.flags = EFEHF_NONE;
        memcpy(hdr.hash, &rHash, sizeof(hdr.hash));
        const uint8_t* pData    =   &rData[0];

        tdHash DataHash = CSTLHelper::Hash(rData);
        {
            CCrySimpleMutexAutoLock Lock(m_Mutex);
            m_Entries[rHash]  =   DataHash;
            if (m_Data.find(DataHash) == m_Data.end())
            {
                m_Data[DataHash]    =   rData;
            }
            else
            {
                hdr.flags |= EFEHF_REFERENCE;
                hdr.dataSize    =   sizeof(tdHash);
                pData   =   reinterpret_cast<const uint8_t*>(&DataHash);
            }
        }



        tdDataVector buf;
        buf.resize(sizeof(hdr) + hdr.dataSize);
        memcpy(&buf[0], &hdr, sizeof(hdr));
        memcpy(&buf[sizeof(hdr)], pData, hdr.dataSize);

        tdDataVector* pPendingCacheEntry = new tdDataVector(buf);
        {
            CCrySimpleMutexAutoLock LockFile(m_FileMutex);
            m_PendingCacheEntries.push_back(pPendingCacheEntry);
            if (m_PendingCacheEntries.size() > 10000)
            {
                printf("Warning: Too many pending entries not saved to disk!!!");
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CCrySimpleCache::LoadCacheFile(const std::string& filename)
{
    AZ::u64 startTimeInMillis = AZStd::GetTimeUTCMilliSecond();

    printf("Loading shader cache from %s\n", filename.c_str());

    tdDataVector rData;

    tdHash hash;

    bool bLoadedOK = true;

    uint32_t Loaded = 0;
    uint32_t num = 0;

    uint64_t nFilePos = 0;
    uint64_t nFilePos2 = 0;

    //////////////////////////////////////////////////////////////////////////
    AZ::IO::SystemFile cacheFile;
    const bool wasSuccessful = cacheFile.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
    if (!wasSuccessful)
    {
        return false;
    }

    AZ::IO::SystemFile::SizeType fileSize = cacheFile.Length();

    uint64_t SizeAdded = 0;
    uint64_t SizeAddedCount = 0;
    uint64_t SizeSaved = 0;
    uint64_t SizeSavedCount = 0;

    while (nFilePos < fileSize)
    {
        SFileEntryHeader hdr;
        AZ::IO::SystemFile::SizeType bytesReadIn = cacheFile.Read(sizeof(hdr), &hdr);
        if (bytesReadIn != sizeof(hdr))
        {
            break;
        }

        if (memcmp(hdr.signature, "SHDR", 4) != 0)
        {
            // Bad Entry!
            bLoadedOK = false;
            printf("\nSkipping Invalid cache entry %d\n at file position: %llu because signature is bad", num, nFilePos);
            break;
        }

        if (hdr.dataSize > MAX_DATA_SIZE || hdr.dataSize == 0)
        {
            // Too big entry, probably invalid.
            bLoadedOK = false;
            printf("\nSkipping Invalid cache entry %d\n at file position: %llu because data size is too big", num, nFilePos);
            break;
        }

        rData.resize(hdr.dataSize);
        bytesReadIn = cacheFile.Read(hdr.dataSize, rData.data());
        if (bytesReadIn != hdr.dataSize)
        {
            break;
        }
        memcpy(&hash, hdr.hash, sizeof(hdr.hash));

        if (hdr.flags & EFEHF_REFERENCE)
        {
            if (hdr.dataSize != sizeof(tdHash))
            {
                // Too big entry, probably invalid.
                bLoadedOK = false;
                printf("\nSkipping Invalid cache entry %d\n at file position: %llu, was flagged as cache reference but size was %d", num, nFilePos, hdr.dataSize);
                break;
            }

            bool bSkip = false;

            tdHash DataHash =   *reinterpret_cast<tdHash*>(&rData[0]);
            tdData::iterator it = m_Data.find(DataHash);
            if (it == m_Data.end())
            {
                // Too big entry, probably invalid.
                bSkip = true; // don't abort reading whole file just yet - skip only this entry
                printf("\nSkipping Invalid cache entry %d\n at file position: %llu, data-hash references to not existing data ", num, nFilePos);
            }

            if (!bSkip)
            {
                m_Entries[hash] =   DataHash;
                SizeSaved += it->second.size();
                SizeSavedCount++;
            }
        }
        else
        {
            tdHash DataHash = CSTLHelper::Hash(rData);
            m_Entries[hash] =   DataHash;
            if (m_Data.find(DataHash) == m_Data.end())
            {
                SizeAdded += rData.size();
                m_Data[DataHash]    =   rData;
                SizeAddedCount++;
            }
            else
            {
                SizeSaved += rData.size();
                SizeSavedCount++;
            }
        }

        if (num % 1000 == 0)
        {
            AZ::u64 endTimeInMillis = AZStd::GetTimeUTCMilliSecond();

            Loaded  =   static_cast<uint32_t>(nFilePos * 100 / fileSize);
            printf("\rLoad:%3u%% %6uk t=%llus Compress: (Count)%llu%% %lluk:%lluk (MB)%llu%% %lluMB:%lluMB", Loaded, num / 1000u, (endTimeInMillis - startTimeInMillis),
                SizeAddedCount / AZStd::GetMax((SizeAddedCount + SizeSavedCount) / 100ull, 1ull),
                SizeAddedCount / 1000, SizeSavedCount / 1000,
                SizeAdded / AZStd::GetMax((SizeAdded + SizeSaved) / 100ull, 1ull),
                SizeAdded / (MAX_DATA_SIZE), SizeSaved / (MAX_DATA_SIZE));
        }

        num++;
        nFilePos += hdr.dataSize + sizeof(SFileEntryHeader);
    }

    printf("\n%d shaders loaded from cache\n", num);

    return bLoadedOK;
}

void CCrySimpleCache::Finalize()
{
    m_CachingEnabled    =   true;
    printf("\n caching enabled\n");
}

//////////////////////////////////////////////////////////////////////////
void CCrySimpleCache::ThreadFunc_SavePendingCacheEntries()
{
    // Check pending entries and save them to disk.
    bool bListEmpty = false;
    do
    {
        tdDataVector* pPendingCacheEntry = 0;

        {
            CCrySimpleMutexAutoLock LockFile(m_FileMutex);
            if (!m_PendingCacheEntries.empty())
            {
                pPendingCacheEntry = m_PendingCacheEntries.front();
                m_PendingCacheEntries.pop_front();
            }
            //CSTLHelper::AppendToFile( SEnviropment::Instance().m_CachePath+"Cache.dat",buf );
            bListEmpty = m_PendingCacheEntries.empty();
        }

        if (pPendingCacheEntry)
        {
            CSTLHelper::AppendToFile(SEnviropment::Instance().m_CachePath + "Cache.dat", *pPendingCacheEntry);
            delete pPendingCacheEntry;
        }
    } while (!bListEmpty);
}
