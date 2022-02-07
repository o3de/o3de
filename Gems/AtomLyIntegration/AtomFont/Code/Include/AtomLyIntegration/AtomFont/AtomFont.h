/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#if !defined(USE_NULLFONT_ALWAYS)

#include <Cry_Vector2.h>
#include <IXml.h>
#include <CryCommon/IFont.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/Asset/AssetCommon.h>
#include <map>
#include <unordered_map>

#include <AzFramework/Font/FontInterface.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <AtomBridge/PerViewportDynamicDrawInterface.h>

namespace AZ
{
    class FFont;

    static constexpr char AtomFontDynamicDrawContextName[] = "AtomFont";


    //! AtomFont is the font system manager. 
    //! AtomFont manages the lifetime of FFont instances, each of which represents an individual font (e.g Courier New Italic)
    //! AtomFont also knows about font families (e.g Courier New + [Italic, Bold, Normal, Bold Italic]), languages, etc, 
    //! and manages their loading & saving together.
    class AtomFont
        : public ICryFont
        , public AzFramework::FontQueryInterface
        , private Data::AssetBus::Handler
    {
        friend class FFont;

    public:
        struct GlyphSize
        {
            union
            {
                struct // unnamed struct
                {
                    int32_t x;
                    int32_t y;
                };
                int32_t data[2];
            };
            GlyphSize() : x(0), y(0) {}
            GlyphSize(const int32_t sizeX, const int32_t sizeY) : x(sizeX), y(sizeY) {}
            GlyphSize(const Vec2i v) : x(v.x), y(v.y) {}

            bool operator==(const GlyphSize& rhs) const {return x == rhs.x && y == rhs.y;}
        };

        static const GlyphSize defaultGlyphSize;    //!< Default glyph size indicates that glyphs in the font texture
                                                    //!< should be rendered at the maximum resolution supported by
                                                    //!< the font texture's glyph cell/slot configuration (configured
                                                    //!< via font XML).

    public:
        AtomFont(ISystem* system);
        virtual ~AtomFont();

        //////////////////////////////////////////////////////////////////////////////////
        // ICryFont interface
        void Release() override;
        IFFont* NewFont(const char* fontName) override;
        IFFont* GetFont(const char* fontName) const override;
        FontFamilyPtr LoadFontFamily(const char* fontFamilyName) override;
        FontFamilyPtr GetFontFamily(const char* fontFamilyName) override;
        void AddCharsToFontTextures(FontFamilyPtr fontFamily, const char* chars, int glyphSizeX = ICryFont::defaultGlyphSizeX, int glyphSizeY = ICryFont::defaultGlyphSizeY) override;
        AZStd::string GetLoadedFontNames() const override;
        void OnLanguageChanged() override;
        void ReloadAllFonts() override;
        //////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // FontQueryInterface implementation
        AzFramework::FontDrawInterface* GetFontDrawInterface(AzFramework::FontId fontId) const override;
        AzFramework::FontDrawInterface* GetDefaultFontDrawInterface() const override;

    public:
        void UnregisterFont(const char* fontName);

    private:
        using FontMap = AZStd::unordered_map<AzFramework::FontId, FFont*>;
        using FontMapItor = FontMap::iterator;
        using FontMapConstItor = FontMap::const_iterator;

        using FontFamilyMap = AZStd::unordered_map<AZStd::string, AZStd::weak_ptr<FontFamily>>;
        using FontFamilyReverseLookupMap = AZStd::unordered_map<FontFamily*, FontFamilyMap::iterator>;

    private:
        //! Convenience method for loading fonts
        IFFont* LoadFont(const char* fontName);

        //! Called when final FontFamily shared_ptr is destroyed; do not call directly.
        void ReleaseFontFamily(FontFamily* fontFamily);

        //! Adds new entries into both font family maps for the given font family
        //!
        //! Note that it's not possible to update Font Family mappings with this
        //! method. The only way to do that would be to release the font family 
        //! and re-load it with the new values.
        //!
        //! \return True only if the Font Family was added to the maps, false for all other cases (such as
        //! when the font family is already mapped).
        bool AddFontFamilyToMaps(const char* fontFamilyFilename, const char* fontFamilyName, FontFamilyPtr fontFamily);

        //! Internal method that (possibly) makes several attempts at locating and loading a given font family XML.
        //! \param fontFamilyName The name of the font family, or path to a font family file.
        //! \param outputDirectory Path to loaded font family (no filename), may need resolving with PathUtil::MakeGamePath.
        //! \param outputFullPath Full path to loaded font family, may need resolving with PathUtil::MakeGamePath.
        XmlNodeRef LoadFontFamilyXml(const char* fontFamilyName, AZStd::string& outputDirectory, AZStd::string& outputFullPath);

        // Data::AssetBus::Handler overrides...
        void OnAssetReady(Data::Asset<Data::AssetData> asset) override;

    private:
        AzFramework::ISceneSystem::SceneEvent::Handler m_sceneEventHandler;

        FontMap m_fonts;
        FontFamilyMap m_fontFamilies; //!< Map font family names to weak ptrs so we can construct shared_ptrs but not keep a ref ourselves.
        FontFamilyReverseLookupMap m_fontFamilyReverseLookup; //<! FontFamily pointer reverse-lookup for quick removal

        AzFramework::FontDrawInterface* m_defaultFontDrawInterface = nullptr;

        int r_persistFontFamilies = 1; //!< Persist fonts for application lifetime to prevent unnecessary work; enabled by default.
        AZStd::vector<FontFamilyPtr> m_persistedFontFamilies; //!< Stores persisted fonts (if "persist font families" is enabled)
    };
}
#endif
