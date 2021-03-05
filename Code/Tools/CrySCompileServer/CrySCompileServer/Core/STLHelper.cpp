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

#include "StdTypes.hpp"
#include "Error.hpp"
#include "STLHelper.hpp"

#include <AzCore/PlatformDef.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Casting/lossy_cast.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <io.h>
#endif

#include "MD5.hpp"
#include <assert.h>
#include <zlib.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <cstdio>

void CSTLHelper::Log(const std::string& rLog)
{
    const std::string Output = rLog + "\n";
    logmessage(Output.c_str());
}

void CSTLHelper::Tokenize(tdEntryVec& rRet, const std::string& Tokens, const std::string& Separator)
{
    rRet.clear();
    std::string::size_type Pt;
    std::string::size_type Start    = 0;
    std::string::size_type SSize    =   Separator.size();

    while ((Pt = Tokens.find(Separator, Start)) != std::string::npos)
    {
        std::string  SubStr =   Tokens.substr(Start, Pt - Start);
        rRet.push_back(SubStr);
        Start = Pt + SSize;
    }

    rRet.push_back(Tokens.substr(Start));
}


void CSTLHelper::Replace(std::string& rRet, const std::string& rSrc, const std::string& rToReplace, const std::string& rReplacement)
{
    std::vector<uint8_t> Out;
    std::vector<uint8_t> In(rSrc.c_str(), rSrc.c_str() + rSrc.size() + 1);
    Replace(Out, In, rToReplace, rReplacement);
    rRet    =   std::string(reinterpret_cast<char*>(&Out[0]));
}

void CSTLHelper::Replace(std::vector<uint8_t>& rRet, const std::vector<uint8_t>& rTokenSrc, const std::string& rToReplace, const std::string& rReplacement)
{
    rRet.clear();
    size_t  SSize   =   rToReplace.size();
    for (size_t a = 0, Size = rTokenSrc.size(); a < Size; a++)
    {
        if (a + SSize < Size && strncmp((const char*)&rTokenSrc[a], rToReplace.c_str(), SSize) == 0)
        {
            for (size_t b = 0, RSize = rReplacement.size(); b < RSize; b++)
            {
                rRet.push_back(rReplacement.c_str()[b]);
            }
            a += SSize - 1;
        }
        else
        {
            rRet.push_back(rTokenSrc[a]);
        }
    }
}

tdToken CSTLHelper::SplitToken(const std::string& rToken, const std::string& rSeparator)
{
#undef min
    using namespace std;
    string Token;
    Remove(Token, rToken, ' ');

    string::size_type Pt = Token.find(rSeparator);
    return tdToken(Token.substr(0, Pt), Token.substr(std::min(Pt + 1, Token.size())));
}

void CSTLHelper::Splitizer(tdTokenList& rTokenList, const tdEntryVec& rFilter, const std::string& rSeparator)
{
    rTokenList.clear();
    for (size_t a = 0, Size = rFilter.size(); a < Size; a++)
    {
        rTokenList.push_back(SplitToken(rFilter[a], rSeparator));
    }
}

void CSTLHelper::Trim(std::string& rStr, const std::string& charsToTrim)
{
    std::string::size_type Pt1 = rStr.find_first_not_of(charsToTrim);
    if (Pt1 == std::string::npos)
    {
        // At this point the string could be empty or it could only contain 'charsToTrim' characters.
        // In case it's the later then trim should be applied by leaving the string empty.
        rStr = "";
        return;
    }

    std::string::size_type Pt2 = rStr.find_last_not_of(charsToTrim) + 1;

    Pt2     =   Pt2 - Pt1;
    rStr    =   rStr.substr(Pt1, Pt2);
}

void CSTLHelper::Remove(std::string& rTokenDst, const std::string& rTokenSrc, const char C)
{
    using namespace std;
    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
    remove_copy_if(rTokenSrc.begin(), rTokenSrc.end(), back_inserter(rTokenDst), [C](char token) { return token == C; });
    AZ_POP_DISABLE_WARNING
}


bool CSTLHelper::ToFile(const std::string& rFileName, const std::vector<uint8_t>&    rOut)
{
    if (rOut.size() == 0)
    {
        return false;
    }

    AZ::IO::SystemFile outputFile;
    const bool wasSuccessful = outputFile.Open(rFileName.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE);

    if (wasSuccessful == false)
    {
        AZ_Error("ShaderCompiler", wasSuccessful, "CSTLHelper::ToFile Could not create file: %s", rFileName.c_str());
        return false;
    }

    outputFile.Write(&rOut[0], rOut.size());

    return true;
}

bool CSTLHelper::FromFile(const std::string& rFileName, std::vector<uint8_t>&    rIn)
{
    AZ::IO::SystemFile inputFile;
    bool wasSuccess = inputFile.Open(rFileName.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_WRITE);
    if (!wasSuccess)
    {
        return false;
    }

    AZ::IO::SystemFile::SizeType fileSize = inputFile.Length();
    if (fileSize <= 0)
    {
        return false;
    }

    size_t nNumRead = 0;
    rIn.resize(fileSize);
    AZ::IO::SystemFile::SizeType actualReadAmount = inputFile.Read(fileSize, &rIn[0]);

    return actualReadAmount == fileSize;
}

bool CSTLHelper::ToFileCompressed(const std::string& rFileName, const std::vector<uint8_t>&  rOut)
{
    std::vector<uint8_t> buf;

    unsigned long sourceLen = (unsigned long)rOut.size();
    unsigned long destLen = compressBound(sourceLen) + 16;

    buf.resize(destLen);
    compress(buf.data(), &destLen, &rOut[0], sourceLen);

    if (destLen > 0)
    {
        AZ::IO::SystemFile outputFile;
        const bool wasSuccessful = outputFile.Open(rFileName.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE);
        AZ_Error("ShaderCompiler", wasSuccessful, "Could not create compressed file: %s", rFileName.c_str());

        if (wasSuccessful == false)
        {
            return false;
        }

        AZ::IO::SystemFile::SizeType bytesWritten = outputFile.Write(&sourceLen, sizeof(sourceLen));
        AZ_Error("ShaderCompiler", bytesWritten == sizeof(sourceLen), "Could not save out size of compressed data to %s", rFileName.c_str());

        if (bytesWritten != sizeof(sourceLen))
        {
            return false;
        }

        bytesWritten = outputFile.Write(buf.data(), destLen);
        AZ_Error("ShaderCompiler", bytesWritten == destLen, "Could not save out compressed data to %s", rFileName.c_str());

        if (bytesWritten != destLen)
        {
            return false;
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool CSTLHelper::FromFileCompressed(const std::string& rFileName, std::vector<uint8_t>&  rIn)
{
    std::vector<uint8_t> buf;
    AZ::IO::SystemFile inputFile;
    const bool wasSuccessful = inputFile.Open(rFileName.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
    AZ_Error("ShaderCompiler", wasSuccessful, "Could not read: ", rFileName.c_str());

    if (!wasSuccessful)
    {
        return false;
    }

    AZ::IO::SystemFile::SizeType FileLen = inputFile.Length();
    AZ_Error("ShaderCompiler", FileLen > 0, "Error getting file-size of ", rFileName.c_str());

    if (FileLen <= 0)
    {
        return false;
    }

    unsigned long uncompressedLen = 0;
    // Possible, expected, loss of data from u64 to u32. Zlib supports only unsigned long
    unsigned long sourceLen = azlossy_caster((FileLen - 4));

    buf.resize(sourceLen);

    AZ::IO::SystemFile::SizeType bytesReadIn = inputFile.Read(sizeof(uncompressedLen), &uncompressedLen);
    AZ_Warning("ShaderCompiler", bytesReadIn == sizeof(uncompressedLen), "Expected to read in %d but read in %d from file %s", sizeof(uncompressedLen), bytesReadIn, rFileName.c_str());

    bytesReadIn = inputFile.Read(buf.size(), buf.data());
    AZ_Warning("ShaderCompiler", bytesReadIn == buf.size(), "Expected to read in %d but read in %d from file %s", buf.size(), bytesReadIn, rFileName.c_str());

    unsigned long nUncompressedBytes = uncompressedLen;
    rIn.resize(uncompressedLen);
    int nRes = uncompress(rIn.data(), &nUncompressedBytes, buf.data(), sourceLen);

    return nRes == Z_OK && nUncompressedBytes == uncompressedLen;
}

//////////////////////////////////////////////////////////////////////////
bool CSTLHelper::AppendToFile(const std::string& rFileName, const std::vector<uint8_t>&  rOut)
{
    AZ::IO::SystemFile outputFile;
    int openMode = AZ::IO::SystemFile::SF_OPEN_APPEND;
    if (!AZ::IO::SystemFile::Exists(rFileName.c_str()))
    {
        openMode = AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
    }
    const bool wasSuccessful = outputFile.Open(rFileName.c_str(), openMode);
    AZ_Error("ShaderCompiler", wasSuccessful, "Could not open file for appending: %s", rFileName.c_str());
    if (wasSuccessful == false)
    {
        return false;
    }

    AZ::IO::SystemFile::SizeType bytesWritten = outputFile.Write(rOut.data(), rOut.size());
    AZ_Warning("ShaderCompiler", bytesWritten == rOut.size(), "Did not write out all the data to the file: %s", rFileName.c_str());
    return true;
}

//////////////////////////////////////////////////////////////////////////
tdHash CSTLHelper::Hash(const uint8_t*  pData, const size_t Size)
{
    tdHash CheckSum;
    cvs_MD5Context MD5Context;
    cvs_MD5Init(MD5Context);
    cvs_MD5Update(MD5Context, pData, static_cast<uint32_t>(Size));
    cvs_MD5Final(CheckSum.hash, MD5Context);
    return CheckSum;
}

static char C2A[17] = "0123456789ABCDEF";

std::string CSTLHelper::Hash2String(const tdHash& rHash)
{
    std::string Ret;
    for (size_t a = 0, Size = std::min<size_t>(sizeof(rHash.hash), 16u); a < Size; a++)
    {
        const uint8_t   C1 = rHash[a] & 0xf;
        const uint8_t   C2 = rHash[a] >> 4;
        Ret += C2A[C1];
        Ret += C2A[C2];
    }
    return Ret;
}

tdHash CSTLHelper::String2Hash(const std::string& rStr)
{
    assert(rStr.size() == 32);
    tdHash  Ret;
    for (size_t a = 0, Size = std::min<size_t>(rStr.size(), 32u); a < Size; a += 2)
    {
        const uint8_t   C1 = rStr.c_str()[a];
        const uint8_t   C2 = rStr.c_str()[a + 1];
        Ret[a >> 1] = C1 - (C1 >= '0' && C1 <= '9' ? '0' : 'A' - 10);
        Ret[a >> 1] |= (C2 - (C2 >= '0' && C2 <= '9' ? '0' : 'A' - 10)) << 4;
    }
    return Ret;
}

//////////////////////////////////////////////////////////////////////////
bool    CSTLHelper::Compress(const std::vector<uint8_t>& rIn, std::vector<uint8_t>& rOut)
{
    unsigned long destLen, sourceLen = (unsigned long)rIn.size();
    destLen = compressBound(sourceLen) + 16;
    rOut.resize(destLen + 4);
    compress(&rOut[4], &destLen, &rIn[0], sourceLen);
    rOut.resize(destLen + 4);
    *(uint32_t*)(&rOut[0]) = sourceLen;
    return true;
}

bool    CSTLHelper::Uncompress(const std::vector<uint8_t>& rIn, std::vector<uint8_t>& rOut)
{
    unsigned long sourceLen = (unsigned long)rIn.size() - 4;
    unsigned long nUncompressed = *(uint32_t*)(&rIn[0]);
    unsigned long nUncompressedBytes = nUncompressed;
    rOut.resize(nUncompressed);
    int nRes = uncompress(&rOut[0], &nUncompressedBytes, &rIn[4], sourceLen);
    return nRes == Z_OK && nUncompressed == nUncompressedBytes;
}
