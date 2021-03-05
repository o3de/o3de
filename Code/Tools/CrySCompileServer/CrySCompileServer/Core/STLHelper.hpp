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

#ifndef __STLHELPER__
#define __STLHELPER__

#include <vector>
#include <string>

typedef std::vector<std::string>                        tdEntryVec;
typedef std::pair<std::string, std::string>  tdToken;
typedef std::vector<tdToken>                                tdTokenList;
typedef std::vector<uint8_t>                tdDataVector;
//typedef std::vector<uint8_t>                              tdHash;

struct tdHash
{
    uint8_t hash[16];

    inline bool operator<(const tdHash& other) const { return memcmp(hash, other.hash, sizeof(hash)) < 0; }
    inline bool operator>(const tdHash& other) const { return memcmp(hash, other.hash, sizeof(hash)) > 0; }
    inline bool operator==(const tdHash& other) const { return memcmp(hash, other.hash, sizeof(hash)) == 0; }
    inline uint8_t& operator[](size_t nIndex) { return hash[nIndex]; }
    inline const uint8_t& operator[](size_t nIndex) const { return hash[nIndex]; }
};

class CSTLHelper
{
    static  tdHash              Hash(const uint8_t* pData, const size_t Size);
public:
    static  void                Tokenize(tdEntryVec& rRet, const std::string& Tokens, const std::string& Separator);
    static  tdToken             SplitToken(const std::string& rToken, const std::string& rSeparator);
    static  void                Splitizer(tdTokenList& rTokenList, const tdEntryVec& rFilter, const std::string& rSeparator);
    static  void                Trim(std::string& rStr, const std::string& charsToTrim);
    static  void                Remove(std::string& rTokenDst, const std::string& rTokenSrc, const char C);
    static  void                Replace(std::vector<uint8_t>& rRet, const std::vector<uint8_t>& rTokenSrc, const std::string& rToReplace, const std::string& rReplacement);
    static  void                Replace(std::string& rRet, const std::string& rSrc, const std::string& rToReplace, const std::string& rReplacement);


    static  bool                ToFile(const std::string& rFileName, const std::vector<uint8_t>& rOut);
    static  bool                FromFile(const std::string& rFileName, std::vector<uint8_t>& rIn);

    static  bool                AppendToFile(const std::string& rFileName, const std::vector<uint8_t>&   rOut);

    static  bool                ToFileCompressed(const std::string& rFileName, const std::vector<uint8_t>&   rOut);
    static  bool                FromFileCompressed(const std::string& rFileName, std::vector<uint8_t>&   rIn);

    static  bool                Compress(const std::vector<uint8_t>& rIn, std::vector<uint8_t>&  rOut);
    static  bool                Uncompress(const std::vector<uint8_t>& rIn, std::vector<uint8_t>& rOut);


    static  void                EndianSwizzleU64(uint64_t& S)
    {
        uint8_t* pT = reinterpret_cast<uint8_t*>(&S);
        uint8_t T;
        T = pT[0];
        pT[0] = pT[7];
        pT[7] = T;
        T = pT[1];
        pT[1] = pT[6];
        pT[6] = T;
        T = pT[2];
        pT[2] = pT[5];
        pT[5] = T;
        T = pT[3];
        pT[3] = pT[4];
        pT[4] = T;
    }
    static  void                EndianSwizzleU32(uint32_t& S)
    {
        uint8_t* pT = reinterpret_cast<uint8_t*>(&S);
        uint8_t T;
        T = pT[0];
        pT[0] = pT[3];
        pT[3] = T;
        T = pT[1];
        pT[1] = pT[2];
        pT[2] = T;
    }
    static  void                EndianSwizzleU16(uint16_t& S)
    {
        uint8_t* pT = reinterpret_cast<uint8_t*>(&S);
        uint8_t T;
        T = pT[0];
        pT[0] = pT[1];
        pT[1] = T;
    }


    static  void            Log(const std::string& rLog);

    static  tdHash          Hash(const std::string& rStr) { return Hash(reinterpret_cast<const uint8_t*>(rStr.c_str()), rStr.size()); }
    static  tdHash          Hash(const std::vector<uint8_t>& rData) { return Hash(&rData[0], rData.size()); }
    static  tdHash          Hash(const std::vector<uint8_t>& rData, size_t Size) { return Hash(&rData[0], Size); }

    static  std::string     Hash2String(const tdHash& rHash);
    static  tdHash          String2Hash(const std::string& rStr);
};

#define CRYSIMPLE_LOG(X) CSTLHelper::Log(X)
#endif

