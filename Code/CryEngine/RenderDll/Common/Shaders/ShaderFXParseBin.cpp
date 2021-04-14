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

#include "RenderDll_precompiled.h"
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzFramework/Archive/IArchive.h>
#include "UnalignedBlit.h"

#include "DeviceManager/Enums.h"
#include "PoundPoundParser.h"

static FOURCC FOURCC_SHADERBIN = MAKEFOURCC('F', 'X', 'B', '0');


SShaderBin SShaderBin::s_Root;
uint32 SShaderBin::s_nCache = 0;
uint32 SShaderBin::s_nMaxFXBinCache = MAX_FXBIN_CACHE;
CShaderManBin::CShaderManBin()
{
    m_pCEF = gRenDev ? &gRenDev->m_cEF : nullptr;
}

int CShaderManBin::Size()
{
    SShaderBin* pSB;
    int nSize = 0;
    nSize += sizeOfMapStr(m_BinPaths);
    nSize += m_BinValidCRCs.size() * sizeof(bool) * sizeof(stl::MapLikeStruct);

    for (pSB = SShaderBin::s_Root.m_Prev; pSB != &SShaderBin::s_Root; pSB = pSB->m_Prev)
    {
        nSize += pSB->Size();
    }
    return nSize;
}

void CShaderManBin::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_BinPaths);
    pSizer->AddObject(m_BinValidCRCs);

    SShaderBin* pSB;
    for (pSB = SShaderBin::s_Root.m_Prev; pSB != &SShaderBin::s_Root; pSB = pSB->m_Prev)
    {
        pSB->GetMemoryUsage(pSizer);
    }
}

uint32 SShaderBin::ComputeCRC()
{
    if (!m_Tokens.size())
    {
        return 0;
    }
    uint32 CRC32;
    if (CParserBin::m_bEndians)
    {
        DWORD* pT = new DWORD [m_Tokens.size()];
        memcpy(pT, &m_Tokens[0], m_Tokens.size() * sizeof(uint32));
        SwapEndian(pT, (size_t)m_Tokens.size(), eBigEndian);
        CRC32 = CCrc32::Compute((char*)pT, m_Tokens.size() * sizeof(uint32));
        delete [] pT;
    }
    else
    {
        CRC32 = CCrc32::Compute((char*)&m_Tokens[0], m_Tokens.size() * sizeof(uint32));
    }
    int nCur = 0;
    Lock();
    while (nCur >= 0)
    {
        nCur = CParserBin::FindToken(nCur, m_Tokens.size() - 1, &m_Tokens[0], eT_include);
        if (nCur >= 0)
        {
            nCur++;
            uint32 nTokName = m_Tokens[nCur];
            const char* szNameInc = CParserBin::GetString(nTokName, m_TokenTable);
            SShaderBin* pBinIncl = gRenDev->m_cEF.m_Bin.GetBinShader(szNameInc, true, 0);
            AZ_Assert(pBinIncl, "Error loading shader '%s' while trying to compute the shader CRC.", szNameInc);
            if (pBinIncl)
            {
                CRC32 += pBinIncl->ComputeCRC();
            }
        }
    }
    Unlock();

    return CRC32;
}

SShaderBin* CShaderManBin::SaveBinShader(
    uint32 nSourceCRC32, const char* szName, bool bInclude, AZ::IO::HandleType srcFileHandle)
{
    SShaderBin* pBin = new SShaderBin;

    CParserBin Parser(pBin);

    int nSize = gEnv->pCryPak->FGetSize(srcFileHandle);
    char* buf = new char [nSize + 1];
    char* pBuf = buf;
    buf[nSize] = 0;
    gEnv->pCryPak->FSeek(srcFileHandle, 0, SEEK_SET);
    gEnv->pCryPak->FRead(buf, nSize, srcFileHandle);

    RemoveCR(buf);
    const char* whiteSpace = " ";

    {
        // Hold the parsing context used to get rid of the ## directives. The constructor will
        // take care of setting AZ_RESTRICTED_PLATFORM appropriately
        PoundPoundContext poundPoundContext(m_pCEF->m_ShadersFilter);

        // Keep parsing until we hit the real EOB.
        bool layerSwitch;
        while (!poundPoundContext.IsEndOfBuffer(&buf, &layerSwitch))
        {
            // This loop handles stripping the input text of comments, whitespace, and the
            // ## include directives
            do 
            {
                SkipCharacters(&buf, whiteSpace);
                SkipComments(&buf, true);

                // If we find the ## characters, preprocess the token lines, allowing it to consume
                // any disabled text between ## directives and then whitespace and comments again
                while (buf[0] == '#' && buf[1] == '#')
                {
                    poundPoundContext.PreprocessLines(&buf);
                    SkipCharacters(&buf, whiteSpace);
                    SkipComments(&buf, true);
                }

                // We need to be able to catch the case where a ##include file has hit end of buffer,
                // but not the parent buffer, in which case we have to keep skipping whitespace and
                // comments again
                layerSwitch = false;
            } while (!poundPoundContext.IsEndOfBuffer(&buf, &layerSwitch) && layerSwitch);

            // Quit parsing if we have hit the real EOB.
            if (poundPoundContext.IsEndOfBuffer(&buf, &layerSwitch))
            {
                break;
            }

            char com[1024];
            bool bKey = false;
            uint32 dwToken = CParserBin::NextToken(buf, com, bKey);
            // If the token is not a key token, find/create a user token for it.
            dwToken = Parser.NewUserToken(dwToken, com, false);
            pBin->m_Tokens.push_back(dwToken);

            SkipCharacters (&buf, whiteSpace);
            SkipComments (&buf, true);
            if (dwToken == eT_include)
            {
                // Skip whitespace to get to the < or " bracket for the include
                SkipCharacters(&buf, whiteSpace);
                AZ_Assert(*buf == '"' || *buf == '<', "Error saving shader %s. Include should be followed by \" or <.", szName);
                char brak = *buf;
                ++buf;
                int n = 0;

                // Get the value in-between the include brackets
                while (*buf != brak)
                {
                    if (*buf <= 0x20)
                    {
                        AZ_Assert(false, "Error saving shader %s. Invalid special character found between include brackets.", szName);
                        break;
                    }
                    com[n++] = *buf;
                    ++buf;
                }
                if (*buf == brak)
                {
                    ++buf;
                }
                com[n] = 0;

                fpStripExtension(com, com);

                // Get or load the included shader
                GetBinShader(com, true, 0);

                dwToken = CParserBin::fxToken(com, NULL);
                dwToken = Parser.NewUserToken(dwToken, com, false);
                pBin->m_Tokens.push_back(dwToken);
            }
            else if (dwToken == eT_if || dwToken == eT_ifdef || dwToken == eT_ifndef)
            {
                bool bFirst = fxIsFirstPass(buf);
                if (!bFirst)
                {
                    if (dwToken == eT_if)
                    {
                        dwToken = eT_if_2;
                    }
                    else if (dwToken == eT_ifdef)
                    {
                        dwToken = eT_ifdef_2;
                    }
                    else
                    {
                        dwToken = eT_ifndef_2;
                    }
                    pBin->m_Tokens[pBin->m_Tokens.size() - 1] = dwToken;
                }
            }
            else if (dwToken == eT_define)
            {
                shFill(&buf, com);
                if (com[0] == '%')
                {
                    pBin->m_Tokens[pBin->m_Tokens.size() - 1] = eT_define_2;
                }
                dwToken = Parser.NewUserToken(eT_unknown, com, false);
                pBin->m_Tokens.push_back(dwToken);

                TArray<char> macro;
                while (*buf == 0x20 || *buf == 0x9)
                {
                    buf++;
                }
                while (*buf != 0xa)
                {
                    if (*buf == '\\')
                    {
                        macro.AddElem('\n');
                        while (*buf != '\n')
                        {
                            buf++;
                        }
                        buf++;
                        continue;
                    }
                    macro.AddElem(*buf);
                    buf++;
                }
                macro.AddElem(0);
                int n = macro.Num() - 2;
                while (n >= 0 && macro[n] <= 0x20)
                {
                    macro[n] = 0;
                    n--;
                }
                char* b = &macro[0];
                while (*b)
                {
                    SkipCharacters (&b, whiteSpace);
                    SkipComments (&b, true);
                    if (!b || !b[0])
                    {
                        break;
                    }
                    bKey = false;
                    dwToken = CParserBin::NextToken(b, com, bKey);
                    dwToken = Parser.NewUserToken(dwToken, com, false);
                    if (dwToken == eT_if || dwToken == eT_ifdef || dwToken == eT_ifndef)
                    {
                        bool bFirst = fxIsFirstPass(b);
                        if (!bFirst)
                        {
                            if (dwToken == eT_if)
                            {
                                dwToken = eT_if_2;
                            }
                            else
                            if (dwToken == eT_ifdef)
                            {
                                dwToken = eT_ifdef_2;
                            }
                            else
                            {
                                dwToken = eT_ifndef_2;
                            }
                        }
                    }
                    pBin->m_Tokens.push_back(dwToken);
                }
                pBin->m_Tokens.push_back(0);
            }
        }
    }
    if (pBin->m_Tokens.size() == 0 || !pBin->m_Tokens[0])
    {
        pBin->m_Tokens.push_back(eT_skip);
    }

    pBin->SetCRC(pBin->ComputeCRC());
    pBin->m_bReadOnly = false;

    char nameFile[256];
    sprintf_s(nameFile, "%s%s.%s", m_pCEF->m_ShadersCache.c_str(), szName, bInclude ? "cfib" : "cfxb");
    stack_string szDst = stack_string(m_pCEF->m_szCachePath.c_str()) + stack_string(nameFile);
    const char* szFileName = szDst;

    AZ::IO::HandleType dstFileHandle = gEnv->pCryPak->FOpen(szFileName, "wb", AZ::IO::IArchive::FLAGS_NEVER_IN_PAK | AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FOPEN_ONDISK);
    if (dstFileHandle != AZ::IO::InvalidHandle)
    {
        SShaderBinHeader Header;
        Header.m_nTokens = pBin->m_Tokens.size();
        Header.m_Magic = FOURCC_SHADERBIN;
        Header.m_CRC32 = pBin->m_CRC32;
        float fVersion = FX_CACHE_VER;
        Header.m_VersionLow = (uint16)(((float)fVersion - (float)(int)fVersion) * 10.1f);
        Header.m_VersionHigh = (uint16)fVersion;
        Header.m_nOffsetStringTable = pBin->m_Tokens.size() * sizeof(DWORD) + sizeof(Header);
        Header.m_nOffsetParamsLocal = 0;
        Header.m_nSourceCRC32 = nSourceCRC32;
        SShaderBinHeader hdTemp, * pHD;
        pHD = &Header;
        if (CParserBin::m_bEndians)
        {
            hdTemp = Header;
            SwapEndian(hdTemp, eBigEndian);
            pHD = &hdTemp;
        }
        gEnv->pCryPak->FWrite((void*)pHD, sizeof(Header), 1, dstFileHandle);
        if (CParserBin::m_bEndians)
        {
            DWORD* pT = new DWORD [pBin->m_Tokens.size()];
            memcpy(pT, &pBin->m_Tokens[0], pBin->m_Tokens.size() * sizeof(DWORD));
            SwapEndian(pT, (size_t)pBin->m_Tokens.size(), eBigEndian);
            gEnv->pCryPak->FWrite((void*)pT, pBin->m_Tokens.size() * sizeof(DWORD), 1, dstFileHandle);
            delete [] pT;
        }
        else
        {
            gEnv->pCryPak->FWrite(&pBin->m_Tokens[0], pBin->m_Tokens.size() * sizeof(DWORD), 1, dstFileHandle);
        }
        FXShaderTokenItor itor;
        for (itor = pBin->m_TokenTable.begin(); itor != pBin->m_TokenTable.end(); itor++)
        {
            STokenD T = *itor;
            if (CParserBin::m_bEndians)
            {
                SwapEndian(T.Token, eBigEndian);
            }
            gEnv->pCryPak->FWrite(&T.Token, sizeof(DWORD), 1, dstFileHandle);
            gEnv->pCryPak->FWrite(T.SToken.c_str(), T.SToken.size() + 1, 1, dstFileHandle);
        }
        Header.m_nOffsetParamsLocal = aznumeric_caster(gEnv->pCryPak->FTell(dstFileHandle));
        gEnv->pCryPak->FSeek(dstFileHandle, 0, SEEK_SET);
        if (CParserBin::m_bEndians)
        {
            hdTemp = Header;
            SwapEndian(hdTemp, eBigEndian);
            pHD = &hdTemp;
        }
        gEnv->pCryPak->FWrite((void*)pHD, sizeof(Header), 1, dstFileHandle);
        gEnv->pCryPak->FClose(dstFileHandle);
    }
    else
    {
        iLog->LogWarning("WARN: CShaderManBin::SaveBinShader: Cannot write shader to file '%s'.", nameFile);
        pBin->m_bReadOnly = true;
    }

    SAFE_DELETE_ARRAY(pBuf);

    return pBin;
}

//=========================================================================================================================================

static void sParseCSV(string& sFlt, std::vector<string>& Filters)
{
    const char* cFlt = sFlt.c_str();
    char Flt[64];
    int nFlt = 0;
    while (true)
    {
        char c = *cFlt++;
        if (!c)
        {
            break;
        }
        if (SkipChar((unsigned char)c))
        {
            if (nFlt)
            {
                Flt[nFlt] = 0;
                Filters.push_back(string(Flt));
                nFlt = 0;
            }
            continue;
        }
        Flt[nFlt++] = c;
    }
    if (nFlt)
    {
        Flt[nFlt] = 0;
        Filters.push_back(string(Flt));
    }
}

//===========================================================================================================================

struct FXParamsSortByName
{
    bool operator () (const SFXParam& left, const SFXParam& right) const
    {
        return left.m_dwName[0] < right.m_dwName[0];
    }
    bool operator () (const uint32 left, const SFXParam& right) const
    {
        return left < right.m_dwName[0];
    }
    bool operator () (const SFXParam& left, uint32 right) const
    {
        return left.m_dwName[0] < right;
    }
};
struct FXSamplersOldSortByName
{
    bool operator () (const STexSamplerFX& left, const STexSamplerFX& right) const
    {
        return left.m_szName < right.m_szName;
    }
    bool operator () (const string left, const STexSamplerFX& right) const
    {
        return left < right.m_szName;
    }
    bool operator () (const STexSamplerFX& left, string right) const
    {
        return left.m_szName < right;
    }
};
struct FXSamplersSortByName
{
    bool operator () (const SFXSampler& left, const SFXSampler& right) const
    {
        return left.m_dwName[0] < right.m_dwName[0];
    }
    bool operator () (const uint32 left, const SFXSampler& right) const
    {
        return left < right.m_dwName[0];
    }
    bool operator () (const SFXSampler& left, const uint32 right) const
    {
        return left.m_dwName[0] < right;
    }
};
struct FXTexturesSortByName
{
    bool operator () (const SFXTexture& left, const SFXTexture& right) const
    {
        return left.m_dwName[0] < right.m_dwName[0];
    }
    bool operator () (const uint32 left, const SFXTexture& right) const
    {
        return left < right.m_dwName[0];
    }
    bool operator () (const SFXTexture& left, const uint32 right) const
    {
        return left.m_dwName[0] < right;
    }
};

int CShaderManBin::mfSizeFXParams(uint32& nCount)
{
    nCount = m_ShaderFXParams.size();
    int nSize = sizeOfMap(m_ShaderFXParams);
    return nSize;
}

void CShaderManBin::mfReleaseFXParams()
{
    m_ShaderFXParams.clear();
}

SShaderFXParams& CShaderManBin::mfGetFXParams(CShader* pSH)
{
    CCryNameTSCRC s = pSH->GetNameCRC();
    ShaderFXParamsItor it = m_ShaderFXParams.find(s);
    if (it != m_ShaderFXParams.end())
    {
        return it->second;
    }
    SShaderFXParams pr;
    std::pair<ShaderFXParams::iterator, bool> insertLocation =
        m_ShaderFXParams.insert(std::pair<CCryNameTSCRC, SShaderFXParams>(s, pr));
    CRY_ASSERT(insertLocation.second);
    return insertLocation.first->second;
}

void CShaderManBin::mfRemoveFXParams(CShader* pSH)
{
    CCryNameTSCRC s = pSH->GetNameCRC();
    ShaderFXParamsItor it = m_ShaderFXParams.find(s);
    if (it != m_ShaderFXParams.end())
    {
        m_ShaderFXParams.erase(it);
    }
}

SFXParam* CShaderManBin::mfAddFXParam(SShaderFXParams& FXP, const SFXParam* pParam)
{
    FXParamsIt it = std::lower_bound(FXP.m_FXParams.begin(), FXP.m_FXParams.end(), pParam->m_dwName[0], FXParamsSortByName());
    if (it != FXP.m_FXParams.end() && pParam->m_dwName[0] == (*it).m_dwName[0])
    {
        SFXParam& pr = *it;
        pr.m_nFlags = pParam->m_nFlags;
        int n = 6;
        SFXParam& p = *it;
        for (int i = 0; i < n; i++)
        {
            if (p.m_Register[i] == 10000)
            {
                p.m_Register[i] = pParam->m_Register[i];
            }
        }
        //CRY_ASSERT(p == *pParam);
        return &(*it);
    }
    FXP.m_FXParams.insert(it, *pParam);
    it = std::lower_bound(FXP.m_FXParams.begin(), FXP.m_FXParams.end(), pParam->m_dwName[0], FXParamsSortByName());
    SFXParam* pFX = &(*it);
    if (pFX->m_Semantic.empty() && pFX->m_Values.c_str()[0] == '(')
    {
        pFX->m_BindingSlot = eConstantBufferShaderSlot_PerMaterial;
    }
    FXP.m_nFlags |= FXP_PARAMS_DIRTY;

    return pFX;
}
SFXParam* CShaderManBin::mfAddFXParam(CShader* pSH, const SFXParam* pParam)
{
    if (!pParam)
    {
        return NULL;
    }

    SShaderFXParams& FXP = mfGetFXParams(pSH);
    return mfAddFXParam(FXP, pParam);
}
void CShaderManBin::mfAddFXSampler(CShader* pSH, const STexSamplerFX* pSamp)
{
    if (!pSamp)
    {
        return;
    }

    SShaderFXParams& FXP = mfGetFXParams(pSH);

    FXSamplersOldIt it = std::lower_bound(FXP.m_FXSamplersOld.begin(), FXP.m_FXSamplersOld.end(), pSamp->m_szName, FXSamplersOldSortByName());
    if (it != FXP.m_FXSamplersOld.end() && pSamp->m_szName == (*it).m_szName)
    {
        CRY_ASSERT(*it == *pSamp);
        return;
    }
    FXP.m_FXSamplersOld.insert(it, *pSamp);
    FXP.m_nFlags |= FXP_SAMPLERS_DIRTY;
}
void CShaderManBin::mfAddFXSampler(CShader* pSH, const SFXSampler* pSamp)
{
    if (!pSamp)
    {
        return;
    }

    SShaderFXParams& FXP = mfGetFXParams(pSH);

    FXSamplersIt it = std::lower_bound(FXP.m_FXSamplers.begin(), FXP.m_FXSamplers.end(), pSamp->m_dwName[0], FXSamplersSortByName());
    if (it != FXP.m_FXSamplers.end() && pSamp->m_dwName[0] == (*it).m_dwName[0])
    {
        CRY_ASSERT(*it == *pSamp);
        return;
    }
    FXP.m_FXSamplers.insert(it, *pSamp);
    FXP.m_nFlags |= FXP_SAMPLERS_DIRTY;
}

//------------------------------------------------------------------------------
// Add a new texture to the shader's textures array based on the texture name
//------------------------------------------------------------------------------
void CShaderManBin::mfAddFXTexture(CShader* pSH, const SFXTexture* pTexture)
{
    if (!pTexture)
    {
        return;
    }

    SShaderFXParams&    FXP = mfGetFXParams(pSH);
    FXTexturesIt        it = std::lower_bound(FXP.m_FXTextures.begin(), FXP.m_FXTextures.end(), pTexture->m_dwName[0], FXTexturesSortByName());

    if (it != FXP.m_FXTextures.end() && pTexture->m_dwName[0] == (*it).m_dwName[0])
    {
        CRY_ASSERT(*it == *pTexture);
        return;
    }
    FXP.m_FXTextures.insert(it, *pTexture);
    FXP.m_nFlags |= FXP_TEXTURES_DIRTY;
}

void CShaderManBin::mfGeneratePublicFXParams(CShader* pSH, CParserBin& Parser)
{
    SShaderFXParams& FXP = mfGetFXParams(pSH);
    if (!(FXP.m_nFlags & FXP_PARAMS_DIRTY))
    {
        return;
    }
    FXP.m_nFlags &= ~FXP_PARAMS_DIRTY;

    uint32 i;
    // Generate public parameters
    for (i = 0; i < FXP.m_FXParams.size(); i++)
    {
        SFXParam* pr = &FXP.m_FXParams[i];
        uint32 nFlags = pr->GetFlags();
        if (nFlags & PF_AUTOMERGED)
        {
            continue;
        }
        if (nFlags & PF_TWEAKABLE_MASK)
        {
            const char* szName = Parser.GetString(pr->m_dwName[0]);
            // Avoid duplicating of public parameters
            int32 j;
            for (j = 0; j < FXP.m_PublicParams.size(); j++)
            {
                SShaderParam* p = &FXP.m_PublicParams[j];
                if (azstricmp(p->m_Name.c_str(), szName) == 0)
                {
                    break;
                }
            }
            if (j == FXP.m_PublicParams.size())
            {
                SShaderParam sp;
                sp.m_Name = szName;
                EParamType eType;
                string szWidget = pr->GetValueForName("UIWidget", eType);
                const char* szVal = pr->m_Values.c_str();
                if (szWidget == "color")
                {
                    sp.m_Type = eType_FCOLOR;
                    if (szVal[0] == '{')
                    {
                        szVal++;
                    }
                    [[maybe_unused]] int n = azsscanf(szVal, "%f, %f, %f, %f", &sp.m_Value.m_Color[0], &sp.m_Value.m_Color[1], &sp.m_Value.m_Color[2], &sp.m_Value.m_Color[3]);
                    AZ_Warning("Shaders", n == 4, "color value only has %d components", n);
                }
                else if (szWidget == "colora")
                {
                    sp.m_Type = eType_FCOLORA;
                    if (szVal[0] == '{')
                    {
                        szVal++;
                    }
                    [[maybe_unused]] int n = azsscanf(szVal, "%f, %f, %f, %f", &sp.m_Value.m_Color[0], &sp.m_Value.m_Color[1], &sp.m_Value.m_Color[2], &sp.m_Value.m_Color[3]);
                    AZ_Warning("Shaders", n == 4, "color value only has %d components", n);
                }
                else
                {
                    sp.m_Type = eType_FLOAT;
                    sp.m_Value.m_Float = (float)atof(szVal);
                }

                bool bAdd = true;
                if (!pr->m_Annotations.empty() && gRenDev->IsEditorMode())
                {
                    //EParamType eType;
                    string sFlt = pr->GetValueForName("Filter", eType);
                    bool bUseScript = true;
                    if (!sFlt.empty())
                    {
                        std::vector<string> Filters;
                        sParseCSV(sFlt, Filters);
                        string strShader = Parser.m_pCurShader->GetName();
                        uint32 h;
                        for (h = 0; h < Filters.size(); h++)
                        {
                            if (!_strnicmp(Filters[h].c_str(), strShader.c_str(), Filters[h].size()))
                            {
                                break;
                            }
                        }
                        if (h == Filters.size())
                        {
                            bUseScript = false;
                            bAdd = false;
                        }
                    }

                    if (bUseScript)
                    {
                        sp.m_Script = pr->m_Annotations.c_str();
                    }
                }

                if (bAdd)
                {
                    FXP.m_PublicParams.push_back(sp);
                }
            }
        }
    }
}

SParamCacheInfo* CShaderManBin::GetParamInfo(SShaderBin* pBin, uint32 dwName, uint64 nMaskGenFX, uint64 maskGenStatic)
{
    const int n = pBin->m_ParamsCache.size();
    for (int i = 0; i < n; i++)
    {
        SParamCacheInfo* pInf = &pBin->m_ParamsCache[i];
        if (pInf->m_dwName == dwName && pInf->m_nMaskGenFX == nMaskGenFX && pInf->m_maskGenStatic == maskGenStatic)
        {
            pBin->m_nCurParamsID = i;
            return pInf;
        }
    }
    pBin->m_nCurParamsID = -1;
    return NULL;
}

bool CShaderManBin::SaveBinShaderLocalInfo(SShaderBin* pBin, uint32 dwName, uint64 nMaskGenFX, uint64 maskGenStatic, TArray<int32>& Funcs, std::vector<SFXParam>& Params, std::vector<SFXSampler>& Samplers, std::vector<SFXTexture>& Textures)
{
    if (GetParamInfo(pBin, dwName, nMaskGenFX, maskGenStatic))
    {
        return true;
    }

    if (pBin->IsReadOnly() && !gEnv->IsEditor()) // if in the editor, allow params to be added in-memory, but not saved to disk
    {
        return false;
    }
    TArray<int32> EParams;
    TArray<int32> ESamplers;
    TArray<int32> ETextures;
    TArray<int32> EFuncs;
    for (uint32 i = 0; i < Params.size(); i++)
    {
        SFXParam& pr = Params[i];
        CRY_ASSERT(pr.m_dwName.size());
        if (pr.m_dwName.size())
        {
            EParams.push_back(pr.m_dwName[0]);
        }
    }
    for (uint32 i = 0; i < Samplers.size(); i++)
    {
        SFXSampler& pr = Samplers[i];
        CRY_ASSERT(pr.m_dwName.size());
        if (pr.m_dwName.size())
        {
            ESamplers.push_back(pr.m_dwName[0]);
        }
    }
    for (uint32 i = 0; i < Textures.size(); i++)
    {
        SFXTexture& pr = Textures[i];
        CRY_ASSERT(pr.m_dwName.size());
        if (pr.m_dwName.size())
        {
            ETextures.push_back(pr.m_dwName[0]);
        }
    }
    pBin->m_nCurParamsID = pBin->m_ParamsCache.size();
    pBin->m_ParamsCache.push_back(SParamCacheInfo());
    SParamCacheInfo& pr = pBin->m_ParamsCache.back();
    pr.m_nMaskGenFX = nMaskGenFX;
    pr.m_maskGenStatic = maskGenStatic;
    pr.m_dwName = dwName;
    pr.m_AffectedFuncs.assign(Funcs.begin(), Funcs.end());
    pr.m_AffectedParams.assign(EParams.begin(), EParams.end());
    pr.m_AffectedSamplers.assign(ESamplers.begin(), ESamplers.end());
    pr.m_AffectedTextures.assign(ETextures.begin(), ETextures.end());
    if (pBin->IsReadOnly())
    {
        return false;
    }
    AZ::IO::HandleType binFileHandle = gEnv->pCryPak->FOpen(pBin->m_szName, "r+b", AZ::IO::IArchive::FLAGS_NEVER_IN_PAK | AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FOPEN_ONDISK);
    CRY_ASSERT(binFileHandle != AZ::IO::InvalidHandle);
    if (binFileHandle == AZ::IO::InvalidHandle)
    {
        return false;
    }
    gEnv->pCryPak->FSeek(binFileHandle, 0, SEEK_END);
    uint64_t nSeek = gEnv->pCryPak->FTell(binFileHandle);
    CRY_ASSERT(nSeek > 0);
    if (nSeek == 0)
    {
        return false;
    }
    SShaderBinParamsHeader sd;
    int32* pFuncs = &Funcs[0];
    int32* pParams = EParams.size() ? &EParams[0] : NULL;
    int32* pSamplers = ESamplers.size() ? &ESamplers[0] : NULL;
    int32* pTextures = ETextures.size() ? &ETextures[0] : NULL;
    sd.nMask = nMaskGenFX;
    sd.nstaticMask = maskGenStatic;
    sd.nName = dwName;
    sd.nFuncs = Funcs.size();
    sd.nParams = EParams.size();
    sd.nSamplers = ESamplers.size();
    sd.nTextures = ETextures.size();
    if (CParserBin::m_bEndians)
    {
        SwapEndian(sd, eBigEndian);
        EFuncs = Funcs;
        if (EParams.size())
        {
            SwapEndian(&EParams[0], (size_t)EParams.size(), eBigEndian);
        }
        if (ESamplers.size())
        {
            SwapEndian(&ESamplers[0], (size_t)ESamplers.size(), eBigEndian);
        }
        if (ETextures.size())
        {
            SwapEndian(&ETextures[0], (size_t)ETextures.size(), eBigEndian);
        }
        SwapEndian(&EFuncs[0], (size_t)EFuncs.size(), eBigEndian);
        pFuncs = &EFuncs[0];
    }
    gEnv->pCryPak->FWrite(&sd, sizeof(sd), 1, binFileHandle);

    if (EParams.size())
    {
        gEnv->pCryPak->FWrite(pParams, EParams.size(), sizeof(int32), binFileHandle);
    }
    if (ESamplers.size())
    {
        gEnv->pCryPak->FWrite(pSamplers, ESamplers.size(), sizeof(int32), binFileHandle);
    }
    if (ETextures.size())
    {
        gEnv->pCryPak->FWrite(pTextures, ETextures.size(), sizeof(int32), binFileHandle);
    }
    gEnv->pCryPak->FWrite(pFuncs, Funcs.size(), sizeof(int32), binFileHandle);
    gEnv->pCryPak->FClose(binFileHandle);

    return true;
}

SShaderBin* CShaderManBin::LoadBinShader(AZ::IO::HandleType fpBin, const char* szName, const char* szNameBin, bool bReadParams)
{
    LOADING_TIME_PROFILE_SECTION(iSystem);

    gEnv->pCryPak->FSeek(fpBin, 0, SEEK_SET);
    SShaderBinHeader Header;
    size_t sizeRead = gEnv->pCryPak->FReadRaw(&Header, 1, sizeof(SShaderBinHeader), fpBin);
    if (sizeRead != sizeof(SShaderBinHeader))
    {
        CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Failed to read header for %s in CShaderManBin::LoadBinShader. Expected %" PRISIZE_T ", got %" PRISIZE_T "", szName, sizeof(SShaderBinHeader), sizeRead);
        return NULL;
    }

    if (CParserBin::m_bEndians)
    {
        SwapEndian(Header, eBigEndian);
    }
    float fVersion = FX_CACHE_VER;
    uint16 MinorVer = (uint16)(((float)fVersion - (float)(int)fVersion) * 10.1f);
    uint16 MajorVer = (uint16)fVersion;
    bool bCheckValid = CRenderer::CV_r_shadersAllowCompilation != 0;
    if (bCheckValid && (Header.m_VersionLow != MinorVer || Header.m_VersionHigh != MajorVer || Header.m_Magic != FOURCC_SHADERBIN))
    {
        return NULL;
    }
    if (Header.m_VersionHigh > 10)
    {
        return NULL;
    }
    SShaderBin* pBin = new SShaderBin;

    pBin->m_SourceCRC32 = Header.m_nSourceCRC32;
    pBin->m_nOffsetLocalInfo = Header.m_nOffsetParamsLocal;

    uint32 CRC32 = Header.m_CRC32;
    pBin->m_CRC32 = CRC32;
    pBin->m_Tokens.resize(Header.m_nTokens);
    sizeRead = gEnv->pCryPak->FReadRaw(&pBin->m_Tokens[0], sizeof(uint32), Header.m_nTokens, fpBin);
    if (sizeRead != Header.m_nTokens)
    {
        CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Failed to read Tokens for %s in CShaderManBin::LoadBinShader. Expected %d, got %" PRISIZE_T "", szName, Header.m_nTokens, sizeRead);
        return NULL;
    }
    if (CParserBin::m_bEndians)
    {
        SwapEndian(&pBin->m_Tokens[0], (size_t)Header.m_nTokens, eBigEndian);
    }

    int nSizeTable = Header.m_nOffsetParamsLocal - Header.m_nOffsetStringTable;
    if (nSizeTable < 0)
    {
        return NULL;
    }
    else if (nSizeTable > 0)
    {
        char* bufTable = new char[nSizeTable];
        char* bufT = bufTable;
        sizeRead = gEnv->pCryPak->FReadRaw(bufTable, 1, nSizeTable, fpBin);
        if (sizeRead != nSizeTable)
        {
            CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Failed to read bufTable for %s in CShaderManBin::LoadBinShader. Expected %d, got %" PRISIZE_T "", szName, nSizeTable, sizeRead);
            return NULL;
        }
        char* bufEnd = &bufTable[nSizeTable];

        // First pass to count the tokens
        uint32 nTokens(0);
        while (bufTable < bufEnd)
        {
            STokenD TD;
            LoadUnaligned(bufTable, TD.Token);
            int nIncr = 4 + strlen(&bufTable[4]) + 1;
            bufTable += nIncr;
            ++nTokens;
        }

        pBin->m_TokenTable.reserve(nTokens);
        bufTable = bufT;
        while (bufTable < bufEnd)
        {
            STokenD TD;
            LoadUnaligned(bufTable, TD.Token);
            if (CParserBin::m_bEndians)
            {
                SwapEndian(TD.Token, eBigEndian);
            }
            FXShaderTokenItor itor = std::lower_bound(pBin->m_TokenTable.begin(), pBin->m_TokenTable.end(), TD.Token, SortByToken());
            assert (itor == pBin->m_TokenTable.end() || (*itor).Token != TD.Token);
            TD.SToken = &bufTable[4];
            pBin->m_TokenTable.insert(itor, TD);
            int nIncr = 4 + strlen(&bufTable[4]) + 1;
            bufTable += nIncr;
        }
        SAFE_DELETE_ARRAY(bufT);
    }

    //if (CRenderer::CV_r_shadersnocompile)
    //  bReadParams = false;
    if (bReadParams)
    {
        int nSeek = pBin->m_nOffsetLocalInfo;
        gEnv->pCryPak->FSeek(fpBin, nSeek, SEEK_SET);
        while (true)
        {
            SShaderBinParamsHeader sd;
            int nSize = gEnv->pCryPak->FReadRaw(&sd, 1, sizeof(sd), fpBin);
            if (nSize != sizeof(sd))
            {
                break;
            }
            if (CParserBin::m_bEndians)
            {
                SwapEndian(sd, eBigEndian);
            }

            if (sd.nParams < 0 || sd.nSamplers < 0 || sd.nTextures < 0 || sd.nFuncs < 0)
            {
                AZ_Assert(false, "Error attempting to read shader binary %s. You may need to delete and re-compile this shader binary from your cache folder.", szNameBin);
                return nullptr;
            }

            SParamCacheInfo pr;
            int n = pBin->m_ParamsCache.size();
            pBin->m_ParamsCache.push_back(pr);
            SParamCacheInfo& prc = pBin->m_ParamsCache[n];
            prc.m_dwName = sd.nName;
            prc.m_nMaskGenFX = sd.nMask;
            prc.m_maskGenStatic = sd.nstaticMask;
            prc.m_AffectedParams.resize(sd.nParams);
            prc.m_AffectedSamplers.resize(sd.nSamplers);
            prc.m_AffectedTextures.resize(sd.nTextures);
            prc.m_AffectedFuncs.resize(sd.nFuncs);

            if (sd.nParams)
            {
                nSize = gEnv->pCryPak->FReadRaw(&prc.m_AffectedParams[0], sizeof(int32), sd.nParams, fpBin);
                if (nSize != sd.nParams)
                {
                    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Failed to read m_AffectedParams for %s in CShaderManBin::LoadBinShader. Expected %d, got %" PRISIZE_T "", szName, sd.nParams, sizeRead);
                    return NULL;
                }
                if (CParserBin::m_bEndians)
                {
                    SwapEndian(&prc.m_AffectedParams[0], sd.nParams, eBigEndian);
                }
            }
            if (sd.nSamplers)
            {
                nSize = gEnv->pCryPak->FReadRaw(&prc.m_AffectedSamplers[0], sizeof(int32), sd.nSamplers, fpBin);
                if (nSize != sd.nSamplers)
                {
                    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Failed to read m_AffectedSamplers for %s in CShaderManBin::LoadBinShader. Expected %d, got %" PRISIZE_T, szName, sd.nSamplers, sizeRead);
                    return NULL;
                }
                if (CParserBin::m_bEndians)
                {
                    SwapEndian(&prc.m_AffectedSamplers[0], sd.nSamplers, eBigEndian);
                }
            }
            if (sd.nTextures)
            {
                nSize = gEnv->pCryPak->FReadRaw(&prc.m_AffectedTextures[0], sizeof(int32), sd.nTextures, fpBin);
                if (nSize != sd.nTextures)
                {
                    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Failed to read m_AffectedTextures for %s in CShaderManBin::LoadBinShader. Expected %d, got %" PRISIZE_T, szName, sd.nTextures, sizeRead);
                    return NULL;
                }
                if (CParserBin::m_bEndians)
                {
                    SwapEndian(&prc.m_AffectedTextures[0], sd.nTextures, eBigEndian);
                }
            }

            CRY_ASSERT(sd.nFuncs > 0);
            nSize = gEnv->pCryPak->FReadRaw(&prc.m_AffectedFuncs[0], sizeof(int32), sd.nFuncs, fpBin);
            if (nSize != sd.nFuncs)
            {
                CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Failed to read nFuncs for %s in CShaderManBin::LoadBinShader. Expected %d, got %" PRISIZE_T "", szName, sd.nFuncs, sizeRead);
                return NULL;
            }
            if (CParserBin::m_bEndians)
            {
                SwapEndian(&prc.m_AffectedFuncs[0], sd.nFuncs, eBigEndian);
            }

            nSeek += (sd.nFuncs) * sizeof(int32) + sizeof(sd);
        }
    }

    char nameLwr[256];
    cry_strcpy(nameLwr, szName);
    azstrlwr(nameLwr, AZ_ARRAY_SIZE(nameLwr));
    pBin->SetName(szNameBin);
    pBin->m_dwName = CParserBin::GetCRC32(nameLwr);

    return pBin;
}

SShaderBin* CShaderManBin::SearchInCache(const char* szName, bool bInclude)
{
    char nameFile[256], nameLwr[256];
    cry_strcpy(nameLwr, szName);
    azstrlwr(nameLwr, AZ_ARRAY_SIZE(nameLwr));
    sprintf_s(nameFile, "%s.%s", nameLwr, bInclude ? "cfi" : "cfx");
    uint32 dwName = CParserBin::GetCRC32(nameFile);

    SShaderBin* pSB;
    for (pSB = SShaderBin::s_Root.m_Prev; pSB != &SShaderBin::s_Root; pSB = pSB->m_Prev)
    {
        if (pSB->m_dwName == dwName)
        {
            pSB->Unlink();
            pSB->Link(&SShaderBin::s_Root);
            return pSB;
        }
    }
    return NULL;
}

bool CShaderManBin::AddToCache(SShaderBin* pSB, bool bInclude)
{
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_shadersediting)
    {
        if (SShaderBin::s_nCache >= SShaderBin::s_nMaxFXBinCache)
        {
            SShaderBin* pS;
            for (pS = SShaderBin::s_Root.m_Prev; pS != &SShaderBin::s_Root; pS = pS->m_Prev)
            {
                if (!pS->m_bLocked)
                {
                    DeleteFromCache(pS);
                    break;
                }
            }
        }
        CRY_ASSERT(SShaderBin::s_nCache < SShaderBin::s_nMaxFXBinCache);
    }

    pSB->m_bInclude = bInclude;
    pSB->Link(&SShaderBin::s_Root);
    SShaderBin::s_nCache++;

    return true;
}

bool CShaderManBin::DeleteFromCache(SShaderBin* pSB)
{
    CRY_ASSERT(pSB != &SShaderBin::s_Root);
    pSB->Unlink();
    SAFE_DELETE(pSB);
    SShaderBin::s_nCache--;

    return true;
}

void CShaderManBin::InvalidateCache(bool bIncludesOnly)
{
    SShaderBin* pSB, * Next;
    for (pSB = SShaderBin::s_Root.m_Next; pSB != &SShaderBin::s_Root; pSB = Next)
    {
        Next = pSB->m_Next;
        if (bIncludesOnly && !pSB->m_bInclude)
        {
            continue;
        }
        DeleteFromCache(pSB);
    }
    SShaderBin::s_nMaxFXBinCache = MAX_FXBIN_CACHE;
    m_bBinaryShadersLoaded = false;

    g_shaderBucketAllocator.cleanup();
    
#if defined(SHADERS_SERIALIZING)
    // Clear our .fxb cache if we are deleting our shader binary cache.
    // We end up initializing part of the shader system with D3D11 as the platform
    // and then switch to the real platform soon after, so make sure this cache is clean.
    gRenDev->m_cEF.ClearSResourceCache();
#endif    
}

#define SEC5_FILETIME 10*1000*1000*5


SShaderBin* CShaderManBin::GetBinShader(const char* szName, bool bInclude, uint32 nRefCRC, bool* pbChanged)
{
    //static float sfTotalTime = 0.0f;

    if (pbChanged)
    {
        if (gRenDev->IsEditorMode())
        {
            *pbChanged = false;
        }
    }

    //float fTime0 = iTimer->GetAsyncCurTime();

    SShaderBin* pSHB = SearchInCache(szName, bInclude);
    if (pSHB)
    {
        return pSHB;
    }
    SShaderBinHeader Header[2];
    memset(&Header, 0, 2 * sizeof(SShaderBinHeader));
    char nameFile[256], nameBin[256];
    AZ::IO::HandleType srcFileHandle = AZ::IO::InvalidHandle;
    sprintf_s(nameFile, "%sCryFX/%s.%s", gRenDev->m_cEF.m_ShadersPath.c_str(), szName, bInclude ? "cfi" : "cfx");
#if !defined(_RELEASE)
    uint32 nSourceCRC32 = 0;
    {
        srcFileHandle = gEnv->pCryPak->FOpen(nameFile, "rb");
        nSourceCRC32 = srcFileHandle != AZ::IO::InvalidHandle ? gEnv->pCryPak->ComputeCRC(nameFile) : 0;
    }
#endif
    //char szPath[1024];
    //getcwd(szPath, 1024);
    sprintf_s(nameBin, "%s%s.%s", m_pCEF->m_ShadersCache.c_str(), szName, bInclude ? "cfib" : "cfxb");
    AZ::IO::HandleType dstFileHandle = AZ::IO::InvalidHandle;
    int i = 0, n = 2;

    // don't load from the shadercache.pak when in editing mode
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersediting)
    {
        i = 1;
    }

    AZStd::string szDst = m_pCEF->m_szCachePath + nameBin;
    byte bValid = 0;
    float fVersion = FX_CACHE_VER;
    for (; i < n; i++)
    {
        if (dstFileHandle != AZ::IO::InvalidHandle)
        {
            gEnv->pCryPak->FClose(dstFileHandle);
        }
        if (!i)
        {
            if (n == 2)
            {
                char nameLwr[256];
                sprintf_s(nameLwr, "%s.%s", szName, bInclude ? "cfi" : "cfx");
                azstrlwr(nameLwr, AZ_ARRAY_SIZE(nameLwr));
                uint32 dwName = CParserBin::GetCRC32(nameLwr);
                FXShaderBinValidCRCItor itor = m_BinValidCRCs.find(dwName);
                if (itor != m_BinValidCRCs.end())
                {
                    CRY_ASSERT(itor->second == false);
                    continue;
                }
            }
            dstFileHandle = gEnv->pCryPak->FOpen(nameBin, "rb");
        }
        else
        {
            dstFileHandle = gEnv->pCryPak->FOpen(szDst.c_str(), "rb", AZ::IO::IArchive::FLAGS_NEVER_IN_PAK | AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FOPEN_ONDISK);
        }
        if (dstFileHandle == AZ::IO::InvalidHandle)
        {
            continue;
        }
        else
        {
            gEnv->pCryPak->FReadRaw(&Header[i], 1, sizeof(SShaderBinHeader), dstFileHandle);
            if (CParserBin::m_bEndians)
            {
                SwapEndian(Header[i], eBigEndian);
            }

#if !defined(_RELEASE)
            // check source crc changes
            if (nSourceCRC32 && nSourceCRC32 != Header[i].m_nSourceCRC32)
            {
                bValid |= 1 << i;
            }
            else
#endif
            {
                uint16 MinorVer = (uint16)(((float)fVersion - (float)(int)fVersion) * 10.1f);
                uint16 MajorVer = (uint16)fVersion;
                if (Header[i].m_VersionLow != MinorVer || Header[i].m_VersionHigh != MajorVer || Header[i].m_Magic != FOURCC_SHADERBIN)
                {
                    bValid |= 4 << i;
                }
                else
                if (nRefCRC && Header[i].m_CRC32 != nRefCRC)
                {
                    bValid |= 0x10 << i;
                }
            }
        }
        if (!(bValid & (0x15 << i)))
        {
            break;
        }
    }
    if (i == n)
    {
#if !defined(_RELEASE) && !defined(CONSOLE_CONST_CVAR_MODE)
        {
            char acTemp[512];
            if (bValid & 1)
            {
                sprintf_s(acTemp, 512, "WARNING: Bin FXShader '%s' source crc mismatch", nameBin);
            }
            if (bValid & 4)
            {
                sprintf_s(acTemp, 512, "WARNING: Bin FXShader '%s' version mismatch (Cache: %d.%d, Expected: %.1f)", nameBin, Header[0].m_VersionHigh, Header[0].m_VersionLow, fVersion);
            }
            if (bValid & 0x10)
            {
                sprintf_s(acTemp, 512, "WARNING: Bin FXShader '%s' CRC mismatch", nameBin);
            }

            if (bValid & 2)
            {
                sprintf_s(acTemp, 512, "WARNING: Bin FXShader USER '%s' source crc mismatch", nameBin);
            }
            if (bValid & 8)
            {
                sprintf_s(acTemp, 512, "WARNING: Bin FXShader USER '%s' version mismatch (Cache: %d.%d, Expected: %.1f)", nameBin, Header[1].m_VersionHigh, Header[1].m_VersionLow, fVersion);
            }
            if (bValid & 0x20)
            {
                sprintf_s(acTemp, 512, "WARNING: Bin FXShader USER '%s' CRC mismatch", nameBin);
            }

            if (bValid)
            {
                LogWarningEngineOnly(acTemp);
            }

            if (dstFileHandle != AZ::IO::InvalidHandle)
            {
                gEnv->pCryPak->FClose(dstFileHandle);
                dstFileHandle = AZ::IO::InvalidHandle;
            }

            if (srcFileHandle != AZ::IO::InvalidHandle)
            {
                // enable shader compilation again, and show big error message
                if (!CRenderer::CV_r_shadersAllowCompilation)
                {
                    if (CRenderer::CV_r_shaderscompileautoactivate)
                    {
                        CRenderer::CV_r_shadersAllowCompilation = 1;
                        CRenderer::CV_r_shadersasyncactivation = 0;

                        gEnv->pLog->LogError("ERROR: LOADING BIN SHADER - REACTIVATING SHADER COMPILATION !");
                    }
                    else
                    {
                        static bool bShowMessageBox = true;

                        if (bShowMessageBox)
                        {
                            AZStd::string result;
                            EBUS_EVENT_RESULT(result, AZ::NativeUI::NativeUIRequestBus, DisplayOkDialog, "Invalid ShaderCache", acTemp, true);
                            if (result == "Cancel")
                            {
                                DebugBreak();
                            }
                            else if (!result.empty())
                            {
                                bShowMessageBox = false;
                                Sleep(33);
                            }
                            else
                            {
                                Warning("Invalid ShaderCache");
                            }
                        }
                    }
                }

                if (CRenderer::CV_r_shadersAllowCompilation)
                {
                    pSHB = SaveBinShader(nSourceCRC32, szName, bInclude, srcFileHandle);
                    CRY_ASSERT(!pSHB->m_Next);
                    if (pbChanged)
                    {
                        *pbChanged = true;
                    }

                    // remove the entries in the looupdata, to be sure that level and global caches have also become invalid for these shaders!
                    gRenDev->m_cEF.m_ResLookupDataMan[CACHE_READONLY].RemoveData(Header[0].m_CRC32);
                    gRenDev->m_cEF.m_ResLookupDataMan[CACHE_USER].RemoveData(Header[1].m_CRC32);

                    // has the shader been successfully written to the dest address
                    dstFileHandle = gEnv->pCryPak->FOpen(szDst.c_str(), "rb", AZ::IO::IArchive::FLAGS_NEVER_IN_PAK | AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FOPEN_ONDISK);
                    if (dstFileHandle != AZ::IO::InvalidHandle)
                    {
                        SAFE_DELETE(pSHB);
                        i = 1;
                    }
                }
            }
        }
#endif
    }
    if (srcFileHandle != AZ::IO::InvalidHandle)
    {
        gEnv->pCryPak->FClose(srcFileHandle);
        srcFileHandle = AZ::IO::InvalidHandle;
    }

    if (!CRenderer::CV_r_shadersAllowCompilation)
    {
        if (pSHB == 0 && dstFileHandle == AZ::IO::InvalidHandle)
        {
            //do only perform the necessary stuff
            dstFileHandle = gEnv->pCryPak->FOpen(nameBin, "rb");
        }
    }
    if (pSHB == 0 && dstFileHandle != AZ::IO::InvalidHandle)
    {
        sprintf_s(nameFile, "%s.%s", szName, bInclude ? "cfi" : "cfx");
        pSHB = LoadBinShader(dstFileHandle, nameFile, i == 0 ? nameBin : szDst.c_str(), !bInclude);
        gEnv->pCryPak->FClose(dstFileHandle);
        dstFileHandle = AZ::IO::InvalidHandle;
        AZ_Assert(pSHB, "Error loading binary shader '%s'.", nameFile);
    }

    if (pSHB)
    {
        if (i == 0)
        {
            pSHB->m_bReadOnly = true;
        }
        else
        {
            pSHB->m_bReadOnly = false;
        }

        AddToCache(pSHB, bInclude);
        if (!bInclude)
        {
            char nm[128];
            nm[0] = '$';
            azstrcpy(&nm[1], AZ_ARRAY_SIZE(nm) - 1, szName);
            CCryNameTSCRC NM = CParserBin::GetPlatformSpecName(nm);
            FXShaderBinPathItor it = m_BinPaths.find(NM);
            if (it == m_BinPaths.end())
            {
                m_BinPaths.insert(FXShaderBinPath::value_type(NM, i == 0 ? nameBin : szDst.c_str()));
            }
            else
            {
                it->second = (i == 0) ? nameBin : szDst.c_str();
            }
        }
    }
    else
    {
        if (dstFileHandle != AZ::IO::InvalidHandle)
        {
            Warning("Error: Failed to get binary shader '%s'", nameFile);
        }
        else
        {
            sprintf_s(nameFile, "%s.%s", szName, bInclude ? "cfi" : "cfx");
            const char* matName = 0;
            if (m_pCEF && m_pCEF->m_pCurInputResources)
            {
                matName = m_pCEF->m_pCurInputResources->m_szMaterialName;
            }
            iLog->LogWarning("WARN: Shader \"%s\" doesn't exist (used in material \"%s\")", nameFile, matName != 0 ? matName : "$unknown$");
        }
    }

    /*
    sfTotalTime += iTimer->GetAsyncCurTime() - fTime0;

    {
        char acTmp[1024];
        sprintf_s(acTmp, "Parsing of bin took: %f \n", sfTotalTime);
        OutputDebugString(acTmp);
    }
    */

    return pSHB;
}

void CShaderManBin::AddGenMacroses(SShaderGen* shG, CParserBin& Parser, uint64 nMaskGen, bool ignoreShaderGenMask /*=false*/)
{
    if (!nMaskGen || !shG)
    {
        return;
    }

    uint32 dwMacro = eT_1;
    for (uint32 i = 0; i < shG->m_BitMask.Num(); i++)
    {
        if (shG->m_BitMask[i]->m_Mask & nMaskGen)
        {
            Parser.AddMacro(shG->m_BitMask[i]->m_dwToken, &dwMacro, 1, ignoreShaderGenMask ? 0 : shG->m_BitMask[i]->m_Mask, Parser.m_Macros[1]);
        }
    }
}

bool CShaderManBin::ParseBinFX_Global_Annotations(CParserBin& Parser, SParserFrame& Frame, bool* bPublic, [[maybe_unused]] CCryNameR techStart[2])
{
    bool bRes = true;

    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(ShaderType)
    FX_TOKEN(ShaderDrawType)
    FX_TOKEN(PreprType)
    FX_TOKEN(Public)
    FX_TOKEN(NoPreview)
    FX_TOKEN(LocalConstants)
    FX_TOKEN(Cull)
    FX_TOKEN(SupportsAttrInstancing)
    FX_TOKEN(SupportsConstInstancing)
    FX_TOKEN(SupportsDeferredShading)
    FX_TOKEN(SupportsFullDeferredShading)
    FX_TOKEN(Decal)
    FX_TOKEN(DecalNoDepthOffset)
    FX_TOKEN(HWTessellation)
    FX_TOKEN(ZPrePass)
    FX_TOKEN(VertexColors)
    FX_TOKEN(NoChunkMerging)
    FX_TOKEN(ForceTransPass)
    FX_TOKEN(AfterHDRPostProcess)
    FX_TOKEN(AfterPostProcess)
    FX_TOKEN(ForceZpass)
    FX_TOKEN(ForceWaterPass)
    FX_TOKEN(ForceDrawLast)
    FX_TOKEN(ForceDrawFirst)
    FX_TOKEN(Hair)
    FX_TOKEN(SkinPass)
    FX_TOKEN(ForceGeneralPass)
    FX_TOKEN(ForceDrawAfterWater)
    FX_TOKEN(DepthFixup)
    FX_TOKEN(SingleLightPass)
    FX_TOKEN(Refractive)
    FX_TOKEN(ForceRefractionUpdate)
    FX_TOKEN(WaterParticle)
    FX_TOKEN(VT_DetailBending)
    FX_TOKEN(VT_DetailBendingGrass)
    FX_TOKEN(VT_WindBending)
    FX_TOKEN(AlphaBlendShadows)
    FX_TOKEN(EyeOverlay)
    FX_END_TOKENS

    int nIndex;
    CShader* ef = Parser.m_pCurShader;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_Public:
            if (bPublic)
            {
                *bPublic = true;
            }
            break;

        case eT_NoPreview:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_NOPREVIEW;
            break;

        case eT_Decal:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_DECAL;
            ef->m_nMDV |= MDV_DEPTH_OFFSET;
            break;

        case eT_DecalNoDepthOffset:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_DECAL;
            break;


        case eT_LocalConstants:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_LOCALCONSTANTS;
            break;

        case eT_VT_DetailBending:
            if (!ef)
            {
                break;
            }
            ef->m_nMDV |= MDV_DET_BENDING;
            break;
        case eT_VT_DetailBendingGrass:
            if (!ef)
            {
                break;
            }
            ef->m_nMDV |= MDV_DET_BENDING | MDV_DET_BENDING_GRASS;
            break;
        case eT_VT_WindBending:
            if (!ef)
            {
                break;
            }
            ef->m_nMDV |= MDV_WIND;
            break;

        case eT_NoChunkMerging:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_NOCHUNKMERGING;
            break;

        case eT_SupportsAttrInstancing:
            if (!ef)
            {
                break;
            }
            if (gRenDev->m_bDeviceSupportsInstancing)
            {
                ef->m_Flags |= EF_SUPPORTSINSTANCING_ATTR;
            }
            break;
        case eT_SupportsConstInstancing:
            if (!ef)
            {
                break;
            }
            if (gRenDev->m_bDeviceSupportsInstancing)
            {
                ef->m_Flags |= EF_SUPPORTSINSTANCING_CONST;
            }
            break;

        case eT_SupportsDeferredShading:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_SUPPORTSDEFERREDSHADING_MIXED;
            break;

        case eT_SupportsFullDeferredShading:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_SUPPORTSDEFERREDSHADING_FULL;
            break;

        case eT_ForceTransPass:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_FORCE_TRANSPASS;
            break;

        case eT_AfterHDRPostProcess:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_AFTERHDRPOSTPROCESS;
            break;

        case eT_AfterPostProcess:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_AFTERPOSTPROCESS;
            break;

        case eT_ForceZpass:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_FORCE_ZPASS;
            break;

        case eT_ForceWaterPass:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_FORCE_WATERPASS;
            break;

        case eT_ForceDrawLast:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_FORCE_DRAWLAST;
            break;
        case eT_ForceDrawFirst:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_FORCE_DRAWFIRST;
            break;

        case eT_Hair:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_HAIR;
            break;

        case eT_ForceGeneralPass:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_FORCE_GENERALPASS;
            break;

        case eT_ForceDrawAfterWater:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_FORCE_DRAWAFTERWATER;
            break;
        case eT_DepthFixup:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_DEPTH_FIXUP;
            break;
        case eT_SingleLightPass:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_SINGLELIGHTPASS;
            break;
        case eT_WaterParticle:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_WATERPARTICLE;
            break;
        case eT_Refractive:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_REFRACTIVE;
            break;
        case eT_ForceRefractionUpdate:
            if (!ef)
            {
                break;
            }
            ef->m_Flags |= EF_FORCEREFRACTIONUPDATE;
            break;

        case eT_ZPrePass:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_ZPREPASS;
            break;

        case eT_HWTessellation:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_HW_TESSELLATION;
            break;

        case eT_AlphaBlendShadows:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_ALPHABLENDSHADOWS;
            break;

        case eT_SkinPass:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_SKINPASS;
            break;

        case eT_EyeOverlay:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_EYE_OVERLAY;
            break;

        case eT_VertexColors:
            if (!ef)
            {
                break;
            }
            ef->m_Flags2 |= EF2_VERTEXCOLORS;
            break;

        case eT_ShaderDrawType:
        {
            if (!ef)
            {
                break;
            }
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_Light)
            {
                ef->m_eSHDType = eSHDT_Light;
            }
            else
            if (eT == eT_Shadow)
            {
                ef->m_eSHDType = eSHDT_Shadow;
            }
            else
            if (eT == eT_Fur)
            {
                ef->m_eSHDType = eSHDT_Fur;
            }
            else
            if (eT == eT_General)
            {
                ef->m_eSHDType = eSHDT_General;
            }
            else
            if (eT == eT_Terrain)
            {
                ef->m_eSHDType = eSHDT_Terrain;
            }
            else
            if (eT == eT_Overlay)
            {
                ef->m_eSHDType = eSHDT_Overlay;
            }
            else
            if (eT == eT_NoDraw)
            {
                ef->m_eSHDType = eSHDT_NoDraw;
                ef->m_Flags |= EF_NODRAW;
            }
            else
            if (eT == eT_Custom)
            {
                ef->m_eSHDType = eSHDT_CustomDraw;
            }
            else
            if (eT == eT_Sky)
            {
                ef->m_eSHDType = eSHDT_Sky;
                ef->m_Flags |= EF_SKY;
            }
            else
            if (eT == eT_OceanShore)
            {
                ef->m_eSHDType = eSHDT_OceanShore;
            }
            else
            {
                Warning("Unknown shader draw type '%s'", Parser.GetString(eT));
                CRY_ASSERT(0);
            }
        }
        break;

        case eT_ShaderType:
        {
            if (!ef)
            {
                break;
            }
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_General)
            {
                ef->m_eShaderType = eST_General;
            }
            else
            if (eT == eT_Metal)
            {
                ef->m_eShaderType = eST_Metal;
            }
            else
            if (eT == eT_Ice)
            {
                ef->m_eShaderType = eST_Ice;
            }
            else
            if (eT == eT_Shadow)
            {
                ef->m_eShaderType = eST_Shadow;
            }
            else
            if (eT == eT_Water)
            {
                ef->m_eShaderType = eST_Water;
            }
            else
            if (eT == eT_FX)
            {
                ef->m_eShaderType = eST_FX;
            }
            else
            if (eT == eT_PostProcess)
            {
                ef->m_eShaderType = eST_PostProcess;
            }
            else
            if (eT == eT_HDR)
            {
                ef->m_eShaderType = eST_HDR;
            }
            else
            if (eT == eT_Sky)
            {
                ef->m_eShaderType = eST_Sky;
            }
            else
            if (eT == eT_Glass)
            {
                ef->m_eShaderType = eST_Glass;
            }
            else
            if (eT == eT_Vegetation)
            {
                // Do nothing here
            }
            else
            if (eT == eT_Particle)
            {
                // Do nothing here
            }
            else
            if (eT == eT_Terrain)
            {
                 //Do nothing here
            }
            else
            if (eT == eT_Compute)
            {
                ef->m_eShaderType = eST_Compute;
            }
            else
            {
                Warning("Unknown shader type '%s'", Parser.GetString(eT));
                CRY_ASSERT(0);
            }
        }
        break;

        case eT_PreprType:
        {
            if (!ef)
            {
                break;
            }
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_GenerateSprites)
            {
                ; // We can't get of the token without cleaning out all the content but we can make it not do anything;
            }
            else
            if (eT == eT_GenerateClouds)
            {
                ef->m_Flags2 |= EF2_PREPR_GENCLOUDS;
            }
            else
            if (eT == eT_ScanWater)
            {
                ef->m_Flags2 |= EF2_PREPR_SCANWATER;
            }
            else
            {
                Warning("Unknown preprocess type '%s'", Parser.GetString(eT));
                CRY_ASSERT(0);
            }
        }
        break;

        case eT_Cull:
        {
            if (!ef)
            {
                break;
            }
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_None || eT == eT_NONE)
            {
                ef->m_eCull = eCULL_None;
            }
            else
            if (eT == eT_CCW || eT == eT_Back)
            {
                ef->m_eCull = eCULL_Back;
            }
            else
            if (eT == eT_CW || eT == eT_Front)
            {
                ef->m_eCull = eCULL_Front;
            }
            else
            {
                CRY_ASSERT(0);
            }
        }
        break;

        default:
            CRY_ASSERT(0);
        }
    }

    Parser.EndFrame(OldFrame);

    return bRes;
}

bool CShaderManBin::ParseBinFX_Global(CParserBin& Parser, SParserFrame& Frame, bool* bPublic, CCryNameR techStart[2])
{
    bool bRes = true;

    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(string)
    FX_END_TOKENS

    int nIndex;
    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_string:
        {
            eT = Parser.GetToken(Parser.m_Name);
            CRY_ASSERT(eT == eT_Script);
            bRes &= ParseBinFX_Global_Annotations(Parser, Parser.m_Data, bPublic, techStart);
        }
        break;

        default:
            CRY_ASSERT(0);
        }
    }

    Parser.EndFrame(OldFrame);

    return bRes;
}

static int sGetTAddress(uint32 nToken)
{
    switch (nToken)
    {
    case eT_Clamp:
        return TADDR_CLAMP;
    case eT_Border:
        return TADDR_BORDER;
    case eT_Wrap:
        return TADDR_WRAP;
    case eT_Mirror:
        return TADDR_MIRROR;
    default:
        CRY_ASSERT(0);
    }
    return -1;
}

void STexSamplerFX::PostLoad()
{
    SHRenderTarget* pRt = m_pTarget;
    if (!pRt)
    {
        return;
    }
    if (!_strnicmp(m_szTexture.c_str(), "$RT_2D", 6))
    {
        if (pRt->m_nIDInPool >= 0)
        {
            if ((int)CTexture::s_CustomRT_2D->Num() <= pRt->m_nIDInPool)
            {
                CTexture::s_CustomRT_2D->Expand(pRt->m_nIDInPool + 1);
            }
        }
        pRt->m_pTarget[0] = CTexture::s_ptexRT_2D;
    }
}

bool CShaderManBin::ParseBinFX_Sampler_Annotations_Script(CParserBin& Parser, SParserFrame& Frame, STexSamplerFX* pSampler)
{
    bool bRes = true;

    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(RenderOrder)
    FX_TOKEN(ProcessOrder)
    FX_TOKEN(RenderCamera)
    FX_TOKEN(RenderType)
    FX_TOKEN(RenderFilter)
    FX_TOKEN(RenderColorTarget1)
    FX_TOKEN(RenderDepthStencilTarget)
    FX_TOKEN(ClearSetColor)
    FX_TOKEN(ClearSetDepth)
    FX_TOKEN(ClearTarget)
    FX_TOKEN(RenderTarget_IDPool)
    FX_TOKEN(RenderTarget_UpdateType)
    FX_TOKEN(RenderTarget_Width)
    FX_TOKEN(RenderTarget_Height)
    FX_TOKEN(GenerateMips)
    FX_END_TOKENS

    SHRenderTarget * pRt = new SHRenderTarget;
    int nIndex;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_RenderOrder:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_PreProcess)
            {
                pRt->m_eOrder = eRO_PreProcess;
            }
            else
            if (eT == eT_PostProcess)
            {
                pRt->m_eOrder = eRO_PostProcess;
            }
            else
            if (eT == eT_PreDraw)
            {
                pRt->m_eOrder = eRO_PreDraw;
            }
            else
            {
                CRY_ASSERT(0);
                Warning("Unknown RenderOrder type '%s'", Parser.GetString(eT));
            }
        }
        break;

        case eT_ProcessOrder:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_WaterReflection)
            {
                pRt->m_nProcessFlags = FSPR_SCANTEXWATER;
            }
            else
            {
                CRY_ASSERT(0);
                Warning("Unknown ProcessOrder type '%s'", Parser.GetString(eT));
            }
        }
        break;

        case eT_RenderCamera:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_WaterPlaneReflected)
            {
                pRt->m_nFlags |= FRT_CAMERA_REFLECTED_WATERPLANE;
            }
            else
            if (eT == eT_PlaneReflected)
            {
                pRt->m_nFlags |= FRT_CAMERA_REFLECTED_PLANE;
            }
            else
            if (eT == eT_Current)
            {
                pRt->m_nFlags |= FRT_CAMERA_CURRENT;
            }
            else
            {
                CRY_ASSERT(0);
                Warning("Unknown RenderCamera type '%s'", Parser.GetString(eT));
            }
        }
        break;

        case eT_GenerateMips:
            pRt->m_nFlags |= FRT_GENERATE_MIPS;
            break;

        case eT_RenderType:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_CurObject)
            {
                pRt->m_nFlags |= FRT_RENDTYPE_CUROBJECT;
            }
            else
            if (eT == eT_CurScene)
            {
                pRt->m_nFlags |= FRT_RENDTYPE_CURSCENE;
            }
            else
            if (eT == eT_RecursiveScene)
            {
                pRt->m_nFlags |= FRT_RENDTYPE_RECURSIVECURSCENE;
            }
            else
            if (eT == eT_CopyScene)
            {
                pRt->m_nFlags |= FRT_RENDTYPE_COPYSCENE;
            }
            else
            {
                CRY_ASSERT(0);
                Warning("Unknown RenderType type '%s'", Parser.GetString(eT));
            }
        }
        break;

        case eT_RenderFilter:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_Refractive)
            {
                pRt->m_nFilterFlags |= FRF_REFRACTIVE;
            }
            else
            if (eT == eT_Heat)
            {
                pRt->m_nFilterFlags |= FRF_HEAT;
            }
            else
            {
                CRY_ASSERT(0);
                Warning("Unknown RenderFilter type '%s'", Parser.GetString(eT));
            }
        }
        break;

        case eT_RenderDepthStencilTarget:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_DepthBuffer || eT == eT_DepthBufferTemp)
            {
                pRt->m_bTempDepth = true;
            }
            else
            if (eT == eT_DepthBufferOrig)
            {
                pRt->m_bTempDepth = false;
            }
            else
            {
                CRY_ASSERT(0);
                Warning("Unknown RenderDepthStencilTarget type '%s'", Parser.GetString(eT));
            }
        }
        break;

        case eT_RenderTarget_IDPool:
            pRt->m_nIDInPool = Parser.GetInt(Parser.GetToken(Parser.m_Data));
            CRY_ASSERT(pRt->m_nIDInPool >= 0 && pRt->m_nIDInPool < 64);
            break;

        case eT_RenderTarget_Width:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_$ScreenSize)
            {
                pRt->m_nWidth = -1;
            }
            else
            {
                pRt->m_nWidth = Parser.GetInt(eT);
            }
        }
        break;

        case eT_RenderTarget_Height:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_$ScreenSize)
            {
                pRt->m_nHeight = -1;
            }
            else
            {
                pRt->m_nHeight = Parser.GetInt(eT);
            }
        }
        break;

        case eT_RenderTarget_UpdateType:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_WaterReflect)
            {
                pRt->m_eUpdateType = eRTUpdate_WaterReflect;
            }
            else
            if (eT == eT_Allways)
            {
                pRt->m_eUpdateType = eRTUpdate_Always;
            }
            else
            {
                CRY_ASSERT(0);
            }
        }
        break;

        case eT_ClearSetColor:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_FogColor)
            {
                pRt->m_nFlags |= FRT_CLEAR_FOGCOLOR;
            }
            else
            {
                string sStr = Parser.GetString(Parser.m_Data);
                shGetColor(sStr.c_str(), pRt->m_ClearColor);
            }
        }
        break;

        case eT_ClearSetDepth:
            pRt->m_fClearDepth = Parser.GetFloat(Parser.m_Data);
            break;

        case eT_ClearTarget:
        {
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_Color)
            {
                pRt->m_nFlags |= FRT_CLEAR_COLOR;
            }
            else
            if (eT == eT_Depth)
            {
                pRt->m_nFlags |= FRT_CLEAR_DEPTH;
            }
            else
            {
                CRY_ASSERT(0);
                Warning("Unknown ClearTarget type '%s'", Parser.GetString(eT));
            }
        }
        break;

        default:
            CRY_ASSERT(0);
        }
    }
    if (pRt->m_eOrder == eRO_PreProcess)
    {
        Parser.m_pCurShader->m_Flags |= EF_PRECACHESHADER;
    }
    pSampler->m_pTarget = pRt;
    pSampler->PostLoad();

    Parser.EndFrame(OldFrame);

    return bRes;
}

bool CShaderManBin::ParseBinFX_Sampler_Annotations(CParserBin& Parser, SParserFrame& Annotations, STexSamplerFX* pSampler)
{
    bool bRes = true;

    SParserFrame OldFrame = Parser.BeginFrame(Annotations);

    FX_BEGIN_TOKENS
        FX_TOKEN(string)
    FX_END_TOKENS

    int nIndex = 0;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        SCodeFragment Fr;
        switch (eT)
        {
        case eT_string:
        {
            eT = Parser.GetToken(Parser.m_Name);
            CRY_ASSERT(eT == eT_Script);
            bRes &= ParseBinFX_Sampler_Annotations_Script(Parser, Parser.m_Data, pSampler);
        }
        break;

        default:
            CRY_ASSERT(0);
        }
    }

    Parser.EndFrame(OldFrame);

    return bRes;
}

bool CShaderManBin::ParseBinFX_Sampler(CParserBin& Parser, SParserFrame& Frame, uint32 dwName, SParserFrame Annotations, EToken samplerType)
{
    bool bRes = true;

    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
    FX_TOKEN(string)
    FX_TOKEN(Texture)
    FX_TOKEN(MinFilter)
    FX_TOKEN(MagFilter)
    FX_TOKEN(MipFilter)
    FX_TOKEN(AddressU)
    FX_TOKEN(AddressV)
    FX_TOKEN(AddressW)
    FX_TOKEN(BorderColor)
    FX_TOKEN(AnisotropyLevel)
    FX_TOKEN(sRGBLookup)
    FX_TOKEN(Global)
    FX_END_TOKENS

    STexSamplerFX samp;
    STexState ST;
    DWORD dwBorderColor = 0;
    uint32 nFilter = 0;
    uint32 nFiltMin = 0;
    uint32 nFiltMip = 0;
    uint32 nFiltMag = 0;
    uint32 nAddressU = 0;
    uint32 nAddressV = 0;
    uint32 nAddressW = 0;
    uint32 nAnisotropyLevel = 0;

    int nIndex = -1;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_string:
        {
            eT = Parser.GetToken(Parser.m_Name);

            SParserFrame stringData = Parser.m_Data;

            // string could have ""s still, if so trim them off.
            if (Parser.GetToken(stringData) == eT_quote)
            {
                stringData.m_nFirstToken++;
                stringData.m_nLastToken--;
            }

#if SHADER_REFLECT_TEXTURE_SLOTS
            if (eT == eT_UIName)
            {
                samp.m_szUIName = Parser.GetString(stringData);
            }
            else if (eT == eT_UIDescription)
            {
                samp.m_szUIDescription = Parser.GetString(stringData);
            }
#endif
        }
        break;

        case eT_Texture:
            samp.m_szTexture = Parser.GetString(Parser.m_Data);
            break;

        case eT_BorderColor:
        {
            string Str = Parser.GetString(Parser.m_Data);
            ColorF colBorder = Col_Black;
            shGetColor(Str.c_str(), colBorder);
            dwBorderColor = colBorder.pack_argb8888();
            ST.m_bActive = true;
        }
        break;

        case eT_sRGBLookup:
            ST.m_bSRGBLookup = Parser.GetBool(Parser.m_Data);
            break;

        case eT_Global:
            break;

        case eT_AnisotropyLevel:
            nAnisotropyLevel = Parser.GetToken(Parser.m_Data);
            break;

        case eT_Filter:
            nFilter = Parser.GetToken(Parser.m_Data);
            ST.m_bActive = true;
            break;
        case eT_MinFilter:
            nFiltMin = Parser.GetToken(Parser.m_Data);
            ST.m_bActive = true;
            break;
        case eT_MagFilter:
            nFiltMag = Parser.GetToken(Parser.m_Data);
            ST.m_bActive = true;
            break;
        case eT_MipFilter:
            nFiltMip = Parser.GetToken(Parser.m_Data);
            ST.m_bActive = true;
            break;
        case eT_AddressU:
            nAddressU = sGetTAddress(Parser.GetToken(Parser.m_Data));
            ST.m_bActive = true;
            break;
        case eT_AddressV:
            nAddressV = sGetTAddress(Parser.GetToken(Parser.m_Data));
            ST.m_bActive = true;
            break;
        case eT_AddressW:
            nAddressW = sGetTAddress(Parser.GetToken(Parser.m_Data));
            ST.m_bActive = true;
            break;

        default:
            CRY_ASSERT(0);
        }
    }

    samp.m_szName = Parser.GetString(dwName);
    if (nFilter > 0)
    {
        switch (nFilter)
        {
        case eT_MIN_MAG_MIP_POINT:
            ST.SetFilterMode(FILTER_POINT);
            break;
        case eT_MIN_MAG_MIP_LINEAR:
            ST.SetFilterMode(FILTER_TRILINEAR);
            break;
        case eT_MIN_MAG_LINEAR_MIP_POINT:
            ST.SetFilterMode(FILTER_BILINEAR);
            break;
        case eT_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
            ST.SetFilterMode(FILTER_BILINEAR);
            ST.SetComparisonFilter(true);
            break;
        default:
        {
            assert(0);
        }
        break;
        }
    }

    if (nFiltMag > 0 && nFiltMin > 0 && nFiltMip > 0)
    {
        if (nFiltMag == eT_LINEAR && nFiltMin == eT_LINEAR && nFiltMip == eT_LINEAR)
        {
            ST.SetFilterMode(FILTER_TRILINEAR);
        }
        else
        if (nFiltMag == eT_LINEAR && nFiltMin == eT_LINEAR && nFiltMip == eT_POINT)
        {
            ST.SetFilterMode(FILTER_BILINEAR);
        }
        else
        if (nFiltMag == eT_LINEAR && nFiltMin == eT_LINEAR && nFiltMip == eT_NONE)
        {
            ST.SetFilterMode(FILTER_LINEAR);
        }
        else
        if (nFiltMag == eT_POINT && nFiltMin == eT_POINT && nFiltMip == eT_POINT)
        {
            ST.SetFilterMode(FILTER_POINT);
        }
        else
        if (nFiltMag == eT_POINT && nFiltMin == eT_POINT && nFiltMip == eT_NONE)
        {
            ST.SetFilterMode(FILTER_NONE);
        }
        else
        if (nFiltMag == eT_ANISOTROPIC || nFiltMin == eT_ANISOTROPIC)
        {
            if (nAnisotropyLevel == eT_0 + 4)
            {
                ST.SetFilterMode(FILTER_ANISO4X);
            }
            else
            if (nAnisotropyLevel == eT_0 + 8)
            {
                ST.SetFilterMode(FILTER_ANISO8X);
            }
            else
            if (nAnisotropyLevel == eT_0 + 16)
            {
                ST.SetFilterMode(FILTER_ANISO16X);
            }
            else
            {
                ST.SetFilterMode(FILTER_ANISO2X);
            }
        }
        else
        {
            Warning("!Unknown sampler filter mode (Min=%s, Mag=%s, Mip=%s) for sampler '%s'", Parser.GetString(nFiltMin), Parser.GetString(nFiltMag), Parser.GetString(nFiltMip), samp.m_szName.c_str());
            CRY_ASSERT(0);
        }
    }
    else
    {
        ST.SetFilterMode(-1);
    }

    ST.SetClampMode(nAddressU, nAddressV, nAddressW);
    ST.SetBorderColor(dwBorderColor);

    samp.m_nTexState = CTexture::GetTexState(ST);
    samp.m_nSlotId = m_pCEF->mfCheckTextureSlotName(samp.m_szTexture);

    if (!Annotations.IsEmpty())
    {
        bRes &= ParseBinFX_Sampler_Annotations(Parser, Annotations, &samp);
    }

    switch (samplerType)
    {
    case eT_sampler1D:
        samp.m_eTexType = eTT_1D;
        break;
    case eT_sampler2D:
    case eT_Texture2D:
        samp.m_eTexType = eTT_2D;
        break;
    case eT_Texture2DArray:
        samp.m_eTexType = eTT_2DArray;
        break;
    case eT_Texture2DMS:
        samp.m_eTexType = eTT_2DMS;
        break;
    case eT_sampler3D:
    case eT_Texture3D:
        samp.m_eTexType = eTT_3D;
        break;
    case eT_samplerCUBE:
    case eT_TextureCube:
        samp.m_eTexType = eTT_Cube;
        break;
    case eT_TextureCubeArray:
        samp.m_eTexType = eTT_CubeArray;
        break;
    default:
#if !defined(_RELEASE)
        __debugbreak();
#endif
        samp.m_eTexType = eTT_2D;
        break;
    }

    mfAddFXSampler(Parser.m_pCurShader, &samp);

    Parser.EndFrame(OldFrame);

    return bRes;
}

bool CShaderManBin::ParseBinFX_Sampler(CParserBin& Parser, SParserFrame& Frame, SFXSampler& Sampl)
{
    bool bRes = true;

    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(string)
    FX_TOKEN(Filter)
    FX_TOKEN(MinFilter)
    FX_TOKEN(MagFilter)
    FX_TOKEN(MipFilter)
    FX_TOKEN(AddressU)
    FX_TOKEN(AddressV)
    FX_TOKEN(AddressW)
    FX_TOKEN(BorderColor)
    FX_TOKEN(AnisotropyLevel)
    FX_TOKEN(sRGBLookup)
    FX_TOKEN(Global)
    FX_END_TOKENS

    STexState ST;
    DWORD dwBorderColor = 0;
    uint32 nFilter = 0;
    uint32 nFiltMin = 0;
    uint32 nFiltMip = 0;
    uint32 nFiltMag = 0;
    uint32 nAddressU = 0;
    uint32 nAddressV = 0;
    uint32 nAddressW = 0;
    uint32 nAnisotropyLevel = 0;

    int nIndex = -1;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_BorderColor:
        {
            string Str = Parser.GetString(Parser.m_Data);
            ColorF colBorder = Col_Black;
            shGetColor(Str.c_str(), colBorder);
            dwBorderColor = colBorder.pack_argb8888();
            ST.m_bActive = true;
        }
        break;

        case eT_sRGBLookup:
            ST.m_bSRGBLookup = Parser.GetBool(Parser.m_Data);
            break;

        case eT_AnisotropyLevel:
            nAnisotropyLevel = Parser.GetToken(Parser.m_Data);
            break;

        case eT_Filter:
            nFilter = Parser.GetToken(Parser.m_Data);
            ST.m_bActive = true;
            break;

        case eT_MinFilter:
            nFiltMin = Parser.GetToken(Parser.m_Data);
            ST.m_bActive = true;
            break;
        case eT_MagFilter:
            nFiltMag = Parser.GetToken(Parser.m_Data);
            ST.m_bActive = true;
            break;
        case eT_MipFilter:
            nFiltMip = Parser.GetToken(Parser.m_Data);
            ST.m_bActive = true;
            break;
        case eT_AddressU:
            nAddressU = sGetTAddress(Parser.GetToken(Parser.m_Data));
            ST.m_bActive = true;
            break;
        case eT_AddressV:
            nAddressV = sGetTAddress(Parser.GetToken(Parser.m_Data));
            ST.m_bActive = true;
            break;
        case eT_AddressW:
            nAddressW = sGetTAddress(Parser.GetToken(Parser.m_Data));
            ST.m_bActive = true;
            break;

        default:
            CRY_ASSERT(0);
        }
    }

    if (nFilter > 0)
    {
        switch (nFilter)
        {
        case eT_MIN_MAG_MIP_POINT:
            ST.SetFilterMode(FILTER_POINT);
            break;
        case eT_MIN_MAG_LINEAR_MIP_POINT:
            ST.SetFilterMode(FILTER_BILINEAR);
            break;
        case eT_MIN_MAG_MIP_LINEAR:
            ST.SetFilterMode(FILTER_TRILINEAR);
            break;
        case eT_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
            ST.SetFilterMode(FILTER_BILINEAR);
            ST.SetComparisonFilter(true);
            break;
        default:
        {
            CRY_ASSERT(0);
        }
        break;
        }
    }
    if (nFiltMag > 0 && nFiltMin > 0 && nFiltMip > 0)
    {
        if (nFiltMag == eT_LINEAR && nFiltMin == eT_LINEAR && nFiltMip == eT_LINEAR)
        {
            ST.SetFilterMode(FILTER_TRILINEAR);
        }
        else
        if (nFiltMag == eT_LINEAR && nFiltMin == eT_LINEAR && nFiltMip == eT_POINT)
        {
            ST.SetFilterMode(FILTER_BILINEAR);
        }
        else
        if (nFiltMag == eT_LINEAR && nFiltMin == eT_LINEAR && nFiltMip == eT_NONE)
        {
            ST.SetFilterMode(FILTER_LINEAR);
        }
        else
        if (nFiltMag == eT_POINT && nFiltMin == eT_POINT && nFiltMip == eT_POINT)
        {
            ST.SetFilterMode(FILTER_POINT);
        }
        else
        if (nFiltMag == eT_POINT && nFiltMin == eT_POINT && nFiltMip == eT_NONE)
        {
            ST.SetFilterMode(FILTER_NONE);
        }
        else
        if (nFiltMag == eT_ANISOTROPIC || nFiltMin == eT_ANISOTROPIC)
        {
            if (nAnisotropyLevel == eT_0 + 4)
            {
                ST.SetFilterMode(FILTER_ANISO4X);
            }
            else
            if (nAnisotropyLevel == eT_0 + 8)
            {
                ST.SetFilterMode(FILTER_ANISO8X);
            }
            else
            if (nAnisotropyLevel == eT_0 + 16)
            {
                ST.SetFilterMode(FILTER_ANISO16X);
            }
            else
            {
                ST.SetFilterMode(FILTER_ANISO2X);
            }
        }
        else
        {
            Warning("!Unknown sampler filter mode (Min=%s, Mag=%s, Mip=%s) for sampler '%s'", Parser.GetString(nFiltMin), Parser.GetString(nFiltMag), Parser.GetString(nFiltMip), Sampl.m_Name.c_str());
            CRY_ASSERT(0);
        }
    }
    else
    {
        ST.SetFilterMode(-1);
    }

    if (ST.m_bActive)
    {
        ST.SetClampMode(nAddressU, nAddressV, nAddressW);
        ST.SetBorderColor(dwBorderColor);

        Sampl.m_nTexState = CTexture::GetTexState(ST);
    }

    Parser.EndFrame(OldFrame);

    return bRes;
}

bool CShaderManBin::ParseBinFX_Texture(CParserBin& Parser, SParserFrame& Frame, SFXTexture& Tex)
{
    bool bRes = true;

    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(Texture)
    FX_TOKEN(UIName)
    FX_TOKEN(UIDescription)
    FX_TOKEN(sRGBLookup)
    FX_TOKEN(Global)
    FX_TOKEN(slot)
    FX_TOKEN(vsslot)
    FX_TOKEN(psslot)
    FX_TOKEN(hsslot)
    FX_TOKEN(dsslot)
    FX_TOKEN(gsslot)
    FX_TOKEN(csslot)
    FX_END_TOKENS

    int nIndex = -1;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_Texture:
        {
            Tex.m_szTexture = Parser.GetString(Parser.m_Data);
        }
        break;
        case eT_UIName:
        {
            if (Parser.GetToken(Parser.m_Data) == eT_quote)
            {
                Parser.m_Data.m_nFirstToken++;
                Parser.m_Data.m_nLastToken--;
            }
            Tex.m_szUIName = Parser.GetString(Parser.m_Data);
        }
        break;
        case eT_UIDescription:
        {
            if (Parser.GetToken(Parser.m_Data) == eT_quote)
            {
                Parser.m_Data.m_nFirstToken++;
                Parser.m_Data.m_nLastToken--;
            }
            Tex.m_szUIDesc = Parser.GetString(Parser.m_Data);
        }
        break;

        case eT_sRGBLookup:
            Tex.m_bSRGBLookup = Parser.GetBool(Parser.m_Data);
            break;

        case eT_Global:
            Tex.m_nFlags |= PF_GLOBAL;
            break;

        case eT_slot:
        case eT_vsslot:
        case eT_psslot:
        case eT_hsslot:
        case eT_dsslot:
        case eT_gsslot:
        case eT_csslot:
            break;

        default:
            CRY_ASSERT(0);
        }
    }


    Parser.EndFrame(OldFrame);

    return bRes;
}

void CShaderManBin::AddAffectedParameter(CParserBin& Parser, std::vector<SFXParam>& AffectedParams, TArray<int>& AffectedFunc, SFXParam* pParam, EHWShaderClass eSHClass, [[maybe_unused]] uint32 dwType, SShaderTechnique* pShTech)
{
    static uint32 eT_ShadowGen[] = {CParserBin::GetCRC32("ShadowGenVS"), CParserBin::GetCRC32("ShadowGenPS"), CParserBin::GetCRC32("ShadowGenGS")};
    if (!CParserBin::PlatformSupportsConstantBuffers())
    {
        //if (pParam->m_Name.c_str()[0] != '_')
        {
            if (!(Parser.m_pCurShader->m_Flags & EF_LOCALCONSTANTS))
            {
                if (pParam->m_Register[eSHClass] >= 0 && pParam->m_Register[eSHClass] < 10000)
                {
                    if ((pShTech->m_Flags & FHF_NOLIGHTS) && pParam->m_BindingSlot == eConstantBufferShaderSlot_PerMaterial && !(pParam->m_nFlags & PF_TWEAKABLE_MASK))
                    {
                        return;
                    }
                    if (pParam->m_Semantic.empty() && pParam->m_Values.c_str()[0] == '(')
                    {
                        pParam->m_BindingSlot = eConstantBufferShaderSlot_PerMaterial;
                    }
                    AffectedParams.push_back(*pParam);
                    return;
                }
            }
        }
    }
    else
    if (pParam->m_BindingSlot == eConstantBufferShaderSlot_PerMaterial)
    {
        if (pParam->m_Register[eSHClass] < 0 || pParam->m_Register[eSHClass] >= 1000)
        {
            return;
        }
    }

    int nFlags = pParam->GetFlags();
    bool bCheckAffect = CParserBin::m_bParseFX ? true : false;

    if (CParserBin::m_nPlatform == SF_D3D11 || CParserBin::m_nPlatform == SF_JASPER || CParserBin::m_nPlatform == SF_ORBIS || CParserBin::m_nPlatform == SF_GL4 || CParserBin::m_nPlatform == SF_GLES3 || CParserBin::m_nPlatform == SF_METAL)
    {
        CRY_ASSERT(eSHClass < eHWSC_Num);
        if (((nFlags & PF_TWEAKABLE_MASK) || pParam->m_Values.c_str()[0] == '(') && pParam->m_Register[eSHClass] >= 0 && pParam->m_Register[eSHClass] < 1000)
        {
            bCheckAffect = false;
        }
    }
    for (uint32 j = 0; j < AffectedFunc.size(); j++)
    {
        SCodeFragment* pCF = &Parser.m_CodeFragments[AffectedFunc[j]];
        if (!bCheckAffect || Parser.FindToken(pCF->m_nFirstToken, pCF->m_nLastToken, pParam->m_dwName[0]) >= 0)
        {
            AffectedParams.push_back(*pParam);
            break;
        }
    }
}

void CShaderManBin::InitShaderDependenciesList(CParserBin& Parser, SCodeFragment* pFunc, TArray<byte>& bChecked, TArray<int>& AffectedFunc)
{
    uint32 i;
    const uint32 numFrags = Parser.m_CodeFragments.size();

    for (i = 0; i < numFrags; ++i)
    {
        if (bChecked[i])
        {
            continue;
        }
        SCodeFragment* s = &Parser.m_CodeFragments[i];
        if (!s->m_dwName)
        {
            bChecked[i] = 1;
            continue;
        }
        if (s->m_eType == eFT_Function || s->m_eType == eFT_StorageClass)
        {
            if (Parser.FindToken(pFunc->m_nFirstToken, pFunc->m_nLastToken, s->m_dwName) >= 0)
            {
                bChecked[i] = 1;
                AffectedFunc.push_back(i);
                InitShaderDependenciesList(Parser, s, bChecked, AffectedFunc);
            }
        }
    }
}

void CShaderManBin::AddAffectedSampler(CParserBin& Parser, std::vector<SFXSampler>& AffectedSamplers, TArray<int>& AffectedFunc, SFXSampler* pSampler, [[maybe_unused]] EHWShaderClass eSHClass, [[maybe_unused]] uint32 dwType, [[maybe_unused]] SShaderTechnique* pShTech)
{
    for (uint32 j = 0; j < AffectedFunc.size(); j++)
    {
        SCodeFragment* pCF = &Parser.m_CodeFragments[AffectedFunc[j]];
        if (Parser.FindToken(pCF->m_nFirstToken, pCF->m_nLastToken, pSampler->m_dwName[0]) >= 0)
        {
            AffectedSamplers.push_back(*pSampler);
            break;
        }
    }
}
void CShaderManBin::AddAffectedTexture(CParserBin& Parser, std::vector<SFXTexture>& AffectedTextures, TArray<int>& AffectedFunc, SFXTexture* pTexture, [[maybe_unused]] EHWShaderClass eSHClass, [[maybe_unused]] uint32 dwType, [[maybe_unused]] SShaderTechnique* pShTech)
{
    for (uint32 j = 0; j < AffectedFunc.size(); j++)
    {
        SCodeFragment* pCF = &Parser.m_CodeFragments[AffectedFunc[j]];
        if (Parser.FindToken(pCF->m_nFirstToken, pCF->m_nLastToken, pTexture->m_dwName[0]) >= 0)
        {
            AffectedTextures.push_back(*pTexture);
            break;
        }
    }
}

void CShaderManBin::CheckStructuresDependencies(CParserBin& Parser, SCodeFragment* pFrag, TArray<byte>& bChecked, TArray<int>& AffectedFunc)
{
    uint32 i;
    const uint32 numFrags = Parser.m_CodeFragments.size();

    for (i = 0; i < numFrags; ++i)
    {
        if (bChecked[i])
        {
            continue;
        }
        SCodeFragment* s = &Parser.m_CodeFragments[i];
        if (s->m_eType == eFT_Structure)
        {
            if (Parser.FindToken(pFrag->m_nFirstToken, pFrag->m_nLastToken, s->m_dwName) >= 0)
            {
                bChecked[i] = 1;
                AffectedFunc.push_back(i);
                CheckStructuresDependencies(Parser, s, bChecked, AffectedFunc);
            }
        }
    }
}

void CShaderManBin::CheckFragmentsDependencies(CParserBin& Parser, TArray<byte>& bChecked, TArray<int>& AffectedFrags)
{
    uint32 i, j;
    const uint32 numFuncs = AffectedFrags.size();
    const uint32 numFrags = Parser.m_CodeFragments.size();

    for (i = 0; i < numFuncs; ++i)
    {
        int nFunc = AffectedFrags[i];
        SCodeFragment* pFunc = &Parser.m_CodeFragments[nFunc];
        for (j = 0; j < numFrags; j++)
        {
            SCodeFragment* s = &Parser.m_CodeFragments[j];
            if (bChecked[j])
            {
                continue;
            }

            if (s->m_eType == eFT_Sampler || s->m_eType == eFT_Structure)
            {
                if (Parser.FindToken(pFunc->m_nFirstToken, pFunc->m_nLastToken, s->m_dwName) >= 0)
                {
                    bChecked[j] = 1;
                    AffectedFrags.push_back(j);
                    if (s->m_eType == eFT_Structure)
                    {
                        CheckStructuresDependencies(Parser, s, bChecked, AffectedFrags);
                    }
                }
            }
            else if (s->m_eType == eFT_ConstBuffer)
            {
                // Make sure cbuffer declaration does not get stripped
                bChecked[j] = 1;
                AffectedFrags.push_back(j);
            }
        }
    }
}

//=================================================================================================

struct SFXRegisterBin
{
    int nReg;
    int nComp;
    int m_nCB;
    uint32 m_nFlags;
    EParamType eType;
    uint32 dwName;
    CCryNameR m_Value;
    SFXRegisterBin()
    {
        eType = eType_UNKNOWN;
        m_nCB = -1;
    }
};

struct SFXPackedName
{
    uint32 dwName[4];

    SFXPackedName()
    {
        dwName[0] = dwName[1] = dwName[2] = dwName[3] = 0;
    }
};

inline bool Compar(const SFXRegisterBin& a, const SFXRegisterBin& b)
{
    if (CParserBin::PlatformSupportsConstantBuffers())
    {
        if (a.m_nCB != b.m_nCB)
        {
            return a.m_nCB < b.m_nCB;
        }
    }
    if (a.nReg != b.nReg)
    {
        return a.nReg < b.nReg;
    }
    if (a.nComp != b.nComp)
    {
        return a.nComp < b.nComp;
    }
    return false;
}

static void sFlushRegs(CParserBin& Parser, int nReg, SFXRegisterBin* MergedRegs[4], std::vector<SFXParam>& NewParams, EHWShaderClass eSHClass, std::vector<SFXPackedName>& PackedNames)
{
    NewParams.resize(NewParams.size() + 1);
    PackedNames.resize(PackedNames.size() + 1);
    SFXParam& p = NewParams[NewParams.size() - 1];
    SFXPackedName& pN = PackedNames[PackedNames.size() - 1];
    int nMaxComp = -1;
    int j;

    int nCompMerged = 0;
    EParamType eType = eType_UNKNOWN;
    int nCB = -1; 
    for (j = 0; j < 4; j++)
    {
        if (MergedRegs[j] && MergedRegs[j]->eType != eType_UNKNOWN)
        {
            if (nCB == -1)
            {
                nCB = MergedRegs[j]->m_nCB;
            }
            else
            if (nCB != MergedRegs[j]->m_nCB)
            {
                CRY_ASSERT(0);
            }
            if (eType == eType_UNKNOWN)
            {
                eType = MergedRegs[j]->eType;
            }
            else
            if (eType != MergedRegs[j]->eType)
            {
                //CRY_ASSERT(0);
            }
        }
    }
    for (j = 0; j < 4; j++)
    {
        char s[16];
        sprintf_s(s, "__%d", j);
        p.m_Name = stack_string(p.m_Name.c_str()) + s;
        if (MergedRegs[j])
        {
            if (MergedRegs[j]->m_nFlags & PF_TWEAKABLE_MASK)
            {
                p.m_nFlags |= (PF_TWEAKABLE_0 << j);
            }
            p.m_nFlags |= MergedRegs[j]->m_nFlags & ~(PF_TWEAKABLE_MASK | PF_SCALAR | PF_SINGLE_COMP | PF_MERGE_MASK);
            nMaxComp = max(nMaxComp, j);
            pN.dwName[j] = MergedRegs[j]->dwName;
            if (nCompMerged)
            {
                p.m_Values = stack_string(p.m_Values.c_str()) += ", ";
            }
            p.m_Name = stack_string(p.m_Name.c_str()) + Parser.GetString(pN.dwName[j]);
            p.m_Values = stack_string(p.m_Values.c_str()) + MergedRegs[j]->m_Value.c_str();
            nCompMerged++;
        }
        else
        {
            if (nCompMerged)
            {
                p.m_Values = stack_string(p.m_Values.c_str()) + ", ";
            }
            p.m_Values = stack_string(p.m_Values.c_str()) + "NULL";
            nCompMerged++;
        }
    }
    p.m_ComponentCount = nMaxComp + 1;
    if (eSHClass == eHWSC_Geometry && !CParserBin::PlatformSupportsGeometryShaders())
    {
        return;
    }
    if (eSHClass == eHWSC_Domain && !CParserBin::PlatformSupportsDomainShaders())
    {
        return;
    }
    if (eSHClass == eHWSC_Hull && !CParserBin::PlatformSupportsHullShaders())
    {
        return;
    }
    if (eSHClass == eHWSC_Compute && !CParserBin::PlatformSupportsComputeShaders())
    {
        return;
    }

    // Get packed name token.
    p.m_dwName.push_back(Parser.NewUserToken(eT_unknown, p.m_Name.c_str(), true)); //pass by crysting so that string references work.
    CRY_ASSERT(eSHClass < eHWSC_Num);
    p.m_Register[eSHClass] = nReg;
    if (eSHClass == eHWSC_Domain || eSHClass == eHWSC_Hull || eSHClass == eHWSC_Compute)
    {
        p.m_Register[eHWSC_Vertex] = nReg;
    }
    p.m_RegisterCount = 1;
    if (eType == eType_HALF)
    {
        eType = eType_FLOAT;
    }
    p.m_eType = eType;
    CRY_ASSERT(eType != eType_UNKNOWN);
    p.m_BindingSlot = nCB;
    p.m_nFlags |= PF_AUTOMERGED;
}

static EToken sCompToken[4] = {eT_x, eT_y, eT_z, eT_w};
bool CShaderManBin::ParseBinFX_Technique_Pass_PackParameters (CParserBin& Parser, std::vector<SFXParam>& AffectedParams, TArray<int>& AffectedFunc, [[maybe_unused]] SCodeFragment* pFunc, EHWShaderClass eSHClass, uint32 dwSHName, std::vector<SFXParam>& PackedParams, TArray<SCodeFragment>& Replaces, TArray<uint32>& NewTokens, TArray<byte>& bMerged)
{
    bool bRes = true;

    uint32 i, j;

    std::vector<SFXRegisterBin> Registers;
    std::vector<SFXPackedName> PackedNames;
    for (i = 0; i < AffectedParams.size(); i++)
    {
        SFXParam* pr = &AffectedParams[i];
        if (pr->m_Annotations.empty())
        {
            continue; // Parameter doesn't have custom register definition
        }
        const char* src = strstr(pr->m_Annotations.c_str(), "register");
        if (src)
        {
            char* b = (char*)&src[8];
            SkipCharacters (&b, kWhiteSpace);
            CRY_ASSERT(b[0] == '=');
            b++;
            SkipCharacters (&b, kWhiteSpace);
            CRY_ASSERT(b[0] == 'c');
            if (b[0] == 'c')
            {
                int nReg = atoi(&b[1]);
                b++;
                while (*b != '.' && *b != ';')
                {
                    if (*b == 0) // Vector without swizzling
                    {
                        break;
                    }
                    b++;
                }
                if (*b == '.')
                {
                    bMerged[i] = 0xff;
                    b++;
                    int nComp = -1;
                    switch (b[0])
                    {
                    case 'x':
                    case 'r':
                        nComp = 0;
                        break;
                    case 'y':
                    case 'g':
                        nComp = 1;
                        break;
                    case 'z':
                    case 'b':
                        nComp = 2;
                        break;
                    case 'w':
                    case 'a':
                        nComp = 3;
                        break;
                    default:
                        CRY_ASSERT(0);
                    }
                    if (nComp >= 0)
                    {
                        pr->m_nFlags |= PF_MERGE;
                        SFXRegisterBin rg;
                        rg.nReg = nReg;
                        rg.nComp = nComp;
                        rg.dwName = pr->m_dwName[0];
                        rg.m_Value = pr->m_Values;
                        rg.m_nFlags = pr->m_nFlags;
                        rg.eType = (EParamType)pr->m_eType;
                        rg.m_nCB = pr->m_BindingSlot;
                        CRY_ASSERT(rg.m_Value.c_str()[0]);
                        Registers.push_back(rg);
                    }
                }
            }
        }
    }
    if (Registers.empty())
    {
        return false;
    }
    std::sort(Registers.begin(), Registers.end(), Compar);
    int nReg = -1;
    int nCB = -1;
    SFXRegisterBin* MergedRegs[4];
    MergedRegs[0] = MergedRegs[1] = MergedRegs[2] = MergedRegs[3] = NULL;
    for (i = 0; i < Registers.size(); i++)
    {
        SFXRegisterBin* rg = &Registers[i];
        if ((!CParserBin::PlatformSupportsConstantBuffers() && rg->nReg != nReg) || (CParserBin::PlatformSupportsConstantBuffers() && (rg->m_nCB != nCB || rg->nReg != nReg)))
        {
            if (nReg >= 0)
            {
                sFlushRegs(Parser, nReg, MergedRegs, PackedParams, eSHClass, PackedNames);
            }
            nReg = rg->nReg;
            nCB = rg->m_nCB;
            MergedRegs[0] = MergedRegs[1] = MergedRegs[2] = MergedRegs[3] = NULL;
        }
        CRY_ASSERT(!MergedRegs[rg->nComp]);
        if (MergedRegs[rg->nComp])
        {
            Warning("register c%d (comp: %d) is used by the %s shader '%s' already", rg->nReg, rg->nComp, eSHClass == eHWSC_Pixel ? "pixel" : "vertex", Parser.GetString(dwSHName));
            CRY_ASSERT(0);
        }
        MergedRegs[rg->nComp] = rg;
    }
    if (MergedRegs[0] || MergedRegs[1] || MergedRegs[2] || MergedRegs[3])
    {
        sFlushRegs(Parser, nReg, MergedRegs, PackedParams, eSHClass, PackedNames);
    }

    // Replace new parameters in shader tokens
    for (uint32 n = 0; n < AffectedFunc.size(); n++)
    {
        SCodeFragment* st = &Parser.m_CodeFragments[AffectedFunc[n]];
        //const char *szName = Parser.GetString(st->m_dwName);

        for (i = 0; i < PackedParams.size(); i++)
        {
            SFXParam* pr = &PackedParams[i];
            SFXPackedName* pn = &PackedNames[i];
            for (j = 0; j < 4; j++)
            {
                uint32 nP = pn->dwName[j];
                if (nP == 0)
                {
                    continue;
                }
                int32 nPos = st->m_nFirstToken;
                while (true)
                {
                    nPos = Parser.FindToken(nPos, st->m_nLastToken, nP);
                    if (nPos < 0)
                    {
                        break;
                    }
                    SCodeFragment Fr;
                    Fr.m_nFirstToken = Fr.m_nLastToken = nPos;
                    Fr.m_dwName = n;
                    Replaces.push_back(Fr);

                    Fr.m_nFirstToken = NewTokens.size();
                    NewTokens.push_back(pr->m_dwName[0]);
                    NewTokens.push_back(eT_dot);
                    NewTokens.push_back(sCompToken[j]);
                    Fr.m_nLastToken = NewTokens.size() - 1;
                    Replaces.push_back(Fr);
                    nPos++;
                }
            }
        }
    }

    for (i = 0; i < Replaces.size(); i += 2)
    {
        SCodeFragment* pFR1 = &Replaces[i];
        for (j = i + 2; j < Replaces.size(); j += 2)
        {
            SCodeFragment* pFR2 = &Replaces[j];
            if (pFR1->m_dwName != pFR2->m_dwName)
            {
                continue;
            }
            if (pFR2->m_nFirstToken < pFR1->m_nFirstToken)
            {
                std::swap(Replaces[i], Replaces[j]);
                std::swap(Replaces[i + 1], Replaces[j + 1]);
            }
        }
    }

    return bRes;
}

//===================================================================================================

void CShaderManBin::AddParameterToScript(CParserBin& Parser, SFXParam* pr, PodArray<uint32>& SHData, EHWShaderClass eSHClass, int nCB)
{
    char str[256];
    int nReg = pr->m_Register[eSHClass];
    if (pr->m_eType == eType_BOOL)
    {
        SHData.push_back(eT_bool);
    }
    else
    if (pr->m_eType == eType_INT)
    {
        SHData.push_back(eT_int);
    }
    else
    {
        int nVal = pr->m_RegisterCount * 4 + pr->m_ComponentCount;
        EToken eT = eT_unknown;
        switch (nVal)
        {
        case 4 + 1:
            eT = (pr->m_eType == eType_FLOAT) ? eT_float : eT_half;
            break;
        case 4 + 2:
            //eT = eT_float2;
            eT = (pr->m_eType == eType_FLOAT) ? eT_float2 : eT_half2;
            break;
        case 4 + 3:
            //eT = eT_float3;
            eT = (pr->m_eType == eType_FLOAT) ? eT_float3 : eT_half3;
            break;
        case 4 + 4:
            //eT = eT_float4;
            eT = (pr->m_eType == eType_FLOAT) ? eT_float4 : eT_half4;
            break;
        case 2 * 4 + 4:
            //eT = eT_float2x4;
            eT = (pr->m_eType == eType_FLOAT) ? eT_float2x4 : eT_half2x4;
            break;
        case 3 * 4 + 4:
            //eT = eT_float3x4;
            eT = (pr->m_eType == eType_FLOAT) ? eT_float3x4 : eT_half3x4;
            break;
        case 4 * 4 + 4:
            //eT = eT_float4x4;
            eT = (pr->m_eType == eType_FLOAT) ? eT_float4x4 : eT_half4x4;
            break;
        case 3 * 4 + 3:
            //eT = eT_float3x3;
            eT = (pr->m_eType == eType_FLOAT) ? eT_float3x3 : eT_half3x3;
            break;
        }
        CRY_ASSERT(eT != eT_unknown);
        if (eT == eT_unknown)
        {
            return;
        }
        SHData.push_back(eT);
    }
    for (uint32 i = 0; i < pr->m_dwName.size(); i++)
    {
        SHData.push_back(pr->m_dwName[i]);
    }

    if (nReg >= 0 && nReg < 10000)
    {
        SHData.push_back(eT_colon);
        if (nCB == eConstantBufferShaderSlot_PerMaterial)
        {
            SHData.push_back(eT_packoffset);
        }
        else
        {
            SHData.push_back(eT_register);
        }
        SHData.push_back(eT_br_rnd_1);
        sprintf_s(str, "c%d", nReg);
        SHData.push_back(Parser.NewUserToken(eT_unknown, str, true));
        SHData.push_back(eT_br_rnd_2);
    }
    SHData.push_back(eT_semicolumn);
}

void CShaderManBin::AddSamplerToScript(CParserBin& Parser, SFXSampler* pr, PodArray<uint32>& SHData, EHWShaderClass eSHClass)
{
    char str[256];
    int nReg = pr->m_Register[eSHClass];
    if (pr->m_eType == eSType_Sampler)
    {
        SHData.push_back(eT_SamplerState);
    }
    else
    if (pr->m_eType == eSType_SamplerComp)
    {
        SHData.push_back(eT_SamplerComparisonState);
    }
    else
    {
        CRY_ASSERT(0);
    }
    for (uint32 i = 0; i < pr->m_dwName.size(); i++)
    {
        SHData.push_back(pr->m_dwName[i]);
    }

    if (nReg >= 0 && nReg < 10000)
    {
        SHData.push_back(eT_colon);
        SHData.push_back(eT_register);
        SHData.push_back(eT_br_rnd_1);
        azsprintf(str, "s%d", nReg);
        SHData.push_back(Parser.NewUserToken(eT_unknown, str, true));
        SHData.push_back(eT_br_rnd_2);
    }
    SHData.push_back(eT_semicolumn);
}
void CShaderManBin::AddTextureToScript(CParserBin& Parser, SFXTexture* pr, PodArray<uint32>& SHData, EHWShaderClass eSHClass)
{
    char str[256];
    int nReg = pr->m_Register[eSHClass];
    if (pr->m_eType == eTT_2D)
    {
        SHData.push_back(eT_Texture2D);
    }
    else
    if (pr->m_eType == eTT_3D)
    {
        SHData.push_back(eT_Texture3D);
    }
    else
    if (pr->m_eType == eTT_2DArray)
    {
        SHData.push_back(eT_Texture2DArray);
    }
    else
    if (pr->m_eType == eTT_2DMS)
    {
        SHData.push_back(eT_Texture2DMS);
    }
    else
    if (pr->m_eType == eTT_Cube)
    {
        SHData.push_back(eT_TextureCube);
    }
    else
    if (pr->m_eType == eTT_CubeArray)
    {
        SHData.push_back(eT_TextureCubeArray);
    }
    else
    {
        CRY_ASSERT(0);
    }
    if (pr->m_Type != eT_unknown)
    {
        SHData.push_back(eT_br_tr_1);
        SHData.push_back(pr->m_Type);
        SHData.push_back(eT_br_tr_2);
    }
    for (uint32 i = 0; i < pr->m_dwName.size(); i++)
    {
        SHData.push_back(pr->m_dwName[i]);
    }

    if (nReg >= 0 && nReg < 10000)
    {
        SHData.push_back(eT_colon);
        SHData.push_back(eT_register);
        SHData.push_back(eT_br_rnd_1);
        azsprintf(str, "t%d", nReg);
        SHData.push_back(Parser.NewUserToken(eT_unknown, str, true));
        SHData.push_back(eT_br_rnd_2);
    }
    SHData.push_back(eT_semicolumn);
}

static EToken GeneratedConstantBufferNames[] = { eT_PER_BATCH, eT_PER_INSTANCE, eT_PER_MATERIAL };

bool CShaderManBin::ParseBinFX_Technique_Pass_GenerateShaderData(CParserBin& Parser, FXMacroBin& Macros, SShaderFXParams& FXParams, uint32 dwSHName, EHWShaderClass eSHClass, uint64& nAffectMask, uint32 dwSHType, PodArray<uint32>& SHData, SShaderTechnique* pShTech)
{
    LOADING_TIME_PROFILE_SECTION(iSystem);
    CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());

    bool bRes = true;

    std::vector<SFXParam> AffectedParams;
    std::vector<SFXSampler> AffectedSamplers;
    std::vector<SFXTexture> AffectedTextures;

    uint32 i, j;
    uint32 nNum;
    static TArray<int> AffectedFragments;
    AffectedFragments.reserve(120);
    AffectedFragments.SetUse(0);

    for (nNum = 0; nNum < Parser.m_CodeFragments.size(); nNum++)
    {
        if (dwSHName == Parser.m_CodeFragments[nNum].m_dwName)
        {
            break;
        }
    }
    if (nNum == Parser.m_CodeFragments.size())
    {
        AZ_Assert(false, "Couldn't find entry function '%s'", Parser.GetString(dwSHName));
        return false;
    }

    SCodeFragment* pFunc = &Parser.m_CodeFragments[nNum];
    SShaderBin* pBin = Parser.m_pCurBinShader;
    SParamCacheInfo* pCache = GetParamInfo(pBin, pFunc->m_dwName, Parser.m_pCurShader->m_nMaskGenFX, Parser.m_pCurShader->m_maskGenStatic);
    if (pCache)
    {
        AffectedFragments.SetUse(0);
        AffectedFragments.reserve(pCache->m_AffectedFuncs.size());
        if (pCache->m_AffectedFuncs.empty() == false)
        {
            AffectedFragments.Copy(&pCache->m_AffectedFuncs[0], pCache->m_AffectedFuncs.size());
        }
    }
    else
    {
        AffectedFragments.push_back(nNum);
        if (CParserBin::m_bParseFX)
        {
            TArray<byte> bChecked;
            bChecked.resize(Parser.m_CodeFragments.size());
            if (bChecked.size() > 0)
            {
                memset(&bChecked[0], 0, sizeof(byte) * bChecked.size());
            }
            bChecked[nNum] = 1;
            InitShaderDependenciesList(Parser, pFunc, bChecked, AffectedFragments);
            CheckFragmentsDependencies(Parser, bChecked, AffectedFragments);
        }
        else
        {
            for (i = 0; i < Parser.m_CodeFragments.size(); i++)
            {
                if (i != nNum)
                {
                    AffectedFragments.push_back(i);
                }
            }
        }
    }

    nAffectMask = 0;
    for (i = 0; i < AffectedFragments.size(); i++)
    {
        SCodeFragment* s = &Parser.m_CodeFragments[AffectedFragments[i]];
        if (s->m_eType != eFT_Function && s->m_eType != eFT_Structure && s->m_eType != eFT_ConstBuffer && s->m_eType != eFT_StorageClass)
        {
            continue;
        }
        FXMacroBinItor itor;
        for (itor = Macros.begin(); itor != Macros.end(); ++itor)
        {
            SMacroBinFX* pr = &itor->second;
            if (!pr->m_nMask)
            {
                continue;
            }
            if ((pr->m_nMask & nAffectMask) == pr->m_nMask)
            {
                continue;
            }
            uint32 dwName = itor->first;
            if (Parser.FindToken(s->m_nFirstToken, s->m_nLastToken, dwName) >= 0)
            {
                nAffectMask |= pr->m_nMask;
            }
        }
    }

    // Generate list of params before first preprocessor pass for affected functions
    TArray<byte> bMerged;
    bMerged.Reserve(FXParams.m_FXParams.size());
    if (pCache)
    {
        for (i = 0; i < pCache->m_AffectedParams.size(); i++)
        {
            uint32 nParam = pCache->m_AffectedParams[i];
            FXParamsIt it = std::lower_bound(FXParams.m_FXParams.begin(), FXParams.m_FXParams.end(), nParam, FXParamsSortByName());
            if (it != FXParams.m_FXParams.end() && nParam == (*it).m_dwName[0])
            {
                SFXParam& pr = (*it);
                if (!(pr.GetFlags() & PF_AUTOMERGED))
                {
                    if (pr.m_Semantic.empty() && pr.m_Values.c_str()[0] == '(')
                    {
                        pr.m_BindingSlot = eConstantBufferShaderSlot_PerMaterial;
                    }
                    AffectedParams.push_back(pr);
                }
            }
        }
        for (i = 0; i < pCache->m_AffectedSamplers.size(); i++)
        {
            uint32 nParam = pCache->m_AffectedSamplers[i];
            FXSamplersIt it = std::lower_bound(FXParams.m_FXSamplers.begin(), FXParams.m_FXSamplers.end(), nParam, FXSamplersSortByName());
            if (it != FXParams.m_FXSamplers.end() && nParam == (*it).m_dwName[0])
            {
                SFXSampler& pr = (*it);
                AffectedSamplers.push_back(pr);
            }
        }
        for (i = 0; i < pCache->m_AffectedTextures.size(); i++)
        {
            uint32 nParam = pCache->m_AffectedTextures[i];
            FXTexturesIt it = std::lower_bound(FXParams.m_FXTextures.begin(), FXParams.m_FXTextures.end(), nParam, FXTexturesSortByName());
            if (it != FXParams.m_FXTextures.end() && nParam == (*it).m_dwName[0])
            {
                SFXTexture& pr = (*it);
                AffectedTextures.push_back(pr);
            }
        }
    }
    else
    {
        for (i = 0; i < FXParams.m_FXParams.size(); i++)
        {
            SFXParam* pr = &FXParams.m_FXParams[i];
            if (!(pr->GetFlags() & PF_AUTOMERGED))
            {
                AddAffectedParameter(Parser, AffectedParams, AffectedFragments, pr, eSHClass, dwSHType, pShTech);
            }
        }
        for (i = 0; i < FXParams.m_FXSamplers.size(); i++)
        {
            SFXSampler* pr = &FXParams.m_FXSamplers[i];
            AddAffectedSampler(Parser, AffectedSamplers, AffectedFragments, pr, eSHClass, dwSHType, pShTech);
        }
        for (i = 0; i < FXParams.m_FXTextures.size(); i++)
        {
            SFXTexture* pr = &FXParams.m_FXTextures[i];
            AddAffectedTexture(Parser, AffectedTextures, AffectedFragments, pr, eSHClass, dwSHType, pShTech);
        }
    }

    if (CParserBin::m_bParseFX)
    {
        FXMacroBinItor itor;
        for (itor = Macros.begin(); itor != Macros.end(); ++itor)
        {
            if (itor->second.m_nMask && !(itor->second.m_nMask & nAffectMask))
            {
                continue;
            }
            SHData.push_back(eT_define);
            SHData.push_back(itor->first);
            SHData.push_back(0);
        }
        std::vector<SFXParam> PackedParams;
        TArray<SCodeFragment> Replaces;
        TArray<uint32> NewTokens;
        ParseBinFX_Technique_Pass_PackParameters (Parser, AffectedParams, AffectedFragments, pFunc, eSHClass, dwSHName, PackedParams, Replaces, NewTokens, bMerged);

        if (!pCache)
        {
            // Update new parameters in shader structures
            for (i = 0; i < PackedParams.size(); i++)
            {
                SFXParam* pr = &PackedParams[i];
                AffectedParams.push_back(*pr);
            }
            if (CRenderer::CV_r_shadersAllowCompilation)
            {
                SaveBinShaderLocalInfo(pBin, pFunc->m_dwName, Parser.m_pCurShader->m_nMaskGenFX, Parser.m_pCurShader->m_maskGenStatic, AffectedFragments, AffectedParams, AffectedSamplers, AffectedTextures);
            }
        }
        else
        {
            for (i = 0; i < pCache->m_AffectedParams.size(); i++)
            {
                int32 nParam = pCache->m_AffectedParams[i];
                for (j = 0; j < PackedParams.size(); j++)
                {
                    SFXParam& pr = PackedParams[j];
                    if (pr.m_dwName[0] == nParam)
                    {
                        AffectedParams.push_back(pr);
                        break;
                    }
                }
            }
        }

        // Update FX parameters
        for (i = 0; i < AffectedParams.size(); i++)
        {
            SFXParam* pr = &AffectedParams[i];
            mfAddFXParam(FXParams, pr);
        }

        // Include all affected functions/structures/parameters in the final script
        if (CParserBin::PlatformSupportsConstantBuffers()) // use CB scopes for D3D10 shaders
        {
            int8 nPrevCB = -1;
            std::vector<SFXParam*> ParamsData;

            for (i = 0; i < AffectedParams.size(); i++)
            {
                SFXParam& pr = AffectedParams[i];
                if (bMerged[i] == 0xff)
                {
                    continue;
                }
                if (pr.m_BindingSlot >= 0)
                {
                    ParamsData.push_back(&pr);
                }
            }

            const int shaderClassIndex = (eSHClass == eHWSC_Vertex) ? 0 : (eSHClass == eHWSC_Pixel) ? 1 : 2;

            std::sort(ParamsData.begin(), ParamsData.end(),
                [shaderClassIndex](const SFXParam* a, const SFXParam* b)
                {
                    const uint16 bindSlot0 = a->m_BindingSlot;
                    const uint16 bindSlot1 = b->m_BindingSlot;
                    const int16 register0 = a->m_Register[shaderClassIndex];
                    const int16 register1 = b->m_Register[shaderClassIndex];

                    if (bindSlot0 != bindSlot1)
                    {
                        return (bindSlot0 < bindSlot1);
                    }
                    return (register0 < register1);
                });

            // First we need to declare semantic variables (in CB scopes in case of DX11)
            for (i = 0; i < ParamsData.size(); i++)
            {
                SFXParam* pPr = ParamsData[i];
                int nCB = pPr->m_BindingSlot;
                nCB = pPr->m_BindingSlot;
                if (nPrevCB != nCB)
                {
                    if (nPrevCB != -1)
                    {
                        SHData.push_back(eT_br_cv_2);
                        SHData.push_back(eT_semicolumn);
                    }
                    SHData.push_back(eT_cbuffer);

                    AZ_Assert(nCB < AZ_ARRAY_SIZE(GeneratedConstantBufferNames), "Trying to generate a constant buffer at an invalid slot");
                    SHData.push_back(GeneratedConstantBufferNames[nCB]);
                    SHData.push_back(eT_colon);
                    SHData.push_back(eT_register);
                    char str[32];
                    sprintf_s(str, "b%d", nCB);
                    SHData.push_back(eT_br_rnd_1);
                    SHData.push_back(Parser.NewUserToken(eT_unknown, str, true));
                    SHData.push_back(eT_br_rnd_2);
                    SHData.push_back(eT_br_cv_1);
                }
                nPrevCB = nCB;
                AddParameterToScript(Parser, pPr, SHData, eSHClass, nCB);
            }
            if (nPrevCB >= 0)
            {
                nPrevCB = -1;
                SHData.push_back(eT_br_cv_2);
                SHData.push_back(eT_semicolumn);
            }

            for (i = 0; i < AffectedSamplers.size(); i++)
            {
                SFXSampler* pr = &AffectedSamplers[i];
                AddSamplerToScript(Parser, pr, SHData, eSHClass);
            }
            for (i = 0; i < AffectedTextures.size(); i++)
            {
                SFXTexture* pr = &AffectedTextures[i];
                AddTextureToScript(Parser, pr, SHData, eSHClass);
            }
        }
        else
        {
            // Update affected parameters in script
#ifdef _DEBUG
            for (i = 0; i < AffectedParams.size(); i++)
            {
                SFXParam* pr = &AffectedParams[i];
                for (j = i + 1; j < AffectedParams.size(); j++)
                {
                    SFXParam* pr1 = &AffectedParams[j];
                    if (pr->m_dwName[0] == pr1->m_dwName[0])
                    {
                        CRY_ASSERT(0);
                    }
                }
            }
#endif
            for (i = 0; i < AffectedParams.size(); i++)
            {
                SFXParam* pr = &AffectedParams[i];
                // Ignore parameters which where packed
                if (bMerged[i] == 0xff)
                {
                    continue;
                }
                AddParameterToScript(Parser, pr, SHData, eSHClass, -1);
            }
        }

        // Generate fragment tokens
        for (i = 0; i < Parser.m_CodeFragments.size(); i++)
        {
            int h = -1;
            SCodeFragment* cf = &Parser.m_CodeFragments[i];
            if (cf->m_dwName)
            {
                for (h = 0; h < (int)AffectedFragments.size(); h++)
                {
                    if (AffectedFragments[h] == i)
                    {
                        break;
                    }
                }
                if (h == AffectedFragments.size())
                {
                    continue;
                }
            }

            Parser.CopyTokens(cf, SHData, Replaces, NewTokens, h);
            if (cf->m_eType == eFT_Sampler)
            {
                if (CParserBin::m_nPlatform == SF_D3D11 || CParserBin::m_nPlatform == SF_JASPER || CParserBin::m_nPlatform == SF_GL4 || CParserBin::m_nPlatform == SF_GLES3 || CParserBin::m_nPlatform == SF_METAL)
                {
                    int nT = Parser.m_Tokens[cf->m_nLastToken - 1];
                    //CRY_ASSERT(nT >= eT_s0 && nT <= eT_s15);
                    if (nT >= eT_s0 && nT <= eT_s15)
                    {
                        nT = nT - eT_s0 + eT_t0;
                        SHData.push_back(eT_colon);
                        SHData.push_back(eT_register);
                        SHData.push_back(eT_br_rnd_1);
                        SHData.push_back(nT);
                        SHData.push_back(eT_br_rnd_2);
                    }
                }
                SHData.push_back(eT_semicolumn);
            }
        }
    }
    return bRes;
}

bool CShaderManBin::ParseBinFX_Technique_Pass_LoadShader([[maybe_unused]] CParserBin& Parser, [[maybe_unused]] FXMacroBin& Macros, [[maybe_unused]] SParserFrame& SHFrame, [[maybe_unused]] SShaderTechnique* pShTech, [[maybe_unused]] SShaderPass* pPass, [[maybe_unused]] EHWShaderClass eSHClass, [[maybe_unused]] SShaderFXParams& FXParams)
{
    assert (gRenDev->m_pRT->IsRenderThread());

    LOADING_TIME_PROFILE_SECTION(iSystem);
    bool bRes = true;

#ifndef NULL_RENDERER

    CRY_ASSERT(!SHFrame.IsEmpty());

    uint32 dwSHName;
    uint32 dwSHType = 0;

    uint32* pTokens = &Parser.m_Tokens[0];

    dwSHName = pTokens[SHFrame.m_nFirstToken];
    uint32 nCur = SHFrame.m_nFirstToken + 1;
    uint32 nTok = pTokens[nCur];
    if (nTok != eT_br_rnd_1)
    {
        nCur += 2;
        nTok = pTokens[nCur];
    }
    nCur++;
    assert (nTok == eT_br_rnd_1);
    if (nTok == eT_br_rnd_1)
    {
        nTok = pTokens[nCur];
        if (nTok != eT_br_rnd_2)
        {
            CRY_ASSERT(!"Local function parameters aren't supported anymore");
        }
    }
    nCur++;
    if (nCur <= SHFrame.m_nLastToken)
    {
        dwSHType = pTokens[nCur];
    }

    enum
    {
        SHDATA_BUFFER_SIZE = 131072
    };

    uint64 nGenMask = 0;
    PodArray<uint32> SHDataBuffer(SHDATA_BUFFER_SIZE);
    bRes &= ParseBinFX_Technique_Pass_GenerateShaderData(Parser, Macros, FXParams, dwSHName, eSHClass, nGenMask, dwSHType, SHDataBuffer, pShTech);
#if !defined(_RELEASE)
    if (SHDataBuffer.size() > SHDATA_BUFFER_SIZE)
    {
        CryLogAlways("CShaderManBin::ParseBinFX_Technique_Pass_LoadShader: SHDataBuffer has been exceeded (buffer=%u, count=%u). Adjust buffer size to remove unnecessary allocs", SHDATA_BUFFER_SIZE, SHDataBuffer.Size());
    }
#endif
    TArray<uint32> SHData(0, SHDataBuffer.Size());
    SHData.Copy(SHDataBuffer.GetElements(), SHDataBuffer.size());

    CHWShader* pSH = NULL;
    bool bValidShader = false;

    CShader* efSave = gRenDev->m_RP.m_pShader;
    gRenDev->m_RP.m_pShader = Parser.m_pCurShader;
    CRY_ASSERT(gRenDev->m_RP.m_pShader != 0);
    if (bRes && (!CParserBin::m_bParseFX || !SHData.empty()))
    {
        const char* szName = Parser.GetString(dwSHName);
        char str[1024];
        sprintf_s(str, "%s@%s", Parser.m_pCurShader->m_NameShader.c_str(), szName);
        pSH = CHWShader::mfForName(str, Parser.m_pCurShader->m_NameFile, Parser.m_pCurShader->m_CRC32, szName, eSHClass, SHData, &Parser.m_TokenTable, dwSHType, Parser.m_pCurShader, nGenMask, Parser.m_pCurShader->m_nMaskGenFX);
    }
    if (pSH)
    {
        bValidShader = true;

        if (eSHClass == eHWSC_Vertex)
        {
            pPass->m_VShader = pSH;
        }
        else
        if (eSHClass == eHWSC_Pixel)
        {
            pPass->m_PShader = pSH;
        }
        else
        if (CParserBin::PlatformSupportsGeometryShaders() && eSHClass == eHWSC_Geometry)
        {
            pPass->m_GShader = pSH;
        }
        else
        if (CParserBin::PlatformSupportsDomainShaders() && eSHClass == eHWSC_Domain)
        {
            pPass->m_DShader = pSH;
        }
        else
        if (CParserBin::PlatformSupportsHullShaders() && eSHClass == eHWSC_Hull)
        {
            pPass->m_HShader = pSH;
        }
        else
        if (CParserBin::PlatformSupportsComputeShaders() && eSHClass == eHWSC_Compute)
        {
            pPass->m_CShader = pSH;
        }
        else
        {
            CryLog("Unsupported/unrecognised shader: %s[%d]", pSH->m_Name.c_str(), eSHClass);
        }
    }

    gRenDev->m_RP.m_pShader = efSave;
#endif

    return bRes;
}

bool CShaderManBin::ParseBinFX_Technique_Pass(CParserBin& Parser, SParserFrame& Frame, SShaderTechnique* pShTech)
{
    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(VertexShader)
    FX_TOKEN(PixelShader)
    FX_TOKEN(GeometryShader)
    FX_TOKEN(DomainShader)
    FX_TOKEN(HullShader)
    FX_TOKEN(ComputeShader)
    FX_TOKEN(ZEnable)
    FX_TOKEN(ZWriteEnable)
    FX_TOKEN(CullMode)
    FX_TOKEN(SrcBlend)
    FX_TOKEN(DestBlend)
    FX_TOKEN(AlphaBlendEnable)
    FX_TOKEN(AlphaFunc)
    FX_TOKEN(AlphaRef)
    FX_TOKEN(ZFunc)
    FX_TOKEN(ColorWriteEnable)
    FX_TOKEN(IgnoreMaterialState)
    FX_END_TOKENS

    bool bRes = true;
    int n = pShTech->m_Passes.Num();
    pShTech->m_Passes.ReserveNew(n + 1);
    SShaderPass* sm = &pShTech->m_Passes[n];
    sm->m_eCull = -1;
    sm->m_AlphaRef = ~0;

    SParserFrame VS, PS, GS, DS, HS, CS;
    FXMacroBin VSMacro, PSMacro, GSMacro, DSMacro, HSMacro, CSMacro;

    byte ZFunc = eCF_LEqual;

    byte ColorWriteMask = 0xff;

    byte AlphaFunc = eCF_Disable;
    byte AlphaRef = 0;
    int State = GS_DEPTHWRITE;

    signed char Cull = -1;
    int nIndex;
    EToken eSrcBlend = eT_unknown;
    EToken eDstBlend = eT_unknown;
    bool bBlend = false;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_VertexShader:
            VS = Parser.m_Data;
            VSMacro = Parser.m_Macros[1];
            break;
        case eT_PixelShader:
            PS = Parser.m_Data;
            PSMacro = Parser.m_Macros[1];
            break;
        case eT_GeometryShader:
            GS = Parser.m_Data;
            GSMacro = Parser.m_Macros[1];
            break;
        case eT_DomainShader:
            DS = Parser.m_Data;
            DSMacro = Parser.m_Macros[1];
            break;
        case eT_HullShader:
            HS = Parser.m_Data;
            HSMacro = Parser.m_Macros[1];
            break;
        case eT_ComputeShader:
            CS = Parser.m_Data;
            CSMacro = Parser.m_Macros[1];
            break;

        case eT_ZEnable:
            if (Parser.GetBool(Parser.m_Data))
            {
                State &= ~GS_NODEPTHTEST;
            }
            else
            {
                State |= GS_NODEPTHTEST;
            }
            break;
        case eT_ZWriteEnable:
            if (Parser.GetBool(Parser.m_Data))
            {
                State |= GS_DEPTHWRITE;
            }
            else
            {
                State &= ~GS_DEPTHWRITE;
            }
            break;
        case eT_CullMode:
            eT = Parser.GetToken(Parser.m_Data);
            if (eT == eT_None)
            {
                Cull = eCULL_None;
            }
            else
            if (eT == eT_CCW || eT == eT_Back)
            {
                Cull = eCULL_Back;
            }
            else
            if (eT == eT_CW || eT == eT_Front)
            {
                Cull = eCULL_Front;
            }
            else
            {
                Warning("unknown CullMode parameter '%s' (Skipping)\n", Parser.GetString(eT));
                CRY_ASSERT(0);
            }
            break;
        case eT_AlphaFunc:
            AlphaFunc = Parser.GetCompareFunc(Parser.GetToken(Parser.m_Data));
            break;
        case eT_ColorWriteEnable:
        {
            if (ColorWriteMask == 0xff)
            {
                ColorWriteMask = 0;
            }
            uint32 nCur = Parser.m_Data.m_nFirstToken;
            while (nCur <= Parser.m_Data.m_nLastToken)
            {
                uint32 nT = Parser.m_Tokens[nCur++];
                if (nT == eT_or)
                {
                    continue;
                }
                if (nT == eT_0)
                {
                    ColorWriteMask |= 0;
                }
                else
                if (nT == eT_RED)
                {
                    ColorWriteMask |= 1;
                }
                else
                if (nT == eT_GREEN)
                {
                    ColorWriteMask |= 2;
                }
                else
                if (nT == eT_BLUE)
                {
                    ColorWriteMask |= 4;
                }
                else
                if (nT == eT_ALPHA)
                {
                    ColorWriteMask |= 8;
                }
                else
                {
                    Warning("unknown WriteMask parameter '%s' (Skipping)\n", Parser.GetString(eT));
                }
            }
        }
        break;
        case eT_ZFunc:
            ZFunc = Parser.GetCompareFunc(Parser.GetToken(Parser.m_Data));
            sm->m_PassFlags |= SHPF_FORCEZFUNC;
            break;
        case eT_AlphaRef:
            AlphaRef = Parser.GetInt(Parser.GetToken(Parser.m_Data));
            break;
        case eT_SrcBlend:
            eSrcBlend = Parser.GetToken(Parser.m_Data);
            break;
        case eT_DestBlend:
            eDstBlend = Parser.GetToken(Parser.m_Data);
            break;
        case eT_AlphaBlendEnable:
            bBlend = Parser.GetBool(Parser.m_Data);
            break;

        case eT_IgnoreMaterialState:
            sm->m_PassFlags |= SHPF_NOMATSTATE;
            break;

        default:
            CRY_ASSERT(0);
        }
    }

    if (bBlend && eSrcBlend && eDstBlend)
    {
        int nSrc = Parser.GetSrcBlend(eSrcBlend);
        int nDst = Parser.GetDstBlend(eDstBlend);
        if (nSrc >= 0 && nDst >= 0)
        {
            State |= nSrc;
            State |= nDst;
        }
    }
    if (ColorWriteMask != 0xff)
    {
        for (int i = 0; i < 4; i++)
        {
            if (!(ColorWriteMask & (1 << i)))
            {
                State |= GS_NOCOLMASK_R << i;
            }
        }
    }

    if (AlphaFunc)
    {
        switch (AlphaFunc)
        {
        case eCF_Greater:
            State |= GS_ALPHATEST_GREATER;
            break;
        case eCF_GEqual:
            State |= GS_ALPHATEST_GEQUAL;
            break;
        case eCF_Less:
            State |= GS_ALPHATEST_LESS;
            break;
        case eCF_LEqual:
            State |= GS_ALPHATEST_LEQUAL;
            break;
        default:
            CRY_ASSERT(0);
        }
    }

    switch (ZFunc)
    {
    case eCF_Equal:
        State |= GS_DEPTHFUNC_EQUAL;
        break;
    case eCF_Greater:
        State |= GS_DEPTHFUNC_GREAT;
        break;
    case eCF_GEqual:
        State |= GS_DEPTHFUNC_GEQUAL;
        break;
    case eCF_Less:
        State |= GS_DEPTHFUNC_LESS;
        break;
    case eCF_NotEqual:
        State |= GS_DEPTHFUNC_NOTEQUAL;
        break;
    case eCF_LEqual:
        State |= GS_DEPTHFUNC_LEQUAL;
        break;
    case eCF_Always:
        State |= GS_DEPTHFUNC_ALWAYS;
        break;
    default:
        CRY_ASSERT(0);
    }

    sm->m_RenderState = State;
    sm->m_eCull = Cull;

    mfGeneratePublicFXParams(Parser.m_pCurShader, Parser);
    SShaderFXParams& FXParams = mfGetFXParams(Parser.m_pCurShader);

    if (!VS.IsEmpty())
    {
        bRes &= ParseBinFX_Technique_Pass_LoadShader(Parser, VSMacro, VS, pShTech, sm, eHWSC_Vertex, FXParams);
    }
    if (!PS.IsEmpty())
    {
        bRes &= ParseBinFX_Technique_Pass_LoadShader(Parser, PSMacro, PS, pShTech, sm, eHWSC_Pixel, FXParams);
    }
    if (CParserBin::PlatformSupportsGeometryShaders() && !GS.IsEmpty())
    {
        bRes &= ParseBinFX_Technique_Pass_LoadShader(Parser, GSMacro, GS, pShTech, sm, eHWSC_Geometry, FXParams);
    }
    if (CParserBin::PlatformSupportsHullShaders() && !HS.IsEmpty())
    {
        bRes &= ParseBinFX_Technique_Pass_LoadShader(Parser, HSMacro, HS, pShTech, sm, eHWSC_Hull, FXParams);
    }
    if (CParserBin::PlatformSupportsDomainShaders() && !DS.IsEmpty())
    {
        bRes &= ParseBinFX_Technique_Pass_LoadShader(Parser, DSMacro, DS, pShTech, sm, eHWSC_Domain, FXParams);
    }
    if (CParserBin::PlatformSupportsComputeShaders() && !CS.IsEmpty())
    {
        bRes &= ParseBinFX_Technique_Pass_LoadShader(Parser, CSMacro, CS, pShTech, sm, eHWSC_Compute, FXParams);
    }

    Parser.EndFrame(OldFrame);

    return bRes;
}

bool CShaderManBin::ParseBinFX_LightStyle_Val(CParserBin& Parser, SParserFrame& Frame, CLightStyle* ls)
{
    int i;
    char str[64];
    const char* pstr1, * pstr2;
    SLightStyleKeyFrame pKeyFrame;

    ls->m_Map.Free();
    string sr = Parser.GetString(Frame);
    const char* lstr = sr.c_str();

    // First count the keyframes
    size_t nKeyFrames = 0;
    while (true)
    {
        pstr1 = strchr(lstr, '|');
        if (!pstr1)
        {
            break;
        }
        pstr2 = strchr(pstr1 + 1, '|');
        if (!pstr2)
        {
            break;
        }
        if (pstr2 - pstr1 - 1 > 0)
        {
            ++nKeyFrames;
        }
        lstr = pstr2;
    }
    ls->m_Map.reserve(nKeyFrames);

    lstr = sr.c_str();
    int n = 0;
    while (true)
    {
        pstr1 = strchr(lstr, '|');
        if (!pstr1)
        {
            break;
        }
        pstr2 = strchr(pstr1 + 1, '|');
        if (!pstr2)
        {
            break;
        }
        if (pstr2 - pstr1 - 1 > 0)
        {
            azstrncpy(str, AZ_ARRAY_SIZE(str), pstr1 + 1, pstr2 - pstr1 - 1);
            str[pstr2 - pstr1 - 1] = 0;
            i = azsscanf(str, "%f %f %f %f %f %f %f", &pKeyFrame.cColor.r, &pKeyFrame.cColor.g, &pKeyFrame.cColor.b, &pKeyFrame.cColor.a,
                    &pKeyFrame.vPosOffset.x, &pKeyFrame.vPosOffset.y, &pKeyFrame.vPosOffset.z);
            switch (i)
            {
            case 1:
            {
                // Only luminance updates
                pKeyFrame.cColor.g = pKeyFrame.cColor.b = pKeyFrame.cColor.r;
                pKeyFrame.cColor.a = 1.0f;
                pKeyFrame.vPosOffset = Vec3(0);
            }
            break;

            case 3:
            {
                // No position/spec mult updates
                pKeyFrame.cColor.a = 1.0f;
                pKeyFrame.vPosOffset = Vec3(0);
            }
            break;

            case 4:
            {
                // No position
                pKeyFrame.vPosOffset = Vec3(0);
            }
            break;

            default:
                break;
            }
            ls->m_Map.AddElem(pKeyFrame);
            n++;
        }

        lstr = pstr2;
    }
    ls->m_Map.Shrink();
    CRY_ASSERT(ls->m_Map.Num() == n);
    if (ls->m_Map.Num() == n)
    {
        return true;
    }
    return false;
}

bool CShaderManBin::ParseBinFX_LightStyle(CParserBin& Parser, SParserFrame& Frame, int nStyle)
{
    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(KeyFrameParams)
    FX_TOKEN(KeyFrameRandColor)
    FX_TOKEN(KeyFrameRandIntensity)
    FX_TOKEN(KeyFrameRandSpecMult)
    FX_TOKEN(KeyFrameRandPosOffset)
    FX_TOKEN(Speed)
    FX_END_TOKENS

    bool bRes = true;

    Parser.m_pCurShader->m_Flags |= EF_LIGHTSTYLE;
    if (CLightStyle::s_LStyles.Num() <= (uint32)nStyle)
    {
        CLightStyle::s_LStyles.ReserveNew(nStyle + 1);
    }
    CLightStyle* ls = CLightStyle::s_LStyles[nStyle];
    if (!ls)
    {
        ls = new CLightStyle;
        ls->m_LastTime = 0;
        ls->m_Color = Col_White;
        CLightStyle::s_LStyles[nStyle] = ls;
    }
    ls->m_TimeIncr = 60;
    int nIndex;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_KeyFrameRandColor:
            ls->m_bRandColor = Parser.GetBool(Parser.m_Data);
            break;

        case eT_KeyFrameRandIntensity:
            ls->m_bRandIntensity = Parser.GetBool(Parser.m_Data);
            break;

        case eT_KeyFrameRandSpecMult:
            ls->m_bRandSpecMult = Parser.GetBool(Parser.m_Data);
            break;

        case eT_KeyFrameRandPosOffset:
            ls->m_bRandPosOffset = Parser.GetBool(Parser.m_Data);
            break;

        case eT_KeyFrameParams:
            bRes &= ParseBinFX_LightStyle_Val(Parser, Parser.m_Data, ls);
            break;

        case eT_Speed:
            ls->m_TimeIncr = Parser.GetFloat(Parser.m_Data);
            break;
        default:
            CRY_ASSERT(0);
        }
    }

    if (ls->m_Map.Num() && (ls->m_bRandPosOffset || ls->m_bRandIntensity || ls->m_bRandSpecMult || ls->m_bRandColor))
    {
        int32 nCount = ls->m_Map.Num();
        for (int f = 0; f < nCount; ++f)
        {
            SLightStyleKeyFrame& pKeyFrame = ls->m_Map[f];
            if (ls->m_bRandPosOffset)
            {
                pKeyFrame.vPosOffset = Vec3(cry_random(-1.0f, 1.0f), cry_random(-1.0f, 1.0f), cry_random(-1.0f, 1.0f));
            }

            if (ls->m_bRandColor)
            {
                pKeyFrame.cColor *= ColorF(cry_random(0.0f, 1.0f), cry_random(0.0f, 1.0f), cry_random(0.0f, 1.0f), 1.0f);
            }

            if (ls->m_bRandIntensity)
            {
                pKeyFrame.cColor *= cry_random(0.0f, 1.0f);
            }

            if (ls->m_bRandSpecMult)
            {
                pKeyFrame.cColor.a = cry_random(0.0f, 1.0f);
            }
        }
    }

    Parser.EndFrame(OldFrame);

    return bRes;
}

bool CShaderManBin::ParseBinFX_Technique_Annotations_String(CParserBin& Parser, SParserFrame& Frame, SShaderTechnique* pShTech, std::vector<SShaderTechParseParams>& techParams, bool* bPublic)
{
    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(Public)
    FX_TOKEN(NoLights)
    FX_TOKEN(NoMaterialState)
    FX_TOKEN(PositionInvariant)
    FX_TOKEN(TechniqueZ)
    FX_TOKEN(TechniqueZPrepass)
    FX_TOKEN(TechniqueShadowGen)
    FX_TOKEN(TechniqueMotionBlur)
    FX_TOKEN(TechniqueCustomRender)
    FX_TOKEN(TechniqueEffectLayer)
    FX_TOKEN(TechniqueDebug)
    FX_TOKEN(TechniqueSoftAlphaTest)
    FX_TOKEN(TechniqueWaterRefl)
    FX_TOKEN(TechniqueWaterCaustic)
    FX_TOKEN(TechniqueThickness)
    FX_END_TOKENS

    bool bRes = true;
    int nIndex;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_Public:
            if (pShTech)
            {
                pShTech->m_Flags |= FHF_PUBLIC;
            }
            else
            if (bPublic)
            {
                *bPublic = true;
            }
            break;

        case eT_PositionInvariant:
            if (pShTech)
            {
                pShTech->m_Flags |= FHF_POSITION_INVARIANT;
            }
            break;

        case eT_NoLights:
            if (pShTech)
            {
                pShTech->m_Flags |= FHF_NOLIGHTS;
            }
            break;

        case eT_NoMaterialState:
            if (Parser.m_pCurShader)
            {
                Parser.m_pCurShader->m_Flags2 |= EF2_IGNORERESOURCESTATES;
            }
            break;

        // Note: When adding/removing batch flags, please, update sDescList in D3DRendPipeline.cpp
        case eT_TechniqueDebug:
        {
        }

        case eT_TechniqueZ:
        case eT_TechniqueZPrepass:
        case eT_TechniqueShadowGen:
        case eT_TechniqueMotionBlur:
        case eT_TechniqueCustomRender:
        case eT_TechniqueEffectLayer:
        case eT_TechniqueSoftAlphaTest:
        case eT_TechniqueWaterRefl:
        case eT_TechniqueWaterCaustic:
        case eT_TechniqueThickness:
        {
            if (!Parser.m_pCurShader)
            {
                break;
            }

            static const uint32 pTechTable[eT_TechniqueMax - eT_TechniqueZ] =
            {
                TTYPE_Z,                                            //eT_TechniqueZ
                TTYPE_SHADOWGEN,                            //eT_TechniqueShadowGen
                TTYPE_MOTIONBLURPASS,                   //eT_TechniqueMotionBlur
                TTYPE_CUSTOMRENDERPASS,             //eT_TechniqueCustomRender
                TTYPE_EFFECTLAYER,                      //eT_TechniqueEffectLayer
                TTYPE_DEBUG,                                    //eT_TechniqueDebug
                TTYPE_SOFTALPHATESTPASS,            //eT_TechniqueSoftAlphaTest
                TTYPE_WATERREFLPASS,                        //eT_TechniqueWaterRefl
                TTYPE_WATERCAUSTICPASS,                 //eT_TechniqueWaterCaustic
                TTYPE_ZPREPASS,                                 //eT_TechniqueZPrepass
                TTYPE_PARTICLESTHICKNESSPASS         //eT_TechniqueThickness
            };

            const CCryNameR pszNameTech = Parser.GetNameString(Parser.m_Data);
            int idx = techParams.size() - 1;
            CRY_ASSERT(idx >= 0);
            CRY_ASSERT(eT - eT_TechniqueZ >= 0);
            techParams[idx].techName[ pTechTable[eT - eT_TechniqueZ] ] = pszNameTech;
        }
        break;

        default:
            CRY_ASSERT(0);
        }
    }

    Parser.EndFrame(OldFrame);

    return bRes;
}

bool CShaderManBin::ParseBinFX_Technique_Annotations(CParserBin& Parser, SParserFrame& Frame, SShaderTechnique* pShTech, std::vector<SShaderTechParseParams>& techParams, bool* bPublic)
{
    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(string)
    FX_END_TOKENS

    bool bRes = true;
    int nIndex;

    while (Parser.ParseObject(sCommands, nIndex))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_string:
        {
            eT = Parser.GetToken(Parser.m_Name);
            CRY_ASSERT(eT == eT_Script);
            bRes &= ParseBinFX_Technique_Annotations_String(Parser, Parser.m_Data, pShTech, techParams, bPublic);
        }
        break;

        default:
            CRY_ASSERT(0);
        }
    }

    Parser.EndFrame(OldFrame);

    return bRes;
}

bool CShaderManBin::ParseBinFX_Technique_CustomRE(CParserBin& Parser, SParserFrame& Frame, SParserFrame& Name, SShaderTechnique* pShTech)
{
    uint32 nName = Parser.GetToken(Name);

    if (nName == eT_LensOptics)
    {
        CRELensOptics* ps = new CRELensOptics;
        if (ps->mfCompile(Parser, Frame))
        {
            pShTech->m_REs.AddElem(ps);
            pShTech->m_Flags |= FHF_RE_LENSOPTICS;
            return true;
        }
        else
        {
            delete ps;
        }
    }
#if 0
    else
    if (nName == eT_Cloud)
    {
        CRECloud* ps = new CRECloud;
        if (ps->mfCompile(Parser, Frame))
        {
            pShTech->m_REs.AddElem(ps);
            pShTech->m_Flags |= FHF_RE_CLOUD;
            return true;
        }
        else
        {
            delete ps;
        }
    }
#endif
    else
    if (nName == eT_Beam)
    {
        CREBeam* ps = new CREBeam;
        if (ps->mfCompile(Parser, Frame))
        {
            pShTech->m_REs.AddElem(ps);
        }
        else
        {
            delete ps;
        }
    }
    else
    if (nName == eT_Ocean)
    {
        CRY_ASSERT(0);
    }

    return true;
}

SShaderTechnique* CShaderManBin::ParseBinFX_Technique(CParserBin& Parser, SParserFrame& Frame, SParserFrame Annotations, std::vector<SShaderTechParseParams>& techParams, bool* bPublic)
{
    LOADING_TIME_PROFILE_SECTION(iSystem);

    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(pass)
    FX_TOKEN(CustomRE)
    FX_TOKEN(Style)
    FX_END_TOKENS

    bool bRes = true;
    SShaderTechnique* pShTech = NULL;
    if (Parser.m_pCurShader)
    {
        pShTech = new SShaderTechnique(Parser.m_pCurShader);
    }

    if (Parser.m_pCurShader)
    {
        SShaderTechParseParams ps;
        techParams.push_back(ps);
    }

    if (!Annotations.IsEmpty())
    {
        ParseBinFX_Technique_Annotations(Parser, Annotations, pShTech, techParams, bPublic);
    }

    while (Parser.ParseObject(sCommands))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_pass:
            if (pShTech)
            {
                bRes &= ParseBinFX_Technique_Pass(Parser, Parser.m_Data, pShTech);
            }
            break;

        case eT_Style:
            if (pShTech)
            {
                ParseBinFX_LightStyle(Parser, Parser.m_Data, Parser.GetInt(Parser.GetToken(Parser.m_Name)));
            }
            break;

        case eT_CustomRE:
            if (pShTech)
            {
                bRes &= ParseBinFX_Technique_CustomRE(Parser, Parser.m_Data, Parser.m_Name, pShTech);
            }
            break;

        default:
            CRY_ASSERT(0);
        }
    }

    if (bRes)
    {
        if (Parser.m_pCurShader && pShTech)
        {
            Parser.m_pCurShader->m_HWTechniques.AddElem(pShTech);
        }
    }
    else
    {
        techParams.pop_back();
    }

    Parser.EndFrame(OldFrame);

    return pShTech;
}

float g_fTimeA;

bool CShaderManBin::ParseBinFX(SShaderBin* pBin, CShader* ef, uint64 nMaskGen)
{
    LOADING_TIME_PROFILE_SECTION_ARGS(pBin->m_szName);

    bool bRes = true;

    float fTimeA = iTimer->GetAsyncCurTime();

#if !defined(SHADER_NO_SOURCES)
    CParserBin Parser(pBin, ef);

    CShader* efGen = ef->m_pGenShader;

    if (efGen && efGen->m_ShaderGenParams)
    {
        AddGenMacroses(efGen->m_ShaderGenParams, Parser, nMaskGen);
    }

    if (ef->m_ShaderGenStaticParams)
    {
        // Just add the defines and not the masks because they could clash with the gen params masks.
        AddGenMacroses(ef->m_ShaderGenStaticParams, Parser, ef->m_maskGenStatic, true);
    }

    pBin->Lock();
    Parser.Preprocess(0, pBin->m_Tokens, &pBin->m_TokenTable);
    ef->m_CRC32 = pBin->m_CRC32;
    ef->m_SourceCRC32 = pBin->m_SourceCRC32;
    pBin->Unlock();
#endif

#if defined(SHADER_NO_SOURCES)
    iLog->LogError("ERROR: Couldn't find binary shader '%s' (0x%x)", ef->GetName(), ef->m_nMaskGenFX);
    return false;
#else
    SParserFrame Frame(0, (Parser.m_Tokens.size() > 0 ? Parser.m_Tokens.size() - 1 : 0));
    Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
    FX_TOKEN(static)
    FX_TOKEN(half)
    FX_TOKEN(half2)
    FX_TOKEN(half3)
    FX_TOKEN(half4)
    FX_TOKEN(half2x4)
    FX_TOKEN(half3x4)
    FX_TOKEN(half4x4)
    FX_TOKEN(float)
    FX_TOKEN(float2)
    FX_TOKEN(float3)
    FX_TOKEN(float4)
    FX_TOKEN(float2x4)
    FX_TOKEN(float3x4)
    FX_TOKEN(float4x4)
    FX_TOKEN(bool)
    FX_TOKEN(int)
    FX_TOKEN(struct)
    FX_TOKEN(sampler1D)
    FX_TOKEN(sampler2D)
    FX_TOKEN(sampler3D)
    FX_TOKEN(samplerCUBE)
    FX_TOKEN(Texture2D)
    FX_TOKEN(RWTexture2D)
    FX_TOKEN(RWTexture2DArray)
    FX_TOKEN(Texture2DArray)
    FX_TOKEN(Texture2DMS)
    FX_TOKEN(TextureCube)
    FX_TOKEN(TextureCubeArray)
    FX_TOKEN(Texture3D)
    FX_TOKEN(RWTexture3D)
    FX_TOKEN(technique)
    FX_TOKEN(SamplerState)
    FX_TOKEN(SamplerComparisonState)
    FX_TOKEN(Buffer)
    FX_TOKEN(RWBuffer)
    FX_TOKEN(StructuredBuffer)
    FX_TOKEN(RWStructuredBuffer)
    FX_TOKEN(ByteAddressBuffer)
    FX_TOKEN(RWByteAddressBuffer)
    FX_TOKEN(cbuffer)
    FX_TOKEN(RasterizerOrderedBuffer)
    FX_TOKEN(RasterizerOrderedByteAddressBuffer)
    FX_TOKEN(RasterizerOrderedStructuredBuffer)
    FX_END_TOKENS

    std::vector<SShaderTechParseParams> techParams;
    CCryNameR techStart[2];

    // From MemReplay analysis of shader params, 200 should be more than enough space
    static decltype(SShaderFXParams::m_FXParams) s_tempFXParams;
    s_tempFXParams.reserve(200);
    s_tempFXParams.clear();

    SShaderFXParams& FXP = mfGetFXParams(Parser.m_pCurShader);
    FXP.m_FXParams.swap(s_tempFXParams);

    ef->mfFree();

    assert (ef->m_HWTechniques.Num() == 0);

    ETokenStorageClass nTokenStorageClass = Parser.ParseObject(sCommands);
    while (nTokenStorageClass)
    {
        EToken eT = Parser.GetToken();
        SCodeFragment Fr;
        SFXParam P;
        string szR;
        switch (eT)
        {
        case eT_struct:
        case eT_cbuffer:
            Fr.m_nFirstToken = Parser.FirstToken();
            Fr.m_nLastToken = Parser.m_CurFrame.m_nCurToken - 1;
            Fr.m_dwName = Parser.m_Tokens[Fr.m_nFirstToken + 1];
#ifdef _DEBUG
            //Fr.m_Name = Parser.GetString(Fr.m_dwName);
#endif
            Fr.m_eType = (eT == eT_cbuffer) ? eFT_ConstBuffer : eFT_Structure;
            Parser.m_CodeFragments.push_back(Fr);
            break;

        case eT_SamplerState:
        case eT_SamplerComparisonState:
        {
            SFXSampler Pr;
            Parser.CopyTokens(Parser.m_Name, Pr.m_dwName);
            if (eT == eT_SamplerState)
            {
                Pr.m_eType = eSType_Sampler;
            }
            else
            if (eT == eT_SamplerComparisonState)
            {
                Pr.m_eType = eSType_SamplerComp;
            }
            else
            {
                CRY_ASSERT(0);
            }

            uint32 nTokAssign = 0;
            if (Parser.m_Assign.IsEmpty() && !Parser.m_Value.IsEmpty())
            {
                nTokAssign = Parser.m_Tokens[Parser.m_Value.m_nFirstToken];
                if (nTokAssign == eT_br_cv_1)
                {
                    nTokAssign = Parser.m_Tokens[Parser.m_Value.m_nFirstToken + 1];
                }
            }
            else
            if (!Parser.m_Assign.IsEmpty())
            {
                nTokAssign = Parser.m_Tokens[Parser.m_Assign.m_nFirstToken];
            }
            Pr.PostLoad(Parser, Parser.m_Name, Parser.m_Annotations, Parser.m_Value, Parser.m_Assign);

            bRes &= ParseBinFX_Sampler(Parser, Parser.m_Data, Pr);

            mfAddFXSampler(Parser.m_pCurShader, &Pr);
        }
        break;

        case eT_Texture2D:
        case eT_Texture2DMS:
        case eT_Texture2DArray:
        case eT_TextureCube:
        case eT_TextureCubeArray:
        case eT_Texture3D:
        {
            SFXTexture Pr;
            Parser.CopyTokens(Parser.m_Name, Pr.m_dwName);

            if (eT == eT_Texture2D)
            {
                Pr.m_eType = eTT_2D;
            }
            else
            if (eT == eT_Texture2DMS)
            {
                Pr.m_eType = eTT_2DMS;
            }
            else
            if (eT == eT_Texture2DArray)
            {
                Pr.m_eType = eTT_2DArray;
            }
            else
            if (eT == eT_Texture3D)
            {
                Pr.m_eType = eTT_3D;
            }
            else
            if (eT == eT_TextureCube)
            {
                Pr.m_eType = eTT_Cube;
            }
            else
            if (eT == eT_TextureCubeArray)
            {
                Pr.m_eType = eTT_CubeArray;
            }
            else
            {
                CRY_ASSERT(0);
            }

            Pr.PostLoad(Parser, Parser.m_Name, Parser.m_Annotations, Parser.m_Value, Parser.m_Assign);

            bRes &= ParseBinFX_Texture(Parser, Parser.m_Data, Pr);
            bRes &= ParseBinFX_Texture(Parser, Parser.m_Annotations, Pr);

            // Texture2D something = TS_identifiersearch;
            if (!azstrnicmp(Pr.m_Values.c_str(), "TM_", 3) ||
                !azstrnicmp(Pr.m_Values.c_str(), "TS_", 3) ||
                !azstrnicmp(Pr.m_Values.c_str(), "TP_", 3))
            {
                Pr.m_Semantic = Pr.m_Values;
                Pr.m_szTexture = "";
                Pr.m_Values = "";
            }

            // Texture2D something = "filepathsearch";
            // Texture2D something = $databasesearch;
            if (Pr.m_Values.c_str()[0])
            {
                Pr.m_Semantic = "";
                Pr.m_szTexture = Pr.m_Values.c_str();
                Pr.m_Values = "";
            }

            mfAddFXTexture(Parser.m_pCurShader, &Pr);
        }
        break;

        case eT_int:
        case eT_bool:
        case eT_half:
        case eT_half2:
        case eT_half3:
        case eT_half4:
        case eT_half2x4:
        case eT_half3x4:
        case eT_half4x4:
        case eT_float:
        case eT_float2:
        case eT_float3:
        case eT_float4:
        case eT_float2x4:
        case eT_float3x4:
        case eT_float4x4:
        {
            SFXParam Pr;
            Parser.CopyTokens(Parser.m_Name, Pr.m_dwName);

            if (eT == eT_float2x4 || eT == eT_half2x4)
            {
                Pr.m_RegisterCount = 2;
            }
            else
            if (eT == eT_float3x4 || eT == eT_half3x4)
            {
                Pr.m_RegisterCount = 3;
            }
            else
            if (eT == eT_float4x4 || eT == eT_half4x4)
            {
                Pr.m_RegisterCount = 4;
            }
            else
            {
                Pr.m_RegisterCount = 1;
            }

            if (eT == eT_float || eT == eT_half || eT == eT_int || eT == eT_bool)
            {
                Pr.m_ComponentCount = 1;
            }
            else
            if (eT == eT_float2 || eT == eT_half2)
            {
                Pr.m_ComponentCount = 2;
            }
            else
            if (eT == eT_float3 || eT == eT_half3)
            {
                Pr.m_ComponentCount = 3;
            }
            else
            if (eT == eT_float4 || eT == eT_float2x4 || eT == eT_float3x4 || eT == eT_float4x4 ||
                eT == eT_half4 || eT == eT_half2x4 || eT == eT_half3x4 || eT == eT_half4x4)
            {
                Pr.m_ComponentCount = 4;
            }

            if (eT == eT_int)
            {
                Pr.m_eType = eType_INT;
            }
            else
            if (eT == eT_bool)
            {
                Pr.m_eType = eType_BOOL;
            }
            else
            if (eT >= eT_half && eT <= eT_half3x3)
            {
                Pr.m_eType = eType_HALF;
            }
            else
            {
                Pr.m_eType = eType_FLOAT;
            }

            if (!Parser.m_Assign.IsEmpty() && Parser.GetToken(Parser.m_Assign) == eT_STANDARDSGLOBAL)
            {
                ParseBinFX_Global(Parser, Parser.m_Annotations, NULL, techStart);
            }
            else
            {
                uint32 nTokAssign = 0;
                if (Parser.m_Assign.IsEmpty() && !Parser.m_Value.IsEmpty())
                {
                    nTokAssign = Parser.m_Tokens[Parser.m_Value.m_nFirstToken];
                    if (nTokAssign == eT_br_cv_1)
                    {
                        nTokAssign = Parser.m_Tokens[Parser.m_Value.m_nFirstToken + 1];
                    }
                }
                else
                if (!Parser.m_Assign.IsEmpty())
                {
                    nTokAssign = Parser.m_Tokens[Parser.m_Assign.m_nFirstToken];
                }
                if (nTokAssign)
                {
                    const char* assign = Parser.GetString(nTokAssign);
                    if (!assign[0] || !_strnicmp(assign, "PB_", 3))
                    {
                        Pr.m_BindingSlot = eConstantBufferShaderSlot_PerBatch;
                    }
                    else
                    if (!_strnicmp(assign, "PI_", 3) || !_strnicmp(assign, "SI_", 3))
                    {
                        Pr.m_BindingSlot = eConstantBufferShaderSlot_PerInstanceLegacy;
                    }
                    else
                    if (!_strnicmp(assign, "PM_", 3))
                    {
                        Pr.m_BindingSlot = eConstantBufferShaderSlot_PerMaterial;
                    }
                    else
                    if (!_strnicmp(assign, "register", 8))
                    {
                        Pr.m_BindingSlot = eConstantBufferShaderSlot_PerBatch;
                    }
                    else
                    {
                        Pr.m_BindingSlot = eConstantBufferShaderSlot_PerBatch;
                    }
                }
                else
                if (CParserBin::m_nPlatform & (SF_D3D11 | SF_ORBIS | SF_JASPER | SF_GL4 | SF_GLES3 | SF_METAL))
                {
                    uint32 nTokName = Parser.GetToken(Parser.m_Name);
                    const char* name = Parser.GetString(nTokName);
                    if (!strncmp(name, "PI_", 3))
                    {
                        Pr.m_BindingSlot = eConstantBufferShaderSlot_PerInstanceLegacy;
                    }
                    else
                    {
                        Pr.m_BindingSlot = eConstantBufferShaderSlot_PerBatch;
                    }
                }

                Pr.PostLoad(Parser, Parser.m_Name, Parser.m_Annotations, Parser.m_Value, Parser.m_Assign);

                EParamType eType;
                string szReg = Pr.GetValueForName("register", eType);
                if (!szReg.empty())
                {
                    CRY_ASSERT(szReg[0] == 'c');
                    Pr.m_Register[eHWSC_Vertex] = atoi(&szReg[1]);
                    Pr.m_Register[eHWSC_Pixel] = Pr.m_Register[eHWSC_Vertex];

                    if (CParserBin::PlatformSupportsGeometryShaders())
                    {
                        Pr.m_Register[eHWSC_Geometry] = Pr.m_Register[eHWSC_Vertex];
                    }
                    if (CParserBin::PlatformSupportsDomainShaders())
                    {
                        Pr.m_Register[eHWSC_Domain] = Pr.m_Register[eHWSC_Vertex];
                    }
                }
                uint32 prFlags = Pr.GetFlags();
                if (prFlags & PF_TWEAKABLE_MASK)
                {
                    Pr.m_BindingSlot = eConstantBufferShaderSlot_PerMaterial;
                    CRY_ASSERT(prFlags & PF_CUSTOM_BINDED);
                }
                mfAddFXParam(Parser.m_pCurShader, &Pr);
            }
        }
        break;

        case eT_sampler1D:
        case eT_sampler2D:
        case eT_sampler3D:
        case eT_samplerCUBE:
        {
            Fr.m_nFirstToken = Parser.FirstToken();

            uint32 nTokenOffset = 1;

            // For DX11 style texture definitions, need to parse out templated type. Also unlike HLSL we *require* it.
            if (eT == eT_Texture2DMS || eT == eT_Texture2D || eT == eT_Texture2DArray || eT == eT_TextureCube || eT == eT_TextureCubeArray || eT == eT_Texture3D)
            {
                // Texture2DMS is particular case, requires (at minimum) type to be specified - parse Texture2DMS<type> tokens
                // Texture2D for typeless resources, is also a particular case, type has to explicitly be specified - parse Texture2D<type> tokens
                nTokenOffset = 4;

                if (Parser.m_Tokens[Fr.m_nFirstToken + 1] != eT_br_tr_1 ||        // <
                    Parser.m_Tokens[Fr.m_nFirstToken + 3] != eT_br_tr_2)          // >
                {
                    // Parser.m_Tokens[Fr.m_nFirstToken + 2] is the templated type, e.g. float4
                    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR_DBGBRK, "[SHADERS] FAILED TO PARSE '%s': Invalid Texture definition without templated type:", ef->GetName());
                    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR_DBGBRK, "[SHADERS] %s %s %s %s"
                        , Parser.GetString(Parser.m_Tokens[Fr.m_nFirstToken])
                        , Parser.GetString(Parser.m_Tokens[Fr.m_nFirstToken + 1])
                        , Parser.GetString(Parser.m_Tokens[Fr.m_nFirstToken + 2])
                        , Parser.GetString(Parser.m_Tokens[Fr.m_nFirstToken + 3])
                        );
                    break;
                }
            }

            Fr.m_nLastToken = Fr.m_nFirstToken + nTokenOffset;
            if (!Parser.m_Assign.IsEmpty())
            {
                Fr.m_nLastToken = Parser.m_Assign.m_nLastToken;
            }
            Fr.m_dwName = Parser.m_Tokens[Fr.m_nFirstToken + nTokenOffset];
            Fr.m_eType = eFT_Sampler;
            Parser.m_CodeFragments.push_back(Fr);

            //// handy for debugging - do not remove plz
            //const char *pszToken0 = Parser.GetString( Parser.m_Tokens[Fr.m_nFirstToken]  );
            //const char *pszToken1 = Parser.GetString( Parser.m_Tokens[Fr.m_nFirstToken+1] );
            //const char *pszToken2 = Parser.GetString( Parser.m_Tokens[Fr.m_nFirstToken+2] );

            bRes &= ParseBinFX_Sampler(Parser, Parser.m_Data, Fr.m_dwName, Parser.m_Annotations, eT);
        }
        break;

        case eT_Buffer:
        case eT_RWBuffer:
        case eT_StructuredBuffer:
        case eT_RWStructuredBuffer:
        case eT_ByteAddressBuffer:
        case eT_RWByteAddressBuffer:
        case eT_RWTexture2D:
        case eT_RWTexture2DArray:
        case eT_RWTexture3D:
        case eT_RasterizerOrderedBuffer:
        case eT_RasterizerOrderedByteAddressBuffer:
        case eT_RasterizerOrderedStructuredBuffer:
        {
            Fr.m_nFirstToken = Parser.FirstToken();
            Fr.m_nLastToken = Fr.m_nFirstToken + 1;
            if (!Parser.m_Assign.IsEmpty())
            {
                Fr.m_nLastToken = Parser.m_Assign.m_nLastToken + 1;
            }
            Fr.m_dwName = Parser.m_Tokens[Parser.m_Name.m_nFirstToken];
            Fr.m_eType = eFT_Structure;
            Parser.m_CodeFragments.push_back(Fr);
        }
        break;

        case eT_technique:
        {
            uint32 nToken = Parser.m_Tokens[Parser.m_Name.m_nFirstToken];
            const char* szName = Parser.GetString(nToken);
            SShaderTechnique* pShTech = ParseBinFX_Technique(Parser, Parser.m_Data, Parser.m_Annotations, techParams, NULL);
            if (szName)
            {
                pShTech->m_NameStr = szName;
                pShTech->m_NameCRC = szName;
            }
        }
        break;

        default:
            //CRY_ASSERT(0);
            break;
        }

        nTokenStorageClass = Parser.ParseObject(sCommands);
    }

    FXP.m_FXParams.swap(s_tempFXParams);
    FXP.m_FXParams.reserve(FXP.m_FXParams.size() + s_tempFXParams.size());
    FXP.m_FXParams.insert(FXP.m_FXParams.end(), s_tempFXParams.begin(), s_tempFXParams.end());

    m_pCEF->mfPostLoadFX(ef, techParams, techStart);

#if SHADER_REFLECT_TEXTURE_SLOTS
    for (uint32 i = 0; i < min((uint32)TTYPE_MAX, ef->m_HWTechniques.size()); i++)
    {
        if (!ef->GetUsedTextureSlots(i))
        {
            ef->m_ShaderTexSlots[i] = GetTextureSlots(Parser, pBin, ef, i, -1);
        }
    }

    for (uint32 i = 0; i < min((uint32)TTYPE_MAX, ef->m_HWTechniques.size()); i++)
    {
        if (ef->m_ShaderTexSlots[i])
        {
            SShaderTechParseParams* linkedTechs = &techParams[i];

            // look through linked techniques (e.g. general has links to zpass, shadowgen, etc)
            for (int j = 0; j < TTYPE_MAX; j++)
            {
                // if we have a linked technique
                if (!linkedTechs->techName[j].empty())
                {
                    // find it in our technique list
                    for (int k = 0; k < ef->m_HWTechniques.size(); k++)
                    {
                        if (linkedTechs->techName[j] == ef->m_HWTechniques[k]->m_NameStr)
                        {
                            // merge slots - any slots that are empty in the master will be filled with the overlay
                            // This leaves the general/main technique authoratitve on slots, but stops slots disappearing
                            // if they're still used in some sub-pass
                            MergeTextureSlots(ef->m_ShaderTexSlots[i], ef->m_ShaderTexSlots[k]);
                            break;
                        }
                    }
                }
            }
        }
    }
#endif

    g_fTimeA += iTimer->GetAsyncCurTime() - fTimeA;

    return bRes;
#endif
}

void CShaderManBin::MergeTextureSlots(SShaderTexSlots* master, SShaderTexSlots* overlay)
{
    if (!master || !overlay)
    {
        return;
    }
 /* [Shader System] - TO DO: switch back to the map usage and replace with this
    for (auto& iter : overlay->m_UsedTextureSlots)
    {
        auto    masterIter = master->m_UsedTextureSlots.find(iter->first);
        if (masterIter == master->m_UsedTextureSlots.end()) // slot was not found - insert the overlay one
        {
            // these structures are never deleted so we can safely share pointers
            // without ref counting. See below in GetTextureSlots for the allocation
            master->m_UsedTextureSlots[iter->first] = iter->second;
        }
    }
*/
    // [Shader System] - TO DO - replace this part with code above after testing
    for (int i = 0; i < EFTT_MAX; i++)
    {
        // these structures are never deleted so we can safely share pointers
        // without ref counting. See below in GetTextureSlots for the allocation
        if (master->m_UsedTextureSlots[i] == NULL && overlay->m_UsedTextureSlots[i] != NULL)
        {
            master->m_UsedTextureSlots[i] = overlay->m_UsedTextureSlots[i];
        }
    }
}

// [Shader System TO DO] - this methods needs to be either removed or become full data driven
SShaderTexSlots* CShaderManBin::GetTextureSlots([[maybe_unused]] CParserBin& Parser, [[maybe_unused]] SShaderBin* pBin, [[maybe_unused]] CShader* ef, [[maybe_unused]] int nTech, int nPass)
{
#if !SHADER_REFLECT_TEXTURE_SLOTS
    return NULL;
#else
    TArray<uint32> referencedSamplers;

    bool bIterPasses    = false;
    int nMaxPasses      = 1;

    if (nPass == -1)
    {
        bIterPasses = true;
        nPass = 0;
    }

    if (nTech < 0 || nPass < 0)
    {
        return NULL;
    }

    // if the technique's pixel shader exists
    if (ef->m_HWTechniques.size() && nTech < ef->m_HWTechniques.size() &&
        ef->m_HWTechniques[nTech]->m_Passes.size() && nPass < ef->m_HWTechniques[nTech]->m_Passes.size() &&
        ef->m_HWTechniques[nTech]->m_Passes[nPass].m_PShader)
    {
        if (bIterPasses)
        {
            nMaxPasses = ef->m_HWTechniques[nTech]->m_Passes.size();
        }

        for (int nPassIter = nPass; nPassIter < nMaxPasses; nPassIter++)
        {
            uint32 dwEntryFuncName = Parser.GetCRC32(ef->m_HWTechniques[nTech]->m_Passes[nPassIter].m_PShader->m_EntryFunc);

            // get the cached info for the entry func
            SParamCacheInfo* pCache = GetParamInfo(pBin, dwEntryFuncName, ef->m_nMaskGenFX, ef->m_maskGenStatic);

            if (pCache)
            {
                // loop over affected fragments from this entry func
                SParamCacheInfo::AffectedFuncsVec& AffectedFragments = pCache->m_AffectedFuncs;
                for (uint32 i = 0; i < AffectedFragments.size(); i++)
                {
                    SCodeFragment* s = &Parser.m_CodeFragments[AffectedFragments[i]];

                    // if it's a sampler, include this sampler name CRC
                    if (s->m_eType == eFT_Sampler)
                    {
                        referencedSamplers.AddElem(s->m_dwName);
                    }
                }
            }
            else
            {
                return NULL;
            }
        }
    }
    else
    {
        return NULL;
    }

    uint32 dependencySlots = 0;

    // check the gen params
    SShaderGen* genParams = ef->GetGenerationParams();
    if (genParams)
    {
        for (uint32 i = 0; i < genParams->m_BitMask.Num(); i++)
        {
            SShaderGenBit* pBit = genParams->m_BitMask[i];
            if (!pBit || (!pBit->m_nDependencySet && !pBit->m_nDependencyReset))
            {
                continue;
            }

            // if any dependency set/reset is allowed for a texture, we must be conservative
            // and count this slot as used
            uint32 setReset = pBit->m_nDependencySet | pBit->m_nDependencyReset;

            // bitmask will fail if we have > 32 fixed texture slots
            CRY_ASSERT(EFTT_MAX <= 32);

            if (setReset & SHGD_TEX_MASK)
            {
                if (setReset & SHGD_TEX_NORMALS)
                {
                    dependencySlots |= 1 << EFTT_NORMALS;
                }

                if (setReset & SHGD_TEX_HEIGHT)
                {
                    dependencySlots |= 1 << EFTT_HEIGHT;
                }

                if (setReset & SHGD_TEX_DETAIL)
                {
                    dependencySlots |= 1 << EFTT_DETAIL_OVERLAY;
                }

                if (setReset & SHGD_TEX_SECOND_SMOOTHNESS)
                {
                    dependencySlots |= 1 << EFTT_SECOND_SMOOTHNESS;
                }

                if (setReset & SHGD_TEX_SPECULAR)
                {
                    dependencySlots |= 1 << EFTT_SPECULAR;
                }

                if (setReset & SHGD_TEX_ENVCM)
                {
                    dependencySlots |= 1 << EFTT_ENV;
                }

                if (setReset & SHGD_TEX_SUBSURFACE)
                {
                    dependencySlots |= 1 << EFTT_SUBSURFACE;
                }

                if (setReset & SHGD_TEX_DECAL)
                {
                    dependencySlots |= 1 << EFTT_DECAL_OVERLAY;
                }

                if (setReset & SHGD_TEX_CUSTOM)
                {
                    dependencySlots |= 1 << EFTT_CUSTOM;
                }

                if (setReset & SHGD_TEX_CUSTOM_SECONDARY)
                {
                    dependencySlots |= 1 << EFTT_CUSTOM_SECONDARY;
                }

                if (setReset & SHGD_TEX_OCC)
                {
                    dependencySlots |= 1 << EFTT_OCCLUSION;
                }

                if (setReset & SHGD_TEX_SPECULAR_2)
                {
                    dependencySlots |= 1 << EFTT_SPECULAR_2;
                }

                if (setReset & SHGD_TEX_EMITTANCE)
                {
                    // Both emittance and decal overlay (emissive intensity) are set by SHGD_TEX_EMITTANCE
                    dependencySlots |= 1 << EFTT_EMITTANCE;
                    dependencySlots |= 1 << EFTT_DECAL_OVERLAY;
                }
            }
        }
    }
    else
    {
        return NULL;
    }

    // since we might find samplers referencing a slot more than once,
    // keep track of the priority of each sampler found.
    int namePriority[EFTT_MAX];     // [Shader System TO DO] - data driven please!
    memset(namePriority, 0, sizeof(namePriority));

    // deliberately 'leaking', these are kept around permanently (this info is only gathered in the editor),
    // and cleaned up at shutdown when the app memory is released.
    SShaderTexSlots*    pSlots = new SShaderTexSlots;

    // Priority order:
    // 0 = forced in because of the dependency set as above, can be overridden if we find a better sampler
    // 1 = referenced from the shader but doesn't include a UIName
    // 2 = includes a UIName but isn't directly referenced. In the case we've forced the sampler because of dependency,
    //     this is probably more descriptive than a priority=1 sampler
    // 3 = both from above, referenced and has UIName. We shouldn't find multiple samplers like this, shader samplers
    //     should be set up so only ever one sampler using a slot has a UIName.
    enum
    {
        PRIORITY_REFERENCED = 0x1, PRIORITY_HASUINAME = 0x2
    };

    SShaderFXParams& FXP = mfGetFXParams(ef);

    // loop over all samplers for this shader
    FXSamplersOldIt it = FXP.m_FXSamplersOld.begin();
    FXSamplersOldIt end = FXP.m_FXSamplersOld.end();
    for (; it != end; ++it)
    {
        int slot = it->m_nSlotId;

        // if the slot is invalid this texture refers something else, skip it
        if (slot == EFTT_MAX)   // [Shader System TO DO] - data driven
        {
            continue;
        }

        uint32 dwName = Parser.GetCRC32(it->m_szName);

        // check if this sampler must be included for dependency set/reset reasons
        bool dependency = (dependencySlots & (1 << slot)) > 0;

        // check if this sampler is referenced from the shader
        bool referenced = referencedSamplers.Find(dwName) >= 0;

        if (dependency || referenced)
        {
            // calculate priority. See above.
            int                     priority = (referenced ? PRIORITY_REFERENCED : 0) | (it->m_szUIName.length() ? PRIORITY_HASUINAME : 0);
            SShaderTextureSlot*     pUsedTexSlot = pSlots->m_UsedTextureSlots[slot];

            // if we don't have this slot filled yet, create a new slot
            if (!pUsedTexSlot)
            {
                // !!IMPORTANT!! - if these slots are deleted/cleaned instead of being allowed to live forever,
                // MAKE SURE you refcount or refactor MergeTextureSlots above - as it will share pointers.
                pUsedTexSlot = new SShaderTextureSlot;

                pUsedTexSlot->m_Name =  it->m_szUIName;
                pUsedTexSlot->m_Description = it->m_szUIDescription;
                pUsedTexSlot->m_TexType = it->m_eTexType;

                // store current priority
                namePriority[slot] = priority;
                pSlots->m_UsedTextureSlots[slot] = pUsedTexSlot;   //  insertion to the map
            }
            else
            {
                // we shouldn't encounter two samplers that are used and have a UIName for the same slot,
                // error in this case
                if (priority == (PRIORITY_REFERENCED | PRIORITY_HASUINAME) && priority == namePriority[slot])
                {
                    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Encountered two samplers with UINames referenced for same slot in shader '%s': '%s' and '%s'\n",
                        ef->GetName(), pUsedTexSlot->m_Name.c_str(), it->m_szUIName.c_str());
                    CRY_ASSERT(0);
                }
                // override if we have a higher priority
                else if (priority > namePriority[slot])
                {
                    pUsedTexSlot->m_Name = it->m_szUIName;
                    pUsedTexSlot->m_Description = it->m_szUIDescription;
                    pUsedTexSlot->m_TexType = it->m_eTexType;

                    namePriority[slot] = priority;
                }
            }
        }
    }

    return pSlots;
#endif
}

bool CShaderManBin::ParseBinFX_Dummy(SShaderBin* pBin, std::vector<string>& ShaderNames, const char* szName)
{
    bool bRes = true;
    CParserBin Parser(pBin, NULL);

    pBin->Lock();
    bool preprocessResult = Parser.Preprocess(0, pBin->m_Tokens, &pBin->m_TokenTable);
    pBin->Unlock();

    if(!preprocessResult)
    {
        // Preprocess already outputs an error, no need to do so here.
        return false;
    }

    SParserFrame Frame(0, Parser.m_Tokens.size() - 1);
    Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(static)
    FX_TOKEN(half)
    FX_TOKEN(half2)
    FX_TOKEN(half3)
    FX_TOKEN(half4)
    FX_TOKEN(half2x4)
    FX_TOKEN(half3x4)
    FX_TOKEN(half4x4)
    FX_TOKEN(float)
    FX_TOKEN(float2)
    FX_TOKEN(float3)
    FX_TOKEN(float4)
    FX_TOKEN(float2x4)
    FX_TOKEN(float3x4)
    FX_TOKEN(float4x4)
    FX_TOKEN(bool)
    FX_TOKEN(int)
    FX_TOKEN(Buffer)
    FX_TOKEN(RWBuffer)
    FX_TOKEN(StructuredBuffer)
    FX_TOKEN(RWStructuredBuffer)
    FX_TOKEN(cbuffer)
    FX_TOKEN(struct)
    FX_TOKEN(sampler1D)
    FX_TOKEN(sampler2D)
    FX_TOKEN(sampler3D)
    FX_TOKEN(samplerCUBE)
    FX_TOKEN(technique)
    FX_TOKEN(SamplerState)
    FX_TOKEN(SamplerComparisonState)
    FX_TOKEN(Texture2D)
    FX_TOKEN(RWTexture2D)
    FX_TOKEN(RWTexture2DArray)
    FX_TOKEN(Texture2DArray)
    FX_TOKEN(Texture2DMS)
    FX_TOKEN(TextureCube)
    FX_TOKEN(TextureCubeArray)
    FX_TOKEN(Texture3D)
    FX_TOKEN(RWTexture3D)
    FX_TOKEN(RasterizerOrderedBuffer)
    FX_TOKEN(RasterizerOrderedByteAddressBuffer)
    FX_TOKEN(RasterizerOrderedStructuredBuffer)
    FX_END_TOKENS

    std::vector<SShaderTechParseParams> techParams;
    CCryNameR techStart[2];
    bool bPublic = false;
    std::vector<string> PubTechniques;

    ETokenStorageClass nTokenStorageClass;

    nTokenStorageClass = Parser.ParseObject(sCommands);
    while (nTokenStorageClass)
    {
        EToken eT = Parser.GetToken();
        SCodeFragment Fr;
        switch (eT)
        {
        case eT_half:
        case eT_float:
            if (!Parser.m_Assign.IsEmpty() && Parser.GetToken(Parser.m_Assign) == eT_STANDARDSGLOBAL)
            {
                ParseBinFX_Global(Parser, Parser.m_Annotations, &bPublic, techStart);
            }
            break;
        case eT_Buffer:
        case eT_RWBuffer:
        case eT_StructuredBuffer:
        case eT_RWStructuredBuffer:
        case eT_cbuffer:
        case eT_struct:
        case eT_SamplerState:
        case eT_SamplerComparisonState:
        case eT_int:
        case eT_bool:

        case eT_half2:
        case eT_half3:
        case eT_half4:
        case eT_half2x4:
        case eT_half3x4:
        case eT_half4x4:

        case eT_float2:
        case eT_float3:
        case eT_float4:
        case eT_float2x4:
        case eT_float3x4:
        case eT_float4x4:

        case eT_Texture2D:
        case eT_RWTexture2D:
        case eT_Texture2DMS:
        case eT_Texture2DArray:
        case eT_RWTexture2DArray:
        case eT_TextureCube:
        case eT_TextureCubeArray:
        case eT_Texture3D:
        case eT_RWTexture3D:
        case eT_sampler1D:
        case eT_sampler2D:
        case eT_sampler3D:
        case eT_samplerCUBE:
        case eT_RasterizerOrderedBuffer:
        case eT_RasterizerOrderedByteAddressBuffer:
        case eT_RasterizerOrderedStructuredBuffer:
            break;

        case eT_technique:
        {
            uint32 nToken = Parser.m_Tokens[Parser.m_Name.m_nFirstToken];
            bool bPublicTechnique = false;
            ParseBinFX_Technique(Parser, Parser.m_Data, Parser.m_Annotations, techParams, &bPublicTechnique);
            if (bPublicTechnique)
            {
                const char* name = Parser.GetString(nToken);
                PubTechniques.push_back(name);
            }
        }
        break;

        default:
            CRY_ASSERT(0);
        }
        nTokenStorageClass = Parser.ParseObject(sCommands);
    }

    if (bPublic)
    {
        ShaderNames.push_back(szName);
    }
    if (PubTechniques.size())
    {
        uint32 i;
        for (i = 0; i < PubTechniques.size(); i++)
        {
            string str = szName;
            str += ".";
            str += PubTechniques[i];
            ShaderNames.push_back(str);
        }
    }

    return bRes;
}

inline bool CompareInstParams(const SCGParam& a, const SCGParam& b)
{
    return (a.m_RegisterOffset < b.m_RegisterOffset);
}

void CShaderMan::mfPostLoadFX(CShader* ef, std::vector<SShaderTechParseParams>& techParams, [[maybe_unused]] CCryNameR techStart[2])
{
    ef->m_HWTechniques.Shrink();

    uint32 i;

    CRY_ASSERT(techParams.size() == ef->m_HWTechniques.Num());
    for (i = 0; i < ef->m_HWTechniques.Num(); i++)
    {
        SShaderTechnique* hw = ef->m_HWTechniques[i];
        SShaderTechParseParams* ps = &techParams[i];
        uint32 n;
        for (n = 0; n < TTYPE_MAX; n++)
        {
            if (ps->techName[n].c_str()[0])
            {
                if (hw->m_NameStr == ps->techName[n])
                {
                    iLog->LogWarning("WARN: technique '%s' refers to itself as the next technique (ignored)", hw->m_NameStr.c_str());
                }
                else
                {
                    uint32 j;
                    for (j = 0; j < ef->m_HWTechniques.Num(); j++)
                    {
                        SShaderTechnique* hw2 = ef->m_HWTechniques[j];
                        if (hw2->m_NameStr == ps->techName[n])
                        {
                            hw->m_nTechnique[n] = j;
                            break;
                        }
                    }
                    if (j == ef->m_HWTechniques.Num())
                    {
                        iLog->LogWarning("WARN: couldn't find technique '%s' in the sequence for technique '%s' (ignored)", ps->techName[n].c_str(), hw->m_NameStr.c_str());
                    }
                }
            }
        }

        SShaderTechnique* hwZWrite = (hw->m_nTechnique[TTYPE_Z] >= 0) ? ef->m_HWTechniques[hw->m_nTechnique[TTYPE_Z]] : NULL;
        hwZWrite = (hw->m_nTechnique[TTYPE_ZPREPASS] >= 0) ? ef->m_HWTechniques[hw->m_nTechnique[TTYPE_ZPREPASS]] : hwZWrite;
        if (hwZWrite && hwZWrite->m_Passes.Num())
        {
            SShaderPass* pass = &hwZWrite->m_Passes[0];
            if (pass->m_RenderState & GS_DEPTHWRITE)
            {
                hw->m_Flags |= FHF_WASZWRITE;
            }
        }

        bool bTransparent = true;
        for (uint32 j = 0; j < hw->m_Passes.Num(); j++)
        {
            SShaderPass* pass = &hw->m_Passes[j];
            if (CParserBin::PlatformSupportsGeometryShaders() && pass->m_GShader)
            {
                hw->m_Flags |= FHF_USE_GEOMETRY_SHADER;
            }
            if (CParserBin::PlatformSupportsHullShaders() && pass->m_HShader)
            {
                hw->m_Flags |= FHF_USE_HULL_SHADER;
            }
            if (CParserBin::PlatformSupportsDomainShaders() && pass->m_DShader)
            {
                hw->m_Flags |= FHF_USE_DOMAIN_SHADER;
            }
            if (!(pass->m_RenderState & GS_BLEND_MASK))
            {
                bTransparent = false;
            }
        }
        if (bTransparent)
        {
            hw->m_Flags |= FHF_TRANSPARENT;
        }
    }
}

//===========================================================================================

void STexSamplerRT::Update()
{
    if (m_pAnimInfo && m_pAnimInfo->m_Time && gRenDev->m_bPauseTimer == 0)
    {
        CRY_ASSERT(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime >= 0);
        uint32 m = (uint32)(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime / m_pAnimInfo->m_Time) % (m_pAnimInfo->m_NumAnimTexs);
        CRY_ASSERT(m < (uint32)m_pAnimInfo->m_TexPics.Num());

        if (m_pTex && (m_pTex != m_pAnimInfo->m_TexPics[m]))
        {
            m_pTex->Release();

            m_pTex = m_pAnimInfo->m_TexPics[m];

            m_pTex->AddRef();
        }
    }
}

void SFXParam::GetCompName(uint32 nId, CryFixedStringT<128>& name)
{
    if (nId < 0 || nId > 3)
    {
        name = "";
        return;
    }

    char nm[16];
    sprintf_s(nm, "__%d", nId);
    const char* s = strstr(m_Name.c_str(), nm);

    if (!s)
    {
        name = "";
        return;
    }

    s += 3;

    uint32 n;
    for (n = 0; s[n] != 0; ++n)
    {
        if (s[n] <= 0x20 || (s[n] == '_' && s[n + 1] == '_'))
        {
            break;
        }
    }
    name.append(s, n);
}

void SFXParam::GetParamComp(uint32 nOffset, CryFixedStringT<128>& param)
{
    const char* szValue = m_Values.c_str();

    if (!szValue[0])
    {
        param = "";
        return;
    }

    if (szValue[0] == '{')
    {
        ++szValue;
    }

    for (uint32 n = 0; n != nOffset; ++n)
    {
        while (szValue[0] != ',' && szValue[0] != ';' && szValue[0] != '}' && szValue[0] != 0)
        {
            ++szValue;
        }

        if (szValue[0] == ';' || szValue[0] == '}' || szValue[0] == 0)
        {
            param = "";
            return;
        }

        ++szValue;
    }

    while (*szValue == ' ' || *szValue == 8)
    {
        ++szValue;
    }

    uint32 n;
    for (n = 0; szValue[n] != 0; ++n)
    {
        if (szValue[n] == ',' || szValue[n] == ';' || szValue[n] == '}')
        {
            break;
        }
    }
    param.append(szValue, n);
}

string SFXParam::GetValueForName(const char* szName, EParamType& eType)
{
    eType = eType_UNKNOWN;
    if (m_Annotations.empty())
    {
        return "";
    }

    char buf[256];
    char tok[128];
    char* szA = (char*)m_Annotations.c_str();
    SkipCharacters(&szA, kWhiteSpace);
    while (true)
    {
        if (!fxFill(&szA, buf))
        {
            break;
        }
        char* b = buf;
        fxFillPr(&b, tok);
        eType = eType_UNKNOWN;
        if (!azstricmp(tok, "string"))
        {
            eType = eType_STRING;
        }
        else
        if (!azstricmp(tok, "float"))
        {
            eType = eType_FLOAT;
        }
        else
        if (!azstricmp(tok, "half"))
        {
            eType = eType_HALF;
        }

        if (eType != eType_UNKNOWN)
        {
            if (!fxFillPr(&b, tok))
            {
                continue;
            }
            if (azstricmp(tok, szName))
            {
                continue;
            }
            SkipCharacters(&b, kWhiteSpace);
            if (b[0] == '=')
            {
                b++;
                if (!fxFillPrC(&b, tok))
                {
                    break;
                }
            }
            return tok;
        }
        else
        {
            if (azstricmp(tok, szName))
            {
                continue;
            }
            eType = eType_STRING;
            if (!fxFillPr(&b, tok))
            {
                continue;
            }
            if (tok[0] == '=')
            {
                if (!fxFillPr(&b, tok))
                {
                    break;
                }
            }
            return tok;
        }
    }

    return "";
}

const char* CShaderMan::mfParseFX_Parameter (const string& script, EParamType eType, const char* szName)
{
    static char sRet[128];
    int nLen = script.length();
    char* pTemp = new char [nLen + 1];
    azstrcpy(pTemp, nLen + 1, script.c_str());
    sRet[0] = 0;
    char* buf = pTemp;

    char* name;
    long cmd;
    char* data;

    enum
    {
        eString = 1
    };

    static STokenDesc commands[] =
    {
        {eString, "String"},

        {0, 0}
    };

    while ((cmd = shGetObject (&buf, commands, &name, &data)) > 0)
    {
        switch (cmd)
        {
        case eString:
        {
            if (eType != eType_STRING)
            {
                break;
            }
            char* szScr = data;
            if (*szScr == '"')
            {
                szScr++;
            }
            int n = 0;
            while (szScr[n] != 0)
            {
                if (szScr[n] == '"')
                {
                    szScr[n] = ' ';
                }
                n++;
            }
            if (!azstricmp(szName, name))
            {
                azstrcpy(sRet, AZ_ARRAY_SIZE(sRet), data);
                break;
            }
        }
        break;
        }
    }

    SAFE_DELETE_ARRAY(pTemp);

    if (sRet[0])
    {
        return sRet;
    }
    else
    {
        return NULL;
    }
}

//------------------------------------------------------------------------------
// Searches the constant params array for a desired constant based on name.
// Can be optimized if has performance hit.
//------------------------------------------------------------------------------
SFXParam* CShaderMan::mfGetFXParameter(std::vector<SFXParam>& Params, const char* param)
{
    uint32 j;
    for (j = 0; j < Params.size(); j++)
    {
        SFXParam* pr = &Params[j];
        char nameParam[256];
        int n = 0;
        const char* szSrc = pr->m_Name.c_str();
        while (*szSrc != 0)
        {
            if (*szSrc == '[')
            {
                break;
            }
            nameParam[n++] = *szSrc;
            szSrc++;
        }
        nameParam[n] = 0;
        if (!azstricmp(nameParam, param))
        {
            return pr;
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------
// Searches the samplers params array for a desired sampler based on name.
// Can be optimized if has performance hit.
//------------------------------------------------------------------------------
SFXSampler* CShaderMan::mfGetFXSampler(std::vector<SFXSampler>& Params, const char* param)
{
    uint32 j;
    for (j = 0; j < Params.size(); j++)
    {
        SFXSampler* pr = &Params[j];
        char nameParam[256];
        int n = 0;
        const char* szSrc = pr->m_Name.c_str();
        while (*szSrc != 0)
        {
            if (*szSrc == '[')
            {
                break;
            }
            nameParam[n++] = *szSrc;
            szSrc++;
        }
        nameParam[n] = 0;
        if (!azstricmp(nameParam, param))
        {
            return pr;
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------
// Searches the texture params array for a desired texture based on name.
// Can be optimized if has performance hit.
//------------------------------------------------------------------------------
SFXTexture* CShaderMan::mfGetFXTexture(std::vector<SFXTexture>& Params, const char* param)
{
    uint32 j;
    for (j = 0; j < Params.size(); j++)
    {
        SFXTexture* pr = &Params[j];
        char nameParam[256];
        int n = 0;
        const char* szSrc = pr->m_Name.c_str();
        while (*szSrc != 0)
        {
            if (*szSrc == '[')
            {
                break;
            }
            nameParam[n++] = *szSrc;
            szSrc++;
        }
        nameParam[n] = 0;
        if (!azstricmp(nameParam, param))
        {
            return pr;
        }
    }
    return NULL;
}


// We have to parse part of the shader to enumerate public techniques
bool CShaderMan::mfAddFXShaderNames(const char* szName, std::vector<string>* ShaderNames, bool bUpdateCRC)
{
    bool bRes = true;
    SShaderBin* pBin = m_Bin.GetBinShader(szName, false, 0);
    if (!pBin)
    {
        return false;
    }
    if (bUpdateCRC)
    {
        uint32 nCRC32 = pBin->ComputeCRC();
        if (nCRC32 != pBin->m_CRC32)
        {
            FXShaderBinValidCRCItor itor = gRenDev->m_cEF.m_Bin.m_BinValidCRCs.find(pBin->m_dwName);
            if (itor == gRenDev->m_cEF.m_Bin.m_BinValidCRCs.end())
            {
                gRenDev->m_cEF.m_Bin.m_BinValidCRCs.insert(FXShaderBinValidCRCItor::value_type(pBin->m_dwName, false));
            }

            m_Bin.DeleteFromCache(pBin);
            pBin = m_Bin.GetBinShader(szName, false, nCRC32);
            if (!pBin)
            {
                return false;
            }
        }
    }

    // Do not parse techniques for consoles
    if (ShaderNames)
    {
        bRes &= m_Bin.ParseBinFX_Dummy(pBin, *ShaderNames, szName);
    }

    return bRes;
}

CTexture* CShaderMan::mfParseFXTechnique_LoadShaderTexture (STexSamplerRT* smp, const char* szName, [[maybe_unused]] SShaderPass* pShPass, [[maybe_unused]] CShader* ef, [[maybe_unused]] int nIndex, [[maybe_unused]] byte ColorOp, [[maybe_unused]] byte AlphaOp, [[maybe_unused]] byte ColorArg, [[maybe_unused]] byte AlphaArg)
{
    CTexture* tp = NULL;
    if (!szName || !szName[0] || gRenDev->m_bShaderCacheGen) // Sampler without texture specified
    {
        return NULL;
    }

    if (szName[0] == '$')
    {
        tp = mfCheckTemplateTexName(szName, (ETEX_Type)smp->m_eTexType);
        if (tp)
        {
            tp->AddRef();
        }
    }
    else
    {
        smp->m_nTexFlags |= FT_DONT_STREAM;    // disable streaming for explicitly specified textures
    }
    if (!tp)
    {
        tp = mfTryToLoadTexture(szName, smp, smp->GetTexFlags(), false);
    }

    return tp;
}

