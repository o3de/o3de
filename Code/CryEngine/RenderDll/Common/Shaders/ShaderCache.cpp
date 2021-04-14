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
#include "I3DEngine.h"
#include "RemoteCompiler.h"
#include "../RenderCapabilities.h"

#include <AzCore/PlatformId/PlatformId.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Archive/IArchive.h>

uint32 SShaderCombIdent::PostCreate()
{
    FUNCTION_PROFILER_RENDER_FLAT
    // using actual CRC is to expensive,  so replace with cheaper version with
    // has more changes of hits
    //uint32 hash = CCrc32::Compute(&m_RTMask, sizeof(SShaderCombIdent)-sizeof(uint32));
    const uint32* acBuffer = alias_cast<uint32*>(&m_RTMask);
    int len = (sizeof(SShaderCombIdent) - sizeof(uint32)) / sizeof(uint32);
    uint32 hash = 5381;
    while (len--)
    {
        int c = *acBuffer++;
        // hash = hash*33 + c
        hash = ((hash << 5) + hash) + c;
    }

    m_nHash = hash;
    m_MDVMask &= ~SF_PLATFORM;
    return hash;
}

bool CShader::mfPrecache(SShaderCombination& cmb, bool bForce, bool bCompressedOnly, CShaderResources* pRes)
{
    bool bRes = true;

    if (!CRenderer::CV_r_shadersAllowCompilation && !bForce)
    {
        return bRes;
    }

    int nAsync = CRenderer::CV_r_shadersasynccompiling;
    CRenderer::CV_r_shadersasynccompiling = 0;

    uint32 i, j;

    //is this required? Messing with the global render state?
    //gRenDev->m_RP.m_pShader = this;
    //gRenDev->m_RP.m_pCurTechnique = NULL;

    for (i = 0; i < m_HWTechniques.Num(); i++)
    {
        SShaderTechnique* pTech = m_HWTechniques[i];
        for (j = 0; j < pTech->m_Passes.Num(); j++)
        {
            SShaderPass& Pass = pTech->m_Passes[j];
            SShaderCombination c = cmb;
            gRenDev->m_RP.m_FlagsShader_MD = cmb.m_MDMask;
            if (Pass.m_PShader)
            {
                bRes &= Pass.m_PShader->mfPrecache(cmb, bForce, false, bCompressedOnly, this, pRes);
            }
            cmb.m_MDMask = gRenDev->m_RP.m_FlagsShader_MD;
            if (Pass.m_VShader)
            {
                bRes &= Pass.m_VShader->mfPrecache(cmb, bForce, false, bCompressedOnly, this, pRes);
            }
            cmb = c;
        }
    }
    CRenderer::CV_r_shadersasynccompiling = nAsync;

    return bRes;
}

SShaderGenComb* CShaderMan::mfGetShaderGenInfo(const char* nmFX)
{
    SShaderGenComb* c = NULL;
    uint32 i;
    for (i = 0; i < m_SGC.size(); i++)
    {
        c = &m_SGC[i];
        if (!azstricmp(c->Name.c_str(), nmFX))
        {
            break;
        }
    }
    SShaderGenComb cmb;
    if (i == m_SGC.size())
    {
        c = NULL;
        cmb.pGen = mfCreateShaderGenInfo(nmFX, false);
        cmb.Name = CCryNameR(nmFX);
        m_SGC.push_back(cmb);
        c = &m_SGC[i];
    }
    return c;
}

static uint64 sGetGL(char** s, CCryNameR& name, uint32& nHWFlags)
{
    uint32 i;

    nHWFlags = 0;
    SShaderGenComb* c = NULL;
    const char* m = strchr(name.c_str(), '@');
    if (!m)
    {
        m = strchr(name.c_str(), '/');
    }
    assert(m);
    if (!m)
    {
        return -1;
    }
    char nmFX[128];
    char nameExt[128];
    nameExt[0] = 0;
    cry_strcpy(nmFX, name.c_str(), (size_t)(m - name.c_str()));
    c = gRenDev->m_cEF.mfGetShaderGenInfo(nmFX);
    if (!c || !c->pGen || !c->pGen->m_BitMask.Num())
    {
        return 0;
    }
    uint64 nGL = 0;
    SShaderGen* pG = c->pGen;
    for (i = 0; i < pG->m_BitMask.Num(); i++)
    {
        SShaderGenBit* pBit = pG->m_BitMask[i];
        if (pBit->m_nDependencySet & (SHGD_HW_BILINEARFP16 | SHGD_HW_SEPARATEFP16))
        {
            nHWFlags |= pBit->m_nDependencySet;
        }
    }
    while (true)
    {
        char theChar;
        int n = 0;
        while ((theChar = **s) != 0)
        {
            if (theChar == ')' || theChar == '|')
            {
                nameExt[n] = 0;
                break;
            }
            nameExt[n++] = theChar;
            ++*s;
        }
        if (!nameExt[0])
        {
            break;
        }
        for (i = 0; i < pG->m_BitMask.Num(); i++)
        {
            SShaderGenBit* pBit = pG->m_BitMask[i];
            if (!azstricmp(pBit->m_ParamName.c_str(), nameExt))
            {
                nGL |= pBit->m_Mask;
                break;
            }
        }
        if (i == pG->m_BitMask.Num())
        {
            if (!strncmp(nameExt, "0x", 2))
            {
                //nGL |= shGetHex(&nameExt[2]);
            }
            else
            {
                //assert(0);
                if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersdebug)
                {
                    iLog->Log("WARNING: Couldn't find global flag '%s' in shader '%s' (skipped)", nameExt, c->Name.c_str());
                }
            }
        }
        if (**s == '|')
        {
            ++*s;
        }
    }
    return nGL;
}

static uint64 sGetFlag(char** s, SShaderGen* shaderGenInfo)
{
    if (!shaderGenInfo)
    {
        return 0;
    }

    uint32 i = 0;
    uint64 mask = 0;
    char name[128] = {0};
    while (true)
    {
        char theChar;
        int n = 0;
        while ((theChar = **s) != 0)
        {
            if (theChar == ')' || theChar == '|')
            {
                name[n] = 0;
                break;
            }
            name[n++] = theChar;
            ++*s;
        }
        if (!name[0])
        {
            break;
        }
        for (i = 0; i < shaderGenInfo->m_BitMask.Num(); i++)
        {
            SShaderGenBit* pBit = shaderGenInfo->m_BitMask[i];
            if (!azstricmp(pBit->m_ParamName.c_str(), name))
            {
                mask |= pBit->m_Mask;
                break;
            }
        }

        if (i == shaderGenInfo->m_BitMask.Num())
        {
            AZ_Warning("ShaderCache", false, "Couldn't find runtime flag '%s' (skipped)", name);
        }

        if (**s == '|')
        {
            ++*s;
        }
    }
    return mask;
}

static int sEOF(bool bFromFile, char* pPtr, AZ::IO::HandleType fileHandle)
{
    int nStatus;
    if (bFromFile)
    {
        nStatus = gEnv->pCryPak->FEof(fileHandle);
    }
    else
    {
        SkipCharacters(&pPtr, kWhiteSpace);
        if (!*pPtr)
        {
            nStatus = 1;
        }
        else
        {
            nStatus = 0;
        }
    }
    return nStatus;
}

void CShaderMan::mfCloseShadersCache(int nID)
{
    if (m_FPCacheCombinations[nID])
    {
        gEnv->pCryPak->FClose(m_FPCacheCombinations[nID]);
        m_FPCacheCombinations[nID] = 0;
    }
}

void sSkipLine(char*& s)
{
    if (!s)
    {
        return;
    }

    char* sEnd = strchr(s, '\n');
    if (sEnd)
    {
        sEnd++;
        s = sEnd;
    }
}

static void sIterateHW_r(FXShaderCacheCombinations* Combinations, SCacheCombination& cmb, int i, uint64 nHW, const char* szName)
{
    string str;
    gRenDev->m_cEF.mfInsertNewCombination(cmb.Ident, cmb.eCL, szName, 0, &str, false);
    CCryNameR nm = CCryNameR(str.c_str());
    FXShaderCacheCombinationsItor it = Combinations->find(nm);
    if (it == Combinations->end())
    {
        cmb.CacheName = str.c_str();
        Combinations->insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
    }
    for (int j = i; j < 64; j++)
    {
        if (((uint64)1 << j) & nHW)
        {
            cmb.Ident.m_GLMask &= ~((uint64)1 << j);
            sIterateHW_r(Combinations, cmb, j + 1, nHW, szName);
            cmb.Ident.m_GLMask |= ((uint64)1 << j);
            sIterateHW_r(Combinations, cmb, j + 1, nHW, szName);
        }
    }
}

void CShaderMan::mfGetShaderListPath(stack_string& nameOut, int nType)
{
    if (nType == 0)
    {
        nameOut = stack_string(m_szCachePath.c_str()) + stack_string("shaders/shaderlist.txt");
    }
    else
    {
        nameOut = stack_string(m_szCachePath.c_str()) + stack_string("shaders/cache/shaderlistactivate.txt");
    }
}

void CShaderMan::mfMergeShadersCombinations(FXShaderCacheCombinations* Combinations, int nType)
{
    FXShaderCacheCombinationsItor itor;
    for (itor = m_ShaderCacheCombinations[nType].begin(); itor != m_ShaderCacheCombinations[nType].end(); itor++)
    {
        SCacheCombination* cmb = &itor->second;
        FXShaderCacheCombinationsItor it = Combinations->find(cmb->CacheName);
        if (it == Combinations->end())
        {
            Combinations->insert(FXShaderCacheCombinationsItor::value_type(cmb->CacheName, *cmb));
        }
    }
}

//==========================================================================================================================================

struct CompareCombItem
{
    bool operator()(const SCacheCombination& p1, const SCacheCombination& p2) const
    {
        int n = azstricmp(p1.Name.c_str(), p2.Name.c_str());
        if (n)
        {
            return n < 0;
        }
        n = p1.nCount - p2.nCount;
        if (n)
        {
            return n > 0;
        }
        return (azstricmp(p1.CacheName.c_str(), p2.CacheName.c_str()) < 0);
    }
};

#define g_szTestResults "TestResults"

void CShaderMan::mfInitShadersCacheMissLog()
{
    m_ShaderCacheMissCallback = 0;
    m_ShaderCacheMissPath = "";

    // don't access the HD if we don't have any logging to file enabled
    if (!CRenderer::CV_r_shaderslogcachemisses)
    {
        return;
    }

    // create valid path
    gEnv->pCryPak->MakeDir(g_szTestResults);

    m_ShaderCacheMissPath = string("@usercache@\\Shaders\\ShaderCacheMisses.txt");  // do we want this here, or maybe in @log@ ?

    // load data which is already stored
    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
    gEnv->pFileIO->Open(m_ShaderCacheMissPath.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle);
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        char str[2048];
        int nLine = 0;

        while (!gEnv->pFileIO->Eof(fileHandle))
        {
            nLine++;
            str[0] = 0;
            AZ::IO::FGetS(str, 2047, fileHandle);
            if (!str[0])
            {
                continue;
            }

            // remove new line at end
            int len = strlen(str);
            if (len > 0)
            {
                str[strlen(str) - 1] = 0;
            }

            m_ShaderCacheMisses.push_back(str);
        }

        std::sort(m_ShaderCacheMisses.begin(), m_ShaderCacheMisses.end());

        gEnv->pFileIO->Close(fileHandle);
        fileHandle = AZ::IO::InvalidHandle;
    }
}

void CShaderMan::mfInitShadersCache(byte bForLevel, FXShaderCacheCombinations* Combinations, const char* pCombinations, int nType)
{
    COMPILE_TIME_ASSERT(SHADER_LIST_VER != SHADER_SERIALISE_VER);

    char str[2048];
    bool bFromFile = (Combinations == NULL);
    stack_string nameComb;
    m_ShaderCacheExportCombinations.clear();
    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
    if (bFromFile)
    {
        if (!gRenDev->IsEditorMode() && !CRenderer::CV_r_shadersdebug && !gRenDev->IsShaderCacheGenMode())
        {
            return;
        }
        mfGetShaderListPath(nameComb, nType);
        fileHandle = gEnv->pCryPak->FOpen(nameComb.c_str(), "r+");
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            fileHandle = gEnv->pCryPak->FOpen(nameComb.c_str(), "w+");
        }
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            AZ::IO::HandleType statusdstFileHandle = AZ::IO::InvalidHandle;
            gEnv->pFileIO->Open(nameComb.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, statusdstFileHandle);

            if (statusdstFileHandle != AZ::IO::InvalidHandle)
            {
                gEnv->pFileIO->Close(statusdstFileHandle);
                CrySetFileAttributes(str, FILE_ATTRIBUTE_ARCHIVE);
                fileHandle = gEnv->pCryPak->FOpen(nameComb.c_str(), "r+");
            }
        }
        m_FPCacheCombinations[nType] = fileHandle;
        Combinations = &m_ShaderCacheCombinations[nType];
    }

    int nLine = 0;
    char* pPtr = (char*)pCombinations;
    char* ss;
    if (fileHandle != AZ::IO::InvalidHandle || !bFromFile)
    {
        while (!sEOF(bFromFile, pPtr, fileHandle))
        {
            nLine++;

            str[0] = 0;
            if (bFromFile)
            {
                gEnv->pCryPak->FGets(str, 2047, fileHandle);
            }
            else
            {
                fxFillCR(&pPtr, str);
            }
            if (!str[0])
            {
                continue;
            }

            if (str[0] == '/' && str[1] == '/') // commented line e.g. // BadLine: Metal@Common_ShadowPS(%BIllum@IlluminationPS(%DIFFUSE|%ENVCMAMB|%ALPHAGLOW|%STAT_BRANCHING)(%_RT_AMBIENT|%_RT_BUMP|%_RT_GLOW)(101)(0)(0)(ps_2_0)
            {
                continue;
            }

            bool bExportEntry = false;
            int size = strlen(str);
            if (str[size - 1] == 0xa)
            {
                str[size - 1] = 0;
            }
            SCacheCombination cmb;
            char* s = str;
            SkipCharacters(&s, kWhiteSpace);
            if (s[0] != '<')
            {
                continue;
            }
            char* st;
            if (!bForLevel)
            {
                int nVer = atoi(&s[1]);
                if (nVer != SHADER_LIST_VER)
                {
                    if (nVer == SHADER_SERIALISE_VER && bFromFile)
                    {
                        bExportEntry = true;
                    }
                    else
                    {
                        continue;
                    }
                }
                if (s[2] != '>')
                {
                    continue;
                }
                s += 3;
            }
            else
            {
                st = s;
                s = strchr(&st[1], '>');
                if (!s)
                {
                    continue;
                }
                cmb.nCount = atoi(st);
                s++;
            }
            if (bForLevel == 2)
            {
                st = s;
                if (s[0] != '<')
                {
                    continue;
                }
                s = strchr(st, '>');
                if (!s)
                {
                    continue;
                }
                int nVer = atoi(&st[1]);

                if (nVer != SHADER_LIST_VER)
                {
                    if (nVer == SHADER_SERIALISE_VER)
                    {
                        bExportEntry = true;
                    }
                    else
                    {
                        continue;
                    }
                }
                s++;
            }
            st = s;
            s = strchr(s, '(');
            char name[128];
            if (s)
            {
                memcpy(name, st, s - st);
                name[s - st] = 0;
                cmb.Name = name;
                s++;
            }
            else
            {
                continue;
            }
            uint32 nHW = 0;
            cmb.Ident.m_GLMask = sGetGL(&s, cmb.Name, nHW);
            if (cmb.Ident.m_GLMask == -1)
            {
                const char* szFileName = nameComb.c_str();
                if (szFileName)
                {
                    iLog->Log("Error: Error in '%s' file (Line: %d)", szFileName, nLine);
                }
                else
                {
                    assert(!bFromFile);
                    iLog->Log("Error: Error in non-file shader (Line: %d)", nLine);
                }
                sSkipLine(s);
                iLog->Log("Error: Error in '%s' file (Line: %d)", nameComb.c_str(), nLine);
                return;
            }

            ss = strchr(s, '(');
            if (!ss)
            {
                sSkipLine(s);
                iLog->Log("Error: Error in '%s' file (Line: %d)", nameComb.c_str(), nLine);
                return;
            }
            s = ++ss;
            cmb.Ident.m_RTMask = sGetFlag(&s, gRenDev->m_cEF.m_pGlobalExt);

            ss = strchr(s, '(');
            if (!ss)
            {
                sSkipLine(s);
                iLog->Log("Error: Error in '%s' file (Line: %d)", nameComb.c_str(), nLine);
                return;
            }
            s = ++ss;
            cmb.Ident.m_LightMask = shGetHex(s);

            ss = strchr(s, '(');
            if (!ss)
            {
                sSkipLine(s);
                iLog->Log("Error: Error in '%s' file (Line: %d)", nameComb.c_str(), nLine);
                return;
            }
            s = ++ss;
            cmb.Ident.m_MDMask = shGetHex(s);

            ss = strchr(s, '(');
            if (!ss)
            {
                sSkipLine(s);
                iLog->Log("Error: Error in '%s' file (Line: %d)", nameComb.c_str(), nLine);
                return;
            }
            s = ++ss;
            cmb.Ident.m_MDVMask = shGetHex(s);

            ss = strchr(s, '(');
            if (!ss)
            {
                sSkipLine(s);
                iLog->Log("Error: Error in '%s' file (Line: %d)", nameComb.c_str(), nLine);
                return;
            }
            s = ss + 1;
            cmb.Ident.m_pipelineState.opaque = shGetHex64(s);

            ss = strchr(s, '(');
            if (!ss)
            {
                sSkipLine(s);
                iLog->Log("Error: Error in '%s' file (Line: %d)", nameComb.c_str(), nLine);
                return;
            }
            s = ss + 1;
            cmb.Ident.m_STMask = sGetFlag(&s, gRenDev->m_cEF.m_staticExt);

            s = strchr(s, '(');
            if (s)
            {
                s++;
                cmb.eCL = CHWShader::mfStringClass(s);
                assert (cmb.eCL < eHWSC_Num);
            }
            else
            {
                cmb.eCL = eHWSC_Num;
            }
            if constexpr (true || cmb.eCL < eHWSC_Num)
            {
                CCryNameR nm = CCryNameR(st);
                if (bExportEntry)
                {
                    FXShaderCacheCombinationsItor it = m_ShaderCacheExportCombinations.find(nm);
                    if (it == m_ShaderCacheExportCombinations.end())
                    {
                        cmb.CacheName = nm;
                        m_ShaderCacheExportCombinations.insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
                    }
                }
                else
                {
                    FXShaderCacheCombinationsItor it = Combinations->find(nm);
                    if (it != Combinations->end())
                    {
                        //assert(false);
                    }
                    else
                    {
                        cmb.CacheName = nm;
                        Combinations->insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
                    }
                    if (nHW)
                    {
                        for (int j = 0; j < 64; j++)
                        {
                            if (((uint64)1 << j) & nHW)
                            {
                                cmb.Ident.m_GLMask &= ~((uint64)1 << j);
                                sIterateHW_r(Combinations, cmb, j + 1, nHW, name);
                                cmb.Ident.m_GLMask |= ((uint64)1 << j);
                                sIterateHW_r(Combinations, cmb, j + 1, nHW, name);
                                cmb.Ident.m_GLMask &= ~((uint64)1 << j);
                            }
                        }
                    }
                }
            }
            else
            {
                iLog->Log("Error: Error in '%s' file (Line: %d)", nameComb.c_str(), nLine);
            }
        }
    }
}

#if !defined(CONSOLE) && !defined(NULL_RENDERER)
static void sResetDepend_r(SShaderGen* pGen, SShaderGenBit* pBit, SCacheCombination& cm)
{
    if (!pBit->m_DependResets.size())
    {
        return;
    }
    uint32 i, j;

    for (i = 0; i < pBit->m_DependResets.size(); i++)
    {
        const char* c = pBit->m_DependResets[i].c_str();
        for (j = 0; j < pGen->m_BitMask.Num(); j++)
        {
            SShaderGenBit* pBit1 = pGen->m_BitMask[j];
            if (!azstricmp(pBit1->m_ParamName.c_str(), c))
            {
                cm.Ident.m_RTMask &= ~pBit1->m_Mask;
                sResetDepend_r(pGen, pBit1, cm);
                break;
            }
        }
    }
}

static void sSetDepend_r(SShaderGen* pGen, SShaderGenBit* pBit, SCacheCombination& cm)
{
    if (!pBit->m_DependSets.size())
    {
        return;
    }
    uint32 i, j;

    for (i = 0; i < pBit->m_DependSets.size(); i++)
    {
        const char* c = pBit->m_DependSets[i].c_str();
        for (j = 0; j < pGen->m_BitMask.Num(); j++)
        {
            SShaderGenBit* pBit1 = pGen->m_BitMask[j];
            if (!azstricmp(pBit1->m_ParamName.c_str(), c))
            {
                cm.Ident.m_RTMask |= pBit1->m_Mask;
                sSetDepend_r(pGen, pBit1, cm);
                break;
            }
        }
    }
}

// Support for single light only
static bool sIterateDL(DWORD& dwDL)
{
    int nLights = dwDL & 0xf;
    int nType[4];
    int i;

    if (!nLights)
    {
        dwDL = 1;
        return true;
    }
    for (i = 0; i < nLights; i++)
    {
        nType[i] = (dwDL >> (SLMF_LTYPE_SHIFT + (i * SLMF_LTYPE_BITS))) & ((1 << SLMF_LTYPE_BITS) - 1);
    }
    switch (nLights)
    {
    case 1:
        if ((nType[0] & 3) < 2)
        {
            nType[0]++;
        }
        else
        {
            return false;
        }
        break;
    case 2:
        if ((nType[0] & 3) == SLMF_DIRECT)
        {
            nType[0] = SLMF_POINT;
            nType[1] = SLMF_POINT;
        }
        else
        {
            nLights = 3;
            nType[0] = SLMF_DIRECT;
            nType[1] = SLMF_POINT;
            nType[2] = SLMF_POINT;
        }
        break;
    case 3:
        if ((nType[0] & 3) == SLMF_DIRECT)
        {
            nType[0] = SLMF_POINT;
            nType[1] = SLMF_POINT;
            nType[2] = SLMF_POINT;
        }
        else
        {
            nLights = 4;
            nType[0] = SLMF_DIRECT;
            nType[1] = SLMF_POINT;
            nType[2] = SLMF_POINT;
            nType[3] = SLMF_POINT;
        }
        break;
    case 4:
        if ((nType[0] & 3) == SLMF_DIRECT)
        {
            nType[0] = SLMF_POINT;
            nType[1] = SLMF_POINT;
            nType[2] = SLMF_POINT;
            nType[3] = SLMF_POINT;
        }
        else
        {
            return false;
        }
    }
    dwDL = nLights;
    for (i = 0; i < nLights; i++)
    {
        dwDL |= nType[i] << (SLMF_LTYPE_SHIFT + i * SLMF_LTYPE_BITS);
    }
    return true;
}

/*static bool sIterateDL(DWORD& dwDL)
{
  int nLights = dwDL & 0xf;
  int nType[4];
  int i;

  if (!nLights)
  {
    dwDL = 1;
    return true;
  }
  for (i=0; i<nLights; i++)
  {
    nType[i] = (dwDL >> (SLMF_LTYPE_SHIFT + (i*SLMF_LTYPE_BITS))) & ((1<<SLMF_LTYPE_BITS)-1);
  }
  switch (nLights)
  {
  case 1:
    if (!(nType[0] & SLMF_RAE_ENABLED))
      nType[0] |= SLMF_RAE_ENABLED;
    else
      if (nType[0]<2)
        nType[0]++;
      else
      {
        nLights = 2;
        nType[0] = SLMF_DIRECT;
        nType[1] = SLMF_POINT;
      }
      break;
  case 2:
    if (!(nType[0] & SLMF_RAE_ENABLED))
      nType[0] |= SLMF_RAE_ENABLED;
    else
      if (!(nType[1] & SLMF_RAE_ENABLED))
        nType[1] |= SLMF_RAE_ENABLED;
      else
        if (nType[0] == SLMF_DIRECT)
          nType[0] = SLMF_POINT;
        else
        {
          nLights = 3;
          nType[0] = SLMF_DIRECT;
          nType[1] = SLMF_POINT;
          nType[2] = SLMF_POINT;
        }
        break;
  case 3:
    if (!(nType[0] & SLMF_RAE_ENABLED))
      nType[0] |= SLMF_RAE_ENABLED;
    else
      if (!(nType[1] & SLMF_RAE_ENABLED))
        nType[1] |= SLMF_RAE_ENABLED;
      else
        if (!(nType[2] & SLMF_RAE_ENABLED))
          nType[2] |= SLMF_RAE_ENABLED;
        else
          if (nType[0] == SLMF_DIRECT)
            nType[0] = SLMF_POINT;
          else
          {
            nLights = 4;
            nType[0] = SLMF_DIRECT;
            nType[1] = SLMF_POINT;
            nType[2] = SLMF_POINT;
            nType[3] = SLMF_POINT;
          }
          break;
  case 4:
    if (!(nType[0] & SLMF_RAE_ENABLED))
      nType[0] |= SLMF_RAE_ENABLED;
    else
      if (!(nType[1] & SLMF_RAE_ENABLED))
        nType[1] |= SLMF_RAE_ENABLED;
      else
        if (!(nType[2] & SLMF_RAE_ENABLED))
          nType[2] |= SLMF_RAE_ENABLED;
        else
          if (!(nType[3] & SLMF_RAE_ENABLED))
            nType[3] |= SLMF_RAE_ENABLED;
          else
            if (nType[0] == SLMF_DIRECT)
              nType[0] = SLMF_POINT;
            else
              return false;
  }
  dwDL = nLights;
  for (i=0; i<nLights; i++)
  {
    dwDL |= nType[i] << (SLMF_LTYPE_SHIFT + i*SLMF_LTYPE_BITS);
  }
  return true;
}*/

void CShaderMan::mfAddLTCombination(SCacheCombination* cmb, FXShaderCacheCombinations& CmbsMapDst, DWORD dwL)
{
    char str[1024];
    char sLT[64];

    SCacheCombination cm;
    cm = *cmb;
    cm.Ident.m_LightMask = dwL;

    const char* c = strchr(cmb->CacheName.c_str(), ')');
    c = strchr(&c[1], ')');
    int len = (int)(c - cmb->CacheName.c_str() + 1);
    assert(len < sizeof(str));
    cry_strcpy(str, cmb->CacheName.c_str(), len);
    cry_strcat(str, "(");
    azsprintf(sLT, "%x", (uint32)dwL);
    cry_strcat(str, sLT);
    c = strchr(&c[2], ')');
    cry_strcat(str, c);
    CCryNameR nm = CCryNameR(str);
    cm.CacheName = nm;
    FXShaderCacheCombinationsItor it = CmbsMapDst.find(nm);
    if (it == CmbsMapDst.end())
    {
        CmbsMapDst.insert(FXShaderCacheCombinationsItor::value_type(nm, cm));
    }
}

void CShaderMan::mfAddLTCombinations(SCacheCombination* cmb, FXShaderCacheCombinations& CmbsMapDst)
{
    if (!CRenderer::CV_r_shadersprecachealllights)
    {
        return;
    }

    DWORD dwL = 0; // 0 lights
    bool bRes = false;
    do
    {
        // !HACK: Do not iterate multiple lights for low spec
        if ((cmb->Ident.m_RTMask & (g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1])) || (dwL & 0xf) <= 1)
        {
            mfAddLTCombination(cmb, CmbsMapDst, dwL);
        }
        bRes = sIterateDL(dwL);
    } while (bRes);
}


void CShaderMan::mfAddRTCombination_r(int nComb, FXShaderCacheCombinations& CmbsMapDst, SCacheCombination* cmb, CHWShader* pSH, bool bAutoPrecache)
{
    uint32 i, j, n;
    uint32 dwType = pSH->m_dwShaderType;
    if (!dwType)
    {
        return;
    }
    for (i = nComb; i < m_pGlobalExt->m_BitMask.Num(); i++)
    {
        SShaderGenBit* pBit = m_pGlobalExt->m_BitMask[i];
        if (bAutoPrecache && !(pBit->m_Flags & (SHGF_AUTO_PRECACHE | SHGF_LOWSPEC_AUTO_PRECACHE)))
        {
            continue;
        }

        // Precache this flag on low-spec only
        if (pBit->m_Flags & SHGF_LOWSPEC_AUTO_PRECACHE)
        {
            if (cmb->Ident.m_RTMask & (g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1]))
            {
                continue;
            }
        }
        // Only in runtime used
        if (pBit->m_Flags & SHGF_RUNTIME)
        {
            continue;
        }
        for (n = 0; n < pBit->m_PrecacheNames.size(); n++)
        {
            if (pBit->m_PrecacheNames[n] == dwType)
            {
                break;
            }
        }
        if (n < pBit->m_PrecacheNames.size())
        {
            SCacheCombination cm;
            cm = *cmb;
            cm.Ident.m_RTMask &= ~pBit->m_Mask;
            cm.Ident.m_RTMask |= (pBit->m_Mask ^ cmb->Ident.m_RTMask) & pBit->m_Mask;
            if (!bAutoPrecache)
            {
                uint64 nBitSet = pBit->m_Mask & cmb->Ident.m_RTMask;
                if (nBitSet)
                {
                    sSetDepend_r(m_pGlobalExt, pBit, cm);
                }
                else
                {
                    sResetDepend_r(m_pGlobalExt, pBit, cm);
                }
            }

            char str[1024];
            const char* c = strchr(cmb->CacheName.c_str(), '(');
            const size_t len = (size_t)(c - cmb->CacheName.c_str());
            cry_strcpy(str, cmb->CacheName.c_str(), len);
            const char* c1 = strchr(&c[1], '(');
            cry_strcat(str, c, (size_t)(c1 - c));
            cry_strcat(str, "(");
            SShaderGen* pG = m_pGlobalExt;
            stack_string sRT;
            for (j = 0; j < pG->m_BitMask.Num(); j++)
            {
                SShaderGenBit* pBit2 = pG->m_BitMask[j];
                if (pBit2->m_Mask & cm.Ident.m_RTMask)
                {
                    if (!sRT.empty())
                    {
                        sRT += "|";
                    }
                    sRT += pBit2->m_ParamName.c_str();
                }
            }
            cry_strcat(str, sRT.c_str());
            c1 = strchr(&c1[1], ')');
            cry_strcat(str, c1);
            CCryNameR nm = CCryNameR(str);
            cm.CacheName = nm;
            // HACK: don't allow unsupported quality mode
            uint64 nQMask = g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1];
            if ((cm.Ident.m_RTMask & nQMask) != nQMask)
            {
                FXShaderCacheCombinationsItor it = CmbsMapDst.find(nm);
                if (it == CmbsMapDst.end())
                {
                    CmbsMapDst.insert(FXShaderCacheCombinationsItor::value_type(nm, cm));
                }
            }
            if (pSH->m_Flags & (HWSG_SUPPORTS_MULTILIGHTS | HWSG_SUPPORTS_LIGHTING))
            {
                mfAddLTCombinations(&cm, CmbsMapDst);
            }
            mfAddRTCombination_r(i + 1, CmbsMapDst, &cm, pSH, bAutoPrecache);
        }
    }
}

void CShaderMan::mfAddRTCombinations(FXShaderCacheCombinations& CmbsMapSrc, FXShaderCacheCombinations& CmbsMapDst, CHWShader* pSH, bool bListOnly)
{
    if (pSH->m_nFrameLoad == gRenDev->GetFrameID())
    {
        return;
    }
    pSH->m_nFrameLoad = gRenDev->GetFrameID();
    uint32 dwType = pSH->m_dwShaderType;
    if (!dwType)
    {
        return;
    }
    const char* str2 = pSH->mfGetEntryName();
    FXShaderCacheCombinationsItor itor;
    for (itor = CmbsMapSrc.begin(); itor != CmbsMapSrc.end(); itor++)
    {
        SCacheCombination* cmb = &itor->second;
        const char* c = strchr(cmb->Name.c_str(), '@');
        if (!c)
        {
            c = strchr(cmb->Name.c_str(), '/');
        }
        assert(c);
        if (!c)
        {
            continue;
        }
        if (azstricmp(&c[1], str2))
        {
            continue;
        }
        /*if (!azstricmp(str2, "MetalReflVS"))
        {
          if (cmb->nGL == 0x1093)
          {
            int nnn = 0;
          }
        }*/
        if (bListOnly)
        {
            if (pSH->m_Flags & (HWSG_SUPPORTS_MULTILIGHTS | HWSG_SUPPORTS_LIGHTING))
            {
                mfAddLTCombinations(cmb, CmbsMapDst);
            }
            mfAddRTCombination_r(0, CmbsMapDst, cmb, pSH, true);
        }
        else
        {
            mfAddRTCombination_r(0, CmbsMapDst, cmb, pSH, false);
        }
    }
}
#endif // CONSOLE

void GenerateMaskString(SShaderGen* shaderInfo, uint64 mask, stack_string& maskStr)
{
    if (!shaderInfo || !mask)
    {
        return;
    }

    for (unsigned i = 0; i < shaderInfo->m_BitMask.Num(); ++i)
    {
        SShaderGenBit* pBit = shaderInfo->m_BitMask[i];
        if (pBit->m_Mask & mask)
        {
            if (!maskStr.empty())
            {
                maskStr += "|";
            }
            maskStr += pBit->m_ParamName.c_str();
        }
    }
}


void CShaderMan::mfInsertNewCombination(SShaderCombIdent& Ident, EHWShaderClass eCL, const char* name, int nID, string* Str, byte bStore)
{
    char str[2048];
    if (!m_FPCacheCombinations[nID] && bStore)
    {
        return;
    }

    stack_string sGL;
    stack_string sRT;
    stack_string staticMask;
    uint32 i, j;
    SShaderGenComb* c = NULL;
    if (Ident.m_GLMask)
    {
        const char* m = strchr(name, '@');
        if (!m)
        {
            m = strchr(name, '/');
        }
        assert(m);
        char nmFX[128];
        if (m)
        {
            cry_strcpy(nmFX, name, (size_t)(m - name));
            c = mfGetShaderGenInfo(nmFX);
            if (c && c->pGen && c->pGen->m_BitMask.Num())
            {
                SShaderGen* pG = c->pGen;
                for (i = 0; i < 64; i++)
                {
                    if (Ident.m_GLMask & ((uint64)1 << i))
                    {
                        for (j = 0; j < pG->m_BitMask.Num(); j++)
                        {
                            SShaderGenBit* pBit = pG->m_BitMask[j];
                            if (pBit->m_Mask & (Ident.m_GLMask & ((uint64)1 << i)))
                            {
                                if (!sGL.empty())
                                {
                                    sGL += "|";
                                }
                                sGL += pBit->m_ParamName.c_str();
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    GenerateMaskString(m_pGlobalExt, Ident.m_RTMask, sRT);
    GenerateMaskString(m_staticExt, Ident.m_STMask, staticMask);

    uint32 nLT = Ident.m_LightMask;
    if (bStore == 1 && Ident.m_LightMask)
    {
        nLT = 1;
    }
    azsprintf(str, "<%d>%s(%s)(%s)(%x)(%x)(%x)(%llx)(%s)(%s)", SHADER_LIST_VER, name, sGL.c_str(), sRT.c_str(), nLT, Ident.m_MDMask, Ident.m_MDVMask, Ident.m_pipelineState.opaque, staticMask.c_str(), CHWShader::mfClassString(eCL));
    if (!bStore)
    {
        if (Str)
        {
            *Str = str;
        }
        return;
    }
    CCryNameR nm;
    if (str[0] == '<' && str[2] == '>')
    {
        nm = CCryNameR(&str[3]);
    }
    else
    {
        nm = CCryNameR(str);
    }
    FXShaderCacheCombinationsItor it = m_ShaderCacheCombinations[nID].find(nm);
    if (it != m_ShaderCacheCombinations[nID].end())
    {
        return;
    }
    SCacheCombination cmb;
    cmb.Name = name;
    cmb.CacheName = nm;
    cmb.Ident = Ident;
    cmb.eCL = eCL;
    {
        stack_string nameOut;
        mfGetShaderListPath(nameOut, nID);

        static CryCriticalSection s_cResLock;
        AUTO_LOCK(s_cResLock); // Not thread safe without this

        if (m_FPCacheCombinations[nID])
        {
            m_ShaderCacheCombinations[nID].insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
            gEnv->pCryPak->FPrintf(m_FPCacheCombinations[nID], "%s\n", str);
            gEnv->pCryPak->FFlush(m_FPCacheCombinations[nID]);
        }
    }
    if (Str)
    {
        *Str = str;
    }
}

inline bool sCompareComb(const SCacheCombination& a, const SCacheCombination& b)
{
    int32 dif;

    char shader1[128], shader2[128];
    char* tech1 = NULL, * tech2 = NULL;
    azstrcpy(shader1, 128, a.Name.c_str());
    azstrcpy(shader2, 128, b.Name.c_str());
    char* c = strchr(shader1, '@');
    if (!c)
    {
        c = strchr(shader1, '/');
    }
    //assert(c);
    if (c)
    {
        *c = 0;
        tech1 = c + 1;
    }
    c = strchr(shader2, '@');
    if (!c)
    {
        c = strchr(shader2, '/');
    }
    //assert(c);
    if (c)
    {
        *c = 0;
        tech2 = c + 1;
    }

    dif = azstricmp(shader1, shader2);
    if (dif != 0)
    {
        return dif < 0;
    }

    if (tech1 == NULL && tech2 != NULL)
    {
        return true;
    }
    if (tech1 != NULL && tech2 == NULL)
    {
        return false;
    }

    if (tech1 != NULL && tech2 != NULL)
    {
        dif = azstricmp(tech1, tech2);
        if (dif != 0)
        {
            return dif < 0;
        }
    }

    if (a.Ident.m_GLMask != b.Ident.m_GLMask)
    {
        return a.Ident.m_GLMask < b.Ident.m_GLMask;
    }

    if (a.Ident.m_STMask != b.Ident.m_STMask)
    {
        return a.Ident.m_STMask < b.Ident.m_STMask;
    }

    if (a.Ident.m_RTMask != b.Ident.m_RTMask)
    {
        return a.Ident.m_RTMask < b.Ident.m_RTMask;
    }

    if (a.Ident.m_pipelineState.opaque != b.Ident.m_pipelineState.opaque)
    {
        return a.Ident.m_pipelineState.opaque < b.Ident.m_pipelineState.opaque;
    }

    if (a.Ident.m_FastCompare1 != b.Ident.m_FastCompare1)
    {
        return a.Ident.m_FastCompare1 < b.Ident.m_FastCompare1;
    }

    if (a.Ident.m_FastCompare2 != b.Ident.m_FastCompare2)
    {
        return a.Ident.m_FastCompare2 < b.Ident.m_FastCompare2;
    }

    return false;
}

#if !defined(CONSOLE) && !defined(NULL_RENDERER)

void CShaderMan::AddGLCombinations(CShader* pSH, std::vector<SCacheCombination>& CmbsGL)
{
    int i, j;
    uint64 nMask = 0;
    if (pSH->m_pGenShader)
    {
        SShaderGen* pG = pSH->m_pGenShader->m_ShaderGenParams;
        for (i = 0; i < pG->m_BitMask.size(); i++)
        {
            SShaderGenBit* pB = pG->m_BitMask[i];
            SCacheCombination cc;
            cc.Name = pB->m_ParamName;
            for (j = 0; j < m_pGlobalExt->m_BitMask.size(); j++)
            {
                SShaderGenBit* pB1 = m_pGlobalExt->m_BitMask[j];
                if (pB1->m_ParamName == pB->m_ParamName)
                {
                    nMask |= pB1->m_Mask;
                    break;
                }
            }
        }
    }
    else
    {
        SCacheCombination cc;
        cc.Ident.m_GLMask = 0;
        CmbsGL.push_back(cc);
    }
}

void CShaderMan::AddGLCombination(FXShaderCacheCombinations& CmbsMap, SCacheCombination& cmb)
{
    char str[1024];
    const char* st = cmb.CacheName.c_str();
    if (st[0] == '<')
    {
        st += 3;
    }
    const char* s = strchr(st, '@');
    char name[128];
    if (!s)
    {
        s = strchr(st, '/');
    }
    if (s)
    {
        memcpy(name, st, s - st);
        name[s - st] = 0;
    }
    else
    {
        azstrcpy(name, 128, st);
    }
#ifdef __GNUC__
    sprintf(str, "%s(%llx)(%x)(%x)", name, cmb.Ident.m_GLMask, cmb.Ident.m_MDMask, cmb.Ident.m_MDVMask);
#else
    azsprintf(str, "%s(%I64x)(%x)(%x)", name, cmb.Ident.m_GLMask, cmb.Ident.m_MDMask, cmb.Ident.m_MDVMask);
#endif
    CCryNameR nm = CCryNameR(str);
    FXShaderCacheCombinationsItor it = CmbsMap.find(nm);
    if (it == CmbsMap.end())
    {
        cmb.CacheName = nm;
        cmb.Name = nm;
        CmbsMap.insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
    }
}

void CShaderMan::AddCombination(SCacheCombination& cmb,  FXShaderCacheCombinations& CmbsMap, [[maybe_unused]] CHWShader* pHWS)
{
    char str[2048];
    azsprintf(str, "%s(%llx)(%llx)(%d)(%d)(%d)(%llx)(%llx)", cmb.Name.c_str(), cmb.Ident.m_GLMask, cmb.Ident.m_RTMask, cmb.Ident.m_LightMask, cmb.Ident.m_MDMask, cmb.Ident.m_MDVMask, cmb.Ident.m_pipelineState.opaque, cmb.Ident.m_STMask);
    CCryNameR nm = CCryNameR(str);
    FXShaderCacheCombinationsItor it = CmbsMap.find(nm);
    if (it == CmbsMap.end())
    {
        cmb.CacheName = nm;
        CmbsMap.insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
    }
}

void CShaderMan::AddLTCombinations(SCacheCombination& cmb, FXShaderCacheCombinations& CmbsMap, CHWShader* pHWS)
{
    assert(pHWS->m_Flags & HWSG_SUPPORTS_LIGHTING);

    // Just single light support

    // Directional light
    cmb.Ident.m_LightMask = 1;
    AddCombination(cmb, CmbsMap, pHWS);

    // Point light
    cmb.Ident.m_LightMask = 0x101;
    AddCombination(cmb, CmbsMap, pHWS);

    // Projected light
    cmb.Ident.m_LightMask = 0x201;
    AddCombination(cmb, CmbsMap, pHWS);
}

void CShaderMan::AddRTCombinations(FXShaderCacheCombinations& CmbsMap, CHWShader* pHWS, CShader* pSH, FXShaderCacheCombinations* Combinations)
{
    SCacheCombination cmb;

    uint32 nType = pHWS->m_dwShaderType;

    uint32 i, j;
    SShaderGen* pGen = m_pGlobalExt;
    int nBits = 0;

    uint32 nBitsPlatform = 0;
    if (CParserBin::m_nPlatform == SF_ORBIS)
    {
        nBitsPlatform |= SHGD_HW_ORBIS;
    }
    else
    if (CParserBin::m_nPlatform == SF_D3D11)
    {
        nBitsPlatform |= SHGD_HW_DX11;
    }
    else
    if (CParserBin::m_nPlatform == SF_GL4)
    {
        nBitsPlatform |= SHGD_HW_GL4;
    }
    else
    if (CParserBin::m_nPlatform == SF_GLES3)
    {
        nBitsPlatform |= SHGD_HW_GLES3;
    }
    // Confetti Nicholas Baldwin: adding metal shader language support
    else
    if (CParserBin::m_nPlatform == SF_METAL)
    {
        nBitsPlatform |= SHGD_HW_METAL;
    }

    uint64 BitMask[64];
    memset(BitMask, 0, sizeof(BitMask));

    // Make a mask of flags affected by this type of shader
    uint64 nRTMask = 0;
    uint64 nSetMask = 0;

    if (nType)
    {
        for (i = 0; i < pGen->m_BitMask.size(); i++)
        {
            SShaderGenBit* pBit = pGen->m_BitMask[i];
            if (!pBit->m_Mask)
            {
                continue;
            }
            if (pBit->m_Flags & SHGF_RUNTIME)
            {
                continue;
            }
            if (nBitsPlatform & pBit->m_nDependencyReset)
            {
                continue;
            }
            for (j = 0; j < pBit->m_PrecacheNames.size(); j++)
            {
                if (pBit->m_PrecacheNames[j] == nType)
                {
                    if (nBitsPlatform & pBit->m_nDependencySet)
                    {
                        nSetMask |= pBit->m_Mask;
                        continue;
                    }
                    BitMask[nBits++] = pBit->m_Mask;
                    nRTMask |= pBit->m_Mask;
                    break;
                }
            }
        }
    }
    if (nBits > 10)
    {
        CryLog("WARNING: Number of runtime bits for shader '%s' - %d: exceed 10 (too many combinations will be produced)...", pHWS->GetName(), nBits);
    }
    if (nBits > 30)
    {
        CryLog("Error: Ignore...");
        return;
    }

    cmb.eCL = pHWS->m_eSHClass;
    string szName = string(pSH->GetName());
    szName += string("@") + string(pHWS->m_EntryFunc.c_str());
    cmb.Name = szName;
    cmb.Ident.m_GLMask = pHWS->m_nMaskGenShader;

    // For unknown shader type just add combinations from the list
    if (!nType)
    {
        FXShaderCacheCombinationsItor itor;
        for (itor = Combinations->begin(); itor != Combinations->end(); itor++)
        {
            SCacheCombination* c = &itor->second;
            if (c->Name == cmb.Name && c->Ident.m_GLMask == pHWS->m_nMaskGenFX)
            {
                cmb = *c;
                AddCombination(cmb, CmbsMap, pHWS);
            }
        }
        return;
    }

    cmb.Ident.m_LightMask = 0;
    cmb.Ident.m_MDMask = 0;
    cmb.Ident.m_MDVMask = 0;
    cmb.Ident.m_RTMask = 0;
    cmb.Ident.m_STMask = 0;

    int nIterations = 1 << nBits;
    for (i = 0; i < nIterations; i++)
    {
        cmb.Ident.m_RTMask = nSetMask;
        cmb.Ident.m_LightMask = 0;
        for (j = 0; j < nBits; j++)
        {
            if ((1 << j) & i)
            {
                cmb.Ident.m_RTMask |= BitMask[j];
            }
        }
        /*if (cmb.nRT == 0x40002)
        {
          int nnn = 0;
        }*/
        AddCombination(cmb, CmbsMap, pHWS);
        if (pHWS->m_Flags & HWSG_SUPPORTS_LIGHTING)
        {
            AddLTCombinations(cmb, CmbsMap, pHWS);
        }
    }
}

void CShaderMan::_PrecacheShaderList(bool bStatsOnly)
{
    float t0 = gEnv->pTimer->GetAsyncCurTime();

    if (!m_pGlobalExt)
    {
        return;
    }

    m_eCacheMode = eSC_BuildGlobalList;

    uint32 nSaveFeatures = gRenDev->m_Features;
    int nAsync = CRenderer::CV_r_shadersasynccompiling;
    if (nAsync != 3)
    {
        CRenderer::CV_r_shadersasynccompiling = 0;
    }

    // Command line shaders precaching
    gRenDev->m_Features |= RFT_HW_SM20 | RFT_HW_SM2X | RFT_HW_SM30;
    m_bActivatePhase = false;
    FXShaderCacheCombinations* Combinations = &m_ShaderCacheCombinations[0];

    std::vector<SCacheCombination> Cmbs;
    std::vector<SCacheCombination> CmbsRT;
    FXShaderCacheCombinations CmbsMap;
    char str1[128], str2[128];

    // Extract global combinations only (including MD and MDV)
    for (FXShaderCacheCombinationsItor itor = Combinations->begin(); itor != Combinations->end(); itor++)
    {
        SCacheCombination* cmb = &itor->second;
        FXShaderCacheCombinationsItor it = CmbsMap.find(cmb->CacheName);
        if (it == CmbsMap.end())
        {
            CmbsMap.insert(FXShaderCacheCombinationsItor::value_type(cmb->CacheName, *cmb));
        }
    }
    for (FXShaderCacheCombinationsItor itor = CmbsMap.begin(); itor != CmbsMap.end(); itor++)
    {
        SCacheCombination* cmb = &itor->second;
        Cmbs.push_back(*cmb);
    }

    mfExportShaders();

    int nEmpty = 0;
    int nProcessed = 0;
    int nCompiled = 0;
    int nMaterialCombinations = 0;

    if (Cmbs.size() >= 1)
    {
        std::stable_sort(Cmbs.begin(), Cmbs.end(), sCompareComb);

        nMaterialCombinations = Cmbs.size();

        m_nCombinationsProcess = Cmbs.size();
        m_bReload = true;
        m_nCombinationsCompiled = 0;
        m_nCombinationsEmpty = 0;
        for (int i = 0; i < Cmbs.size(); i++)
        {
            SCacheCombination* cmb = &Cmbs[i];
            azstrcpy(str1, 128, cmb->Name.c_str());
            char* c = strchr(str1, '@');
            if (!c)
            {
                c = strchr(str1, '/');
            }
            //assert(c);
            if (c)
            {
                *c = 0;
                m_szShaderPrecache = &c[1];
            }
            else
            {
                c = strchr(str1, '(');
                if (c)
                {
                    *c = 0;
                    m_szShaderPrecache = "";
                }
            }
            gRenDev->m_RP.m_FlagsShader_RT = 0;
            gRenDev->m_RP.m_FlagsShader_LT = 0;
            gRenDev->m_RP.m_FlagsShader_MD = 0;
            gRenDev->m_RP.m_FlagsShader_MDV = 0;
            m_staticFlags = cmb->Ident.m_STMask;
            CShader* pSH = CShaderMan::mfForName(str1, 0, NULL, cmb->Ident.m_GLMask);

            gRenDev->m_RP.m_pShader = pSH;
            assert(gRenDev->m_RP.m_pShader != 0);

            std::vector<SCacheCombination>* pCmbs = &Cmbs;
            FXShaderCacheCombinations CmbsMapRTSrc;
            FXShaderCacheCombinations CmbsMapRTDst;

            for (int m = 0; m < pSH->m_HWTechniques.Num(); m++)
            {
                SShaderTechnique* pTech = pSH->m_HWTechniques[m];
                for (int n = 0; n < pTech->m_Passes.Num(); n++)
                {
                    SShaderPass* pPass = &pTech->m_Passes[n];
                    if (pPass->m_PShader)
                    {
                        pPass->m_PShader->m_nFrameLoad = -10;
                    }
                    if (pPass->m_VShader)
                    {
                        pPass->m_VShader->m_nFrameLoad = -10;
                    }
                }
            }

            for (; i < Cmbs.size(); i++)
            {
                SCacheCombination* cmba = &Cmbs[i];
                azstrcpy(str2, 128, cmba->Name.c_str());
                c = strchr(str2, '@');
                if (!c)
                {
                    c = strchr(str2, '/');
                }
                assert(c);
                if (c)
                {
                    *c = 0;
                }
                if (azstricmp(str1, str2) || cmb->Ident.m_GLMask != cmba->Ident.m_GLMask || cmb->Ident.m_STMask != cmba->Ident.m_STMask)
                {
                    break;
                }
                CmbsMapRTSrc.insert(FXShaderCacheCombinationsItor::value_type(cmba->CacheName, *cmba));
            }
            // surrounding for(;;i++) will increment this again
            i--;
            m_nCombinationsProcess -= CmbsMapRTSrc.size();

            for (FXShaderCacheCombinationsItor itor = CmbsMapRTSrc.begin(); itor != CmbsMapRTSrc.end(); itor++)
            {
                SCacheCombination* cmb2 = &itor->second;
                azstrcpy(str2, 128, cmb2->Name.c_str());
                FXShaderCacheCombinationsItor it = CmbsMapRTDst.find(cmb2->CacheName);
                if (it == CmbsMapRTDst.end())
                {
                    CmbsMapRTDst.insert(FXShaderCacheCombinationsItor::value_type(cmb2->CacheName, *cmb2));
                }
            }

            for (int m = 0; m < pSH->m_HWTechniques.Num(); m++)
            {
                SShaderTechnique* pTech = pSH->m_HWTechniques[m];
                for (int n = 0; n < pTech->m_Passes.Num(); n++)
                {
                    SShaderPass* pPass = &pTech->m_Passes[n];
                    if (pPass->m_PShader)
                    {
                        mfAddRTCombinations(CmbsMapRTSrc, CmbsMapRTDst, pPass->m_PShader, true);
                    }
                    if (pPass->m_VShader)
                    {
                        mfAddRTCombinations(CmbsMapRTSrc, CmbsMapRTDst, pPass->m_VShader, true);
                    }
                }
            }

            CmbsRT.clear();
            for (FXShaderCacheCombinationsItor itor = CmbsMapRTDst.begin(); itor != CmbsMapRTDst.end(); itor++)
            {
                SCacheCombination* cmb2 = &itor->second;
                CmbsRT.push_back(*cmb2);
            }
            pCmbs = &CmbsRT;
            m_nCombinationsProcessOverall = CmbsRT.size();
            m_nCombinationsProcess = 0;

            CmbsMapRTDst.clear();
            CmbsMapRTSrc.clear();
            uint32 nFlags = HWSF_PRECACHE | HWSF_STOREDATA;
            if (bStatsOnly)
            {
                nFlags |= HWSF_FAKE;
            }
            for (int jj = 0; jj < pCmbs->size(); jj++)
            {
                m_nCombinationsProcess++;
                SCacheCombination* cmba = &(*pCmbs)[jj];
                c = (char*)strchr(cmba->Name.c_str(), '@');
                if (!c)
                {
                    c = (char*)strchr(cmba->Name.c_str(), '/');
                }
                assert(c);
                if (!c)
                {
                    continue;
                }
                m_szShaderPrecache = &c[1];
                for (int m = 0; m < pSH->m_HWTechniques.Num(); m++)
                {
                    SShaderTechnique* pTech = pSH->m_HWTechniques[m];
                    for (int n = 0; n < pTech->m_Passes.Num(); n++)
                    {
                        SShaderPass* pPass = &pTech->m_Passes[n];
                        gRenDev->m_RP.m_FlagsShader_RT = cmba->Ident.m_RTMask;
                        gRenDev->m_RP.m_FlagsShader_LT = cmba->Ident.m_LightMask;
                        gRenDev->m_RP.m_FlagsShader_MD = cmba->Ident.m_MDMask;
                        gRenDev->m_RP.m_FlagsShader_MDV = cmba->Ident.m_MDVMask;
                        // Adjust some flags for low spec
                        CHWShader* shaders[] = { pPass->m_PShader, pPass->m_VShader };
                        for (int i2 = 0; i2 < 2; i2++)
                        {
                            CHWShader* shader = shaders[i2];
                            if (shader && (!m_szShaderPrecache || !azstricmp(m_szShaderPrecache, shader->m_EntryFunc.c_str()) != 0))
                            {
                                uint64 nFlagsOrigShader_RT = gRenDev->m_RP.m_FlagsShader_RT & shader->m_nMaskAnd_RT | shader->m_nMaskOr_RT;
                                uint64 nFlagsOrigShader_GL = shader->m_nMaskGenShader;
                                uint32 nFlagsOrigShader_LT = gRenDev->m_RP.m_FlagsShader_LT;

                                shader->mfSetV(nFlags);

                                if (nFlagsOrigShader_RT != gRenDev->m_RP.m_FlagsShader_RT || nFlagsOrigShader_GL != shader->m_nMaskGenShader || nFlagsOrigShader_LT != gRenDev->m_RP.m_FlagsShader_LT)
                                {
                                    m_nCombinationsEmpty++;
                                    if (!bStatsOnly)
                                    {
                                        shader->mfAddEmptyCombination(pSH, nFlagsOrigShader_RT, nFlagsOrigShader_GL, nFlagsOrigShader_LT);
                                    }
                                    shader->m_nMaskGenShader = nFlagsOrigShader_GL;
                                }
                            }
                        }

                        if (CParserBin::m_nPlatform == SF_D3D11 || CParserBin::m_nPlatform == SF_JASPER || CParserBin::m_nPlatform == SF_ORBIS || CParserBin::m_nPlatform == SF_GL4)
                        {
                            CHWShader* d3d11Shaders[] = { pPass->m_GShader, pPass->m_HShader, pPass->m_CShader, pPass->m_DShader };
                            for (int i2 = 0; i2 < 4; i2++)
                            {
                                CHWShader* shader = d3d11Shaders[i2];
                                if (shader && (!m_szShaderPrecache || !azstricmp(m_szShaderPrecache, shader->m_EntryFunc.c_str()) != 0))
                                {
                                    shader->mfSetV(nFlags);
                                }
                            }
                        }

                        if (bStatsOnly)
                        {
                            static int nLastCombs = 0;
                            if (m_nCombinationsCompiled != nLastCombs && !(m_nCombinationsCompiled & 0x7f))
                            {
                                nLastCombs = m_nCombinationsCompiled;
                                CryLog("-- Processed: %d, Compiled: %d, Referenced (Empty): %d...", m_nCombinationsProcess, m_nCombinationsCompiled, m_nCombinationsEmpty);
                            }
                        }
#ifdef WIN32
                        if (!m_bActivatePhase)
                        {
                            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
                        }
#endif
                    }
                }
                pSH->mfFlushPendedShaders();
                iLog->Update();
                IRenderer* pRenderer = gEnv->pRenderer;
                if (pRenderer)
                {
                    pRenderer->FlushRTCommands(true, true, true);
                }
            }

            pSH->mfFlushCache();

            // HACK HACK HACK:
            // should be bigger than 0, but could cause issues right now when checking for RT
            // combinations when no shadertype is defined and the previous shader line
            // was still async compiling -- needs fix in HWShader for m_nMaskGenFX
            CHWShader::mfFlushPendedShadersWait(0);
            if (!m_bActivatePhase)
            {
                SAFE_RELEASE(pSH);
            }

            nProcessed += m_nCombinationsProcess;
            nCompiled += m_nCombinationsCompiled;
            nEmpty += m_nCombinationsEmpty;

            m_nCombinationsProcess = 0;
            m_nCombinationsCompiled = 0;
            m_nCombinationsEmpty = 0;
        }
    }
    CHWShader::mfFlushPendedShadersWait(-1);


    // Optimise shader resources
    SOptimiseStats Stats;
    for (FXShaderCacheNamesItor it = CHWShader::m_ShaderCacheList.begin(); it != CHWShader::m_ShaderCacheList.end(); it++)
    {
        const char* szName = it->first.c_str();
        SShaderCache* c = CHWShader::mfInitCache(szName, NULL, false, it->second, false);
        if (c)
        {
            SOptimiseStats _Stats;
            CHWShader::mfOptimiseCacheFile(c, false, &_Stats);
            Stats.nEntries += _Stats.nEntries;
            Stats.nUniqueEntries += _Stats.nUniqueEntries;
            Stats.nSizeCompressed += _Stats.nSizeCompressed;
            Stats.nSizeUncompressed += _Stats.nSizeUncompressed;
            Stats.nTokenDataSize += _Stats.nTokenDataSize;
        }
        c->Release();
    }

    CHWShader::m_ShaderCacheList.clear();

    m_eCacheMode = eSC_Normal;
    m_bReload = false;
    m_szShaderPrecache = NULL;
    m_bActivatePhase = false;
    CRenderer::CV_r_shadersasynccompiling = nAsync;

    gRenDev->m_Features = nSaveFeatures;

    float t1 = gEnv->pTimer->GetAsyncCurTime();
    CryLogAlways("All shaders combinations compiled in %.2f seconds", (t1 - t0));
    CryLogAlways("Combinations: (Material: %d, Processed: %d; Compiled: %d; Removed: %d)", nMaterialCombinations, nProcessed, nCompiled, nEmpty);
    CryLogAlways("-- Shader cache overall stats: Entries: %d, Unique Entries: %d, Size: %d, Compressed Size: %d, Token data size: %d", Stats.nEntries, Stats.nUniqueEntries, Stats.nSizeUncompressed, Stats.nSizeCompressed, Stats.nTokenDataSize);

    m_nCombinationsProcess = -1;
    m_nCombinationsCompiled = -1;
    m_nCombinationsEmpty = -1;
}

#endif

void CHWShader::mfGenName(uint64 GLMask, uint64 RTMask, uint32 LightMask, uint32 MDMask, uint32 MDVMask, uint64 PSS, uint64 STMask, EHWShaderClass eClass, char* dstname, int nSize, byte bType)
{
    assert(dstname);
    dstname[0] = 0;

    char str[32];
    if (bType != 0 && GLMask)
    {
        azsprintf(str, "(GL%llx)", GLMask);
        cry_strcat(dstname, nSize, str);
    }
    if (bType != 0)
    {
        azsprintf(str, "(RT%llx)", RTMask);
        cry_strcat(dstname, nSize, str);
    }
    if (bType != 0 && LightMask)
    {
        azsprintf(str, "(LT%x)", LightMask);
        cry_strcat(dstname, nSize, str);
    }
    if (bType != 0 && MDMask)
    {
        azsprintf(str, "(MD%x)", MDMask);
        cry_strcat(dstname, nSize, str);
    }
    if (bType != 0 && MDVMask)
    {
        azsprintf(str, "(MDV%x)", MDVMask);
        cry_strcat(dstname, nSize, str);
    }
    if (bType != 0 && PSS)
    {
        azsprintf(str, "(PSS%llx)", PSS);
        cry_strcat(dstname, nSize, str);
    }
    if (bType != 0 && STMask)
    {
        azsprintf(str, "(ST%llx)", STMask);
        cry_strcat(dstname, nSize, str);
    }
    if (bType != 0)
    {
        azsprintf(str, "(%s)", mfClassString(eClass));
        cry_strcat(dstname, nSize, str);
    }
}

#if !defined(CONSOLE) && !defined(NULL_RENDERER)

void CShaderMan::mfPrecacheShaders(bool bStatsOnly)
{
    AZ_Assert(CRenderer::CV_r_shadersPlatform != static_cast<int>(AZ::PlatformID::PLATFORM_MAX), "You must set a shaders platform (r_shadersPlatform) before precaching the shaders");
    CHWShader::mfFlushPendedShadersWait(-1);

    if (CRenderer::CV_r_shadersorbis)
    {
#ifdef WATER_TESSELLATION_RENDERER
        CRenderer::CV_r_WaterTessellationHW = 0;
#endif
        gRenDev->m_bDeviceSupportsFP16Filter = true;
        gRenDev->m_bDeviceSupportsFP16Separate = false;
        gRenDev->m_bDeviceSupportsGeometryShaders = true;
        gRenDev->m_Features |= RFT_HW_SM30;

        CParserBin::m_bShaderCacheGen = true;

        gRenDev->m_Features |= RFT_HW_SM50;
        CParserBin::SetupForOrbis();
        CryLogAlways("\nStarting shader compilation for Orbis...");
        mfInitShadersList(NULL);
        mfPreloadShaderExts();
        _PrecacheShaderList(bStatsOnly);
    }
    else
    if (CRenderer::CV_r_shadersdx11)
    {
        gRenDev->m_bDeviceSupportsFP16Filter = true;
        gRenDev->m_bDeviceSupportsFP16Separate = false;
        gRenDev->m_bDeviceSupportsTessellation = true;
        gRenDev->m_bDeviceSupportsGeometryShaders = true;
        gRenDev->m_Features |= RFT_HW_SM30;

        CParserBin::m_bShaderCacheGen = true;

        gRenDev->m_Features |= RFT_HW_SM50;
        CParserBin::SetupForD3D11();
        CryLogAlways("\nStarting shader compilation for D3D11...");
        mfInitShadersList(NULL);
        mfPreloadShaderExts();
        _PrecacheShaderList(bStatsOnly);
    }
    else
    if (CRenderer::CV_r_shadersGL4)
    {
        gRenDev->m_bDeviceSupportsFP16Filter = true;
        gRenDev->m_bDeviceSupportsFP16Separate = false;
        gRenDev->m_bDeviceSupportsTessellation = true;
        gRenDev->m_bDeviceSupportsGeometryShaders = true;
        gRenDev->m_Features |= RFT_HW_SM30;

        CParserBin::m_bShaderCacheGen = true;

        gRenDev->m_Features |= RFT_HW_SM50;
        CParserBin::SetupForGL4();
        CryLogAlways("\nStarting shader compilation for GLSL 4...");
        mfInitShadersList(NULL);
        mfPreloadShaderExts();
        _PrecacheShaderList(bStatsOnly);
    }
    else
    if (CRenderer::CV_r_shadersGLES3)
    {
        gRenDev->m_bDeviceSupportsFP16Filter = true;
        gRenDev->m_bDeviceSupportsFP16Separate = false;
        gRenDev->m_bDeviceSupportsTessellation = false;
        gRenDev->m_bDeviceSupportsGeometryShaders = false;
        gRenDev->m_Features |= RFT_HW_SM30;

        CParserBin::m_bShaderCacheGen = true;

        gRenDev->m_Features |= RFT_HW_SM50;
        CParserBin::SetupForGLES3();
        CryLogAlways("\nStarting shader compilation for GLSL-ES 3...");
        mfInitShadersList(NULL);
        mfPreloadShaderExts();
        _PrecacheShaderList(bStatsOnly);
    }
    else
    if (CRenderer::CV_r_shadersMETAL)
    {
        AZ_Assert(CRenderer::CV_r_shadersPlatform == static_cast<int>(AZ::PlatformID::PLATFORM_APPLE_OSX) || CRenderer::CV_r_shadersPlatform == static_cast<int>(AZ::PlatformID::PLATFORM_APPLE_IOS), "Invalid platform (%d) for metal shaders", CRenderer::CV_r_shadersPlatform);
        gRenDev->m_bDeviceSupportsFP16Filter = true;
        gRenDev->m_bDeviceSupportsFP16Separate = false;
        gRenDev->m_bDeviceSupportsTessellation = false;
        gRenDev->m_bDeviceSupportsGeometryShaders = false;
        gRenDev->m_Features |= RFT_HW_SM30;

        CParserBin::m_bShaderCacheGen = true;

        gRenDev->m_Features |= RFT_HW_SM50;
        CParserBin::SetupForMETAL();
        CryLogAlways("\nStarting shader compilation for METAL...");
        mfInitShadersList(NULL);
        mfPreloadShaderExts();
        _PrecacheShaderList(bStatsOnly);
    }
    
#if defined(AZ_PLATFORM_WINDOWS)
    CRenderer::CV_r_shadersPlatform = static_cast<int>(AZ::PlatformID::PLATFORM_WINDOWS_64);
    CParserBin::SetupForD3D11();
#elif defined(AZ_PLATFORM_MAC)
    CRenderer::CV_r_shadersPlatform = static_cast<int>(AZ::PlatformID::PLATFORM_APPLE_OSX);
    CParserBin::SetupForMETAL();
#endif

    gRenDev->m_cEF.m_Bin.InvalidateCache();
}

void CShaderMan::mfGetShaderList()
{
    if (CRenderer::CV_r_shadersorbis)
    {
        CParserBin::m_bShaderCacheGen = true;
        CParserBin::SetupForOrbis();
    }
    else
    if (CRenderer::CV_r_shadersdx11)
    {
        CParserBin::m_bShaderCacheGen = true;
        CParserBin::SetupForD3D11();
        CryLogAlways("\nGet shader list for D3D11...");
    }
    else
    if (CRenderer::CV_r_shadersGL4)
    {
        CParserBin::m_bShaderCacheGen = true;
        CParserBin::SetupForGL4();
        CryLogAlways("\nGet shader list for GLSL 4...");
    }
    else
    if (CRenderer::CV_r_shadersGLES3)
    {
        CParserBin::m_bShaderCacheGen = true;
        CParserBin::SetupForGLES3();
        CryLogAlways("\nGet shader list for GLSL-ES 3...");
    }
    else
    if (CRenderer::CV_r_shadersMETAL)
    {
        CParserBin::m_bShaderCacheGen = true;
        CParserBin::SetupForMETAL();
        CryLogAlways("\nGet shader list for METAL...");
    }

    std::vector<uint8> Data;
    if (NRemoteCompiler::ESOK == NRemoteCompiler::CShaderSrv::Instance().GetShaderList(Data))
    {
        CryLogAlways("\nGet shader list Succeeded...\nStart Writing shader list to @user@\\cache\\shaders\\shaderlist.txt ...");
        mfCloseShadersCache(0);
        mfCloseShadersCache(1);
        AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen("@user@\\cache\\shaders\\shaderlist.txt", "w+b");
        if (fileHandle != AZ::IO::InvalidHandle)
        {
            size_t writtenSoFar = 0;
            size_t remaining = Data.size();
            while (remaining)
            {
                writtenSoFar += gEnv->pCryPak->FWrite(Data.data() + writtenSoFar, 1, remaining, fileHandle);
                remaining -= writtenSoFar;
            }
            gEnv->pCryPak->FClose(fileHandle);
            CryLogAlways("\nFinished writing shader list to @user@\\cache\\shaders\\shaderlist.txt ...");
        }
        else
        {
            CryLogAlways("\nFailed writing shader list to @user@\\cache\\shaders\\shaderlist.txt ...");
        }
    }
    else
    {
        CryLogAlways("\nGet shader list Failed...");
    }
    
    CParserBin::SetupForD3D11();
}

void CShaderMan::mfExportShaders()
{
}

void CShaderMan::mfOptimiseShaders(const char* szFolder, bool bForce)
{
    CHWShader::mfFlushPendedShadersWait(-1);

    float t0 = gEnv->pTimer->GetAsyncCurTime();
    SShaderCache* pCache;
    uint32 i;

    std::vector<CCryNameR> Names;
    mfGatherFilesList(szFolder, Names, 0, false);

    SOptimiseStats Stats;
    for (i = 0; i < Names.size(); i++)
    {
        const char* szName = Names[i].c_str();
        constexpr AZStd::string_view userCache = "@usercache@/";
        if (szName == userCache)
        {
            szName += userCache.size();
        }
        pCache = CHWShader::mfInitCache(szName, NULL, false, 0, false);
        if (!pCache || !pCache->m_pRes[CACHE_USER])
        {
            continue;
        }
        SOptimiseStats _Stats;
        CHWShader::mfOptimiseCacheFile(pCache, bForce, &_Stats);
        Stats.nEntries += _Stats.nEntries;
        Stats.nUniqueEntries += _Stats.nUniqueEntries;
        Stats.nSizeCompressed += _Stats.nSizeCompressed;
        Stats.nSizeUncompressed += _Stats.nSizeUncompressed;
        Stats.nTokenDataSize += _Stats.nTokenDataSize;
        Stats.nDirDataSize += _Stats.nDirDataSize;
        pCache->Release();
    }

    float t1 = gEnv->pTimer->GetAsyncCurTime();
    CryLog("-- All shaders combinations optimized in %.2f seconds", t1 - t0);
    CryLog("-- Shader cache overall stats: Entries: %d, Unique Entries: %d, Size: %.3f, Compressed Size: %.3f, Token data size: %.3f, Directory Size: %.3f Mb", Stats.nEntries, Stats.nUniqueEntries, Stats.nSizeUncompressed / 1024.0f / 1024.0f, Stats.nSizeCompressed / 1024.0f / 1024.0f, Stats.nTokenDataSize / 1024.0f / 1024.0f, Stats.nDirDataSize / 1024.0f / 1024.0f);
}

struct SMgData
{
    CCryNameTSCRC Name;
    int nSize;
    uint32 CRC;
    uint32 flags;
    byte* pData;
    int nID;
    byte bProcessed;
};

static int snCurListID;
typedef std::map<CCryNameTSCRC, SMgData> ShaderData;
typedef ShaderData::iterator ShaderDataItor;

static void sAddToList(SShaderCache* pCache, ShaderData& Data)
{
    uint32 i;
    CResFile* pRes = pCache->m_pRes[CACHE_USER];
    ResDir* Dir = pRes->mfGetDirectory();
    for (i = 0; i < Dir->size(); i++)
    {
        SDirEntry* pDE = &(*Dir)[i];
        if (pDE->Name == CShaderMan::s_cNameHEAD)
        {
            continue;
        }
        ShaderDataItor it = Data.find(pDE->Name);
        if (it == Data.end())
        {
            SMgData d;
            d.nSize = pRes->mfFileRead(pDE);
            SDirEntryOpen* pOE = pRes->mfGetOpenEntry(pDE);
            assert(pOE);
            if (!pOE)
            {
                continue;
            }
            d.flags = pDE->flags;
            if (pDE->flags & RF_RES_$)
            {
                d.pData = new byte[d.nSize];
                memcpy(d.pData, pOE->pData, d.nSize);
                d.bProcessed = 0;
                d.Name = pDE->Name;
                d.CRC = 0;
                d.nID = snCurListID++;
                Data.insert(ShaderDataItor::value_type(d.Name, d));
                continue;
            }
            if (d.nSize < sizeof(SShaderCacheHeaderItem))
            {
                assert(0);
                continue;
            }
            d.pData = new byte[d.nSize];
            memcpy(d.pData, pOE->pData, d.nSize);
            SShaderCacheHeaderItem* pItem = (SShaderCacheHeaderItem*)d.pData;
            d.bProcessed = 0;
            d.Name = pDE->Name;
            d.CRC = pItem->m_CRC32;
            d.nID = snCurListID++;
            Data.insert(ShaderDataItor::value_type(d.Name, d));
        }
    }
}

struct SNameData
{
    CCryNameR Name;
    bool bProcessed;
};

void CShaderMan::_MergeShaders()
{
    float t0 = gEnv->pTimer->GetAsyncCurTime();
    SShaderCache* pCache;
    uint32 i, j;

    std::vector<CCryNameR> NM;
    std::vector<SNameData> Names;
    mfGatherFilesList(m_ShadersMergeCachePath.c_str(), NM, 0, true);
    for (i = 0; i < NM.size(); i++)
    {
        SNameData dt;
        dt.bProcessed = false;
        dt.Name = NM[i];
        Names.push_back(dt);
    }

    uint32 CRC32 = 0;
    for (i = 0; i < Names.size(); i++)
    {
        if (Names[i].bProcessed)
        {
            continue;
        }
        Names[i].bProcessed = true;
        const char* szNameA = Names[i].Name.c_str();
        iLog->Log(" Merging shader resource '%s'...", szNameA);
        char szDrv[16], szDir[256], szName[256], szExt[32], szName1[256], szName2[256];
#ifdef AZ_COMPILER_MSVC
        _splitpath_s(szNameA, szDrv, szDir, szName, szExt);
#else
        _splitpath(szNameA, szDrv, szDir, szName, szExt);
#endif
        azsprintf(szName1, "%s%s", szName, szExt);
        uint32 nLen = strlen(szName1);
        pCache = CHWShader::mfInitCache(szNameA, NULL, false, CRC32, false);
        SResFileLookupData* pData;
        if (pCache->m_pRes[CACHE_USER])
        {
            pData = pCache->m_pRes[CACHE_USER]->GetLookupData(false, 0, 0);
            if (pData)
            {
                CRC32 = pData->m_CRC32;
            }
        }
        else
        if (pCache->m_pRes[CACHE_READONLY])
        {
            pData = pCache->m_pRes[CACHE_READONLY]->GetLookupData(false, 0, 0);
            if (pData)
            {
                CRC32 = pData->m_CRC32;
            }
        }
        else
        {
            assert(0);
        }
        ShaderData Data;
        snCurListID = 0;
        sAddToList(pCache, Data);
        SAFE_RELEASE(pCache);
        for (j = i + 1; j < Names.size(); j++)
        {
            if (Names[j].bProcessed)
            {
                continue;
            }
            const char* szNameB = Names[j].Name.c_str();
#ifdef AZ_COMPILER_MSVC
            _splitpath_s(szNameB, szDrv, szDir, szName, szExt);
#else
            _splitpath(szNameB, szDrv, szDir, szName, szExt);
#endif
            azsprintf(szName2, "%s%s", szName, szExt);
            if (!azstricmp(szName1, szName2))
            {
                Names[j].bProcessed = true;
                SShaderCache* pCache1 = CHWShader::mfInitCache(szNameB, NULL, false, 0, false);
                pData = pCache1->m_pRes[CACHE_USER]->GetLookupData(false, 0, 0);
                assert(pData && pData->m_CRC32 == CRC32);
                if (!pData || pData->m_CRC32 != CRC32)
                {
                    Warning("WARNING: CRC mismatch for %s", szNameB);
                }
                sAddToList(pCache1, Data);
                SAFE_RELEASE(pCache1);
            }
        }
        char szDest[256];
        cry_strcpy(szDest, m_ShadersCache.c_str());
        const char* p = &szNameA[strlen(szNameA) - nLen - 2];
        while (*p != '/' && *p != '\\')
        {
            p--;
        }
        cry_strcat(szDest, p + 1);
        pCache = CHWShader::mfInitCache(szDest, NULL, true, CRC32, false);
        CResFile* pRes = pCache->m_pRes[CACHE_USER];
        pRes->mfClose();
        pRes->mfOpen(RA_CREATE, &gRenDev->m_cEF.m_ResLookupDataMan[CACHE_USER]);

        pRes->GetLookupData(true, CRC32, FX_CACHE_VER);
        pRes->mfFlush();

        int nDeviceShadersCounter = 0x10000000;
        ShaderDataItor it;
        for (it = Data.begin(); it != Data.end(); it++)
        {
            SMgData* pD = &it->second;
            SDirEntry de;
            de.Name = pD->Name;
            de.size = pD->nSize;
            de.flags = pD->flags;
            if (pD->flags & RF_RES_$)
            {
                de.flags &= ~RF_COMPRESS;
            }
            else
            {
                de.flags |= RF_COMPRESS;
                de.offset = nDeviceShadersCounter++;
            }
            byte* pNew = new byte[de.size];
            memcpy(pNew, pD->pData, pD->nSize);
            de.flags |= RF_TEMPDATA;
            pRes->mfFileAdd(&de);
            SDirEntryOpen* pOE = pRes->mfOpenEntry(&de);
            pOE->pData = pNew;
        }
        for (it = Data.begin(); it != Data.end(); it++)
        {
            SMgData* pD = &it->second;
            delete [] pD->pData;
        }
        Data.clear();
        pRes->mfFlush();
        iLog->Log(" ...%d result items...", pRes->mfGetNumFiles());
        pCache->Release();
    }

    mfOptimiseShaders(gRenDev->m_cEF.m_ShadersCache.c_str(), true);

    float t1 = gEnv->pTimer->GetAsyncCurTime();
    CryLog("All shaders files merged in %.2f seconds", t1 - t0);
}

void CShaderMan::mfMergeShaders()
{
    CHWShader::mfFlushPendedShadersWait(-1);

    CParserBin::SetupForD3D11();
    _MergeShaders();
}

//////////////////////////////////////////////////////////////////////////
bool CShaderMan::CheckAllFilesAreWritable(const char* szDir) const
{
#if (defined(WIN32) || defined(WIN64))
    assert(szDir);

    auto pack = gEnv->pCryPak;
    assert(pack);

    string sPathWithFilter = string(szDir) + "/*";

    // Search files that match filter specification.
    AZ::IO::ArchiveFileIterator handle;
    if (handle = pack->FindFirst(sPathWithFilter.c_str()); handle)
    {
        do
        {
            if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) != AZ::IO::FileDesc::Attribute::Subdirectory)
            {
                string fullpath = string(szDir) + "/" + string(handle.m_filename.data(), handle.m_filename.size());

                AZ::IO::HandleType outFileHandle = pack->FOpen(fullpath.c_str(), "rb");
                if (outFileHandle == AZ::IO::InvalidHandle)
                {
                    handle = pack->FindNext(handle);
                    continue;
                }
                if (pack->IsInPak(outFileHandle))
                {
                    pack->FClose(outFileHandle);
                    handle = pack->FindNext(handle);
                    continue;
                }
                pack->FClose(outFileHandle);

                outFileHandle = pack->FOpen(fullpath.c_str(), "ab");

                if (outFileHandle != AZ::IO::InvalidHandle)
                {
                    pack->FClose(outFileHandle);
                }
                else
                {
                    gEnv->pLog->LogError("ERROR: Shader cache is not writable (file: '%s')", fullpath.c_str());
                    return false;
                }
            }

            handle = pack->FindNext(handle);
        } while (handle);

        pack->FindClose(handle);

        gEnv->pLog->LogToFile("Shader cache directory '%s' was successfully tested for being writable", szDir);
    }
    else
    {
        CryLog("Shader cache directory '%s' does not exist", szDir);
    }

#endif

    return true;
}

#endif // CONSOLE

bool CShaderMan::mfPreloadBinaryShaders()
{
    LOADING_TIME_PROFILE_SECTION;
    AZ_TRACE_METHOD();
    // don't preload binary shaders if we are in editing mode
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_shadersediting)
    {
        return false;
    }

    // don't load all binary shaders twice
    if (m_Bin.m_bBinaryShadersLoaded)
    {
        return true;
    }

    bool bFound = iSystem->GetIPak()->LoadPakToMemory("Engine/ShadersBin.pak", AZ::IO::IArchive::eInMemoryPakLocale_CPU);
    if (!bFound)
    {
        return false;
    }

#ifndef _RELEASE
    // also load shaders pak file to memory because shaders are also read, when data not found in bin, and to check the CRC
    // of the source shaders against the binary shaders in non release mode
    iSystem->GetIPak()->LoadPakToMemory("Engine/Shaders.pak", AZ::IO::IArchive::eInMemoryPakLocale_CPU);
#endif

    AZStd::string allFilesPath = m_ShadersCache + "/*";

    AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst (allFilesPath.c_str());
    if (!handle)
    {
        return false;
    }
    std::vector<string> FilesCFX;
    std::vector<string> FilesCFI;

    do
    {
        if (gEnv->pSystem && gEnv->pSystem->IsQuitting())
        {
            return false;
        }
        if (handle.m_filename.front() == '.')
        {
            continue;
        }
        if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
        {
            continue;
        }
        const char* szExt = fpGetExtension(handle.m_filename.data());
        if (!azstricmp(szExt, ".cfib"))
        {
            FilesCFI.emplace_back(handle.m_filename.data(), handle.m_filename.size());
        }
        else
        if (!azstricmp(szExt, ".cfxb"))
        {
            FilesCFX.emplace_back(handle.m_filename.data(), handle.m_filename.size());
        }
    } while (handle = gEnv->pCryPak->FindNext(handle));

    if (FilesCFX.size() + FilesCFI.size() > MAX_FXBIN_CACHE)
    {
        SShaderBin::s_nMaxFXBinCache = FilesCFX.size() + FilesCFI.size();
    }
    uint32 i;
    char sName[256];

    {
        LOADING_TIME_PROFILE_SECTION_NAMED("CShaderMan::mfPreloadBinaryShaders(): FilesCFI");
        for (i = 0; i < FilesCFI.size(); i++)
        {
            if (gEnv->pSystem && gEnv->pSystem->IsQuitting())
            {
                return false;
            }

            const string& file = FilesCFI[i];
            cry_strcpy(sName, file.c_str());
            fpStripExtension(sName, sName);
            [[maybe_unused]] SShaderBin* pBin = m_Bin.GetBinShader(sName, true, 0);
            AZ_Error("Rendering", pBin, "Error pre-loading binary shader %s", file.c_str());
        }
    }

    {
        LOADING_TIME_PROFILE_SECTION_NAMED("CShaderMan::mfPreloadBinaryShaders(): FilesCFX");
        for (i = 0; i < FilesCFX.size(); i++)
        {
            if (gEnv->pSystem && gEnv->pSystem->IsQuitting())
            {
                return false;
            }

            const string& file = FilesCFX[i];
            cry_strcpy(sName, file.c_str());
            fpStripExtension(sName, sName);
            [[maybe_unused]] SShaderBin* pBin = m_Bin.GetBinShader(sName, false, 0);
            AZ_Error("Rendering", pBin, "Error pre-loading binary shader %s", file.c_str());
        }
    }

    gEnv->pCryPak->FindClose (handle);

    // Unload pak from memory.
    iSystem->GetIPak()->LoadPakToMemory("Engine/ShadersBin.pak", AZ::IO::IArchive::eInMemoryPakLocale_Unload);

#ifndef _RELEASE
    iSystem->GetIPak()->LoadPakToMemory("Engine/Shaders.pak", AZ::IO::IArchive::eInMemoryPakLocale_Unload);
#endif

    m_Bin.m_bBinaryShadersLoaded = true;

    return SShaderBin::s_nMaxFXBinCache > 0;
}
