/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CryFont interface.


#ifndef CRYINCLUDE_CRYCOMMON_IFONT_H
#define CRYINCLUDE_CRYCOMMON_IFONT_H
#pragma once

#include <Cry_Math.h>
#include <Cry_Color.h>
#include <smartptr.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>

struct ISystem;

struct ICryFont;
struct IFFont;
struct FontFamily;

struct SVF_P2F_C4B_T2F_F4B;

extern "C"
#ifdef CRYFONT_EXPORTS
DLL_EXPORT
#else
DLL_IMPORT
#endif
ICryFont * CreateCryFontInterface(ISystem * pSystem);

typedef ICryFont*(* PFNCREATECRYFONTINTERFACE)(ISystem* pSystem);
typedef AZStd::shared_ptr<FontFamily> FontFamilyPtr;

namespace IFFontConstants
{
    //! The default scale applied to individual glyphs when rendering to the font texture.
    //!
    //! This is a "best guess" to try and fit all glyphs of a font within the
    //! bounds of a font texture slot. This value can be defined on a per-font
    //! basis via the "sizeratio" *.font XML attribute.
    const float defaultSizeRatio = 0.8f;
}

//////////////////////////////////////////////////////////////////////////////////////////////
struct ICryFont
{
    static const int defaultGlyphSizeX = -1;    //!< Default glyph size indicates that glyphs in the font texture
                                                //!< should be rendered at the maximum resolution supported by
                                                //!< the font texture's glyph cell/slot configuration (configured
                                                //!< via font XML).

    static const int defaultGlyphSizeY = -1;    //!< Default glyph size indicates that glyphs in the font texture
                                                //!< should be rendered at the maximum resolution supported by
                                                //!< the font texture's glyph cell/slot configuration (configured
                                                //!< via font XML).

    // <interfuscator:shuffle>
    virtual ~ICryFont(){}
    //
    virtual void Release() = 0;
    // Summary:
    //   Creates a named font (case insensitive)
    virtual IFFont* NewFont(const char* pFontName) = 0;
    // Summary:
    //   Gets a named font (case insensitive)
    virtual IFFont* GetFont(const char* pFontName) const = 0;

    //! \brief Loads and initializes a Font Family from a *.fontfamily file
    //! \param pFontFamilyName Name of font family to load (or name of a *.fontfamily file)
    virtual FontFamilyPtr LoadFontFamily(const char* pFontFamilyName) = 0;

    // Summary:
    //   Gets font family (case insensitive)
    virtual FontFamilyPtr GetFontFamily(const char* pFontFamilyName) = 0;

    //! \brief Adds the characters in the given string to all of the font textures within the font family.
    //! All font styles within the given font family (bold, italic, etc.) will have the given 
    //! characters added to their font textures.
    //!
    //! \param pFontFamily  The font family to add the characters to.
    //! \param pChars       String of characters to add to font textures (UTF-8 supported).
    //! \param glyphSizeX   Width (in pixels) of the characters to be rendered at in the font texture.
    //! \param glyphSizeY   Height (in pixels) of the characters to be rendered at in the font texture.
    virtual void AddCharsToFontTextures(FontFamilyPtr pFontFamily, const char* pChars, int glyphSizeX = defaultGlyphSizeX, int glyphSizeY = defaultGlyphSizeY) = 0;

    // Summary:
    //   All font names separated by ,
    // Example:
    //   "console,default,hud"
    virtual AZStd::string GetLoadedFontNames() const = 0;

    //! \brief Called when the g_language (current language) setting changes.
    //!
    //! Mainly used to reload font family resources for the new language.
    virtual void OnLanguageChanged() = 0;
    
    //! \brief Reload all fonts
    virtual void ReloadAllFonts() = 0;

    // </interfuscator:shuffle>
};

//////////////////////////////////////////////////////////////////////////////////////////////
#define TTFFLAG_SMOOTH_NONE                 0x00000000      // No smooth.
#define TTFFLAG_SMOOTH_BLUR                 0x00000001      // Smooth by blurring it.
#define TTFFLAG_SMOOTH_SUPERSAMPLE  0x00000002      // Smooth by rendering the characters into a bigger texture, and then resize it to the normal size using bilinear filtering.

#define TTFFLAG_SMOOTH_MASK                 0x0000000f      // Mask for retrieving.
#define TTFFLAG_SMOOTH_SHIFT                0                           // Shift amount for retrieving.

#define TTFLAG_SMOOTH_AMOUNT_2X         0x00010000      // Blur / supersample [2x]
#define TTFLAG_SMOOTH_AMOUNT_4X         0x00020000      // Blur / supersample [4x]

#define TTFFLAG_SMOOTH_AMOUNT_MASK  0x000f0000      // Mask for retrieving.
#define TTFFLAG_SMOOTH_AMOUNT_SHIFT 16                      // Shift amount for retrieving.


#define TTFFLAG_CREATE(smooth, amount)      ((((smooth) << TTFFLAG_SMOOTH_SHIFT) & TTFFLAG_SMOOTH_MASK) | (((amount) << TTFFLAG_SMOOTH_AMOUNT_SHIFT) & TTFFLAG_SMOOTH_AMOUNT_MASK))
#define TTFFLAG_GET_SMOOTH(flag)                    (((flag) & TTFLAG_SMOOTH_MASK) >> TTFFLAG_SMOOTH_SHIFT)
#define TTFFLAG_GET_SMOOTH_AMOUNT(flag)     (((flag) & TTFLAG_SMOOTH_SMOUNT_MASK) >> TTFFLAG_SMOOTH_AMOUNT_SHIFT)

//////////////////////////////////////////////////////////////////////////
struct STextDrawContext
{
    unsigned int m_fxIdx;

    Vec2 m_size;
    Vec2i m_requestSize;
    float m_widthScale;
    float m_lineSpacing;

    float m_clipX;
    float m_clipY;
    float m_clipWidth;
    float m_clipHeight;

    int m_drawTextFlags;

    bool m_proportional;
    bool m_sizeIn800x600;
    bool m_clippingEnabled;
    bool m_framed;

    ColorB m_colorOverride;

    Matrix34 m_transform;

    int m_baseState;
    bool m_overrideViewProjMatrices;
    bool m_kerningEnabled;
    bool m_processSpecialChars;
    bool m_pixelAligned;                    //!< toggles whether rendering is pixel aligned

    float m_tracking;                       //!< extra space between characters in pixels (prior to any transform)

    STextDrawContext()
        : m_fxIdx(0)
        , m_size(16.0f, 16.0f)
        , m_requestSize(static_cast<int32>(m_size.x), static_cast<int32>(m_size.y))
        , m_widthScale(1.0f)
        , m_lineSpacing(0.f)
        , m_clipX(0)
        , m_clipY(0)
        , m_clipWidth(0)
        , m_clipHeight(0)
        , m_proportional(true)
        , m_sizeIn800x600(true)
        , m_clippingEnabled(false)
        , m_framed(false)
        , m_colorOverride(0, 0, 0, 0)
        , m_drawTextFlags(0)
        , m_transform(IDENTITY)
        , m_baseState(-1) // indicates not set, would like to set to GS_DEPTHFUNC_LEQUAL but header dependencies preclude that
        , m_overrideViewProjMatrices(true) // the old behavior that overrides the currently set view and projection matrices
        , m_kerningEnabled(true)
        , m_processSpecialChars(true)
        , m_pixelAligned(true)
        , m_tracking(0.0f)
    {
    }

    void Reset() { *this = STextDrawContext(); }
    void SetEffect(unsigned int fxIdx) { m_fxIdx = fxIdx; }
    void SetSize(const Vec2& size) { m_size = size; }
    void SetCharWidthScale(float widthScale) { m_widthScale = widthScale; }
    void SetClippingRect(float x, float y, float width, float height) { m_clipX = x; m_clipY = y; m_clipWidth = width; m_clipHeight = height; }
    void SetProportional(bool proportional) { m_proportional = proportional; }
    void SetSizeIn800x600(bool sizeIn800x600) { m_sizeIn800x600 = sizeIn800x600; }
    void EnableClipping(bool enable) { m_clippingEnabled = enable; }
    void EnableFrame(bool enable) { m_framed = enable; }
    void SetColor(const ColorF& col) { m_colorOverride = col; }
    void SetFlags(int flags) { m_drawTextFlags = flags; }
    void SetTransform(const Matrix34& transform) { m_transform = transform; }
    void SetBaseState(int baseState) { m_baseState = baseState; }
    void SetOverrideViewProjMatrices(bool overrideViewProjMatrices) { m_overrideViewProjMatrices = overrideViewProjMatrices; }
    void SetLineSpacing(float lineSpacing) { m_lineSpacing = lineSpacing; }

    float GetCharWidth() const { return m_size.x; }
    float GetCharHeight() const { return m_size.y; }
    float GetCharWidthScale() const { return m_widthScale; }
    int GetFlags() const { return m_drawTextFlags; }
    float GetLineSpacing() const { return m_lineSpacing; }

    bool IsColorOverridden() const { return m_colorOverride.a != 0; }
};

//////////////////////////////////////////////////////////////////////////////////////////////
struct IFFont
{
    // <interfuscator:shuffle>
    virtual ~IFFont() {}

    // We don't derive from CBaseResource currently, that could be an enhancement
    virtual int32 AddRef() = 0;
    virtual int32 Release() = 0;

    //! \brief Loads a font from a TTF file.
    //! \param pFontFilePath Path to font file
    //! \param width Desired width of font texture.
    //! \param height Desired width of font texture.
    //! \param widthNumSlots Number of character slots across the width (X-axis) of the font texture
    //! \param heightNumSlots Number of character slots across the height (Y-axis) of the font texture
    //! \param flags Flags governing font, such as smoothness (see TTFFLAG_CREATE).
    //! \param sizeRatio Scale to apply to the font size when storing glyphs in font texture slots.
    virtual bool Load(const char* pFontFilePath, unsigned int width, unsigned int height, unsigned int widthNumSlots, unsigned int heightNumSlots, unsigned int flags, float sizeRatio) = 0;

    //! \brief Loads a font from a XML file.
    //! \param pXMLFile Path to font XML file
    virtual bool Load(const char* pXMLFile) = 0;

    // Summary:
    //   Frees internally memory internally allocated by Load().
    virtual void Free() = 0;

    // Summary:
    //   Draws a formatted string (UTF-8).
    virtual void DrawString(float x, float y, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) = 0;

    // Summary:
    //   Draws a formatted string (UTF-8), taking z into account.
    virtual void DrawString(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) = 0;

    // Summary:
    //   Computes the text size (UTF-8).
    virtual Vec2 GetTextSize(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) = 0;

    // Description:
    //   Computes virtual text-length (UTF-8) (because of special chars...).
    virtual size_t GetTextLength(const char* pStr, const bool asciiMultiLine) const = 0;

    // Description:
    //   Wraps text based on specified maximum line width (UTF-8)
    virtual void WrapText(AZStd::string& result, float maxWidth, const char* pStr, const STextDrawContext& ctx) = 0;

    // Description:
    //   useful for special feature rendering interleaved with fonts (e.g. box behind the text)
    virtual void GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const = 0;

    virtual unsigned int GetEffectId(const char* pEffectName) const = 0;
    virtual unsigned int GetNumEffects() const = 0;
    virtual const char* GetEffectName(unsigned int effectId) const = 0;
    virtual Vec2 GetMaxEffectOffset(unsigned int effectId) const = 0;
    virtual bool DoesEffectHaveTransparency(unsigned int effectId) const = 0;

    //! \brief Adds the given UTF-8 string of chars to this font's font texture.
    //! \param pChars String of UTF-8 chars to add to font texture
    //! \param glyphSizeX   Width (in pixels) of the characters to be rendered at in the font texture.
    //! \param glyphSizeY   Height (in pixels) of the characters to be rendered at in the font texture.
    virtual void AddCharsToFontTexture(const char* pChars, int glyphSizeX = ICryFont::defaultGlyphSizeX, int glyphSizeY = ICryFont::defaultGlyphSizeY) = 0;

    //! \brief Returns XY kerning offsets (positive or negative) for two given glyphs.
    //!
    //! Kerning values are only returned for fonts that have a 'kern' table defined.
    //! Even fonts that do have a 'kern' defined do not define kerning values for all
    //! possible combination of characters. Zero values will be returned for those
    //! cases.
    virtual Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph, const STextDrawContext& ctx) const = 0;

    //! \brief Returns the ascender of the font
    virtual float GetAscender(const STextDrawContext& ctx) const = 0;

    //! \brief Returns the y offset from the top to the baseline
    virtual float GetBaseline(const STextDrawContext& ctx) const = 0;

    //! \brief Returns the scaling applied to glyphs before being rendered to the font texture from FreeType.
    virtual float GetSizeRatio() const = 0;

    //! \brief Get the number of quads required to render the text string
    //! This is an upper limit. Due to clipping the rendering of the text string might actually use less quads.
    virtual uint32 GetNumQuadsForText(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) = 0;

    //! \brief Write the quads for the text into the given vertex and index buffers
    //! The actual number of quads written to the buffer is returned
    virtual uint32 WriteTextQuadsToBuffers(SVF_P2F_C4B_T2F_F4B* verts, uint16* indices, uint32 maxQuads,
        float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) = 0;

    //! \brief get the font texture for this font, will return -1 if there is no valid font texture
    virtual int GetFontTextureId() = 0;

    //! \brief get the font texture version for this font. Incremented each time the texture is changed
    virtual uint32 GetFontTextureVersion() = 0;

    // </interfuscator:shuffle>
};

//////////////////////////////////////////////////////////////////////////////////////////////
//! Correlates several font files to define a family of fonts, specifically for styling.
struct FontFamily
{
    FontFamily()
        : normal(nullptr)
        , bold(nullptr)
        , italic(nullptr)
        , boldItalic(nullptr)
    {
    }

    FontFamily(const FontFamily&) = delete;
    FontFamily(const FontFamily&&) = delete;
    FontFamily& operator=(const FontFamily&) = delete;
    FontFamily& operator=(const FontFamily&&) = delete;

    AZStd::string familyName;
    IFFont* normal;
    IFFont* bold;
    IFFont* italic;
    IFFont* boldItalic;

};

//////////////////////////////////////////////////////////////////////////
struct IFFont_RenderProxy
{
    // <interfuscator:shuffle>
    virtual ~IFFont_RenderProxy() {}
    virtual void RenderCallback(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) = 0;
    // </interfuscator:shuffle>
};

//////////////////////////////////////////////////////////////////////////
//! Simple bus that notifies listeners of font changes
class FontNotifications
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

    virtual ~FontNotifications() = default;

    virtual void OnFontsReloaded() = 0;

    virtual void OnFontTextureUpdated(IFFont* /*font*/) {}
};

using FontNotificationBus = AZ::EBus<FontNotifications>;

#endif // CRYINCLUDE_CRYCOMMON_IFONT_H
