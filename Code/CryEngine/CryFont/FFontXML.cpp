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

// Description : XML parsing to load a font.


#include "CryFont_precompiled.h"

#if !defined(USE_NULLFONT_ALWAYS)

#include "FFont.h"
#include "FontTexture.h"
#include <Cry_Math.h>
#include <CryPath.h>

#if defined(WIN32) || defined(WIN64)
#   include <shlobj.h>
#   include <StringUtils.h>
#endif

//////////////////////////////////////////////////////////////////////////
// Xml parser implementation

enum
{

    ELEMENT_UNKNOWN         = 0,
    ELEMENT_FONT            = 1,
    ELEMENT_EFFECT          = 2,
    ELEMENT_EFFECTFILE      = 3,
    ELEMENT_PASS            = 4,
    ELEMENT_PASS_COLOR      = 5,
    ELEMENT_PASS_POSOFFSET  = 12,
    ELEMENT_PASS_BLEND      = 14
};

static inline int GetBlendModeFromString(const string& str, bool dst)
{
    int blend = GS_BLSRC_ONE;

    if (str == "zero")
    {
        blend = dst ? GS_BLDST_ZERO : GS_BLSRC_ZERO;
    }
    else if (str == "one")
    {
        blend = dst ? GS_BLDST_ONE : GS_BLSRC_ONE;
    }
    else if (str == "srcalpha" ||
             str == "src_alpha")
    {
        blend = dst ? GS_BLDST_SRCALPHA : GS_BLSRC_SRCALPHA;
    }
    else if (str == "invsrcalpha" ||
             str == "inv_src_alpha")
    {
        blend = dst ? GS_BLDST_ONEMINUSSRCALPHA : GS_BLSRC_ONEMINUSSRCALPHA;
    }
    else if (str == "dstalpha" ||
             str == "dst_alpha")
    {
        blend = dst ? GS_BLDST_DSTALPHA : GS_BLSRC_DSTALPHA;
    }
    else if (str == "invdstalpha" ||
             str == "inv_dst_alpha")
    {
        blend = dst ? GS_BLDST_ONEMINUSDSTALPHA : GS_BLSRC_ONEMINUSDSTALPHA;
    }
    else if (str == "dstcolor" ||
             str == "dst_color")
    {
        blend = GS_BLSRC_DSTCOL;
    }
    else if (str == "srccolor" ||
             str == "src_color")
    {
        blend = GS_BLDST_SRCCOL;
    }
    else if (str == "invdstcolor" ||
             str == "inv_dst_color")
    {
        blend = GS_BLSRC_ONEMINUSDSTCOL;
    }
    else if (str == "invsrccolor" ||
             str == "inv_src_color")
    {
        blend = GS_BLDST_ONEMINUSSRCCOL;
    }

    return blend;
}

class CXmlFontShader
{
public:
    CXmlFontShader(CFFont* pFont)
    {
        m_pFont = pFont;
        m_nElement = ELEMENT_UNKNOWN;
        m_pEffect = NULL;
        m_pPass = NULL;
        m_FontTexSize.set(0, 0);

        static const int defaultSlotWidthSize = 16;
        static const int defaultSlotHeightSize = 8;
        m_SlotSizes.set(defaultSlotWidthSize, defaultSlotHeightSize);

        m_FontSmoothAmount = 0;
        m_FontSmoothMethod = FONT_SMOOTH_NONE;
    }

    ~CXmlFontShader()
    {
    }

    void ScanXmlNodesRecursively(XmlNodeRef node)
    {
        if (!node)
        {
            return;
        }

        FoundElement(node->getTag());

        for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
        {
            const char* key = "";
            const char* value = "";
            if (node->getAttributeByIndex(i, &key, &value))
            {
                FoundAttribute(key, value);
            }
        }

        for (int i = 0, count = node->getChildCount(); i < count; ++i)
        {
            XmlNodeRef child = node->getChild(i);
            ScanXmlNodesRecursively(child);
        }
    }

private:
    // notify methods
    void FoundElement(const string& name)
    {
        //MessageBox(NULL, string("[" + name + "]").c_str(), "FoundElement", MB_OK);
        // process the previous element
        switch (m_nElement)
        {
        case ELEMENT_FONT:
        {
            if (!m_FontTexSize.x || !m_FontTexSize.y)
            {
                m_FontTexSize.set(512, 512);
            }

            bool fontLoaded = m_pFont->Load(m_strFontPath.c_str(), m_FontTexSize.x, m_FontTexSize.y, m_SlotSizes.x, m_SlotSizes.y, TTFFLAG_CREATE(m_FontSmoothMethod, m_FontSmoothAmount), m_SizeRatio);
#if defined(WIN32) || defined(WIN64)
            if (!fontLoaded)
            {
                TCHAR sysFontPath[MAX_PATH];
                if (SUCCEEDED(SHGetFolderPath(0, CSIDL_FONTS, 0, SHGFP_TYPE_DEFAULT, sysFontPath)))
                {
                    const char* pFontPath = m_strFontPath.c_str();
                    const char* pFontName = CryStringUtils::FindFileNameInPath(pFontPath);

                    string newFontPath(sysFontPath);
                    newFontPath += "/";
                    newFontPath += pFontName;
                    m_pFont->Load(newFontPath, m_FontTexSize.x, m_FontTexSize.y, m_SlotSizes.x, m_SlotSizes.y, TTFFLAG_CREATE(m_FontSmoothMethod, m_FontSmoothAmount), m_SizeRatio);
                }
            }
#endif
        }
        break;

        default:
            break;
        }

        // Translate the m_nElement name to a define
        if (name == "font")
        {
            m_nElement = ELEMENT_FONT;
        }
        else if (name == "effect")
        {
            m_nElement = ELEMENT_EFFECT;
        }
        else if (name == "effectfile")
        {
            m_nElement = ELEMENT_EFFECTFILE;
        }
        else if (name == "pass")
        {
            m_pPass = NULL;
            m_nElement = ELEMENT_PASS;
            if (m_pEffect)
            {
                m_pPass = m_pEffect->AddPass();
            }
        }
        else if (name == "color")
        {
            m_nElement = ELEMENT_PASS_COLOR;
        }
        else if (name == "pos" ||
                 name == "offset")
        {
            m_nElement = ELEMENT_PASS_POSOFFSET;
        }
        else if (name == "blend" ||
                 name == "blending")
        {
            m_nElement = ELEMENT_PASS_BLEND;
        }
        else
        {
            m_nElement = ELEMENT_UNKNOWN;
        }
    }

    void FoundAttribute(const string& name, const string& value)
    {
        //MessageBox(NULL, string(name + "\n" + value).c_str(), "FoundAttribute", MB_OK);
        switch (m_nElement)
        {
        case ELEMENT_FONT:
            if (name == "path")
            {
                m_strFontPath = value;
            }
            else if (name == "w")
            {
                m_FontTexSize.x = (long)atof(value.c_str());
            }
            else if (name == "h")
            {
                m_FontTexSize.y = (long)atof(value.c_str());
            }
            else if (name == "widthslots")
            {
                m_SlotSizes.x = (int)atoi(value.c_str());
            }
            else if (name == "heightslots")
            {
                m_SlotSizes.y = (int)atoi(value.c_str());
            }
            else if (name == "sizeratio")
            {
                m_SizeRatio = static_cast<float>(atof(value.c_str()));
            }
            else if (name == "smooth")
            {
                if (value == "blur")
                {
                    m_FontSmoothMethod = FONT_SMOOTH_BLUR;
                }
                else if (value == "supersample")
                {
                    m_FontSmoothMethod = FONT_SMOOTH_SUPERSAMPLE;
                }
                else if (value == "none")
                {
                    m_FontSmoothMethod = FONT_SMOOTH_NONE;
                }
            }
            else if (name == "smooth_amount")
            {
                m_FontSmoothAmount = (long)atof(value.c_str());
            }
            break;

        case ELEMENT_EFFECT:
            if (name == "name")
            {
                if (value == "default")
                {
                    m_pEffect = m_pFont->GetDefaultEffect();
                    m_pEffect->ClearPasses();
                }
                else
                {
                    m_pEffect = m_pFont->AddEffect(value.c_str());
                }
            }
            break;

        case ELEMENT_EFFECTFILE:
            if (name == "path")
            {
                m_strFontEffectPath = value;
            }
            break;

        case ELEMENT_PASS_COLOR:
            if (!m_pPass)
            {
                break;
            }
            if (name == "r")
            {
                m_pPass->m_color.r = (uint8)((float)atof(value.c_str()) * 255.0f);
            }
            else if (name == "g")
            {
                m_pPass->m_color.g = (uint8)((float)atof(value.c_str()) * 255.0f);
            }
            else if (name == "b")
            {
                m_pPass->m_color.b = (uint8)((float)atof(value.c_str()) * 255.0f);
            }
            else if (name == "a")
            {
                m_pPass->m_color.a = (uint8)((float)atof(value.c_str()) * 255.0f);
            }
            break;

        case ELEMENT_PASS_POSOFFSET:
            if (!m_pPass)
            {
                break;
            }
            if (name == "x")
            {
                m_pPass->m_posOffset.x = (float)atoi(value.c_str());
            }
            else if (name == "y")
            {
                m_pPass->m_posOffset.y = (float)atoi(value.c_str());
            }
            break;

        case ELEMENT_PASS_BLEND:
            if (!m_pPass)
            {
                break;
            }
            if (name == "src")
            {
                m_pPass->m_blendSrc = GetBlendModeFromString(value, false);
            }
            else if (name == "dst")
            {
                m_pPass->m_blendDest = GetBlendModeFromString(value, true);
            }
            else if (name == "type")
            {
                if (value == "modulate")
                {
                    m_pPass->m_blendSrc = GS_BLSRC_SRCALPHA;
                    m_pPass->m_blendDest = GS_BLDST_ONEMINUSSRCALPHA;
                }
                else if (value == "additive")
                {
                    m_pPass->m_blendSrc = GS_BLSRC_SRCALPHA;
                    m_pPass->m_blendDest = GS_BLDST_ONE;
                }
            }
            break;

        default:
        case ELEMENT_UNKNOWN:
            break;
        }
    }

public:
    CFFont* m_pFont;

    unsigned long m_nElement;

    CFFont::SEffect*        m_pEffect;
    CFFont::SRenderingPass* m_pPass;

    string    m_strFontPath;
    string    m_strFontEffectPath;
    vector2l  m_FontTexSize;
    Vec2i     m_SlotSizes;
    float     m_SizeRatio = IFFontConstants::defaultSizeRatio;

    int       m_FontSmoothMethod;
    int       m_FontSmoothAmount;
};

//////////////////////////////////////////////////////////////////////////
// Main loading function
bool CFFont::Load(const char* pXMLFile)
{
    m_curPath = "";
    if (pXMLFile)
    {
        m_curPath = PathUtil::GetPath(pXMLFile);
    }

    XmlNodeRef root = GetISystem()->LoadXmlFromFile(pXMLFile);
    if (!root)
    {
        return false;
    }

    CXmlFontShader xmlfs(this);
    xmlfs.ScanXmlNodesRecursively(root);

    // if this was not a valid font XML file then return false
    if (!m_pFontTexture || !m_pFontBuffer)
    {
        return false;
    }

    // if there was a font effect file then parse that for effects
    if (!xmlfs.m_strFontEffectPath.empty())
    {
        XmlNodeRef fontEffectRoot = GetISystem()->LoadXmlFromFile(xmlfs.m_strFontEffectPath.c_str());
        if (!fontEffectRoot)
        {
            AZ_Warning("Font", false, "Error parsing font file %s, 'effectfile' pathname %s could not be found.",
                pXMLFile, xmlfs.m_strFontEffectPath.c_str());
            return false;
        }

        if (m_effects.size() > 1 || (m_effects.size() == 1 && (m_effects[0].m_name != "default" || m_effects[0].m_passes.size() > 1)))
        {
            AZ_Warning("Font", false, "Error parsing font file %s, 'effectfile' and 'effect' cannot both be used in the same font file.",
                pXMLFile);
            m_effects.clear();
        }

        // parse the font effects file, adding to this font object
        CXmlFontShader xmlfsEffect(this);
        xmlfsEffect.ScanXmlNodesRecursively(fontEffectRoot);
    }

    return true;
}

#endif

