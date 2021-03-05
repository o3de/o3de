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

#include "ResourceCompilerPC_precompiled.h"
#include "ConvertContext.h"
#include "ChunkCompiler.h"
#include "../Cry3DEngine/CGF/ChunkFileReaders.h"
#include "../Cry3DEngine/CGF/ChunkFileWriters.h"
#include "CryHeaders.h"
#include "IChunkFile.h"
#include "IConfig.h"
#include "IResCompiler.h"
#include "IResourceCompilerHelper.h"
#include "StringHelpers.h"
#include "UpToDateFileHelpers.h"


static bool writeChunkFile(
    ChunkFile::MemorylessChunkFileWriter::EChunkFileFormat eFormat,
    const char* dstFilename,
    ChunkFile::IReader* pReader,
    const char* srcFilename,
    const std::vector<IChunkFile::ChunkDesc>& chunks)
{
    ChunkFile::OsFileWriter writer;

    if (!writer.Create(dstFilename))
    {
        RCLogError("Failed to create '%s'", dstFilename);
        return false;
    }

    ChunkFile::MemorylessChunkFileWriter wr(eFormat, &writer);

    wr.SetAlignment(4);

    while (wr.StartPass())
    {
        for (uint32 i = 0; i < chunks.size(); ++i)
        {
            wr.StartChunk((chunks[i].bSwapEndian ? eEndianness_NonNative : eEndianness_Native), chunks[i].chunkType, chunks[i].chunkVersion, chunks[i].chunkId);

            uint32 srcSize = chunks[i].size;
            if (srcSize <= 0)
            {
                continue;
            }

            if (!pReader->SetPos(chunks[i].fileOffset) != 0)
            {
                RCLogError("Failed to read (seek) file %s.", srcFilename);
                return false;
            }

            while (srcSize > 0)
            {
                char bf[4 * 1024];
                const uint32 sz = Util::getMin((uint32)sizeof(bf), srcSize);
                srcSize -= sz;

                if (!pReader->Read(bf, sz))
                {
                    RCLogError("Failed to read %u byte(s) from file %s.", (uint)sz, srcFilename);
                    return false;
                }

                wr.AddChunkData(bf, sz);
            }
        }
    }

    if (!wr.HasWrittenSuccessfully())
    {
        RCLogError("Failed to write %s.", dstFilename);
        return false;
    }

    return true;
}


static bool convertChunkFile(const uint32 version, const char* srcFilename, const char* dstFilename)
{
    if (srcFilename == 0 || srcFilename[0] == 0 || dstFilename == 0 || dstFilename[0] == 0)
    {
        RCLogError("Empty name of a chunk file. Contact RC programmer.");
        return false;
    }

    ChunkFile::CryFileReader f;

    if (!f.Open(srcFilename))
    {
        RCLogError("File to open file %s for reading", srcFilename);
        return false;
    }

    std::vector<IChunkFile::ChunkDesc> chunks;

    string s;
    s = ChunkFile::GetChunkTableEntries_0x746(&f, chunks);
    if (!s.empty())
    {
        s = ChunkFile::GetChunkTableEntries_0x744_0x745(&f, chunks);
        if (s.empty())
        {
            s = ChunkFile::StripChunkHeaders_0x744_0x745(&f, chunks);
        }
    }

    if (!s.empty())
    {
        RCLogError("%s", s.c_str());
        return false;
    }

    const ChunkFile::MemorylessChunkFileWriter::EChunkFileFormat eFormat =
        (version == 0x745)
        ? ChunkFile::MemorylessChunkFileWriter::eChunkFileFormat_0x745
        : ChunkFile::MemorylessChunkFileWriter::eChunkFileFormat_0x746;

    const bool bOk = writeChunkFile(eFormat, dstFilename, &f, srcFilename, chunks);
    return bOk;
}


//////////////////////////////////////////////////////////////////////////
CChunkCompiler::CChunkCompiler()
{
    m_refCount = 1;
}


CChunkCompiler::~CChunkCompiler()
{
}


//////////////////////////////////////////////////////////////////////////
// ICompiler + IConvertor methods.
//////////////////////////////////////////////////////////////////////////
void CChunkCompiler::Release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}


//////////////////////////////////////////////////////////////////////////
// ICompiler methods.
//////////////////////////////////////////////////////////////////////////
string CChunkCompiler::GetOutputFileNameOnly() const
{
    const string sourceFileFinal = m_CC.m_config->GetAsString("overwritefilename", m_CC.m_sourceFileNameOnly.c_str(), m_CC.m_sourceFileNameOnly.c_str());
    return sourceFileFinal;
}


string CChunkCompiler::GetOutputPath() const
{
    return PathHelpers::Join(m_CC.GetOutputFolder(), GetOutputFileNameOnly());
}


bool CChunkCompiler::Process()
{
    const string sourceFile = m_CC.GetSourcePath();
    const string outputFile = GetOutputPath();

    if (!m_CC.m_bForceRecompiling && UpToDateFileHelpers::FileExistsAndUpToDate(GetOutputPath(), m_CC.GetSourcePath()))
    {
        // The file is up-to-date
        m_CC.m_pRC->AddInputOutputFilePair(m_CC.GetSourcePath(), GetOutputPath());
        return true;
    }

    if (m_CC.m_config->GetAsBool("SkipMissing", false, true))
    {
        // Skip missing source files.
        const DWORD dwFileSpecAttr = GetFileAttributes(sourceFile.c_str());
        if (dwFileSpecAttr == INVALID_FILE_ATTRIBUTES)
        {
            // Skip missing file instead of reporting it as an error.
            return true;
        }
    }

    bool bOk = false;

#if !defined(AZ_PLATFORM_LINUX) && !defined(AZ_PLATFORM_APPLE) // Exception handling not enabled on linux/mac builds
    try
#endif // !defined(AZ_PLATFORM_LINUX)
    {
        const uint32 version =
            StringHelpers::EndsWith(m_CC.m_config->GetAsString("targetversion", "0x746", "0x746"), "745")
            ? 0x745
            : 0x746;
        bOk = convertChunkFile(version, sourceFile.c_str(), outputFile.c_str());
    }
#if !defined(AZ_PLATFORM_LINUX) && !defined(AZ_PLATFORM_APPLE)// Exception handling not enabled on linux builds
    catch (char*)
    {
        RCLogError("Unexpected failure in processing %s - contact an RC programmer.", sourceFile.c_str());
        return false;
    }
#endif // !defined(AZ_PLATFORM_LINUX) && !defined(AZ_PLATFORM_APPLE)

    if (bOk)
    {
        if (!UpToDateFileHelpers::SetMatchingFileTime(GetOutputPath(), m_CC.GetSourcePath()))
        {
            return false;
        }
        m_CC.m_pRC->AddInputOutputFilePair(m_CC.GetSourcePath(), GetOutputPath());
    }
    return bOk;
}


//////////////////////////////////////////////////////////////////////////
// IConvertor methods.
//////////////////////////////////////////////////////////////////////////
ICompiler* CChunkCompiler::CreateCompiler()
{
    // Only ever return one compiler, since we don't support multithreading. Since
    // the compiler is just this object, we can tell whether we have already returned
    // a compiler by checking the ref count.
    if (m_refCount >= 2)
    {
        return 0;
    }

    // Until we support multithreading for this convertor, the compiler and the
    // convertor may as well just be the same object.
    ++m_refCount;
    return this;
}


const char* CChunkCompiler::GetExt(int index) const
{
    return (index == 0) ? "chunk" : 0;
}
