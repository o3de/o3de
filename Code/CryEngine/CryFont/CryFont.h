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

#ifndef CRYINCLUDE_CRYFONT_CRYFONT_H
#define CRYINCLUDE_CRYFONT_CRYFONT_H
#pragma once


#if !defined(USE_NULLFONT_ALWAYS)

#include <IXml.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <map>

class CFFont;


class CCryFont
    : public ICryFont
{
    friend class CFFont;

public:

    static const Vec2i defaultGlyphSize;    //!< Default glyph size indicates that glyphs in the font texture
                                            //!< should be rendered at the maximum resolution supported by
                                            //!< the font texture's glyph cell/slot configuration (configured
                                            //!< via font XML).

public:
    CCryFont(ISystem* pSystem);
    virtual ~CCryFont();

    virtual void Release();
    virtual IFFont* NewFont(const char* pFontName);
    virtual IFFont* GetFont(const char* pFontName) const;
    virtual FontFamilyPtr LoadFontFamily(const char* pFontFamilyName) override;
    virtual FontFamilyPtr GetFontFamily(const char* pFontFamilyName) override;
    virtual void AddCharsToFontTextures(FontFamilyPtr pFontFamily, const char* pChars, int glyphSizeX = ICryFont::defaultGlyphSizeX, int glyphSizeY = ICryFont::defaultGlyphSizeY) override;
    virtual void SetRendererProperties(IRenderer* pRenderer);
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual string GetLoadedFontNames() const;
    virtual void OnLanguageChanged() override;
    virtual void ReloadAllFonts() override;

public:
    void UnregisterFont(const char* pFontName);
    bool RndPropIsRGBA() const { return m_rndPropIsRGBA; }
    float RndPropHalfTexelOffset() const { return m_rndPropHalfTexelOffset; }

private:
    typedef std::map<string, CFFont*> FontMap;
    typedef FontMap::iterator FontMapItor;
    typedef FontMap::const_iterator FontMapConstItor;

    typedef AZStd::map<AZStd::string, AZStd::weak_ptr<FontFamily>> FontFamilyMap;
    typedef AZStd::map<FontFamily*, FontFamilyMap::iterator> FontFamilyReverseLookupMap;

private:
    //! Convenience method for loading fonts
    IFFont* LoadFont(const char* fontName);

    //! Called when final FontFamily shared_ptr is destroyed; do not call directly.
    void ReleaseFontFamily(FontFamily* pFontFamily);

    //! Adds new entries into both font family maps for the given font family
    //!
    //! Note that it's not possible to update Font Family mappings with this
    //! method. The only way to do that would be to release the font family 
    //! and re-load it with the new values.
    //!
    //! \return True only if the Font Family was added to the maps, false for all other cases (such as
    //! when the font family is already mapped).
    bool AddFontFamilyToMaps(const char* pFontFamilyFilename, const char* pFontFamilyName, FontFamilyPtr fontFamily);

    //! Internal method that (possibly) makes several attempts at locating and loading a given font family XML.
    //! \param pFontFamilyName The name of the font family, or path to a font family file.
    //! \param outputDirectory Path to loaded font family (no filename), may need resolving with PathUtil::MakeGamePath.
    //! \param outputFullPath Full path to loaded font family, may need resolving with PathUtil::MakeGamePath.
    XmlNodeRef LoadFontFamilyXml(const char* pFontFamilyName, string& outputDirectory, string& outputFullPath);

private:
    FontMap m_fonts;
    FontFamilyMap m_fontFamilies; //!< Map font family names to weak ptrs so we can construct shared_ptrs but not keep a ref ourselves.
    FontFamilyReverseLookupMap m_fontFamilyReverseLookup; //<! FontFamily pointer reverse-lookup for quick removal
    ISystem* m_pSystem;
    bool m_rndPropIsRGBA;
    float m_rndPropHalfTexelOffset;

    int r_persistFontFamilies = 1; //!< Persist fonts for application lifetime to prevent unnecessary work; enabled by default.
    AZStd::vector<FontFamilyPtr> m_persistedFontFamilies; //!< Stores persisted fonts (if "persist font families" is enabled)

};

#endif

#endif // CRYINCLUDE_CRYFONT_CRYFONT_H
