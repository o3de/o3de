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

// Description : Font class.


#ifndef CRYINCLUDE_CRYFONT_FFONT_H
#define CRYINCLUDE_CRYFONT_FFONT_H
#pragma once


#if !defined(USE_NULLFONT_ALWAYS)

#include <vector>
#include <Cry_Math.h>
#include <Cry_Color.h>
#include <CryString.h>
#include "CryFont.h"

#include <AzCore/std/parallel/mutex.h>

struct ISystem;
class CFontTexture;


class CFFont
    : public IFFont
    , public IFFont_RenderProxy
{
public:
    //! Determines how characters of different sizes should be handled during render.
    enum class SizeBehavior
    {
        Scale,      //!< Default behavior; glyphs rendered at different sizes are rendered on scaled geometry
        Rerender    //!< Similar to Scale, but the glyph in the font texture is re-rendered to match the target
                    //!< size, as long as the size isn't greater than the maximum glyph/slot resolution as
                    //!< configured for the font texture in the font XML.
    };

    //! The hinting visual algorithm to be used (when hinting is enabled)
    enum class HintStyle
    {
        Normal, //!< Default hinting behavior provided by font renderer
        Light   //!< Produces fuzzier glyphs but more accurately tracks glyph shape
    };

    //! Chooses whether hinting info should be obtained from the font, turned off entirely, or automatically generated
    enum class HintBehavior
    {
        Default,    //!< Obtain hinting data from font itself
        AutoHint,   //!< Procedurally derive hinting information from glyph
        NoHinting,  //!< Disable hinting entirely
    };

    //! Simple struct used to communicate font hinting parameters to font renderer.
    struct FontHintParams
    {
        FontHintParams() : hintStyle(HintStyle::Normal), hintBehavior(HintBehavior::Default) { }

        HintStyle hintStyle;
        HintBehavior hintBehavior;
    };

    struct SRenderingPass
    {
        ColorB m_color;
        Vec2 m_posOffset;
        int m_blendSrc;
        int m_blendDest;

        SRenderingPass()
            : m_color(255, 255, 255, 255)
            , m_posOffset(0, 0)
            , m_blendSrc(GS_BLSRC_SRCALPHA)
            , m_blendDest(GS_BLDST_ONEMINUSSRCALPHA)
        {
        }

        void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}
    };

    struct SEffect
    {
        string m_name;
        std::vector<SRenderingPass> m_passes;

        SEffect(const char* name)
            : m_name(name)
        {
            assert(name);
        }

        SRenderingPass* AddPass()
        {
            m_passes.push_back(SRenderingPass());
            return &m_passes[m_passes.size() - 1];
        }

        void ClearPasses()
        {
            m_passes.resize(0);
        }

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(m_name);
            pSizer->AddObject(m_passes);
        }
    };

    typedef std::vector<SEffect> Effects;
    typedef Effects::iterator EffectsIt;

public:
    virtual int32 AddRef() override;
    virtual int32 Release() override;
    virtual bool Load(const char* pFontFilePath, unsigned int width, unsigned int height, unsigned int widthNumSlots, unsigned int heightNumSlots, unsigned int flags, float sizeRatio);
    virtual bool Load(const char* pXMLFile);
    virtual void Free();
    virtual void DrawString(float x, float y, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    virtual void DrawString(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    virtual Vec2 GetTextSize(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    virtual size_t GetTextLength(const char* pStr, const bool asciiMultiLine) const;
    virtual void WrapText(string& result, float maxWidth, const char* pStr, const STextDrawContext& ctx);
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const;
    virtual unsigned int GetEffectId(const char* pEffectName) const;
    virtual unsigned int GetNumEffects() const;
    virtual const char* GetEffectName(unsigned int effectId) const;
    virtual Vec2 GetMaxEffectOffset(unsigned int effectId) const;
    virtual bool DoesEffectHaveTransparency(unsigned int effectId) const;
    virtual void AddCharsToFontTexture(const char* pChars, int glyphSizeX = ICryFont::defaultGlyphSizeX, int glyphSizeY = ICryFont::defaultGlyphSizeY) override;
    virtual Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph, const STextDrawContext& ctx) const override;
    virtual float GetAscender(const STextDrawContext& ctx) const override;
    virtual float GetBaseline(const STextDrawContext& ctx) const override;

    virtual uint32 GetNumQuadsForText(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    virtual uint32 WriteTextQuadsToBuffers(SVF_P2F_C4B_T2F_F4B* verts, uint16* indices, uint32 maxQuads, float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    virtual int GetFontTextureId();
    virtual uint32 GetFontTextureVersion();

    virtual float GetSizeRatio() const override { return m_sizeRatio; }

public:
    virtual void RenderCallback(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);

public:
    CFFont(ISystem* pSystem, CCryFont* pCryFont, const char* pFontName);

    bool InitTexture();
    bool InitCache();

    CFontTexture* GetFontTexture() const { return m_pFontTexture; }
    const string& GetName() const { return m_name; }

    SEffect* AddEffect(const char* pEffectName);
    SEffect* GetDefaultEffect();

private:
    virtual ~CFFont();

    void Prepare(const char* pStr, bool updateTexture, const Vec2i& glyphSize = CCryFont::defaultGlyphSize);
    void DrawStringUInternal(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);
    Vec2 GetTextSizeUInternal(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx);

    using AddFunction = AZStd::function<void(const Vec3&, const Vec3&, const Vec3&, const Vec3&, const Vec2&, const Vec2&, const Vec2&, const Vec2&, uint32)>;
    using BeginPassFunction = AZStd::function<void(const SRenderingPass* pPass)>;
    
    //! This function is used by both RenderCallback and WriteTextQuadsToBuffers
    //! To do this is takes two function pointers that implement the appropriate AddQuad and BeginPass behavior
    void CreateQuadsForText(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx,
        AddFunction AddQuad, BeginPassFunction BeginPass);

    struct TextScaleInfoInternal
    {
        TextScaleInfoInternal(const Vec2& _scale, float _rcpCellWidth)
            : scale(_scale), rcpCellWidth(_rcpCellWidth) { }

        Vec2 scale;
        float rcpCellWidth;
    };

    TextScaleInfoInternal CalculateScaleInternal(const STextDrawContext& ctx) const;

    Vec2 GetRestoredFontSize(const STextDrawContext& ctx) const;

private:
    string m_name;
    string m_curPath;

    CFontTexture* m_pFontTexture;

    size_t m_fontBufferSize;
    unsigned char* m_pFontBuffer;

    int m_texID;    
    uint32 m_textureVersion;

    ISystem* m_pSystem;

    AZStd::recursive_mutex m_fontMutex; //!< Controls access between main and render threads. It's common for one thread
                                        //!< to add un-cached glyphs to the font texture while another is accessing the
                                        //!< font texture.

    CCryFont* m_pCryFont;

    bool m_fontTexDirty;

    Effects m_effects;

    SVF_P3F_C4B_T2F* m_pDrawVB;

    volatile int32 m_nRefCount;

    bool m_monospacedFont; //!< True if this font is fixed/monospaced, false otherwise (obtained from FreeType)

    float m_sizeRatio = IFFontConstants::defaultSizeRatio;
    SizeBehavior m_sizeBehavior = SizeBehavior::Scale;   //!< Changes how glyphs rendered at different sizes are rendered.
    FontHintParams m_fontHintParams; //!< How the font should be hinted when its loaded and rendered to the font texture
};

#endif

#endif // CRYINCLUDE_CRYFONT_FFONT_H
