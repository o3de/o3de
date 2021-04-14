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

#ifndef __CSHADER_H__
#define __CSHADER_H__

#include "CShaderBin.h"
#include "ShaderSerialize.h"
#include "ShaderCache.h"
#include "../ResFileLookupDataMan.h"

#include <unordered_map>
#include <AzCore/std/parallel/mutex.h>

#include <Terrain/Bus/TerrainBus.h>

struct SRenderBuf;
class CRendElementBase;
struct SEmitter;
struct SParticleInfo;
struct SPartMoveStage;
struct SSunFlare;

//===================================================================

#define MAX_ENVLIGHTCUBEMAPS 16
#define ENVLIGHTCUBEMAP_SIZE 16
#define MAX_ENVLIGHTCUBEMAPSCANDIST_UPDATE 16
#define MAX_ENVLIGHTCUBEMAPSCANDIST_THRESHOLD 2

#define MAX_ENVCUBEMAPS 16
#define MAX_ENVCUBEMAPSCANDIST_THRESHOLD 1

#define MAX_ENVTEXTURES 16
#define MAX_ENVTEXSCANDIST 0.1f

//===============================================================================

struct SMacroFX
{
    string m_szMacro;
    uint32 m_nMask;
};

typedef AZStd::unordered_map<string, SMacroFX, stl::hash_string<const char*>, stl::equality_string<const char*> > FXMacro;

typedef FXMacro::iterator FXMacroItor;

//////////////////////////////////////////////////////////////////////////
// Helper class for shader parser, holds temporary strings vector etc...
//////////////////////////////////////////////////////////////////////////
struct CShaderParserHelper
{
    CShaderParserHelper()
    {
    }
    ~CShaderParserHelper()
    {
    }

    char* GetTempStringArray(int nIndex, int nLen)
    {
        m_tempString.reserve(nLen + 1);
        return (char*)&(m_tempStringArray[nIndex])[0];
    }

    std::vector<char> m_tempStringArray[32];
    std::vector<char> m_tempString;
};
extern CShaderParserHelper* g_pShaderParserHelper;

enum EShaderFlagType
{
    eSFT_Global = 0,
    eSFT_Runtime,
    eSFT_MDV,
    eSFT_LT,
};

enum EShaderFilterOperation
{
    eSFO_Expand = 0, // expand all permutations of the mask
    eSFO_And,       // and against the mask
    eSFO_Eq,        // set the mask
};

// includes or excludes
struct CShaderListFilter
{
    CShaderListFilter()
        : m_bInclude(true)
    {}

    bool m_bInclude;
    string m_ShaderName;

    struct Predicate
    {
        Predicate()
            : m_Negated(false)
            , m_Flags(eSFT_Global)
            , m_Op(eSFO_And)
            , m_Mask(0)
        {}

        bool m_Negated;
        EShaderFlagType m_Flags;
        EShaderFilterOperation m_Op;
        uint64 m_Mask;
    };
    std::vector<Predicate> m_Predicates;
};

//==============================================================================

#define PD_INDEXED 1
#define PD_MERGED  4

//==============================================================================
// [Shader System] - the following structures represent the raw data parsed from 
// the shaders.  It is in a way part of a processor semi reflection mechanism.
// To Do:   inherit and share usage
//------------------------------------------------------------------------------
struct SParamDB
{
    const char* szName;
    const char* szAliasName;
    ECGParam eParamType;
    uint32 nFlags;
    void (* ParserFunc)(const char* szScr, const char* szAnnotations, std::vector<STexSamplerFX>* pSamplers, SCGParam* vpp, int nComp, CShader* ef);
    SParamDB()
        : szName(nullptr)
        , szAliasName(nullptr)
        , eParamType(ECGP_Unknown)
        , nFlags(0)
        , ParserFunc(nullptr)
    {
    }
    SParamDB(const char* inName, ECGParam ePrmType, uint32 inFlags)
    {
        szName = inName;
        szAliasName = NULL;
        nFlags = inFlags;
        ParserFunc = NULL;
        eParamType = ePrmType;
    }
    SParamDB(const char* inName, ECGParam ePrmType, uint32 inFlags, void (*InParserFunc)(const char* szScr, const char* szAnnotations, std::vector<STexSamplerFX>* pSamplers, SCGParam * vpp, int nComp, CShader * ef))
    {
        szName = inName;
        szAliasName = NULL;
        nFlags = inFlags;
        ParserFunc = InParserFunc;
        eParamType = ePrmType;
    }
};

struct SSamplerDB
{
    const char* szName;
    ECGSampler eSamplerType;
    uint32 nFlags;
    void (* ParserFunc)(const char* szScr, const char* szAnnotations, std::vector<SFXSampler>* pSamplers, SCGSampler* vpp, CShader* ef);
    SSamplerDB()
    {
        szName = NULL;
        nFlags = 0;
        ParserFunc = NULL;
        eSamplerType = ECGS_Unknown;
    }
    SSamplerDB(const char* inName, ECGSampler ePrmType, uint32 inFlags)
    {
        szName = inName;
        nFlags = inFlags;
        ParserFunc = NULL;
        eSamplerType = ePrmType;
    }
    SSamplerDB(const char* inName, ECGSampler ePrmType, uint32 inFlags, void (*InParserFunc)(const char* szScr, const char* szAnnotations, std::vector<SFXSampler>* pSamplers, SCGSampler * vpp, CShader * ef))
    {
        szName = inName;
        nFlags = inFlags;
        ParserFunc = InParserFunc;
        eSamplerType = ePrmType;
    }
};

//------------------------------------------------------------------------------
// [Shaders System]
// szName- a texture functional name which is only used here - possibly obsolete.
// The texture type is not a type but a slot mapping - should be changed
// nFlags- does not seem to be in use.  Should be tested if used / removed if possible.
// ParserFunc - is not set anywhere and should be removed.
//------------------------------------------------------------------------------
struct STextureDB
{
    const char* szName;
    ECGTexture eTextureType;
    uint32 nFlags;
    void (* ParserFunc)(const char* szScr, const char* szAnnotations, std::vector<SFXTexture>* pSamplers, SCGTexture* vpp, CShader* ef);
    STextureDB()
    {
        szName = NULL;
        nFlags = 0;
        ParserFunc = NULL;
        eTextureType = ECGT_Unknown;
    }
    STextureDB(const char* inName, ECGTexture ePrmType, uint32 inFlags)
    {
        szName = inName;
        nFlags = inFlags;
        ParserFunc = NULL;
        eTextureType = ePrmType;
    }
    STextureDB(const char* inName, ECGTexture ePrmType, uint32 inFlags, void (*InParserFunc)(const char* szScr, const char* szAnnotations, std::vector<SFXTexture>* pSamplers, SCGTexture * vpp, CShader * ef))
    {
        szName = inName;
        nFlags = inFlags;
        ParserFunc = InParserFunc;
        eTextureType = ePrmType;
    }
};


enum EShaderCacheMode
{
    eSC_Normal = 0,
    eSC_BuildGlobal = 2,
    eSC_BuildGlobalList = 3,
    eSC_Preactivate = 4,
};

enum EShaderLanguage
{
    eSL_Unknown,
    eSL_Orbis,
    eSL_D3D11,
    eSL_GL4_1,
    eSL_GL4_4,
    eSL_GLES3_0,
    eSL_GLES3_1,
    eSL_METAL,
    eSL_Jasper,

    eSL_MAX
};

EShaderLanguage GetShaderLanguage();

const char *GetShaderLanguageName();

const char *GetShaderLanguageResourceName();

AZStd::string GetShaderListFilename();

//////////////////////////////////////////////////////////////////////////
class CShaderMan
    : public ISystemEventListener
#if defined(SHADERS_SERIALIZING)
    , public CShaderSerialize
#endif
    , public Terrain::TerrainShaderRequestBus::Handler
    , public AZ::MaterialNotificationEventBus::Handler
{
    friend class CShader;
    friend class CParserBin;

    //////////////////////////////////////////////////////////////////////////
    // ISystemEventListener interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    //////////////////////////////////////////////////////////////////////////

    void OnShaderLoaded(IShader* shader) override;

private:
    CTexAnim* mfReadTexSequence(const char *name, int Flags, bool bFindOnly);
    int mfReadTexSequence(STexSamplerRT* smp, const char* name, int Flags, bool bFindOnly);

    CShader* mfNewShader(const char* szName);

    void mfCompileLevelsList(std::vector<string>& List, char* scr);
    bool mfCompileShaderLevelPolicies(SShaderLevelPolicies* pPL, char* scr);
    bool mfCompileShaderGen(SShaderGen* shg, char* scr);
    SShaderGenBit* mfCompileShaderGenProperty(char* scr);

    void mfSetResourceTexState(SEfResTexture* Tex);
    CTexture* mfTryToLoadTexture(const char* nameTex, STexSamplerRT* smp, int Flags, bool bFindOnly);
    CTexture* mfFindResourceTexture(const char* nameTex, const char* path, int Flags, SEfResTexture* Tex);
    CTexture* mfLoadResourceTexture(const char* nameTex, const char* path, int Flags, SEfResTexture* Tex);


    bool mfLoadResourceTexture(ResourceSlotIndex Id, SInputShaderResources& RS, uint32 CustomFlags, bool bReplaceMeOnFail = false);
    bool mfLoadResourceTexture(ResourceSlotIndex Id, CShaderResources& RS, uint32 CustomFlags, bool bReplaceMeOnFail = false);

    void mfLoadDefaultTexture(ResourceSlotIndex Id, CShaderResources& RS, EEfResTextures Alias);

    void mfCheckShaderResTextures(TArray<SShaderPass>& Dst, CShader* ef, CShaderResources* Res);
    void mfCheckShaderResTexturesHW(TArray<SShaderPass>& Dst, CShader* ef, CShaderResources* Res);
    CTexture* mfCheckTemplateTexName(const char* mapname, ETEX_Type eTT);

    CShader* mfCompile(CShader* ef, char* scr);

    bool mfUpdateMergeStatus(SShaderTechnique* hs, std::vector<SCGParam>* p);
    void mfRefreshResources(CShaderResources* Res);

    bool mfReloadShaderFile(const char* szName, int nFlags);
#if !defined(CONSOLE) && !defined(NULL_RENDERER)
    bool CheckAllFilesAreWritable(const char* szDir) const;
#endif

    AZStd::mutex m_shaderLoadMutex;

public:
    char* m_pCurScript;
    CShaderManBin m_Bin;
    CResFileLookupDataMan m_ResLookupDataMan[2];  // CACHE_READONLY, CACHE_USER

    const char* mfTemplateTexIdToName(int Id);
    SShaderGenComb* mfGetShaderGenInfo(const char* nmFX);

    bool mfReloadShaderIncludes(const char* szPath, int nFlags);
    bool mfReloadAllShaders(int nFlags, uint32 nFlagsHW);
    bool mfReloadFile(const char* szPath, const char* szName, int nFlags);

    void ParseShaderProfiles();
    void ParseShaderProfile(char* scr, SShaderProfile* pr);

    EEfResTextures mfCheckTextureSlotName(const char* mapname);
    SParamDB* mfGetShaderParamDB(const char* szSemantic);
    const char* mfGetShaderParamName(ECGParam ePR);

    bool mfParseParamComp(int comp, SCGParam* pCurParam, const char* szSemantic, char* params, const char* szAnnotations, SShaderFXParams& FXParams, CShader* ef, uint32 nParamFlags, EHWShaderClass eSHClass, bool bExpressionOperand);
    bool mfParseCGParam(char* scr, const char* szAnnotations, SShaderFXParams& FXParams, CShader* ef, std::vector<SCGParam>* pParams, int nComps, uint32 nParamFlags, EHWShaderClass eSHClass, bool bExpressionOperand);
    bool mfParseFXParameter(SShaderFXParams& FXParams, SFXParam* pr, const char* ParamName, CShader* ef, bool bInstParam, int nParams, std::vector<SCGParam>* pParams, EHWShaderClass eSHClass, bool bExpressionOperand);

    bool mfParseFXTexture(SShaderFXParams& FXParams, SFXTexture* pr, const char* ParamName, CShader* ef, int nParams, std::vector<SCGTexture>* pParams, EHWShaderClass eSHClass);
    bool mfParseFXSampler(SShaderFXParams& FXParams, SFXSampler* pr, const char* ParamName, CShader* ef, int nParams, std::vector<SCGSampler>* pParams, EHWShaderClass eSHClass);

    void mfCheckObjectDependParams(std::vector<SCGParam>& PNoObj, std::vector<SCGParam>& PObj, EHWShaderClass eSH, CShader* pFXShader);

    void mfBeginFrame();

    void mfGetShaderListPath(stack_string& nameOut, int nType);

public:
    bool m_bInitialized;
    bool m_bLoadedSystem;

    AZStd::string m_ShadersPath;
    AZStd::string m_ShadersCache;
    AZStd::string m_ShadersFilter;
    AZStd::string m_ShadersMergeCachePath;
    AZStd::string m_szCachePath;

    int m_nFrameForceReload;

    char m_HWPath[128];

    CShader* m_pCurShader;
    static SResourceContainer* s_pContainer; // List/Map of objects for shaders resource class

    std::vector<string> m_ShaderNames;

    static CCryNameTSCRC s_cNameHEAD;

    static CShader* s_DefaultShader;
    static CShader* s_shPostEffects;    // engine specific post process effects
    static CShader* s_shPostDepthOfField; // depth of field
    static CShader* s_shPostMotionBlur;
    static CShader* s_shPostSunShafts;

    // Deferred rendering passes
    static CShader* s_shDeferredShading;
    static CShader* s_ShaderDeferredCaustics;
    static CShader* s_ShaderDeferredRain;
    static CShader* s_ShaderDeferredSnow;

#ifndef NULL_RENDERER
    static CShader* s_ShaderFPEmu;
    static CShader* s_ShaderUI;
    static CShader* s_ShaderFallback;
    static CShader* s_ShaderStars;
    static CShader* s_ShaderShadowBlur;
    static CShader* s_ShaderShadowMaskGen;
#if defined(FEATURE_SVO_GI)
    static CShader* s_ShaderSVOGI;
#endif
    static CShader* s_shHDRPostProcess;
    static CShader* s_shPostEffectsGame;  // game specific post process effects
    static CShader* s_shPostAA;
    static CShader* s_ShaderDebug;
    static CShader* s_ShaderLensOptics;
    static CShader* s_ShaderSoftOcclusionQuery;
    static CShader* s_ShaderLightStyles;
    static CShader* s_ShaderCommon;
    static CShader* s_ShaderOcclTest;
    static CShader* s_ShaderDXTCompress;
    static CShader* s_ShaderStereo;
    static CShader* s_ShaderFur;
    static CShader* s_ShaderVideo;
#else
    static SShaderItem s_DefaultShaderItem;
#endif

    AZStd::unordered_set<IShader*> m_systemShaders;

    const SInputShaderResources* m_pCurInputResources;
    SShaderGen* m_pGlobalExt;
    SShaderGen* m_staticExt;    // Shader gen info for static flags (Statics.ext)
    uint64 m_staticFlags;       // Enabled global flags used for generating the shaders.
    SShaderLevelPolicies* m_pLevelsPolicies;

    Vec4 m_TempVecs[16];
    Vec4 m_RTRect;
    std::vector<SShaderGenComb> m_SGC;

    int m_nCombinationsProcess;
    int m_nCombinationsProcessOverall;
    int m_nCombinationsCompiled;
    int m_nCombinationsEmpty;

    EShaderCacheMode m_eCacheMode;

    bool m_bActivatePhase;
    const char* m_szShaderPrecache;

    FXShaderCacheCombinations m_ShaderCacheCombinations[2];
    FXShaderCacheCombinations m_ShaderCacheExportCombinations;
    AZ::IO::HandleType m_FPCacheCombinations[2];

    typedef std::vector<CCryNameTSCRC> ShaderCacheMissesVec;
    ShaderCacheMissesVec m_ShaderCacheMisses;
    string m_ShaderCacheMissPath;
    ShaderCacheMissCallback m_ShaderCacheMissCallback;

    SShaderCacheStatistics m_ShaderCacheStats;

    uint32 m_nFrameLastSubmitted;
    uint32 m_nFrameSubmit;
    SShaderProfile m_ShaderProfiles[eST_Max];
    SShaderProfile m_ShaderFixedProfiles[eSQ_Max];

    int m_bActivated;

    CShaderParserHelper m_shaderParserHelper;

    bool m_bReload;


    // Shared common global flags data

    // Map used for storing automatically-generated flags and mapping old shader names masks to generated ones
    //  map< shader flag names, mask >
    typedef std::map< string, uint64 > MapNameFlags;
    typedef MapNameFlags::iterator MapNameFlagsItor;
    MapNameFlags m_pShaderCommonGlobalFlag;

    MapNameFlags m_pSCGFlagLegacyFix;
    uint64 m_nSGFlagsFix;

    // Map stored for convenience mapping betweens old flags and new ones
    //  map < shader name , map< shader flag names, mask > >
    typedef std::map< string, MapNameFlags* > ShaderMapNameFlags;
    typedef ShaderMapNameFlags::iterator ShaderMapNameFlagsItor;
    ShaderMapNameFlags m_pShadersGlobalFlags;

    typedef std::map<CCryNameTSCRC, SShaderGen*> ShaderExt;
    typedef ShaderExt::iterator ShaderExtItor;
    ShaderExt m_ShaderExts;
    PerFrameParameters m_PF;

    // Concatenated list of shader names using automatic masks generation
    string m_pShadersRemapList;

    // Helper functors for cleaning up

    struct SShaderMapNameFlagsContainerDelete
    {
        void operator()(ShaderMapNameFlags::value_type& pObj)
        {
            SAFE_DELETE(pObj.second);
        }
    };

public:
    CShaderMan()
    {
        m_bInitialized = false;
        m_bLoadedSystem = false;
        s_DefaultShader = NULL;
        m_pGlobalExt = NULL;
        m_staticExt = nullptr;
        m_staticFlags = 0;
        g_pShaderParserHelper = &m_shaderParserHelper;
        m_nCombinationsProcess = -1;
        m_nCombinationsProcessOverall = -1;
        m_nCombinationsCompiled = -1;
        m_nCombinationsEmpty = -1;
        m_szShaderPrecache = NULL;
        memset(m_TempVecs, 0, sizeof(Vec4) * 16);
        memset(&m_RTRect, 0, sizeof(Vec4));
        m_eCacheMode = eSC_Normal;
        m_nFrameSubmit = 1;

        Terrain::TerrainShaderRequestBus::Handler::BusConnect();
        AZ::MaterialNotificationEventBus::Handler::BusConnect();
    }

    void ShutDown();
    void mfReleaseShaders();

    SShaderGen* mfCreateShaderGenInfo(const char* szName, bool bRuntime);
    void mfRemapShaderGenInfoBits(const char* szName, SShaderGen* pShGen);

    uint64 mfGetRemapedShaderMaskGen(const char* szName, uint64 nMaskGen = 0, bool bFixup = 0);
    string mfGetShaderBitNamesFromMaskGen(const char* szName, uint64 nMaskGen);

    bool mfUsesGlobalFlags(const char* szShaderName);
    AZStd::string mfGetShaderBitNamesFromGlobalMaskGen(uint64 nMaskGen);
    uint64 mfGetShaderGlobalMaskGenFromString(const char* szShaderGen);

    void mfInitGlobal(void);
    void mfInitLevelPolicies(void);
    void mfInitLookups(void);

    void InitStaticFlags();
    void AddStaticFlag(EHWSSTFlag flag);
    void RemoveStaticFlag(EHWSSTFlag flag);
    bool HasStaticFlag(EHWSSTFlag flag);

    void mfPreloadShaderExts(void);
    void mfInitCommonGlobalFlags(void);
    void mfInitCommonGlobalFlagsLegacyFix(void);
    bool mfRemapCommonGlobalFlagsWithLegacy(void);
    void mfCreateCommonGlobalFlags(const char* szName);
    void mfSaveCommonGlobalFlagsToDisk(const char* szName, uint32 nMaskCount);

    void mfInit(void);
    void mfPostInit(void);
    void mfSortResources();
    CShaderResources* mfCreateShaderResources(const SInputShaderResources* Res, bool bShare);
    bool mfRefreshResourceConstants(CShaderResources* Res);
    inline bool mfRefreshResourceConstants(SShaderItem& SI) { return mfRefreshResourceConstants((CShaderResources*)SI.m_pShaderResources); }
    bool mfUpdateTechnik (SShaderItem& SI, CCryNameTSCRC& Name);
    SShaderItem mfShaderItemForName (const char* szName, bool bShare, int flags, SInputShaderResources* Res = NULL, uint64 nMaskGen = 0);
    CShader* mfForName (const char* name, int flags, const CShaderResources* Res = NULL, uint64 nMaskGen = 0);

    bool mfRefreshSystemShader(const char* szName, CShader*& pSysShader)
    {
        if (!pSysShader)
        {
            CryComment("Load System Shader (refresh) '%s'...", szName);

            pSysShader = mfForName(szName, EF_SYSTEM);
            if (pSysShader)
            {
                if (pSysShader->m_Flags & EF_NOTFOUND)
                {
                    pSysShader = NULL;
                    CryComment("Load System Shader Failed %s", szName);
                    return false;
                }
                CryComment("ok");
                m_systemShaders.emplace(pSysShader);
                return true;
            }
        }

        return false;
    }

    void RT_ParseShader(CShader* pSH, uint64 nMaskGen, uint32 flags, CShaderResources* Res);
    void RT_SetShaderQuality(EShaderType eST, EShaderQuality eSQ);

    void CreateShaderMaskGenString(const CShader* pSH, stack_string& flagString);
    void CreateShaderExportRequestLine(const CShader* pSH, stack_string& exportString);

    SFXParam* mfGetFXParameter(std::vector<SFXParam>& Params, const char* param);
    SFXSampler* mfGetFXSampler(std::vector<SFXSampler>& Params, const char* param);
    SFXTexture* mfGetFXTexture(std::vector<SFXTexture>& Params, const char* param);
    const char* mfParseFX_Parameter (const string& buf, EParamType eType, const char* szName);
    void    mfParseFX_Annotations_Script (char* buf, CShader * ef, std::vector<SFXStruct>&Structs, bool* bPublic, CCryNameR techStart[2]);
    void    mfParseFX_Annotations (char* buf, CShader * ef, std::vector<SFXStruct>&Structs, bool* bPublic, CCryNameR techStart[2]);
    void    mfParseFXTechnique_Annotations_Script (char* buf, CShader* ef, std::vector<SFXStruct>& Structs, SShaderTechnique* pShTech, bool* bPublic, std::vector<SShaderTechParseParams>& techParams);
    void    mfParseFXTechnique_Annotations (char* buf, CShader* ef, std::vector<SFXStruct>& Structs, SShaderTechnique* pShTech, bool* bPublic, std::vector<SShaderTechParseParams>& techParams);
    void    mfParseFXSampler_Annotations_Script (char* buf, CShader* ef, std::vector<SFXStruct>& Structs, STexSamplerFX* pSamp);
    void    mfParseFXSampler_Annotations (char* buf, CShader* ef, std::vector<SFXStruct>& Structs, STexSamplerFX* pSamp);
    void    mfParseFX_Global (SFXParam & pr, CShader * ef, std::vector<SFXStruct>&Structs, CCryNameR techStart[2]);
    bool    mfParseDummyFX_Global (std::vector<SFXStruct>&Structs, char* annot, CCryNameR techStart[2]);
    const string& mfParseFXTechnique_GenerateShaderScript (std::vector<SFXStruct>& Structs, FXMacro& Macros, std::vector<SFXParam>& Params, std::vector<SFXParam>& AffectedParams, const char* szEntryFunc, CShader* ef, EHWShaderClass eSHClass, const char* szShaderName, uint32& nAffectMask, const char* szType);
    bool    mfParseFXTechnique_MergeParameters (std::vector<SFXStruct>& Structs, std::vector<SFXParam>& Params, std::vector<int>& AffectedFunc, SFXStruct* pMainFunc, CShader* ef, EHWShaderClass eSHClass, const char* szShaderName, std::vector<SFXParam>& NewParams);
    CTexture* mfParseFXTechnique_LoadShaderTexture (STexSamplerRT* smp, const char* szName, SShaderPass* pShPass, CShader* ef, int nIndex, byte ColorOp, byte AlphaOp, byte ColorArg, byte AlphaArg);
    bool    mfParseFXTechnique_LoadShader (const char* szShaderCom, SShaderPass* pShPass, CShader* ef, std::vector<STexSamplerFX>& Samplers, std::vector<SFXStruct>& Structs, std::vector<SFXParam>& Params, FXMacro& Macros, EHWShaderClass eSHClass);
    bool    mfParseFXTechniquePass (char* buf, char* annotations, SShaderTechnique* pShTech, CShader* ef, std::vector<STexSamplerFX>& Samplers, std::vector<SFXStruct>& Structs, std::vector<SFXParam>& Params);
    bool    mfParseFXTechnique_CustomRE (char* buf, const char* name, SShaderTechnique* pShTech, CShader* ef);
    SShaderTechnique* mfParseFXTechnique (char* buf, char* annotations, CShader* ef, std::vector<STexSamplerFX>& Samplers, std::vector<SFXStruct>& Structs, std::vector<SFXParam>& Params, bool* bPublic, std::vector<SShaderTechParseParams>& techParams);
    bool    mfParseFXSampler(char* buf, char* name, char* annotations, CShader* ef, std::vector<STexSamplerFX>& Samplers, std::vector<SFXStruct>& Structs);
    bool    mfParseLightStyle(CLightStyle* ls, char* buf);
    bool    mfParseFXLightStyle(char* buf, int nID, CShader* ef, std::vector<SFXStruct>& Structs);
    CShader* mfParseFX (char* buf, CShader* ef, CShader* efGen, uint64 nMaskGen);
    void    mfPostLoadFX(CShader * efT, std::vector<SShaderTechParseParams>&techParams, CCryNameR techStart[2]);
    bool     mfParseDummyFX(char* buf, std::vector<string>& ShaderNames, const char* szName);
    bool     mfAddFXShaderNames(const char* szName, std::vector<string>* ShaderNames, bool bUpdateCRC);
    bool     mfInitShadersDummy(bool bUpdateCRC);

    uint64   mfGetRTForName(char* buf);
    uint32     mfGetGLForName(char* buf, CShader* ef);

    void mfFillGenMacroses(SShaderGen* shG, TArray<char>& buf, uint64 nMaskGen);
    bool mfModifyGenFlags(CShader* efGen, const CShaderResources* pRes, uint64& nMaskGen, uint64& nMaskGenHW);

    bool mfGatherShadersList(const char* szPath, bool bCheckIncludes, bool bUpdateCRC, std::vector<string>* Names);
    void mfGatherFilesList(const char* szPath, std::vector<CCryNameR>& Names, int nLevel, bool bUseFilter, bool bMaterial = false);
    int  mfInitShadersList(std::vector<string>* ShaderNames);
    void mfSetDefaults(void);
    void mfLoadSystemShader(const char* szName, CShader*& pStorage);
    void mfReleaseSystemShaders ();
    void mfLoadBasicSystemShaders ();
    void mfLoadDefaultSystemShaders ();
    void mfCloseShadersCache(int nID);
    void mfInitShadersCacheMissLog();

    void mfInitShadersCache(byte bForLevel, FXShaderCacheCombinations* Combinations = NULL, const char* pCombinations = NULL, int nType = 0);
    void mfMergeShadersCombinations(FXShaderCacheCombinations* Combinations, int nType);
    void mfInsertNewCombination(SShaderCombIdent& Ident, EHWShaderClass eCL, const char* name, int nID, string* Str = NULL, byte bStore = 1);
    const char* mfGetLevelListName() const;
    void mfExportShaders();

    bool mfReleasePreactivatedShaderData();
    bool mfPreactivateShaders2(const char* szPak, const char* szPath, bool bPersistent, const char* szBindRoot);
    bool mfPreactivate2(CResFileLookupDataMan& LevelLookup, const AZStd::string& pathPerLevel, const AZStd::string& pathGlobal, bool bVS, bool bPersistent);

    bool mfPreloadBinaryShaders();

    bool LoadShaderStartupCache();
    void UnloadShaderStartupCache();

#if !defined(CONSOLE) && !defined(NULL_RENDERER)
    void AddCombination(SCacheCombination& cmb,  FXShaderCacheCombinations& CmbsMap, CHWShader* pHWS);
    void AddGLCombinations(CShader* pSH, std::vector<SCacheCombination>& CmbsGL);
    void AddLTCombinations(SCacheCombination& cmb, FXShaderCacheCombinations& CmbsMap, CHWShader* pHWS);
    void AddRTCombinations(FXShaderCacheCombinations& CmbsMap, CHWShader* pHWS, CShader* pSH, FXShaderCacheCombinations* Combinations);
    void AddGLCombination(FXShaderCacheCombinations& CmbsMap, SCacheCombination& cc);
    void FilterShaderCombinations(std::vector<SCacheCombination>& Cmbs, const std::vector<CShaderListFilter>& Filters);
    void mfPrecacheShaders(bool bStatsOnly);
    void mfGetShaderList();
    void _PrecacheShaderList(bool bStatsOnly);
    void mfOptimiseShaders(const char* szFolder, bool bForce);
    void mfMergeShaders();
    void _MergeShaders();
    void mfAddRTCombinations(FXShaderCacheCombinations& CmbsMapSrc, FXShaderCacheCombinations& CmbsMapDst, CHWShader* pSH, bool bListOnly);
    void mfAddRTCombination_r(int nComb, FXShaderCacheCombinations& CmbsMapDst, SCacheCombination* cmb, CHWShader* pSH, bool bAutoPrecache);
    void mfAddLTCombinations(SCacheCombination* cmb, FXShaderCacheCombinations& CmbsMapDst);
    void mfAddLTCombination(SCacheCombination* cmb, FXShaderCacheCombinations& CmbsMapDst, DWORD dwL);
#endif

    int Size()
    {
        int nSize = sizeof(*this);

        nSize += m_SGC.capacity();
        nSize += m_Bin.Size();

        return nSize;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_Bin);
        pSizer->AddObject(m_SGC);
        pSizer->AddObject(m_ShaderNames);
        pSizer->AddObject(m_ShaderCacheCombinations[0]);
        pSizer->AddObject(m_ShaderCacheCombinations[1]);
    }

    void RefreshShader(const AZStd::string_view name, CShader* shader) override
    {
        mfRefreshSystemShader(name.data(), shader);
    }

    void ReleaseShader(CShader* shader) const override
    {
        SAFE_RELEASE_FORCE(shader);
    }

    static float EvalWaveForm(SWaveForm* wf);
    static float EvalWaveForm(SWaveForm2* wf);
    static float EvalWaveForm2(SWaveForm* wf, float frac);
};

//=====================================================================

#endif  // __CSHADER_H__
