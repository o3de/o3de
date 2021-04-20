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

#include "RenderDll_precompiled.h"

#include "TextureManager.h"
#include "Texture.h"
#include "ISystem.h"
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/API/AtomActiveInterface.h>
#include <AzCore/Interface/Interface.h>


//////////////////////////////////////////////////////////////////////////
CTextureManager* CTextureManager::s_Instance = nullptr;
//------------------------------------------------------------------------------
void CTextureManager::ReleaseTextureSemantics()
{
    for (int slot = 0; slot < EFTT_MAX; slot++)
    {
        delete[] m_TexSlotSemantics[slot].suffix;
        m_TexSlotSemantics[slot].suffix = nullptr;
        m_TexSlotSemantics[slot].def = nullptr;
        m_TexSlotSemantics[slot].neutral = nullptr;
    }
}

void CTextureManager::ReleaseTextures()
{
    AZ_Warning("[Shaders System]", false, "Textures Manager - releasing all textures");

    // Remove fixed default textures
    for (auto& iter : m_DefaultTextures)
    {
        SAFE_RELEASE(iter.second);
    }
    m_DefaultTextures.clear();

    // Remove fixed engine textures
    for (auto& iter : m_EngineTextures)
    {
        SAFE_RELEASE(iter.second);
    }
    m_EngineTextures.clear();

    // Remove references to static engine textures
    m_StaticEngineTextureReferences.clear();

    // Material textures should be released by releasing the materials themselves.
    m_MaterialTextures.clear();
}

//------------------------------------------------------------------------------
// [Shaders System]
// The following method should be enhanced to load the user defined default textures
// from an XML file data 'defaultTextures.xml'
// Sample code for the load is provided below to be followed in the future.
//------------------------------------------------------------------------------
void CTextureManager::LoadDefaultTextures()
{
    struct TextureEntry
    {
        const char* szTextureName;
        const char* szFileName;
        uint32 flags;
    };

    // The following texture names are the name by which the texture pointers will 
    // be indexed.    Notice that they match to the previously stored textures with
    // name pattern removing 's_ptex'[Name]. Example: 's_ptexWhite' will now be 'White'
    const TextureEntry texturesFromFile[] =
    {
        {"NoTextureCM",                 "EngineAssets/TextureMsg/ReplaceMeCM.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"White",                       "EngineAssets/Textures/White.dds",                         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"Gray",                        "EngineAssets/Textures/Grey.dds",                          FT_DONT_RELEASE | FT_DONT_STREAM },
        {"Black",                       "EngineAssets/Textures/Black.dds",                         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"BlackAlpha",                  "EngineAssets/Textures/BlackAlpha.dds",                    FT_DONT_RELEASE | FT_DONT_STREAM },
        {"BlackCM",                     "EngineAssets/Textures/BlackCM.dds",                       FT_DONT_RELEASE | FT_DONT_STREAM },
        {"DefaultProbeCM",              "EngineAssets/Shading/defaultProbe_cm.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"FlatBump",                    "EngineAssets/Textures/White_ddn.dds",                     FT_DONT_RELEASE | FT_DONT_STREAM | FT_TEX_NORMAL_MAP },
        {"PaletteDebug",                "EngineAssets/Textures/palletteInst.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"PaletteTexelsPerMeter",       "EngineAssets/Textures/TexelsPerMeterGrad.dds",            FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconShaderCompiling",         "EngineAssets/Icons/ShaderCompiling.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconStreaming",               "EngineAssets/Icons/Streaming.dds",                        FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconStreamingTerrainTexture", "EngineAssets/Icons/StreamingTerrain.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconNullSoundSystem",         "EngineAssets/Icons/NullSoundSystem.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconNavigationProcessing",    "EngineAssets/Icons/NavigationProcessing.dds",             FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ShadowJitterMap",             "EngineAssets/Textures/rotrandom.dds",                     FT_DONT_RELEASE | FT_DONT_STREAM },
        {"EnvironmentBRDF",             "EngineAssets/Shading/environmentBRDF.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ScreenNoiseMap",              "EngineAssets/Textures/JumpNoiseHighFrequency_x27y19.dds", FT_DONT_RELEASE | FT_DONT_STREAM | FT_NOMIPS },
        {"DissolveNoiseMap",            "EngineAssets/Textures/noise.dds",                         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"GrainFilterMap",              "EngineAssets/ScreenSpace/grain_bayer_mul.dds",            FT_DONT_RELEASE | FT_DONT_STREAM | FT_NOMIPS },
        {"FilmGrainMap",                "EngineAssets/ScreenSpace/film_grain.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM | FT_NOMIPS },
        {"VignettingMap",               "EngineAssets/Shading/vignetting.dds",                     FT_DONT_RELEASE | FT_DONT_STREAM },
        {"AOJitter",                    "EngineAssets/ScreenSpace/PointsOnSphere4x4.dds",          FT_DONT_RELEASE | FT_DONT_STREAM },
        {"AOVOJitter",                  "EngineAssets/ScreenSpace/PointsOnSphereVO4x4.dds",        FT_DONT_RELEASE | FT_DONT_STREAM },
        {"NormalsFitting",              "EngineAssets/ScreenSpace/NormalsFitting.dds",             FT_DONT_RELEASE | FT_DONT_STREAM },
        {"AverageMemoryUsage",          "EngineAssets/Icons/AverageMemoryUsage.dds",               FT_DONT_RELEASE | FT_DONT_STREAM },
        {"LowMemoryUsage",              "EngineAssets/Icons/LowMemoryUsage.dds",                   FT_DONT_RELEASE | FT_DONT_STREAM },
        {"HighMemoryUsage",             "EngineAssets/Icons/HighMemoryUsage.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"LivePreview",                 "EngineAssets/Icons/LivePreview.dds",                      FT_DONT_RELEASE | FT_DONT_STREAM },
#if !defined(_RELEASE)
        {"NoTexture",                   "EngineAssets/TextureMsg/ReplaceMe.dds",                   FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling",        "EngineAssets/TextureMsg/TextureCompiling.dds",            FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_a",      "EngineAssets/TextureMsg/TextureCompiling_a.dds",          FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_cm",     "EngineAssets/TextureMsg/TextureCompiling_cm.dds",         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_ddn",    "EngineAssets/TextureMsg/TextureCompiling_ddn.dds",        FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_ddna",   "EngineAssets/TextureMsg/TextureCompiling_ddna.dds",       FT_DONT_RELEASE | FT_DONT_STREAM },
        {"DefaultMergedDetail",         "EngineAssets/Textures/GreyAlpha.dds",                     FT_DONT_RELEASE | FT_DONT_STREAM },
        {"MipMapDebug",                 "EngineAssets/TextureMsg/MipMapDebug.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorBlue",                   "EngineAssets/TextureMsg/color_Blue.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorCyan",                   "EngineAssets/TextureMsg/color_Cyan.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorGreen",                  "EngineAssets/TextureMsg/color_Green.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorPurple",                 "EngineAssets/TextureMsg/color_Purple.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorRed",                    "EngineAssets/TextureMsg/color_Red.dds",                   FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorWhite",                  "EngineAssets/TextureMsg/color_White.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorYellow",                 "EngineAssets/TextureMsg/color_Yellow.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorOrange",                 "EngineAssets/TextureMsg/color_Orange.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorMagenta",                "EngineAssets/TextureMsg/color_Magenta.dds",               FT_DONT_RELEASE | FT_DONT_STREAM },
#else
        {"NoTexture",                   "EngineAssets/TextureMsg/ReplaceMeRelease.dds",            FT_DONT_RELEASE | FT_DONT_STREAM },
#endif
    };

    // Reduced list of default textures to load.
    const TextureEntry texturesFromFileReduced[] =
    {
        {"NoTextureCM",                 "EngineAssets/TextureMsg/ReplaceMeCM.tif",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"White",                       "EngineAssets/Textures/White.tif",                         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"Gray",                        "EngineAssets/Textures/Grey.dds",                          FT_DONT_RELEASE | FT_DONT_STREAM },
        {"Black",                       "EngineAssets/Textures/Black.tif",                         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"BlackAlpha",                  "EngineAssets/Textures/BlackAlpha.tif",                    FT_DONT_RELEASE | FT_DONT_STREAM },
        {"BlackCM",                     "EngineAssets/Textures/BlackCM.tif",                       FT_DONT_RELEASE | FT_DONT_STREAM },
        {"FlatBump",                    "EngineAssets/Textures/White_ddn.tif",                     FT_DONT_RELEASE | FT_DONT_STREAM | FT_TEX_NORMAL_MAP },
        {"AverageMemoryUsage",          "EngineAssets/Icons/AverageMemoryUsage.tif",               FT_DONT_RELEASE | FT_DONT_STREAM },
        {"LowMemoryUsage",              "EngineAssets/Icons/LowMemoryUsage.tif",                   FT_DONT_RELEASE | FT_DONT_STREAM },
        {"HighMemoryUsage",             "EngineAssets/Icons/HighMemoryUsage.tif",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"LivePreview",                 "EngineAssets/Icons/LivePreview.tif",                      FT_DONT_RELEASE | FT_DONT_STREAM },
#if !defined(_RELEASE)
        {"NoTexture",                   "EngineAssets/TextureMsg/ReplaceMe.tif",                   FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling",        "EngineAssets/TextureMsg/TextureCompiling.tif",            FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_a",      "EngineAssets/TextureMsg/TextureCompiling_a.tif",          FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_cm",     "EngineAssets/TextureMsg/TextureCompiling_cm.tif",         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_ddn",    "EngineAssets/TextureMsg/TextureCompiling_ddn.tif",        FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_ddna",   "EngineAssets/TextureMsg/TextureCompiling_ddna.tif",       FT_DONT_RELEASE | FT_DONT_STREAM },
        {"DefaultMergedDetail",         "EngineAssets/Textures/GreyAlpha.tif",                     FT_DONT_RELEASE | FT_DONT_STREAM },
        {"MipMapDebug",                 "EngineAssets/TextureMsg/MipMapDebug.tif",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorBlue",                   "EngineAssets/TextureMsg/color_Blue.tif",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorCyan",                   "EngineAssets/TextureMsg/color_Cyan.tif",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorGreen",                  "EngineAssets/TextureMsg/color_Green.tif",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorPurple",                 "EngineAssets/TextureMsg/color_Purple.tif",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorRed",                    "EngineAssets/TextureMsg/color_Red.tif",                   FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorWhite",                  "EngineAssets/TextureMsg/color_White.tif",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorYellow",                 "EngineAssets/TextureMsg/color_Yellow.tif",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorOrange",                 "EngineAssets/TextureMsg/color_Orange.tif",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorMagenta",                "EngineAssets/TextureMsg/color_Magenta.tif",               FT_DONT_RELEASE | FT_DONT_STREAM },
#else
        {"NoTexture",                   "EngineAssets/TextureMsg/ReplaceMeRelease.tif",            FT_DONT_RELEASE | FT_DONT_STREAM },
#endif
    };

    // Loop over the appropriate texture list and load the textures, storing them in a map keyed by texture name.
    // Use reduced subset of textures for Other.
    //if (AZ::Interface<AzFramework::AtomActiveInterface>::Get())
    //{
    //    for (const TextureEntry& entry : texturesFromFileReduced)
    //    {
    //        // Use EF_LoadTexture rather than CTexture::ForName
    //        CTexture* pNewTexture = static_cast<CTexture*>(gEnv->pRenderer->EF_LoadTexture(entry.szFileName, entry.flags));
    //        if (pNewTexture)
    //        {
    //            CCryNameTSCRC texEntry(entry.szTextureName);
    //            m_DefaultTextures[texEntry] = pNewTexture;
    //        }
    //        else
    //        {
    //            AZ_Assert(false, "Error - CTextureManager failed to load default texture %s", entry.szFileName);
    //            AZ_Warning("[Shaders System]", false, "Error - CTextureManager failed to load default texture %s", entry.szFileName);
    //        }
    //    }
    //}
    //else
    //{
        for (const TextureEntry& entry : texturesFromFile)
        {
            CTexture* pNewTexture = CTexture::ForName(entry.szFileName, entry.flags, eTF_Unknown);
            if (pNewTexture)
            {
                CCryNameTSCRC texEntry(entry.szTextureName);
                m_DefaultTextures[texEntry] = pNewTexture;
            }
            else
            {
                AZ_Assert(false, "Error - CTextureManager failed to load default texture %s", entry.szFileName);
                AZ_Warning("[Shaders System]", false, "Error - CTextureManager failed to load default texture %s", entry.szFileName);
            }
        }
    //}

    m_texNoTexture = GetDefaultTexture("NoTexture");
    m_texNoTextureCM = GetDefaultTexture("NoTextureCM");
    m_texWhite = GetDefaultTexture("White");
    m_texBlack = GetDefaultTexture("Black");
    m_texBlackCM = GetDefaultTexture("BlackCM");
}

void CTextureManager::LoadMaterialTexturesSemantics()
{
    // Shaders System] - To Do - replace the following with the actual pointers
    CTexture*       pNoTexture = CTextureManager::Instance()->GetNoTexture();
    CTexture*       pWhiteTexture = CTextureManager::Instance()->GetWhiteTexture();
    CTexture*       pGrayTexture = CTextureManager::Instance()->GetDefaultTexture("Gray");
    CTexture*       pFlatBumpTexture = CTextureManager::Instance()->GetDefaultTexture("FlatBump");

    // [Shaders System] - this needs to move to shader code and be reflected, hence data driven per 
    // texture usage / declaration in the shader.
    MaterialTextureSemantic     texSlotSemantics[] =
    {
        // NOTE: must be in order with filled holes to allow direct lookup
        MaterialTextureSemantic( EFTT_DIFFUSE,          4, pNoTexture,         pWhiteTexture,    "_diff" ),
        MaterialTextureSemantic( EFTT_NORMALS,          2, pFlatBumpTexture,   pFlatBumpTexture, "_ddn" ),
        MaterialTextureSemantic( EFTT_SPECULAR,         1, pWhiteTexture,      pWhiteTexture,    "_spec" ),
        MaterialTextureSemantic( EFTT_ENV,              0, pWhiteTexture,      pWhiteTexture,    "_cm" ),
        MaterialTextureSemantic( EFTT_DETAIL_OVERLAY,   3, pGrayTexture,       pWhiteTexture,    "_detail" ),
        MaterialTextureSemantic( EFTT_SECOND_SMOOTHNESS, 2, pWhiteTexture,     pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_HEIGHT,           2, pWhiteTexture,      pWhiteTexture,    "_displ" ),
        MaterialTextureSemantic( EFTT_DECAL_OVERLAY,    3, pGrayTexture,       pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_SUBSURFACE,       3, pWhiteTexture,      pWhiteTexture,    "_sss" ),
        MaterialTextureSemantic( EFTT_CUSTOM,           4, pWhiteTexture,      pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_CUSTOM_SECONDARY, 2, pFlatBumpTexture,   pFlatBumpTexture, "" ),
        MaterialTextureSemantic( EFTT_OPACITY,          4, pWhiteTexture,      pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_SMOOTHNESS,       2, pWhiteTexture,      pWhiteTexture,    "_ddna" ),
        MaterialTextureSemantic( EFTT_EMITTANCE,        1, pWhiteTexture,      pWhiteTexture,    "_em" ),
        MaterialTextureSemantic( EFTT_OCCLUSION,        4, pWhiteTexture,      pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_SPECULAR_2,       4, pWhiteTexture,      pWhiteTexture,    "_spec" ),

        // This is the terminator for the name-search
        MaterialTextureSemantic( EFTT_UNKNOWN,          0, CTexture::s_pTexNULL,      CTexture::s_pTexNULL,     "" ),
    };

    for (int i = 0; i < EFTT_MAX; i++)
    {
        SAFE_DELETE_ARRAY(m_TexSlotSemantics[i].suffix);      // make sure we are not leaving snail marks
        m_TexSlotSemantics[i] = texSlotSemantics[i];
    }
}

void CTextureManager::CreateStaticEngineTextureReferences()
{
    struct textureReferenceEntry {
        const char* textureName;
        CTexture** staticTextureReference;
    };

    textureReferenceEntry staticTextureReferences[] =
    {
        { "$HDRTarget",                     &CTexture::s_ptexHDRTarget },
        { "$HDRTargetPrev",                 &CTexture::s_ptexHDRTargetPrev },
        // alias for shaders that use $HDR_TargetPrev see ShaderTemplate.cpp
        { "$HDR_TargetPrev",                &CTexture::s_ptexHDRTargetPrev },
        { "$SceneTarget",                   &CTexture::s_ptexSceneTarget },
        { "$CurrSceneTarget",               &CTexture::s_ptexCurrSceneTarget },
        { "$SceneNormalsMapMS",             &CTexture::s_ptexSceneNormalsMapMS},
        { "$SceneDiffuseAccMS",             &CTexture::s_ptexSceneDiffuseAccMapMS },
        { "$SceneSpecularAccMS",            &CTexture::s_ptexSceneSpecularAccMapMS },
        { "$SceneTargetR11G11B10F_0",       &CTexture::s_ptexSceneTargetR11G11B10F[0] },
        { "$SceneTargetR11G11B10F_1",       &CTexture::s_ptexSceneTargetR11G11B10F[1] },
        { "$SceneTargetScaled0R11G11B10F",  &CTexture::s_ptexSceneTargetScaledR11G11B10F[0] },
        { "$SceneTargetScaled1R11G11B10F",  &CTexture::s_ptexSceneTargetScaledR11G11B10F[1] },
        { "$SceneTargetScaled2R11G11B10F",  &CTexture::s_ptexSceneTargetScaledR11G11B10F[2] },
        { "$SceneTargetScaled3R11G11B10F",  &CTexture::s_ptexSceneTargetScaledR11G11B10F[3] },
        { "$SceneNormalsMap",               &CTexture::s_ptexSceneNormalsMap },
        { "$SceneNormalsBent",              &CTexture::s_ptexSceneNormalsBent },
        { "$SceneDiffuse",                  &CTexture::s_ptexSceneDiffuse },
        { "$SceneSpecular",                 &CTexture::s_ptexSceneSpecular },
        { "$SceneDiffuseAcc",               &CTexture::s_ptexSceneDiffuseAccMap },
        { "$SceneSpecularAcc",              &CTexture::s_ptexSceneSpecularAccMap },
        { "$MipColors_Diffuse",             &CTexture::s_ptexMipColors_Diffuse },
        { "$MipColors_Bump",                &CTexture::s_ptexMipColors_Bump },
        { "$RT_2D",                         &CTexture::s_ptexRT_2D },
        { "$RainOcclusion",                 &CTexture::s_ptexRainOcclusion },
        { "$RainSSOcclusion0",              &CTexture::s_ptexRainSSOcclusion[0] },
        { "$RainSSOcclusion1",              &CTexture::s_ptexRainSSOcclusion[1] },
        { "$RainDropsAccumRT_0",            &CTexture::s_ptexRainDropsRT[0] },
        { "$RainDropsAccumRT_1",            &CTexture::s_ptexRainDropsRT[1] },
        { "FromObj",                        &CTexture::s_ptexFromObj },
        { "SvoTree",                        &CTexture::s_ptexSvoTree },
        { "SvoTris",                        &CTexture::s_ptexSvoTris },
        { "SvoGlobalCM",                    &CTexture::s_ptexSvoGlobalCM },
        { "SvoRgbs",                        &CTexture::s_ptexSvoRgbs },
        { "SvoNorm",                        &CTexture::s_ptexSvoNorm },
        { "SvoOpac",                        &CTexture::s_ptexSvoOpac },
        { "$FromObjCM",                     &CTexture::s_ptexFromObjCM },
        { "$RT_ShadowPool",                 &CTexture::s_ptexRT_ShadowPool },
        { "$RT_ShadowStub",                 &CTexture::s_ptexRT_ShadowStub },
        { "$ModelHud",                      &CTexture::s_ptexModelHudBuffer },
        { "$Velocity",                      &CTexture::s_ptexVelocity },
        { "$VelocityTilesTmp0",             &CTexture::s_ptexVelocityTiles[0] },
        { "$VelocityTilesTmp1",             &CTexture::s_ptexVelocityTiles[1] },
        { "$VelocityTiles",                 &CTexture::s_ptexVelocityTiles[2] },
        { "$VelocityObjects",               &CTexture::s_ptexVelocityObjects[0] },
        { "$VelocityObjects_R",             &CTexture::s_ptexVelocityObjects[1] },
        { "$WaterRipplesDDN_0",             &CTexture::s_ptexWaterRipplesDDN },
        { "$WaterOceanMap",                 &CTexture::s_ptexWaterOcean },
        { "$WaterVolumeTemp",               &CTexture::s_ptexWaterVolumeTemp },
        { "$WaterVolumeDDN",                &CTexture::s_ptexWaterVolumeDDN },
        { "$WaterVolumeRefl",               &CTexture::s_ptexWaterVolumeRefl[0] },
        { "$WaterVolumeReflPrev",           &CTexture::s_ptexWaterVolumeRefl[1] },
        { "$WaterVolumeCaustics",           &CTexture::s_ptexWaterCaustics[0] },
        { "$WaterVolumeCausticsTemp",       &CTexture::s_ptexWaterCaustics[0] },
        { "$BackBuffer",                    &CTexture::s_ptexBackBuffer },
        { "$PrevFrameScale",                &CTexture::s_ptexPrevFrameScaled },
        { "$BackBufferScaled_d2",           &CTexture::s_ptexBackBufferScaled[0] },
        { "$BackBufferScaled_d4",           &CTexture::s_ptexBackBufferScaled[1] },
        { "$BackBufferScaled_d8",           &CTexture::s_ptexBackBufferScaled[2] },
        { "$BackBufferScaledTemp_d2",       &CTexture::s_ptexBackBufferScaledTemp[0] },
        { "$BackBufferScaledTemp_d4",       &CTexture::s_ptexBackBufferScaledTemp[1] },
        { "$AmbientLookup",                 &CTexture::s_ptexAmbientLookup },
        { "$ShadowMask",                    &CTexture::s_ptexShadowMask },
        { "$FlaresGather",                  &CTexture::s_ptexFlaresGather },
        { "$DepthBufferQuarter",            &CTexture::s_ptexDepthBufferQuarter },
        { "$ZTarget",                       &CTexture::s_ptexZTarget },
        { "$ZTargetDownSample0",            &CTexture::s_ptexZTargetDownSample[0]},
        { "$ZTargetDownSample1",            &CTexture::s_ptexZTargetDownSample[1]},
        { "$ZTargetDownSample2",            &CTexture::s_ptexZTargetDownSample[2]},
        { "$ZTargetDownSample3",            &CTexture::s_ptexZTargetDownSample[3]},
        { "$FurZTarget",                    &CTexture::s_ptexFurZTarget },
        { "$ZTargetScaled",                 &CTexture::s_ptexZTargetScaled },
        { "$ZTargetScaled2",                &CTexture::s_ptexZTargetScaled2 },
        { "$CloudsLM",                      &CTexture::s_ptexCloudsLM },
        { "$VolObj_Density",                &CTexture::s_ptexVolObj_Density },
        { "$VolObj_Shadow",                 &CTexture::s_ptexVolObj_Shadow },
        { "$ColorChart",                    &CTexture::s_ptexColorChart },
        { "$SkyDomeMie",                    &CTexture::s_ptexSkyDomeMie },
        { "$SkyDomeRayleigh",               &CTexture::s_ptexSkyDomeRayleigh },
        { "$SkyDomeMoon",                   &CTexture::s_ptexSkyDomeMoon },
        { "$VolumetricInscattering",        &CTexture::s_ptexVolumetricFog },
        { "$DensityColorVolume",            &CTexture::s_ptexVolumetricFogDensityColor },
        { "$DensityVolume",                 &CTexture::s_ptexVolumetricFogDensity },
        { "$ClipVolumeStencilVolume",       &CTexture::s_ptexVolumetricClipVolumeStencil },
        { "$DefaultEnvironmentProbe",       &CTexture::s_defaultEnvironmentProbeDummy },
#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
        { "$GmemStenLinDepth",              &CTexture::s_ptexGmemStenLinDepth },
#endif
    };

    for (textureReferenceEntry& entry : staticTextureReferences)
    {
        CCryNameTSCRC   hashEntry(entry.textureName);
        m_StaticEngineTextureReferences[hashEntry] = entry.staticTextureReference;
    }

    char textureName[256];
    int i;

    for ( i = 0; i < MAX_OCCLUSION_READBACK_TEXTURES; i++)
    {
        azsprintf(textureName, "$FlaresOcclusion_%d", i);
        CCryNameTSCRC   hashEntry(textureName);
        m_StaticEngineTextureReferences[hashEntry] = &CTexture::s_ptexFlaresOcclusionRing[i];
    }

    for (i = 0; i < AZ_ARRAY_SIZE(CTexture::s_ptexFromRE); i++)
    {
        azsprintf(textureName, "$FromRE_%d", i);
        CCryNameTSCRC   hashEntry(textureName);
        m_StaticEngineTextureReferences[hashEntry] = &CTexture::s_ptexFromRE[i];
    }

    // The shader parser also accetps $FromRE as a valid alias for $FromRE_0
    m_StaticEngineTextureReferences[CCryNameTSCRC("$FromRE")] = &CTexture::s_ptexFromRE[0];

    for (i = 0; i < 8; i++)
    {
        azsprintf(textureName, "$ShadowID_%d", i);
        CCryNameTSCRC   hashEntry(textureName);
        m_StaticEngineTextureReferences[hashEntry] = &CTexture::s_ptexShadowID[i];
    }

    for (i = 0; i < 2; i++)
    {
        azsprintf(textureName, "$FromRE%d_FromContainer", i);
        CCryNameTSCRC   hashEntry(textureName);
        m_StaticEngineTextureReferences[hashEntry] = &CTexture::s_ptexFromRE_FromContainer[i];
    }
}

/*
//==============================================================================
// The following two method should be implemented in order to finish rip off the rest
// of the horrible CTexture slots management via static declarations in class CTexture!
// The first one is the initialization method for all engine textures slots and
// the second is a better candidate than LoadDefajultTextures that can replace it
// in order to have it data driven.
//------------------------------------------------------------------------------
void CTextureManager::CreateEngineTextures()
{
    ...
    Replace the code in CTexture::LoadDefaultSystemTextures() with a proper code to be 
    placed here.
    ...
}

void CTextureManager::LoadDefaultTexturesFromFile()
{
#if !defined(NULL_RENDERER)
    if (m_DefaultTextures.size())
    {
        return;
    }

    uint32 nDefaultFlags = FT_DONT_STREAM;

    XmlNodeRef root = GetISystem()->LoadXmlFromFile("EngineAssets/defaulttextures.xml");
    if (root)
    {
        // we loop over this twice.
        // we are looping from the back to the front to make sure that the order of the assets are correct ,when we try to load them the second time.
        for (int i = root->getChildCount() - 1; i >= 0; i--)
        {
            XmlNodeRef      textureEntry = root->getChild(i);
            if (!textureEntry->isTag("TextureEntry"))
            {
                continue;
            }

            // make an ASYNC request to move it to the top of the queue:
            XmlString   fileName;
            if (textureEntry->getAttr("FileName", fileName))
            {
                EBUS_EVENT(AzFramework::AssetSystemRequestBus, GetAssetStatus, fileName.c_str(), false );
            }
        }

        for (int i = 0; i < root->getChildCount(); i++)
        {
            XmlNodeRef entry = root->getChild(i);
            if (!entry->isTag("entry"))
            {
                continue;
            }

            if (entry->getAttr("nomips", nNoMips) && nNoMips)

                uint32 nFlags = nDefaultFlags;

            // check attributes to modify the loading flags
            int nNoMips = 0;
            if (entry->getAttr("nomips", nNoMips) && nNoMips)
            {
                nFlags |= FT_NOMIPS;
            }

            // default textures should be compiled synchronously:
            const char*     texName = entry->getContent();
            if (!gEnv->pCryPak->IsFileExist(texName))
            {
                // make a SYNC request to block until its ready.
                EBUS_EVENT(AzFramework::AssetSystemRequestBus, CompileAssetSync, texName);
            }

            CTexture* pTexture = CTexture::ForName(texName, nFlags, eTF_Unknown);
            if (pTexture)
            {
                CCryNameTSCRC nameID(texName);
                m_DefaultTextures[nameID] = pTexture;
            }
        }
    }
#endif
}
*/

