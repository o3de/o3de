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

#include "Cry3DEngine_precompiled.h"
#include "IShader.h"
#include "MaterialHelpers.h"

/* -----------------------------------------------------------------------
  * These functions are used in Cry3DEngine, CrySystem, CryRenderD3D11,
  * Editor, ResourceCompilerMaterial and more
  */

//////////////////////////////////////////////////////////////////////////
namespace
{
    static struct
    {
        EEfResTextures slot;
        const char* ename;
        bool adjustable;
        const char* name;
        const char* description;
        const char* suffix;
    }
    s_TexSlotSemantics[] =
    {
        // NOTE: must be in order with filled holes to allow direct lookup
        { EFTT_DIFFUSE,           "EFTT_DIFFUSE",           true,  "Diffuse"          , "Base surface color. Alpha mask is contained in alpha channel."          , "_diff" },
        { EFTT_NORMALS,           "EFTT_NORMALS",           true,  "Bumpmap"          , "Normal direction for each pixel simulating bumps on the surface. Smoothness map contained in alpha channel."          , "_ddn" }, // Ideally "Normal" but need to keep backwards-compatibility
        { EFTT_SPECULAR,          "EFTT_SPECULAR",          true,  "Specular"         , "Reflective and shininess intensity and color of reflective highlights"         , "_spec" },
        { EFTT_ENV,               "EFTT_ENV",               true,  "Environment"      , "Deprecated"    , "_cm" },
        { EFTT_DETAIL_OVERLAY,    "EFTT_DETAIL_OVERLAY",    true,  "Detail"           , "Increases micro and macro surface bump, diffuse and gloss detail. To use, enable the 'Detail Mapping' shader gen param. "           , "_detail" },
        { EFTT_SECOND_SMOOTHNESS, "EFTT_SECOND_SMOOTHNESS", false, "SecondSmoothness" , ""              , "" },
        { EFTT_HEIGHT,            "EFTT_HEIGHT",            true,  "Heightmap"        , "Height for offset bump, POM, silhouette POM, and displacement mapping defined by a Grayscale texture"        , "_displ" },
        { EFTT_DECAL_OVERLAY,     "EFTT_DECAL_OVERLAY",     true,  "Decal"            , ""              , "" }, // called "DecalOverlay" in the shaders
        { EFTT_SUBSURFACE,        "EFTT_SUBSURFACE",        true,  "SubSurface"       , ""              , "_sss" }, // called "Subsurface" in the shaders
        { EFTT_CUSTOM,            "EFTT_CUSTOM",            true,  "Custom"           , ""              , "" }, // called "CustomMap" in the shaders
        { EFTT_CUSTOM_SECONDARY,  "EFTT_CUSTOM_SECONDARY",  true,  "[1] Custom"       , ""              , "" },
        { EFTT_OPACITY,           "EFTT_OPACITY",           true,  "Opacity"          , "SubSurfaceScattering map to simulate thin areas for light to penetrate"          , "" },
        { EFTT_SMOOTHNESS,        "EFTT_SMOOTHNESS",        false, "Smoothness"       , ""              , "_ddna" },
        { EFTT_EMITTANCE,         "EFTT_EMITTANCE",         true,  "Emittance"        , "Multiplies the emissive color with RGB texture. Emissive alpha mask is contained in alpha channel."        , "_em" },
        { EFTT_OCCLUSION,         "EFTT_OCCLUSION",         true,  "Occlusion"        , "Grayscale texture to mask diffuse lighting response and simulate darker areas"        , "" },
        { EFTT_SPECULAR_2,        "EFTT_SPECULAR_2",        true,  "Specular2"        , ""              , "_spec"   },

        // Backwards compatible names are found here and mapped to the updated enum
        { EFTT_NORMALS,           "EFTT_BUMP",              false, "Normal"           , ""              , "" }, // called "Bump" in the shaders
        { EFTT_SMOOTHNESS,        "EFTT_GLOSS_NORMAL_A",    false, "GlossNormalA"     , ""              , "" },
        { EFTT_HEIGHT,            "EFTT_BUMPHEIGHT",        false, "Height"           , ""              , ""        }, // called "BumpHeight" in the shaders

        // This is the terminator for the name-search
        { EFTT_UNKNOWN,          "EFTT_UNKNOWN",          false, NULL          , ""        },
    };

#if 0
    static class Verify
    {
    public:
        Verify()
        {
            for (int i = 0; s_TexSlotSemantics[i].name; i++)
            {
                if (s_TexSlotSemantics[i].slot != i)
                {
                    throw std::runtime_error("Invalid texture slot lookup array.");
                }
            }
        }
    }
    s_VerifyTexSlotSemantics;
#endif
}

// This should be done per shader (hence, semantics lookup map should be constructed per shader type)
EEfResTextures MaterialHelpers::FindTexSlot(const char* texName) const
{
    for (int i = 0; s_TexSlotSemantics[i].name; i++)
    {
        if (azstricmp(s_TexSlotSemantics[i].name, texName) == 0)
        {
            return s_TexSlotSemantics[i].slot;
        }
    }

    return EFTT_UNKNOWN;
}

const char* MaterialHelpers::FindTexName(EEfResTextures texSlot) const
{
    for (int i = 0; s_TexSlotSemantics[i].name; i++)
    {
        if (s_TexSlotSemantics[i].slot == texSlot)
        {
            return s_TexSlotSemantics[i].name;
        }
    }

    return NULL;
}

const char* MaterialHelpers::LookupTexName(EEfResTextures texSlot) const
{
    assert((texSlot >= 0) && (texSlot < EFTT_MAX));
    return s_TexSlotSemantics[texSlot].name;
}

const char* MaterialHelpers::LookupTexDesc(EEfResTextures texSlot) const
{
    assert((texSlot >= 0) && (texSlot < EFTT_MAX));
    return s_TexSlotSemantics[texSlot].description;
}

const char* MaterialHelpers::LookupTexEnum(EEfResTextures texSlot) const
{
    assert((texSlot >= 0) && (texSlot < EFTT_MAX));
    return s_TexSlotSemantics[texSlot].ename;
}

const char* MaterialHelpers::LookupTexSuffix(EEfResTextures texSlot) const
{
    assert((texSlot >= 0) && (texSlot < EFTT_MAX));
    return s_TexSlotSemantics[texSlot].suffix;
}

bool MaterialHelpers::IsAdjustableTexSlot(EEfResTextures texSlot) const
{
    assert((texSlot >= 0) && (texSlot < EFTT_MAX));
    return s_TexSlotSemantics[texSlot].adjustable;
}

//////////////////////////////////////////////////////////////////////////
// [Shader System TO DO] - automate these lookups to be data driven!
bool MaterialHelpers::SetGetMaterialParamFloat(IRenderShaderResources& pShaderResources, const char* sParamName, float& v, bool bGet) const
{
    EEfResTextures texSlot = EFTT_UNKNOWN;

    if (!azstricmp("emissive_intensity", sParamName))
    {
        texSlot = EFTT_EMITTANCE;
    }
    else if (!azstricmp("shininess", sParamName))
    {
        texSlot = EFTT_SMOOTHNESS;
    }
    else if (!azstricmp("opacity", sParamName))
    {
        texSlot = EFTT_OPACITY;
    }

    if (!azstricmp("alpha", sParamName))
    {
        if (bGet)
        {
            v = pShaderResources.GetAlphaRef();
        }
        else
        {
            pShaderResources.SetAlphaRef(v);
        }

        return true;
    }
    else if (texSlot != EFTT_UNKNOWN)
    {
        if (bGet)
        {
            v = pShaderResources.GetStrengthValue(texSlot);
        }
        else
        {
            pShaderResources.SetStrengthValue(texSlot, v);
        }

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool MaterialHelpers::SetGetMaterialParamVec3(IRenderShaderResources& pShaderResources, const char* sParamName, Vec3& v, bool bGet) const
{
    EEfResTextures texSlot = EFTT_UNKNOWN;

    if (!azstricmp("diffuse", sParamName))
    {
        texSlot = EFTT_DIFFUSE;
    }
    else if (!azstricmp("specular", sParamName))
    {
        texSlot = EFTT_SPECULAR;
    }
    else if (!azstricmp("emissive_color", sParamName))
    {
        texSlot = EFTT_EMITTANCE;
    }

    if (texSlot != EFTT_UNKNOWN)
    {
        if (bGet)
        {
            v = pShaderResources.GetColorValue(texSlot).toVec3();
        }
        else
        {
            pShaderResources.SetColorValue(texSlot, ColorF(v, 1.0f));
        }

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void MaterialHelpers::SetTexModFromXml(SEfTexModificator& pTextureModifier, const XmlNodeRef& modNode) const
{
    // Modificators
    float f;
    uint8 c;

    modNode->getAttr("TexMod_RotateType", pTextureModifier.m_eRotType);
    modNode->getAttr("TexMod_TexGenType", pTextureModifier.m_eTGType);
    modNode->getAttr("TexMod_bTexGenProjected", pTextureModifier.m_bTexGenProjected);

    for (int baseu = 'U', u = baseu; u <= 'W'; u++)
    {
        char RT[] = "Rotate?";
        RT[6] = u;

        if (modNode->getAttr(RT, f))
        {
            pTextureModifier.m_Rot            [u - baseu] = Degr2Word(f);
        }

        char RR[] = "TexMod_?RotateRate";
        RR[7] = u;
        char RP[] = "TexMod_?RotatePhase";
        RP[7] = u;
        char RA[] = "TexMod_?RotateAmplitude";
        RA[7] = u;
        char RC[] = "TexMod_?RotateCenter";
        RC[7] = u;

        if (modNode->getAttr(RR, f))
        {
            pTextureModifier.m_RotOscRate     [u - baseu] = Degr2Word(f);
        }
        if (modNode->getAttr(RP, f))
        {
            pTextureModifier.m_RotOscPhase    [u - baseu] = Degr2Word(f);
        }
        if (modNode->getAttr(RA, f))
        {
            pTextureModifier.m_RotOscAmplitude[u - baseu] = Degr2Word(f);
        }
        if (modNode->getAttr(RC, f))
        {
            pTextureModifier.m_RotOscCenter   [u - baseu] =           f;
        }

        if (u > 'V')
        {
            continue;
        }

        char TL[] = "Tile?";
        TL[4] = u;
        char OF[] = "Offset?";
        OF[6] = u;

        if (modNode->getAttr(TL, f))
        {
            pTextureModifier.m_Tiling         [u - baseu] = f;
        }
        if (modNode->getAttr(OF, f))
        {
            pTextureModifier.m_Offs           [u - baseu] = f;
        }

        char OT[] = "TexMod_?OscillatorType";
        OT[7] = u;
        char OR[] = "TexMod_?OscillatorRate";
        OR[7] = u;
        char OP[] = "TexMod_?OscillatorPhase";
        OP[7] = u;
        char OA[] = "TexMod_?OscillatorAmplitude";
        OA[7] = u;

        if (modNode->getAttr(OT, c))
        {
            pTextureModifier.m_eMoveType      [u - baseu] = c;
        }
        if (modNode->getAttr(OR, f))
        {
            pTextureModifier.m_OscRate        [u - baseu] = f;
        }
        if (modNode->getAttr(OP, f))
        {
            pTextureModifier.m_OscPhase       [u - baseu] = f;
        }
        if (modNode->getAttr(OA, f))
        {
            pTextureModifier.m_OscAmplitude   [u - baseu] = f;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
static SEfTexModificator defaultTexMod;
static bool defaultTexMod_Initialized = false;

void MaterialHelpers::SetXmlFromTexMod(const SEfTexModificator& pTextureModifier, XmlNodeRef& node) const
{
    if (!defaultTexMod_Initialized)
    {
        ZeroStruct(defaultTexMod);
        defaultTexMod.m_Tiling[0] = 1;
        defaultTexMod.m_Tiling[1] = 1;
        defaultTexMod_Initialized = true;
    }

    if (memcmp(&pTextureModifier, &defaultTexMod, sizeof(pTextureModifier)) == 0)
    {
        return;
    }

    XmlNodeRef modNode = node->newChild("TexMod");
    if (modNode)
    {
        // Modificators
        float f;
        uint16 s;
        uint8 c;

        modNode->setAttr("TexMod_RotateType", pTextureModifier.m_eRotType);
        modNode->setAttr("TexMod_TexGenType", pTextureModifier.m_eTGType);
        modNode->setAttr("TexMod_bTexGenProjected", pTextureModifier.m_bTexGenProjected);

        for (int baseu = 'U', u = baseu; u <= 'W'; u++)
        {
            char RT[] = "Rotate?";
            RT[6] = u;

            if ((s = pTextureModifier.m_Rot            [u - baseu]) != defaultTexMod.m_Rot            [u - baseu])
            {
                modNode->setAttr(RT, Word2Degr(s));
            }

            char RR[] = "TexMod_?RotateRate";
            RR[7] = u;
            char RP[] = "TexMod_?RotatePhase";
            RP[7] = u;
            char RA[] = "TexMod_?RotateAmplitude";
            RA[7] = u;
            char RC[] = "TexMod_?RotateCenter";
            RC[7] = u;

            if ((s = pTextureModifier.m_RotOscRate     [u - baseu]) != defaultTexMod.m_RotOscRate     [u - baseu])
            {
                modNode->setAttr(RR, Word2Degr(s));
            }
            if ((s = pTextureModifier.m_RotOscPhase    [u - baseu]) != defaultTexMod.m_RotOscPhase    [u - baseu])
            {
                modNode->setAttr(RP, Word2Degr(s));
            }
            if ((s = pTextureModifier.m_RotOscAmplitude[u - baseu]) != defaultTexMod.m_RotOscAmplitude[u - baseu])
            {
                modNode->setAttr(RA, Word2Degr(s));
            }
            if ((f = pTextureModifier.m_RotOscCenter   [u - baseu]) != defaultTexMod.m_RotOscCenter   [u - baseu])
            {
                modNode->setAttr(RC,           f);
            }

            if (u > 'V')
            {
                continue;
            }

            char TL[] = "Tile?";
            TL[4] = u;
            char OF[] = "Offset?";
            OF[6] = u;

            if ((f = pTextureModifier.m_Tiling         [u - baseu]) != defaultTexMod.m_Tiling         [u - baseu])
            {
                modNode->setAttr(TL, f);
            }
            if ((f = pTextureModifier.m_Offs           [u - baseu]) != defaultTexMod.m_Offs           [u - baseu])
            {
                modNode->setAttr(OF, f);
            }

            char OT[] = "TexMod_?OscillatorType";
            OT[7] = u;
            char OR[] = "TexMod_?OscillatorRate";
            OR[7] = u;
            char OP[] = "TexMod_?OscillatorPhase";
            OP[7] = u;
            char OA[] = "TexMod_?OscillatorAmplitude";
            OA[7] = u;

            if ((c = pTextureModifier.m_eMoveType      [u - baseu]) != defaultTexMod.m_eMoveType      [u - baseu])
            {
                modNode->setAttr(OT, c);
            }
            if ((f = pTextureModifier.m_OscRate        [u - baseu]) != defaultTexMod.m_OscRate        [u - baseu])
            {
                modNode->setAttr(OR, f);
            }
            if ((f = pTextureModifier.m_OscPhase       [u - baseu]) != defaultTexMod.m_OscPhase       [u - baseu])
            {
                modNode->setAttr(OP, f);
            }
            if ((f = pTextureModifier.m_OscAmplitude   [u - baseu]) != defaultTexMod.m_OscAmplitude   [u - baseu])
            {
                modNode->setAttr(OA, f);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialHelpers::SetTexturesFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const
{
    const char* texmap = "";
    const char* fileName = "";

    XmlNodeRef texturesNode = node->findChild("Textures");
    if (texturesNode)
    {
        for (int c = 0; c < texturesNode->getChildCount(); c++)
        {
            XmlNodeRef texNode = texturesNode->getChild(c);
            texmap = texNode->getAttr("Map");

            // [Shader System TO DO] - this must become per shader (and not global) according to the parser
            uint8   texSlot = MaterialHelpers::FindTexSlot(texmap);

            // [Shader System TO DO] - in the new system simply gather texture slot names, then identify name usage 
            // and accordingly match the slot (dynamically associated per shader by the parser).
            if (texSlot == EFTT_UNKNOWN)
            {
                continue;
            }

            fileName = texNode->getAttr("File");

            // legacy.  Some textures used to be referenced using "engine\\" or "engine/" - this is no longer valid
            if (
                (strlen(fileName) > 7) &&
                (azstrnicmp(fileName, "engine", 6) == 0) &&
                ((fileName[6] == '\\') || (fileName[6] == '/'))
                )
            {
                fileName = fileName + 7;
            }

            // legacy:  Files were saved into a mtl with many leading forward or back slashes, we eat them all here.  We want it to start with a rel path.
            const char* actualFileName = fileName;
            while ((actualFileName[0]) && ((actualFileName[0] == '\\') || (actualFileName[0] == '/')))
            {
                ++actualFileName;
            }
            fileName = actualFileName;

            // Next insert the texture resource if did not exist
            TexturesResourcesMap*       pTextureReourcesMap = pShaderResources.GetTexturesResourceMap();
            SEfResTexture*              pTextureRes = &(*pTextureReourcesMap)[texSlot];

            pTextureRes->m_Name = fileName;
            texNode->getAttr("IsTileU", pTextureRes->m_bUTile);
            texNode->getAttr("IsTileV", pTextureRes->m_bVTile);
            texNode->getAttr("TexType", pTextureRes->m_Sampler.m_eTexType);

            int     filter = pTextureRes->m_Filter;
            if (texNode->getAttr("Filter", filter))
            {
                pTextureRes->m_Filter = filter;
            }

            // Next look for modulation node - add it only if exist
            XmlNodeRef  modNode = texNode->findChild("TexMod");
            if (modNode)
                SetTexModFromXml( *(pTextureRes->AddModificator()), modNode);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
static SInputShaderResources    defaultShaderResource;      // for comparison with the default values
static SEfResTexture            defaultTextureResource;     // for comparison with the default values

void MaterialHelpers::SetXmlFromTextures( SInputShaderResources& pShaderResources, XmlNodeRef& node) const
{
    // Save texturing data.
    XmlNodeRef texturesNode = node->newChild("Textures");

    for (auto& iter : *(pShaderResources.GetTexturesResourceMap()) )
    {
        EEfResTextures texId = static_cast<EEfResTextures>(iter.first);
        const SEfResTexture* pTextureRes = &(iter.second);
        if (pTextureRes && !pTextureRes->m_Name.empty() && IsAdjustableTexSlot(texId))
        {
            XmlNodeRef texNode = texturesNode->newChild("Texture");

            texNode->setAttr("Map", MaterialHelpers::LookupTexName(texId));
            texNode->setAttr("File", pTextureRes->m_Name.c_str());

            if (pTextureRes->m_Filter != defaultTextureResource.m_Filter)
            {
                texNode->setAttr("Filter", pTextureRes->m_Filter);
            }
            if (pTextureRes->m_bUTile != defaultTextureResource.m_bUTile)
            {
                texNode->setAttr("IsTileU", pTextureRes->m_bUTile);
            }
            if (pTextureRes->m_bVTile != defaultTextureResource.m_bVTile)
            {
                texNode->setAttr("IsTileV", pTextureRes->m_bVTile);
            }
            if (pTextureRes->m_Sampler.m_eTexType != defaultTextureResource.m_Sampler.m_eTexType)
            {
                texNode->setAttr("TexType", pTextureRes->m_Sampler.m_eTexType);
            }

            //////////////////////////////////////////////////////////////////////////
            // Save texture modificators Modificators
            //////////////////////////////////////////////////////////////////////////
            SetXmlFromTexMod( *pTextureRes->GetModificator(), texNode);
        }
        /* [Shader System] - TO DO: test to see if slots can be removed
        else
        {
            AZ_Assert(!pTextureRes->m_Name.empty(), "Shader resource texture error - Texture exists without a name");
        }
        */
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialHelpers::SetVertexDeformFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const
{
    if (defaultShaderResource.m_DeformInfo.m_eType != pShaderResources.m_DeformInfo.m_eType)
    {
        node->setAttr("vertModifType", pShaderResources.m_DeformInfo.m_eType);
    }

    XmlNodeRef deformNode = node->findChild("VertexDeform");
    if (deformNode)
    {
        int deform_type = eDT_Unknown;
        deformNode->getAttr("Type", deform_type);
        pShaderResources.m_DeformInfo.m_eType = (EDeformType)deform_type;
        deformNode->getAttr("DividerX", pShaderResources.m_DeformInfo.m_fDividerX);
        deformNode->getAttr("NoiseScale", pShaderResources.m_DeformInfo.m_vNoiseScale);

        XmlNodeRef waveX = deformNode->findChild("WaveX");
        if (waveX)
        {
            int type = eWF_None;
            waveX->getAttr("Type", type);
            pShaderResources.m_DeformInfo.m_WaveX.m_eWFType = (EWaveForm)type;
            waveX->getAttr("Amp", pShaderResources.m_DeformInfo.m_WaveX.m_Amp);
            waveX->getAttr("Level", pShaderResources.m_DeformInfo.m_WaveX.m_Level);
            waveX->getAttr("Phase", pShaderResources.m_DeformInfo.m_WaveX.m_Phase);
            waveX->getAttr("Freq", pShaderResources.m_DeformInfo.m_WaveX.m_Freq);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialHelpers::SetXmlFromVertexDeform(const SInputShaderResources& pShaderResources, XmlNodeRef& node) const
{
    int vertModif = pShaderResources.m_DeformInfo.m_eType;
    node->setAttr("vertModifType", vertModif);

    if (pShaderResources.m_DeformInfo.m_eType != eDT_Unknown)
    {
        XmlNodeRef deformNode = node->newChild("VertexDeform");

        deformNode->setAttr("Type", pShaderResources.m_DeformInfo.m_eType);
        deformNode->setAttr("DividerX", pShaderResources.m_DeformInfo.m_fDividerX);
        deformNode->setAttr("NoiseScale", pShaderResources.m_DeformInfo.m_vNoiseScale);

        XmlNodeRef waveX = deformNode->newChild("WaveX");
        waveX->setAttr("Type", pShaderResources.m_DeformInfo.m_WaveX.m_eWFType);
        waveX->setAttr("Amp", pShaderResources.m_DeformInfo.m_WaveX.m_Amp);
        waveX->setAttr("Level", pShaderResources.m_DeformInfo.m_WaveX.m_Level);
        waveX->setAttr("Phase", pShaderResources.m_DeformInfo.m_WaveX.m_Phase);
        waveX->setAttr("Freq", pShaderResources.m_DeformInfo.m_WaveX.m_Freq);

    }
}

//////////////////////////////////////////////////////////////////////////
static inline ColorF ToCFColor(const Vec3& col)
{
    return ColorF(col);
}

void MaterialHelpers::SetLightingFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const
{
    // Load lighting data.
    Vec3 vColor;
    Vec4 vColor4;
    if (node->getAttr("Diffuse", vColor4))
    {
        pShaderResources.m_LMaterial.m_Diffuse = ColorF(vColor4.x, vColor4.y, vColor4.z, vColor4.w);
    }
    else if (node->getAttr("Diffuse", vColor))
    {
        pShaderResources.m_LMaterial.m_Diffuse = ToCFColor(vColor);
    }
    if (node->getAttr("Specular", vColor4))
    {
        pShaderResources.m_LMaterial.m_Specular = ColorF(vColor4.x, vColor4.y, vColor4.z, vColor4.w);
    }
    else if (node->getAttr("Specular", vColor))
    {
        pShaderResources.m_LMaterial.m_Specular = ToCFColor(vColor);
    }
    if (node->getAttr("Emittance", vColor4))
    {
        pShaderResources.m_LMaterial.m_Emittance = ColorF(vColor4.x, vColor4.y, vColor4.z, vColor4.w);
    }

    node->getAttr("Shininess", pShaderResources.m_LMaterial.m_Smoothness);
    node->getAttr("Opacity", pShaderResources.m_LMaterial.m_Opacity);
    node->getAttr("AlphaTest", pShaderResources.m_AlphaRef);
    node->getAttr("VoxelCoverage", pShaderResources.m_VoxelCoverage);
}

//////////////////////////////////////////////////////////////////////////
static inline Vec3 ToVec3(const ColorF& col)
{
    return Vec3(col.r, col.g, col.b);
}

static inline Vec4 ToVec4(const ColorF& col)
{
    return Vec4(col.r, col.g, col.b, col.a);
}

void MaterialHelpers::SetXmlFromLighting(const SInputShaderResources& pShaderResources, XmlNodeRef& node) const
{
    // Save ligthing data.
    if (defaultShaderResource.m_LMaterial.m_Diffuse != pShaderResources.m_LMaterial.m_Diffuse)
    {
        node->setAttr("Diffuse", ToVec4(pShaderResources.m_LMaterial.m_Diffuse));
    }
    if (defaultShaderResource.m_LMaterial.m_Specular != pShaderResources.m_LMaterial.m_Specular)
    {
        node->setAttr("Specular", ToVec4(pShaderResources.m_LMaterial.m_Specular));
    }
    if (defaultShaderResource.m_LMaterial.m_Emittance != pShaderResources.m_LMaterial.m_Emittance)
    {
        node->setAttr("Emittance", ToVec4(pShaderResources.m_LMaterial.m_Emittance));
    }

    if (defaultShaderResource.m_LMaterial.m_Opacity != pShaderResources.m_LMaterial.m_Opacity)
    {
        node->setAttr("Opacity", pShaderResources.m_LMaterial.m_Opacity);
    }
    if (defaultShaderResource.m_LMaterial.m_Smoothness != pShaderResources.m_LMaterial.m_Smoothness)
    {
        node->setAttr("Shininess", pShaderResources.m_LMaterial.m_Smoothness);
    }

    if (defaultShaderResource.m_AlphaRef != pShaderResources.m_AlphaRef)
    {
        node->setAttr("AlphaTest", pShaderResources.m_AlphaRef);
    }
    if (defaultShaderResource.m_VoxelCoverage != pShaderResources.m_VoxelCoverage)
    {
        node->setAttr("VoxelCoverage", pShaderResources.m_VoxelCoverage);
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialHelpers::SetShaderParamsFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const
{
    int nA = node->getNumAttributes();
    if (!nA)
    {
        return;
    }

    for (int i = 0; i < nA; i++)
    {
        const char* key = NULL, * val = NULL;
        node->getAttributeByIndex(i, &key, &val);

        // try to set existing param first
        bool bFound = false;

        for (int j = 0; j < pShaderResources.m_ShaderParams.size(); j++)
        {
            SShaderParam* pParam = &pShaderResources.m_ShaderParams[j];

            if (pParam->m_Name == key)
            {
                bFound = true;

                switch (pParam->m_Type)
                {
                case eType_BYTE:
                    node->getAttr(key, pParam->m_Value.m_Byte);
                    break;
                case eType_SHORT:
                    node->getAttr(key, pParam->m_Value.m_Short);
                    break;
                case eType_INT:
                    node->getAttr(key, pParam->m_Value.m_Int);
                    break;
                case eType_FLOAT:
                    node->getAttr(key, pParam->m_Value.m_Float);
                    break;
                case eType_FCOLOR:
                case eType_FCOLORA:
                {
                    Vec3 vValue;
                    node->getAttr(key, vValue);

                    pParam->m_Value.m_Color[0] = vValue.x;
                    pParam->m_Value.m_Color[1] = vValue.y;
                    pParam->m_Value.m_Color[2] = vValue.z;
                }
                break;
                case eType_VECTOR:
                {
                    Vec4 vValue;
                    if (node->getAttr(key, vValue))
                    {
                        pParam->m_Value.m_Color[0] = vValue.x;
                        pParam->m_Value.m_Color[1] = vValue.y;
                        pParam->m_Value.m_Color[2] = vValue.z;
                        pParam->m_Value.m_Color[3] = vValue.w;
                    }
                    else
                    {
                        Vec3 vValue3;
                        if (node->getAttr(key, vValue3))
                        {
                            pParam->m_Value.m_Color[0] = vValue3.x;
                            pParam->m_Value.m_Color[1] = vValue3.y;
                            pParam->m_Value.m_Color[2] = vValue3.z;
                            pParam->m_Value.m_Color[3] = 1.0f;
                        }
                    }
                }
                break;
                default:
                    break;
                }
            }
        }

        if (!bFound)
        {
            assert(val && key);

            SShaderParam Param;
            Param.m_Name = key;
            Param.m_Value.m_Color[0] = Param.m_Value.m_Color[1] = Param.m_Value.m_Color[2] = Param.m_Value.m_Color[3] = 0;

            int res = azsscanf(val, "%f,%f,%f,%f", &Param.m_Value.m_Color[0], &Param.m_Value.m_Color[1], &Param.m_Value.m_Color[2], &Param.m_Value.m_Color[3]);
            assert(res);

            pShaderResources.m_ShaderParams.push_back(Param);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void MaterialHelpers::SetXmlFromShaderParams(const SInputShaderResources& pShaderResources, XmlNodeRef& node) const
{
    for (int i = 0; i < pShaderResources.m_ShaderParams.size(); i++)
    {
        const SShaderParam* pParam = &pShaderResources.m_ShaderParams[i];
        switch (pParam->m_Type)
        {
        case eType_BYTE:
            node->setAttr(pParam->m_Name.c_str(), (int)pParam->m_Value.m_Byte);
            break;
        case eType_SHORT:
            node->setAttr(pParam->m_Name.c_str(), (int)pParam->m_Value.m_Short);
            break;
        case eType_INT:
            node->setAttr(pParam->m_Name.c_str(), (int)pParam->m_Value.m_Int);
            break;
        case eType_FLOAT:
            node->setAttr(pParam->m_Name.c_str(), (float)pParam->m_Value.m_Float);
            break;
        case eType_FCOLOR:
            node->setAttr(pParam->m_Name.c_str(), Vec3(pParam->m_Value.m_Color[0], pParam->m_Value.m_Color[1], pParam->m_Value.m_Color[2]));
            break;
        case eType_VECTOR:
            node->setAttr(pParam->m_Name.c_str(), Vec3(pParam->m_Value.m_Vector[0], pParam->m_Value.m_Vector[1], pParam->m_Value.m_Vector[2]));
            break;
        default:
            break;
        }
    }
}

//------------------------------------------------------------------------------
// [Shader System TO DO] - the following function supports older version of data 
// and converts them.
// This needs to go away soon!
//------------------------------------------------------------------------------
void MaterialHelpers::MigrateXmlLegacyData(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const
{
    float glowAmount;

    // Migrate glow from 3.8.3 to emittance
    if (node->getAttr("GlowAmount", glowAmount) && glowAmount > 0)
    {
        SEfResTexture*     pTextureRes = pShaderResources.GetTextureResource(EFTT_DIFFUSE);
        if (pTextureRes && (pTextureRes->m_Sampler.m_eTexType == eTT_2D))
        {
            // The following line will create and insert a new texture data slot if did not exist.
            pShaderResources.m_TexturesResourcesMap[EFTT_EMITTANCE].m_Name = pTextureRes->m_Name;
        }
        const float legacyHDRDynMult = 2.0f;
        const float legacyIntensityScale = 10.0f;  // Legacy scale factor 10000 divided by 1000 for kilonits

        // Clamp this at EMISSIVE_INTENSITY_SOFT_MAX because some previous glow parameters become extremely bright.
        pShaderResources.m_LMaterial.m_Emittance.a = min(powf(glowAmount * legacyHDRDynMult, legacyHDRDynMult) * legacyIntensityScale, EMISSIVE_INTENSITY_SOFT_MAX);

        std::string materialName = node->getAttr("Name");
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Material %s has had legacy GlowAmount automatically converted to Emissive Intensity.  The material parameters related to Emittance should be manually adjusted for this material.", materialName.c_str());
    }
    
    // In Lumberyard version 1.9 BlendLayer2Specular became a color instead of a single float, so it needs to be updated
    XmlNodeRef publicParamsNode = node->findChild("PublicParams");
    if (publicParamsNode && publicParamsNode->haveAttr("BlendLayer2Specular"))
    {
        // Check to see if the BlendLayer2Specular is a float
        AZStd::string blendLayer2SpecularString(publicParamsNode->getAttr("BlendLayer2Specular"));

        // If there are no commas in the string representation, it must be a single float instead of a color
        if (blendLayer2SpecularString.find(',') == AZStd::string::npos)
        {
            float blendLayer2SpecularFloat = 0.0f;
            publicParamsNode->getAttr("BlendLayer2Specular", blendLayer2SpecularFloat);
            publicParamsNode->setAttr("BlendLayer2Specular", Vec4(blendLayer2SpecularFloat, blendLayer2SpecularFloat, blendLayer2SpecularFloat, 0.0));
        }
    }
}
