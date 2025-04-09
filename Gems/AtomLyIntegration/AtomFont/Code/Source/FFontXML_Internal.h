/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#if !defined(USE_NULLFONT_ALWAYS)

#include <AtomLyIntegration/AtomFont/FFont.h>
#include <AtomLyIntegration/AtomFont/FontTexture.h>
#include <CryCommon/Cry_Math.h>
#include <CryCommon/CryPath.h>
#include <AzCore/PlatformIncl.h>

//////////////////////////////////////////////////////////////////////////
// Xml parser implementation

namespace AtomFontInternal
{
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

    inline int GetBlendModeFromString(const AZStd::string& str, bool dst)
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

    inline int CreateTTFFontFlag(AZ::FontSmoothMethod smoothMethod, AZ::FontSmoothAmount smoothAmount)
    {
        return (
                ((static_cast<int>(smoothMethod) << TTFFLAG_SMOOTH_SHIFT) & TTFFLAG_SMOOTH_MASK) | 
                ((static_cast<int>(smoothAmount) << TTFFLAG_SMOOTH_AMOUNT_SHIFT) & TTFFLAG_SMOOTH_AMOUNT_MASK)
            );
    }

    inline AZ::FontSmoothMethod TranslateSmoothMethod(const AZStd::string& value)
    {
        AZ::FontSmoothMethod smoothMethod = AZ::FontSmoothMethod::None;
        if (value == "blur")
        {
            smoothMethod = AZ::FontSmoothMethod::Blur;
        }
        else if (value == "supersample")
        {
            smoothMethod = AZ::FontSmoothMethod::SuperSample;
        }
        return smoothMethod;
    }

    inline AZ::FontSmoothAmount TranslateSmoothAmount(int value)
    {
        AZ::FontSmoothAmount smoothAmount = AZ::FontSmoothAmount::None;
        if (value == 1)
        {
            smoothAmount = AZ::FontSmoothAmount::x2;
        }
        else if (value > 1)
        {
            smoothAmount = AZ::FontSmoothAmount::x4;
        }
        return smoothAmount;
    }

    class XmlFontShader
    {
        static const int DefaultSlotWidthSize = 16;
        static const int DefaultSlotHeightSize = 8;
    public:
        XmlFontShader(AZ::FFont* font)
        : m_font(font)
        , m_nElement(ELEMENT_UNKNOWN)
        , m_slotSizes(DefaultSlotWidthSize, DefaultSlotHeightSize)
        , m_effect(NULL)
        , m_pass(NULL)
        , m_FontTexSize(0, 0)
        , m_FontSmoothAmount(AZ::FontSmoothAmount::None)
        , m_FontSmoothMethod(AZ::FontSmoothMethod::None)
        {
        }

        ~XmlFontShader()
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
        void FoundElementImpl();

        // notify methods
        void FoundElement(const AZStd::string& name)
        {
            // process the previous element
            switch (m_nElement)
            {
            case ELEMENT_FONT:
            {
                if (!m_FontTexSize.x || !m_FontTexSize.y)
                {
                    m_FontTexSize.set(512, 512);
                }

                bool fontLoaded = m_font->Load(
                                            m_strFontPath.c_str(), 
                                            static_cast<unsigned int>(m_FontTexSize.x), 
                                            static_cast<unsigned int>(m_FontTexSize.y), 
                                            m_slotSizes.x, 
                                            m_slotSizes.y, 
                                            CreateTTFFontFlag(m_FontSmoothMethod, m_FontSmoothAmount), 
                                            m_SizeRatio
                                            );
                if (!fontLoaded)
                {
                    FoundElementImpl();
                }
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
                m_pass = NULL;
                m_nElement = ELEMENT_PASS;
                if (m_effect)
                {
                    m_pass = m_effect->AddPass();
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

        void FoundAttribute(const AZStd::string& name, const AZStd::string& value)
        {
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
                    m_slotSizes.x = (int)atoi(value.c_str());
                }
                else if (name == "heightslots")
                {
                    m_slotSizes.y = (int)atoi(value.c_str());
                }
                else if (name == "sizeratio")
                {
                    m_SizeRatio = static_cast<float>(atof(value.c_str()));
                }
                else if (name == "smooth")
                {
                    m_FontSmoothMethod = TranslateSmoothMethod(value);
                }
                else if (name == "smooth_amount")
                {
                    m_FontSmoothAmount = TranslateSmoothAmount((int)atof(value.c_str()));
                }
                break;

            case ELEMENT_EFFECT:
                if (name == "name")
                {
                    if (value == "default")
                    {
                        m_effect = m_font->GetDefaultEffect();
                        m_effect->ClearPasses();
                    }
                    else
                    {
                        m_effect = m_font->AddEffect(value.c_str());
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
                if (!m_pass)
                {
                    break;
                }
                if (name == "r")
                {
                    m_pass->m_color.r = (uint8_t)((float)atof(value.c_str()) * 255.0f);
                }
                else if (name == "g")
                {
                    m_pass->m_color.g = (uint8_t)((float)atof(value.c_str()) * 255.0f);
                }
                else if (name == "b")
                {
                    m_pass->m_color.b = (uint8_t)((float)atof(value.c_str()) * 255.0f);
                }
                else if (name == "a")
                {
                    m_pass->m_color.a = (uint8_t)((float)atof(value.c_str()) * 255.0f);
                }
                break;

            case ELEMENT_PASS_POSOFFSET:
                if (!m_pass)
                {
                    break;
                }
                if (name == "x")
                {
                    m_pass->m_posOffset.x = (float)atoi(value.c_str());
                }
                else if (name == "y")
                {
                    m_pass->m_posOffset.y = (float)atoi(value.c_str());
                }
                break;

            case ELEMENT_PASS_BLEND:
                if (!m_pass)
                {
                    break;
                }
                if (name == "src")
                {
                    m_pass->m_blendSrc = GetBlendModeFromString(value, false);
                }
                else if (name == "dst")
                {
                    m_pass->m_blendDest = GetBlendModeFromString(value, true);
                }
                else if (name == "type")
                {
                    if (value == "modulate")
                    {
                        m_pass->m_blendSrc = GS_BLSRC_SRCALPHA;
                        m_pass->m_blendDest = GS_BLDST_ONEMINUSSRCALPHA;
                    }
                    else if (value == "additive")
                    {
                        m_pass->m_blendSrc = GS_BLSRC_SRCALPHA;
                        m_pass->m_blendDest = GS_BLDST_ONE;
                    }
                }
                break;

            default:
            case ELEMENT_UNKNOWN:
                break;
            }
        }

    public:
        AZ::FFont* m_font;

        unsigned long m_nElement;

        AZ::FFont::FontEffect*  m_effect;
        AZ::FFont::FontRenderingPass* m_pass;

        AZStd::string           m_strFontPath;
        AZStd::string           m_strFontEffectPath;
        vector2l                m_FontTexSize;
        AZ::AtomFont::GlyphSize m_slotSizes;
        float                   m_SizeRatio = IFFontConstants::defaultSizeRatio;

        AZ::FontSmoothMethod    m_FontSmoothMethod;
        AZ::FontSmoothAmount    m_FontSmoothAmount;
    };
}

#endif

