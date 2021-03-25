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

// Description : implementation of the Shaders parser part of shaders manager.


#include "RenderDll_precompiled.h"
#include "I3DEngine.h"
#include "CryHeaders.h"
#include "CryPath.h"
#include <AzFramework/Archive/IArchive.h>

#if defined(WIN32) || defined(WIN64)
#include <direct.h>
#include <io.h>
#elif defined(LINUX)
#endif


//============================================================
// Compile functions
//============================================================

SShaderGenBit* CShaderMan::mfCompileShaderGenProperty(char* scr)
{
    char* name;
    long cmd;
    char* params;
    char* data;

    SShaderGenBit* shgm = new SShaderGenBit;

    enum
    {
        eName = 1, eProperty, eDescription, eMask, eHidden, eRuntime, ePrecache, eDependencySet, eDependencyReset, eDependFlagSet, eDependFlagReset, eAutoPrecache, eLowSpecAutoPrecache
    };
    static STokenDesc commands[] =
    {
        {eName, "Name"},
        {eProperty, "Property"},
        {eDescription, "Description"},
        {eMask, "Mask"},
        {eHidden, "Hidden"},
        {ePrecache, "Precache"},
        {eRuntime, "Runtime"},
        {eAutoPrecache, "AutoPrecache"},
        {eLowSpecAutoPrecache, "LowSpecAutoPrecache"},
        {eDependencySet, "DependencySet"},
        {eDependencyReset, "DependencyReset"},
        {eDependFlagSet, "DependFlagSet"},
        {eDependFlagReset, "DependFlagReset"},

        {0, 0}
    };

    shgm->m_PrecacheNames.reserve(45);

    while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
    {
        data = NULL;
        if (name)
        {
            data = name;
        }
        else
        if (params)
        {
            data = params;
        }

        switch (cmd)
        {
        case eName:
            shgm->m_ParamName = data;
            shgm->m_dwToken = CCrc32::Compute(data);
            break;

        case eProperty:
            if (gRenDev->IsEditorMode())
            {
                shgm->m_ParamProp = data;
            }
            break;

        case eDescription:
            if (gRenDev->IsEditorMode())
            {
                shgm->m_ParamDesc = data;
            }
            break;

        case eHidden:
            shgm->m_Flags |= SHGF_HIDDEN;
            break;

        case eRuntime:
            shgm->m_Flags |= SHGF_RUNTIME;
            break;

        case eAutoPrecache:
            shgm->m_Flags |= SHGF_AUTO_PRECACHE;
            break;
        case eLowSpecAutoPrecache:
            shgm->m_Flags |= SHGF_LOWSPEC_AUTO_PRECACHE;
            break;

        case ePrecache:
            shgm->m_PrecacheNames.push_back(CParserBin::GetCRC32(data));
            shgm->m_Flags |= SHGF_PRECACHE;
            break;

        case eDependFlagSet:
            shgm->m_DependSets.push_back(data);
            break;

        case eDependFlagReset:
            shgm->m_DependResets.push_back(data);
            break;

        case eMask:
            if (data && data[0])
            {
                if (data[0] == '0' && (data[1] == 'x' || data[1] == 'X'))
                {
                    shgm->m_Mask = shGetHex64(&data[2]);
                }
                else
                {
                    shgm->m_Mask = shGetInt(data);
                }
            }
            break;

        case eDependencySet:
            if (data && data[0])
            {
                if (!azstricmp(data, "$LM_Diffuse"))
                {
                    shgm->m_nDependencySet |= SHGD_LM_DIFFUSE;
                }
                else
                if (!azstricmp(data, "$TEX_Detail"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_DETAIL;
                }
                else
                if (!azstricmp(data, "$TEX_Normals"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_NORMALS;
                }
                else
                if (!azstricmp(data, "$TEX_Height"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_HEIGHT;
                }
                else
                if (!azstricmp(data, "$TEX_SecondSmoothness"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_SECOND_SMOOTHNESS;
                }
                else
                if (!azstricmp(data, "$TEX_Specular"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_SPECULAR;
                }
                else
                if (!azstricmp(data, "$TEX_EnvCM"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_ENVCM;
                }
                else
                if (!azstricmp(data, "$TEX_Subsurface"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_SUBSURFACE;
                }
                else
                if (!azstricmp(data, "$HW_BilinearFP16"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_BILINEARFP16;
                }
                else
                if (!azstricmp(data, "$HW_SeparateFP16"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_SEPARATEFP16;
                }
                else
                if (!azstricmp(data, "$TEX_Custom"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_CUSTOM;
                }
                else
                if (!azstricmp(data, "$TEX_CustomSecondary"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_CUSTOM_SECONDARY;
                }
                else
                if (!azstricmp(data, "$TEX_Occ"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_OCC;
                }
                else
                if (!azstricmp(data, "$HW_WaterTessellation"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_WATER_TESSELLATION;
                }
                else
                if (!azstricmp(data, "$HW_SilhouettePom"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_SILHOUETTE_POM;
                }
                else
                if (!azstricmp(data, "$HW_SpecularAntialiasing"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_SAA;
                }
                else
                if (!azstricmp(data, "$UserEnabled"))
                {
                    shgm->m_nDependencySet |= SHGD_USER_ENABLED;
                }
                else
                if (!azstricmp(data, "$HW_ORBIS"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_ORBIS;
                }
                else
                if (!azstricmp(data, "$HW_DX11"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_DX11;
                }
                else
                if (!azstricmp(data, "$HW_GL4"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_GL4;
                }
                else
                if (!azstricmp(data, "$HW_GLES3"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_GLES3;
                }
                else
                if (!azstricmp(data, "$TEX_Emittance"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_EMITTANCE;
                }

                // backwards compatible names
                else if (!azstricmp(data, "$HW_METAL"))
                {
                    shgm->m_nDependencySet |= SHGD_HW_METAL;
                }
                else if (!azstricmp(data, "$TEX_Bump"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_NORMALS;
                }
                else if (!azstricmp(data, "$TEX_BumpHeight"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_HEIGHT;
                }
                else if (!azstricmp(data, "$TEX_Translucency"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_SECOND_SMOOTHNESS;
                }
                else if (!azstricmp(data, "$TEX_BumpDif"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_SECOND_SMOOTHNESS;
                }
                else if (!azstricmp(data, "$TEX_Gloss"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_SPECULAR;
                }

                else
                {
                    assert(0);
                }
            }
            break;

        case eDependencyReset:
            if (data && data[0])
            {
                if (!azstricmp(data, "$LM_Diffuse"))
                {
                    shgm->m_nDependencyReset |= SHGD_LM_DIFFUSE;
                }
                else
                if (!azstricmp(data, "$TEX_Detail"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_DETAIL;
                }
                else
                if (!azstricmp(data, "$TEX_Normals"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_NORMALS;
                }
                else
                if (!azstricmp(data, "$TEX_Height"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_HEIGHT;
                }
                else
                if (!azstricmp(data, "$TEX_SecondSmoothness"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_SECOND_SMOOTHNESS;
                }
                else
                if (!azstricmp(data, "$TEX_Specular"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_SPECULAR;
                }
                else
                if (!azstricmp(data, "$TEX_EnvCM"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_ENVCM;
                }
                else
                if (!azstricmp(data, "$TEX_Subsurface"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_SUBSURFACE;
                }
                else
                if (!azstricmp(data, "$HW_BilinearFP16"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_BILINEARFP16;
                }
                else
                if (!azstricmp(data, "$HW_SeparateFP16"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_SEPARATEFP16;
                }
                else
                if (!azstricmp(data, "$TEX_Custom"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_CUSTOM;
                }
                else
                if (!azstricmp(data, "$TEX_CustomSecondary"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_CUSTOM_SECONDARY;
                }
                else
                if (!azstricmp(data, "$TEX_Occ"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_OCC;
                }
                else
                if (!azstricmp(data, "$TEX_Decal"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_DECAL;
                }
                else
                if (!azstricmp(data, "$HW_WaterTessellation"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_WATER_TESSELLATION;
                }
                else
                if (!azstricmp(data, "$HW_SilhouettePom"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_SILHOUETTE_POM;
                }
                else
                if (!azstricmp(data, "$HW_SpecularAntialiasing"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_SAA;
                }
                else
                if (!azstricmp(data, "$UserEnabled"))
                {
                    shgm->m_nDependencyReset |= SHGD_USER_ENABLED;
                }
                else
                if (!azstricmp(data, "$HW_DX11"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_DX11;
                }
                else
                if (!azstricmp(data, "$HW_GL4"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_GL4;
                }
                else
                if (!azstricmp(data, "$HW_GLES3"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_GLES3;
                }
                else
                if (!azstricmp(data, "$HW_METAL"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_METAL;
                }
                else
                if (!azstricmp(data, "$HW_ORBIS"))
                {
                    shgm->m_nDependencyReset |= SHGD_HW_ORBIS;
                }
                else
                if (!azstricmp(data, "$TEX_Emittance"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_EMITTANCE;
                }

                // backwards compatible names
                else
                if (!azstricmp(data, "$TEX_Bump"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_NORMALS;
                }
                else
                if (!azstricmp(data, "$TEX_BumpHeight"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_HEIGHT;
                }
                else
                if (!azstricmp(data, "$TEX_Translucency"))
                {
                    shgm->m_nDependencyReset |= SHGD_TEX_SECOND_SMOOTHNESS;
                }
                else
                if (!azstricmp(data, "$TEX_BumpDif"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_SECOND_SMOOTHNESS;
                }
                else
                if (!azstricmp(data, "$TEX_Gloss"))
                {
                    shgm->m_nDependencySet |= SHGD_TEX_SPECULAR;
                }

                else
                {
                    assert(0);
                }
            }
            break;
        }
    }
    shgm->m_NameLength = strlen(shgm->m_ParamName.c_str());

    return shgm;
}

bool CShaderMan::mfCompileShaderGen(SShaderGen* shg, char* scr)
{
    char* name;
    long cmd;
    char* params;
    char* data;

    SShaderGenBit* shgm;

    enum
    {
        eProperty = 1, eVersion, eUsesCommonGlobalFlags
    };
    static STokenDesc commands[] =
    {
        {eProperty, "Property"},
        {eVersion, "Version"},
        {eUsesCommonGlobalFlags, "UsesCommonGlobalFlags"},
        {0, 0}
    };

    while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
    {
        data = NULL;
        if (name)
        {
            data = name;
        }
        else
        if (params)
        {
            data = params;
        }

        switch (cmd)
        {
        case eProperty:
            shgm = mfCompileShaderGenProperty(params);
            if (shgm)
            {
                shg->m_BitMask.AddElem(shgm);
            }
            break;

        case eUsesCommonGlobalFlags:
            break;

        case eVersion:
            break;
        }
    }

    return shg->m_BitMask.Num() != 0;
}

void CShaderMan::mfCompileLevelsList(std::vector<string>& List, char* scr)
{
    char* name;
    long cmd;
    char* params;
    char* data;

    enum
    {
        eName = 1, eVersion
    };
    static STokenDesc commands[] =
    {
        {eName, "Name"},
        {0, 0}
    };

    while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
    {
        data = NULL;
        if (name)
        {
            data = name;
        }
        else
        if (params)
        {
            data = params;
        }

        switch (cmd)
        {
        case eName:
            if (data && data[0])
            {
                List.push_back(string("Levels/") + string(data) + string("/"));
            }
            break;
        }
    }
}

bool CShaderMan::mfCompileShaderLevelPolicies(SShaderLevelPolicies* pPL, char* scr)
{
    char* name;
    long cmd;
    char* params;
    char* data;

    enum
    {
        eGlobalList = 1, ePerLevelList, eVersion
    };
    static STokenDesc commands[] =
    {
        {eGlobalList, "GlobalList"},
        {ePerLevelList, "PerLevelList"},
        {eVersion, "Version"},
        {0, 0}
    };

    while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
    {
        data = NULL;
        if (name)
        {
            data = name;
        }
        else
        if (params)
        {
            data = params;
        }

        switch (cmd)
        {
        case eGlobalList:
            mfCompileLevelsList(pPL->m_WhiteGlobalList, params);
            break;
        case ePerLevelList:
            mfCompileLevelsList(pPL->m_WhitePerLevelList, params);
            break;

        case eVersion:
            break;
        }
    }

    return pPL->m_WhiteGlobalList.size() != 0;
}

string CShaderMan::mfGetShaderBitNamesFromMaskGen(const char* szFileName, uint64 nMaskGen)
{
    // debug/helper function: returns ShaderGen bit names string based on nMaskGen

    if (!nMaskGen)
    {
        return "NO_FLAGS";
    }

    string pszShaderName = PathUtil::GetFileName(szFileName);
    pszShaderName.MakeUpper();

    uint64 nMaskGenRemaped = 0;
    ShaderMapNameFlagsItor pShaderRmp = m_pShadersGlobalFlags.find(pszShaderName.c_str());
    if (pShaderRmp == m_pShadersGlobalFlags.end() || !pShaderRmp->second)
    {
        return "NO_FLAGS";
    }

    string pszShaderBitNames;

    // get shader bit names
    MapNameFlagsItor pIter = m_pShaderCommonGlobalFlag.begin();
    MapNameFlagsItor pEnd = m_pShaderCommonGlobalFlag.end();
    for (; pIter != pEnd; ++pIter)
    {
        if (nMaskGen & pIter->second)
        {
            pszShaderBitNames += pIter->first.c_str();
        }
    }

    return pszShaderBitNames;
}

void CShaderMan::mfRemapShaderGenInfoBits(const char* szName, SShaderGen* pShGen)
{
    // No data to proceed
    if (!pShGen || pShGen->m_BitMask.empty())
    {
        return;
    }

    // Check if shader uses common flags at all
    string pszShaderName = PathUtil::GetFileName(szName);
    pszShaderName.MakeUpper();
    if ((int32) m_pShadersRemapList.find(pszShaderName.c_str()) < 0)
    {
        return;
    }

    MapNameFlags* pOldShaderFlags = 0;
    if (m_pShadersGlobalFlags.find(pszShaderName.c_str()) == m_pShadersGlobalFlags.end())
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_shaderLoadMutex);

        pOldShaderFlags = new MapNameFlags;
        m_pShadersGlobalFlags.insert(ShaderMapNameFlagsItor::value_type(pszShaderName.c_str(),    pOldShaderFlags));
    }

    uint32 nBitCount = pShGen->m_BitMask.size();
    for (uint32 b = 0; b < nBitCount; ++b)
    {
        SShaderGenBit* pGenBit = pShGen->m_BitMask[b];
        if (!pGenBit)
        {
            continue;
        }

        // Store old shader flags
        if (pOldShaderFlags)
        {
            pOldShaderFlags->insert(MapNameFlagsItor::value_type(pGenBit->m_ParamName.c_str(), pGenBit->m_Mask));
        }

        // lookup for name - and update mask value
        MapNameFlagsItor pRemaped = m_pShaderCommonGlobalFlag.find(pGenBit->m_ParamName.c_str());
        if (pRemaped != m_pShaderCommonGlobalFlag.end())
        {
            pGenBit->m_Mask = pRemaped->second;
        }

        int test = 2;
    }
}

bool CShaderMan::mfUsesGlobalFlags(const char* szShaderName)
{
    // Check if shader uses common flags at all
    string pszName = PathUtil::GetFileName(szShaderName);

    AZStd::unique_lock<AZStd::mutex> lock(m_shaderLoadMutex);

    string pszShaderNameUC = pszName;
    pszShaderNameUC.MakeUpper();
    if ((int32) m_pShadersRemapList.find(pszShaderNameUC.c_str()) < 0)
    {
        return false;
    }

    return true;
}

uint64 CShaderMan::mfGetShaderGlobalMaskGenFromString(const char* szShaderGen)
{
    assert(szShaderGen);
    if (!szShaderGen || szShaderGen[0] == '\0')
    {
        return 0;
    }

    AZStd::unique_lock<AZStd::mutex> lock(m_shaderLoadMutex);

    uint64 nMaskGen = 0;
    MapNameFlagsItor pEnd = m_pShaderCommonGlobalFlag.end();

    char* pCurrOffset = (char*)szShaderGen;
    while (pCurrOffset)
    {
        char* pBeginOffset = (char*)strstr(pCurrOffset, "%");
        assert(pBeginOffset);
        PREFAST_ASSUME(pBeginOffset);
        pCurrOffset = (char*)strstr(pBeginOffset + 1, "%");

        char pCurrFlag[64] = "\0";
        int nLen = pCurrOffset ? pCurrOffset - pBeginOffset : strlen(pBeginOffset);
        memcpy(pCurrFlag, pBeginOffset, nLen);

        MapNameFlagsItor pIter = m_pShaderCommonGlobalFlag.find(pCurrFlag);
        if (pIter != pEnd)
        {
            nMaskGen |= pIter->second;
        }
    }

    return nMaskGen;
}

AZStd::string CShaderMan::mfGetShaderBitNamesFromGlobalMaskGen(uint64 nMaskGen)
{
    if (!nMaskGen)
    {
        return "\0";
    }

    AZStd::string shaderBitNames = "\0";
    MapNameFlagsItor pIter = m_pShaderCommonGlobalFlag.begin();
    MapNameFlagsItor pEnd = m_pShaderCommonGlobalFlag.end();
    for (; pIter != pEnd; ++pIter)
    {
        if (nMaskGen & pIter->second)
        {
            shaderBitNames += pIter->first.c_str();
        }
    }

    return shaderBitNames;
}

uint64 CShaderMan::mfGetRemapedShaderMaskGen(const char* szName, uint64 nMaskGen, bool bFixup)
{
    if (!nMaskGen)
    {
        return 0;
    }

    AZStd::unique_lock<AZStd::mutex> lock(m_shaderLoadMutex);

    uint64 nMaskGenRemaped = 0;
    string szShaderName = PathUtil::GetFileName(szName);// some shaders might be using concatenated names (eg: terrain.layer), get only first name
    szShaderName.MakeUpper();
    ShaderMapNameFlagsItor pShaderRmp = m_pShadersGlobalFlags.find(szShaderName.c_str());

    if (pShaderRmp == m_pShadersGlobalFlags.end() || !pShaderRmp->second)
    {
        return nMaskGen;
    }

    if (bFixup)
    {
        // if some flag was removed, disable in mask
        nMaskGen = nMaskGen & m_nSGFlagsFix;
        return nMaskGen;
    }

    // old bitmask - remap to common shared bitmasks
    for (uint64 j = 0; j < 64; ++j)
    {
        uint64 nMask = (((uint64)1) << j);
        if (nMaskGen & nMask)
        {
            MapNameFlagsItor pIter = pShaderRmp->second->begin();
            MapNameFlagsItor pEnd = pShaderRmp->second->end();
            for (; pIter != pEnd; ++pIter)
            {
                // found match - enable bit for remapped mask
                if ((pIter->second) & nMask)
                {
                    const char* pFlagName = pIter->first.c_str();
                    MapNameFlagsItor pMatchIter = m_pShaderCommonGlobalFlag.find(pIter->first);
                    if (pMatchIter != m_pShaderCommonGlobalFlag.end())
                    {
                        nMaskGenRemaped |= pMatchIter->second;
                    }

                    break;
                }
            }
        }
    }

    return nMaskGenRemaped;
}


SShaderGen* CShaderMan::mfCreateShaderGenInfo(const char* szName, bool bRuntime)
{
    CCryNameTSCRC s = szName;
    if (!bRuntime)
    {
        ShaderExtItor it = m_ShaderExts.find(s);
        if (it != m_ShaderExts.end())
        {
            return it->second;
        }
    }
    SShaderGen* pShGen = NULL;
    char szN[256];
    cry_strcpy(szN, "Shaders/");
    cry_strcat(szN, szName);
    cry_strcat(szN, ".ext");
    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(szN, "rb", AZ::IO::IArchive::FOPEN_HINT_QUIET);
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        pShGen = new SShaderGen;
        AZ::u64 fileSize = 0;
        AZ::IO::FileIOBase::GetInstance()->Size(fileHandle, fileSize);
        char* buf = new char [fileSize + 1];
        if (buf)
        {
            buf[fileSize] = 0;
            gEnv->pCryPak->FRead(buf, fileSize, fileHandle);
            mfCompileShaderGen(pShGen, buf);
            mfRemapShaderGenInfoBits(szName, pShGen);
            delete [] buf;
        }
        gEnv->pCryPak->FClose(fileHandle);
    }
    if (pShGen && !bRuntime)
    {
        pShGen->m_BitMask.Shrink();
        m_ShaderExts.insert(std::pair<CCryNameTSCRC, SShaderGen*>(s, pShGen));
    }

    return pShGen;
}
