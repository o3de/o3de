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

// Description : Opens a temporary file for read only access, where the file could be
//               located in a zip or pak file. Note that if the file specified
//               already exists it does not delete it when finished.


#include "TempFilePakExtraction.h"
#include "FileUtil.h"
#include "PathHelpers.h"
#include "IPakSystem.h"


TempFilePakExtraction::TempFilePakExtraction(const char* filename, const char* tempPath, IPakSystem* pPakSystem)
    : m_strOriginalFileName(filename)
    , m_strTempFileName(filename)
{
    if (!pPakSystem || !tempPath)
    {
        return;
    }

    {
        FILE* fileOnDisk = nullptr;
        azfopen(&fileOnDisk, m_strOriginalFileName.c_str(), "rb");
        if (fileOnDisk)
        {
            fclose(fileOnDisk);
            return;
        }
    }

    // Choose the name for the temporary file.
    string tempFullFileName;
    {
        uint32 tempNumber = 0;
        {
            LARGE_INTEGER performanceCount;
            if (QueryPerformanceCounter(&performanceCount))
            {
                tempNumber = performanceCount.u.LowPart;
            }
        }

        string tempName;
        {
            // CryEngine's pak system supports filenames in format "@pakFilename|fileInPak",
            // so let's handle such cases by using fileInPak part of the filename.
            const size_t pos = m_strOriginalFileName.find_last_of('|');
            if (pos != string::npos)
            {
                tempName = m_strOriginalFileName.substr(pos + 1, string::npos);
                if (tempName.empty())
                {
                    tempName = "BadFilenameSyntax";
                }
            }
            else
            {
                tempName = m_strOriginalFileName;
            }
            tempName = PathHelpers::GetFilename(tempName);
        }

        int tryCount = 2000;
        while (--tryCount >= 0)
        {
            tempFullFileName.Format("%sRC%04x_%s", tempPath, (tempNumber & 0xFFFF), tempName.c_str());

            if (!FileUtil::FileExists(tempFullFileName.c_str()))
            {
                FILE* f = nullptr;
                azfopen(&f, tempFullFileName.c_str(), "wb");
                if (f)
                {
                    fclose(f);
                    break;
                }
            }

            tempFullFileName.clear();
            ++tempNumber;
        }

        if (tempFullFileName.empty())
        {
            return;
        }
    }

    if (pPakSystem->ExtractNoOverwrite(m_strOriginalFileName.c_str(), tempFullFileName.c_str()))
    {
        m_strTempFileName = tempFullFileName;
        AZ::IO::SystemFile::SetWritable(m_strTempFileName.c_str(), false);
    }
    else
    {        
        AZ::IO::LocalFileIO().Remove(tempFullFileName.c_str());
    }
}


TempFilePakExtraction::~TempFilePakExtraction()
{
    if (HasTempFile())
    {
#if defined(AZ_PLATFORM_WINDOWS)
        SetFileAttributesA(m_strTempFileName.c_str(), FILE_ATTRIBUTE_ARCHIVE);
#endif        
        AZ::IO::LocalFileIO().Remove(m_strTempFileName.c_str());
    }
}


bool TempFilePakExtraction::HasTempFile() const
{
    return (m_strOriginalFileName != m_strTempFileName);
}
