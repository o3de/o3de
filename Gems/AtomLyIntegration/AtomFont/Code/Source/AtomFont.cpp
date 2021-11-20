/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : AtomFont class.



#if !defined(USE_NULLFONT_ALWAYS)

#include <AtomLyIntegration/AtomFont/AtomFont.h>
#include <AtomLyIntegration/AtomFont/FFont.h>
#include <AtomLyIntegration/AtomFont/FontTexture.h>
#include <AtomLyIntegration/AtomFont/FontRenderer.h>

#include <CryCommon/CryPath.h>
#include <CryCommon/ILocalizationManager.h>

#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Archive/IArchive.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

// Static member definitions
const AZ::AtomFont::GlyphSize AZ::AtomFont::defaultGlyphSize = AZ::AtomFont::GlyphSize(ICryFont::defaultGlyphSizeX, ICryFont::defaultGlyphSizeY);

#if !defined(_RELEASE)
static void DumfontTexture(IConsoleCmdArgs* cmdArgs)
{
    if (cmdArgs->GetArgCount() != 2)
    {
        return;
    }

    const char* fontName = cmdArgs->GetArg(1);

    if (fontName && *fontName && *fontName != '0')
    {
        AZStd::string fontFilePath("@engroot@/");
        fontFilePath += fontName;
        fontFilePath += ".bmp";

        AZ::FFont* font = (AZ::FFont*) gEnv->pCryFont->GetFont(fontName);
        if (font)
        {
            font->GetFontTexture()->WriteToFile(fontFilePath.c_str());
            gEnv->pLog->LogWithType(IMiniLog::eInputResponse, "Dumped \"%s\" texture to \"%s\"!", fontName, fontFilePath.c_str());
        }
    }
}

static void DumfontNames([[maybe_unused]] IConsoleCmdArgs* cmdArgs)
{
    AZStd::string names = gEnv->pCryFont->GetLoadedFontNames();
    gEnv->pLog->LogWithType(IMiniLog::eInputResponse, "Currently loaded fonts: %s", names.c_str());
}

static void ReloadFonts([[maybe_unused]] IConsoleCmdArgs* cmdArgs)
{
    gEnv->pCryFont->ReloadAllFonts();
}
#endif

namespace
{
    //! Stores paths to styled font assets for a given set of languages
    //! This struct stores the XML data contained within the <font> tag of
    //! an enclosing <fontfamily> definition:
    //!
    //! <fontfamily name="FontFamilyName">
    //!     <font lang="Language1, Language2">
    //!         <file path="regular.font" />
    //!         <file path="bold.font" tags="b" />
    //!         <file path="italic.font" tags="i" />
    //!         <file path="bolditalic.font" tags="b,i" />
    //!     </font>
    //! </fontfamily>
    struct FontTagXml
    {
        //! \return True if all font asset paths are non-empty, false otherwise
        bool IsValid() const
        {
            // Note that "lang" can be empty
            return !m_fontFilename.empty()
                && !m_boldFontFilename.empty()
                && !m_italicFontFilename.empty()
                && !m_boldItalicFontFilename.empty();
        }

        AZStd::string m_lang;               //!< Stores a comma-separated list of languages this collection of fonts applies to.
                                            //!< If this is an empty string, it implies that these set of fonts will be applied
                                            //!< by default (when a language is being used but no fonts in the font family are
                                            //!< mapped to that language).

        AZStd::string m_fontFilename;              //!< Font used when no styling is applied.
        AZStd::string m_boldFontFilename;          //!< Bold-styled font
        AZStd::string m_italicFontFilename;        //!< Italic-styled font
        AZStd::string m_boldItalicFontFilename;    //!< Bold-italic-styled font
    };

    //! Stores parsed font family XML data.
    //! This struct contains the name of the font family and a list of font
    //! file XML data for all the language-specific mappings of this
    //! font family.
    //!
    //! Example XML:
    //!
    //! <fontfamily name="FontFamilyName">
    //!     <font>
    //!         <file path="regular.font" />
    //!         <file path="bold.font" tags="b" />
    //!         <file path="italic.font" tags="i" />
    //!         <file path="bolditalic.font" tags="b,i" />
    //!     </font>
    //!     <font lang="korean">
    //!         <file path="../korean/korean-regular.font" />
    //!         <file path="../korean/korean-italic.font" tags="b" />
    //!         <file path="../korean/korean-bold.font" tags="i" />
    //!         <file path="../korean/korean-bolditalic.font" tags="b,i" />
    //!     </font>
    //!     <font lang="chinesesimplified">
    //!         <file path="../chinesesimplified/chinesesimplified-regular.font" />
    //!         <file path="../chinesesimplified/chinesesimplified-bold.font" tags="b" />
    //!         <file path="../chinesesimplified/chinesesimplified-italic.font" tags="i" />
    //!         <file path="../chinesesimplified/chinesesimplified-bolditalic.font" tags="b,i" />
    //!     </font>
    //! </fontfamily>
    struct FontFamilyTagXml
    {
        //! Returns true if all font file fields were parsed, false otherwise.
        bool IsValid() const
        {
            for (const FontTagXml& fontTagXml : m_fontTagsXml)
            {
                if (!fontTagXml.IsValid())
                {
                    return false;
                }
            }

            // Every font family must have a name
            return !m_fontFamilyName.empty();
        }

        AZStd::string m_fontFamilyName;                  //!< Value of the "name" font-family tag attribute
        AZStd::list<FontTagXml> m_fontTagsXml;    //!< List of child <font> tag data.
    };

    //! Returns true if the XML tree was traversed successfully, false otherwise.
    //!
    //! Note that, if this function returns true, it simply means that there were
    //! no unexpected structure issues with the given XML tree, it doesn't 
    //! necessarily mean that all the required fields were parsed.
    bool ParseFontFamilyXml(const XmlNodeRef& node, FontFamilyTagXml& xmlData)
    {
        if (!node)
        {
            return false;
        }

        // <fontfamily>
        if (AZStd::string(node->getTag()) == "fontfamily")
        {
            const int numAttributes = node->getNumAttributes();

            if (numAttributes <= 0)
            {
                // Expecting at least one attribute
                return false;
            }

            AZStd::string name;

            for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (AZStd::string(key) == "name")
                    {
                        name = value;
                    }
                    else
                    {
                        // Unexpected font tag attribute
                        return false;
                    }
                }
            }

            AZ::StringFunc::TrimWhiteSpace(name, true, true);
            if (!name.empty())
            {
                xmlData.m_fontFamilyName = name;
            }
            else
            {
                // Font family must have a name
                return false;
            }
        }

        // <font>
        if (AZStd::string(node->getTag()) == "font")
        {
            xmlData.m_fontTagsXml.push_back(FontTagXml());

            AZStd::string lang;
            for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (AZStd::string(key) == "lang")
                    {
                        lang = value;
                    }
                    else
                    {
                        // Unexpected font tag attribute
                        return false;
                    }
                }
            }

            AZ::StringFunc::TrimWhiteSpace(lang, true, true);
            if (!lang.empty())
            {
                xmlData.m_fontTagsXml.back().m_lang = lang;
            }
        }

        // <file>
        else if (AZStd::string(node->getTag()) == "file")
        {
            const int numAttributes = node->getNumAttributes();

            if (numAttributes <= 0)
            {
                // Expecting at least one attribute
                return false;
            }

            AZStd::string path;
            AZStd::string tags;

            for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (AZStd::string(key) == "path")
                    {
                        path = value;
                    }
                    else if (AZStd::string(key) == "tags")
                    {
                        tags = value;
                    }
                    else
                    {
                        // Unexpected font tag attribute
                        return false;
                    }
                }
            }

            AZ::StringFunc::TrimWhiteSpace(tags, true, true);
            if (tags.empty())
            {
                xmlData.m_fontTagsXml.back().m_fontFilename = path;
            }
            else if (tags == "b")
            {
                xmlData.m_fontTagsXml.back().m_boldFontFilename = path;
            }
            else if (tags == "i")
            {
                xmlData.m_fontTagsXml.back().m_italicFontFilename = path;
            }
            else
            {
                // We'll just assume any other tag indicates bold italic
                xmlData.m_fontTagsXml.back().m_boldItalicFontFilename = path;
            }
        }

        for (int i = 0, count = node->getChildCount(); i < count; ++i)
        {
            XmlNodeRef child = node->getChild(i);
            if (!ParseFontFamilyXml(child, xmlData))
            {
                return false;
            }
        }

        return true;
    }

    //! Only attempt XML file load if file exists.
    //! There are use-cases where the XML path is not fully known (such as
    //! when referencing font family names from font family XML files), and
    //! attempting to load the XML files directly via ISystem() methods can
    //! produce a lot of warning noise.
    XmlNodeRef SafeLoadXmlFromFile(const AZStd::string& xmlPath)
    {
        if (gEnv->pCryPak->IsFileExist(xmlPath.c_str()))
        {
            return GetISystem()->LoadXmlFromFile(xmlPath.c_str());
        }

        return XmlNodeRef();
    }

}

AZ::AtomFont::AtomFont([[maybe_unused]] ISystem* system)
{
    CryLogAlways("Using FreeType %d.%d.%d", FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH);

    // Persist fonts for application lifetime to prevent unnecessary work
    REGISTER_CVAR(r_persistFontFamilies, r_persistFontFamilies, VF_NULL, "Persist loaded font families for lifetime of application.");

#if !defined(_RELEASE)
    REGISTER_COMMAND("r_DumfontTexture", DumfontTexture, 0,
        "Dumps the specified font's texture to a bitmap file\n"
        "Use r_DumfontTexture to get the loaded font names\n"
        "Usage: r_DumfontTexture <fontname>");
    REGISTER_COMMAND("r_DumfontNames", DumfontNames, 0,
        "Logs a list of fonts currently loaded");
    REGISTER_COMMAND("r_ReloadFonts", ReloadFonts, VF_NULL,
        "Reload all fonts");
#endif
    AZ::Interface<AzFramework::FontQueryInterface>::Register(this);

    // Queue a load for the font per viewport dynamic draw context shader, and wait for it to load
    static const char* shaderFilepath = "Shaders/SimpleTextured.azshader";
    Data::Asset<RPI::ShaderAsset> shaderAsset = RPI::AssetUtils::GetAssetByProductPath<RPI::ShaderAsset>(shaderFilepath, RPI::AssetUtils::TraceLevel::Assert);
    shaderAsset.QueueLoad();
    Data::AssetBus::Handler::BusConnect(shaderAsset.GetId());

}

AZ::AtomFont::~AtomFont()
{
    Data::AssetBus::Handler::BusDisconnect();

    AZ::Interface<AzFramework::FontQueryInterface>::Unregister(this);
    m_defaultFontDrawInterface = nullptr;

    // Persist fonts for application lifetime to prevent unnecessary work
    m_persistedFontFamilies.clear();

    for (FontMapItor it = m_fonts.begin(), itEnd = m_fonts.end(); it != itEnd; )
    {
        FFont* font = it->second;
        ++it; // iterate as Release() below will remove font from the map
        SAFE_RELEASE(font);
    }
}

void AZ::AtomFont::Release()
{
    delete this;
}

IFFont* AZ::AtomFont::NewFont(const char* fontName)
{
    AZStd::string name = fontName;
    AZStd::to_lower(name.begin(), name.end());
    AzFramework::FontId fontId = GetFontId(name.c_str());

    FontMapItor it = m_fonts.find(fontId);
    if (it != m_fonts.end())
    {
        return it->second;
    }

    FFont* font = new FFont(this, name.c_str());
    m_fonts.insert(FontMapItor::value_type(fontId, font));
    if(!m_defaultFontDrawInterface)
    {
        m_defaultFontDrawInterface = static_cast<AzFramework::FontDrawInterface*>(font);
    }
    return font;
}

IFFont* AZ::AtomFont::GetFont(const char* fontName) const
{
    AZStd::string name = fontName;
    AZStd::to_lower(name.begin(), name.end());
    AzFramework::FontId fontId = GetFontId(name.c_str());
    FontMapConstItor it = m_fonts.find(fontId);
    return it != m_fonts.end() ? it->second : 0;
}

AzFramework::FontDrawInterface* AZ::AtomFont::GetFontDrawInterface(AzFramework::FontId fontId) const
{
    FontMapConstItor it = m_fonts.find(fontId);
    return (it != m_fonts.end()) ? it->second : nullptr;
}

AzFramework::FontDrawInterface* AZ::AtomFont::GetDefaultFontDrawInterface() const
{
    return m_defaultFontDrawInterface;
}

FontFamilyPtr AZ::AtomFont::LoadFontFamily(const char* fontFamilyName)
{
    FontFamilyPtr fontFamily(nullptr);
    AZStd::string fontFamilyPath;
    AZStd::string fontFamilyFullPath;
    
    XmlNodeRef root = LoadFontFamilyXml(fontFamilyName, fontFamilyPath, fontFamilyFullPath);

    if (root)
    {
        FontFamilyTagXml xmlData;
        const bool parseSuccess = ParseFontFamilyXml(root, xmlData);
        if (parseSuccess && xmlData.IsValid())
        {
            const char* currentLanguage = gEnv->pSystem->GetLocalizationManager()->GetLanguage();
            FontTagXml* defaultFont = nullptr;
            FontTagXml* langSpecificFont = nullptr;

            // Note that we don't break out of this for-loop early because we
            // want to find both the default font family and the 
            // language-specific font family. We prefer the lang-specific 
            // family but will fall back on the default if it doesn't exist.
            for (FontTagXml& fontTagXml : xmlData.m_fontTagsXml)
            {
                if (fontTagXml.m_lang.empty())
                {
                    defaultFont = &fontTagXml;
                }
                else
                {
                    // "lang" font-tag attribute could be comma-separated
                    AZStd::vector<AZStd::string> tokens;
                    AZ::StringFunc::Tokenize(fontTagXml.m_lang, tokens, ',');
                    for(AZStd::string& langToken : tokens)
                    {
                        AZ::StringFunc::TrimWhiteSpace(langToken, true, true);
                        if (langToken == currentLanguage)
                        {
                            langSpecificFont = &fontTagXml;
                            break;
                        }
                    }
                }
            }

            if (langSpecificFont || defaultFont)
            {
                // Prefer lang-specific font-family over default, if it exists
                FontTagXml* fontTagXml = langSpecificFont ? langSpecificFont : defaultFont;

                // Pre-pend font family's path to make font family XML paths 
                // relative to font family file
                fontTagXml->m_fontFilename = fontFamilyPath + fontTagXml->m_fontFilename;
                fontTagXml->m_boldFontFilename = fontFamilyPath + fontTagXml->m_boldFontFilename;
                fontTagXml->m_italicFontFilename = fontFamilyPath + fontTagXml->m_italicFontFilename;
                fontTagXml->m_boldItalicFontFilename = fontFamilyPath + fontTagXml->m_boldItalicFontFilename;

                IFFont* normal = LoadFont(fontTagXml->m_fontFilename.c_str());
                IFFont* bold = LoadFont(fontTagXml->m_boldFontFilename.c_str());
                IFFont* italic = LoadFont(fontTagXml->m_italicFontFilename.c_str());
                IFFont* boldItalic = LoadFont(fontTagXml->m_boldItalicFontFilename.c_str());

                // Only continue if all fonts were created successfully
                if (normal && bold && italic && boldItalic)
                {
                    fontFamily.reset(new FontFamily(),
                        [this](FontFamily* fontFamily)
                    {
                        ReleaseFontFamily(fontFamily);
                    });

                    // Map the font family name both by path and by name defined
                    // within the Font Family XML itself. This allows font 
                    // families to also be referenced simply by name.
                    if (!AddFontFamilyToMaps(fontFamilyFullPath.c_str(), xmlData.m_fontFamilyName.c_str(), fontFamily))
                    {
                        SAFE_RELEASE(normal);
                        SAFE_RELEASE(bold);
                        SAFE_RELEASE(italic);
                        SAFE_RELEASE(boldItalic);

                        return nullptr;
                    }

                    fontFamily->familyName = xmlData.m_fontFamilyName;
                    fontFamily->normal = normal;
                    fontFamily->bold = bold;
                    fontFamily->italic = italic;
                    fontFamily->boldItalic = boldItalic;
                }
                else
                {
                    SAFE_RELEASE(normal);
                    SAFE_RELEASE(bold);
                    SAFE_RELEASE(italic);
                    SAFE_RELEASE(boldItalic);
                }
            }
        }
    }

    if (!fontFamily)
    {
        // Unable to load font family XML, so load font normally and associate
        // it with a font family
        IFFont* font = LoadFont(fontFamilyName);

        if (font)
        {
            // Create a font family from a single font by assigning all the
            // font family stylings to the same font
            fontFamily.reset(new FontFamily(),
                [this](FontFamily* fontFamily)
            {
                ReleaseFontFamily(fontFamily);
            });

            // Use filepath as familyName so font loading/unloading doesn't break with duplicate file names
            fontFamily->familyName = fontFamilyName;

            if (!AddFontFamilyToMaps(fontFamilyName, fontFamily->familyName.c_str(), fontFamily))
            {
                SAFE_RELEASE(font);

                return nullptr;
            }

            // Assign all stylings to the same font
            fontFamily->normal = font;
            fontFamily->bold = font;
            fontFamily->italic = font;
            fontFamily->boldItalic = font;

            // The other three stylings need to have their ref count 
            // incremented (even though in this particular case its all the
            // same font) because when ReleaseFontFamily executes all fonts
            // in the family will be (corresondingly) Release'd.
            fontFamily->bold->AddRef();
            fontFamily->italic->AddRef();
            fontFamily->boldItalic->AddRef();
        }
    }

    // Persist fonts for application lifetime to prevent unnecessary work
    if (r_persistFontFamilies > 0)
    {
        m_persistedFontFamilies.emplace_back(FontFamilyPtr(fontFamily));
    }

    return fontFamily;
}

FontFamilyPtr AZ::AtomFont::GetFontFamily(const char* fontFamilyName)
{
    FontFamilyPtr fontFamily = nullptr;

    // The given string could either be: a font family name (defined in font
    // family XML), a file path (for regular fonts mapped as font families),
    // or just the filename of a font itself. Fonts are mapped by font family
    // name or by filepath, so attempt lookup using the map first since it's
    // the fastest.
    AZStd::string loweredName = AZStd::string(fontFamilyName);
    AZ::StringFunc::TrimWhiteSpace(loweredName, true, true);
    AZStd::to_lower(loweredName.begin(), loweredName.end());
    auto it = m_fontFamilies.find(PathUtil::MakeGamePath(loweredName).c_str());
    if (it != m_fontFamilies.end())
    {
        fontFamily = FontFamilyPtr(it->second);
    }
    else
    {
        // Iterate through all fonts, returning the first match where simply
        // the filename of a font could be a match. This case will likely be
        // hit when text markup references a font that doesn't belong to a 
        // font family.
        for (const auto& fontFamilyIter : m_fontFamilies)
        {
            const AZStd::string& mappedFontFamilyName = fontFamilyIter.first;
            AZStd::string mappedFilenameNoExtension = PathUtil::GetFileName(mappedFontFamilyName.c_str());

            AZStd::string searchStringFilenameNoExtension = PathUtil::GetFileName(loweredName);

            if (mappedFilenameNoExtension == searchStringFilenameNoExtension)
            {
                fontFamily = FontFamilyPtr(fontFamilyIter.second);
                break;
            }
        }
    }

    return fontFamily;
}

void AZ::AtomFont::AddCharsToFontTextures(FontFamilyPtr fontFamily, const char* chars, int glyphSizeX, int glyphSizeY)
{
    fontFamily->normal->AddCharsToFontTexture(chars, glyphSizeX, glyphSizeY);
    fontFamily->bold->AddCharsToFontTexture(chars, glyphSizeX, glyphSizeY);
    fontFamily->italic->AddCharsToFontTexture(chars, glyphSizeX, glyphSizeY);
    fontFamily->boldItalic->AddCharsToFontTexture(chars, glyphSizeX, glyphSizeY);
}

AZStd::string AZ::AtomFont::GetLoadedFontNames() const
{
    AZStd::string ret;
    for (FontMapConstItor it = m_fonts.begin(), itEnd = m_fonts.end(); it != itEnd; ++it)
    {
        FFont* font = it->second;
        if (font)
        {
            if (!ret.empty())
            {
                ret += ",";
            }
            ret += font->GetName();
        }
    }
    return ret;
}

void AZ::AtomFont::OnLanguageChanged()
{
    ReloadAllFonts();

    EBUS_EVENT(LanguageChangeNotificationBus, LanguageChanged);
}

void AZ::AtomFont::ReloadAllFonts()
{
    // Persist fonts for application lifetime to prevent unnecessary work
    m_persistedFontFamilies.clear();

    AZStd::list<AZStd::string> fontFamilyFilenames;
    AZStd::list<FontFamily*> fontFamilyWeakPtrs;

    // Iterate through all currently loaded font families
    for (auto it : m_fontFamilyReverseLookup)
    {
        fontFamilyWeakPtrs.push_back(it.first);
        fontFamilyFilenames.push_back(it.second->first);
    }

    // Release font-family resources and unmap them
    for (auto fontFamily : fontFamilyWeakPtrs)
    {
        ReleaseFontFamily(fontFamily);
    }

    // Reload the font families
    for (auto familyFilename : fontFamilyFilenames)
    {
        LoadFontFamily(familyFilename.c_str());
    }

    // All UI text components need to reload their font assets (both in-game
    // and in-editor).
    EBUS_EVENT(FontNotificationBus, OnFontsReloaded);
}

void AZ::AtomFont::UnregisterFont(const char* fontName)
{
    AZStd::string name(fontName);
    AZStd::to_lower(name.begin(), name.end());
    AzFramework::FontId fontId = GetFontId(name.c_str());
    FontMapItor it = m_fonts.find(fontId);

#if defined(AZ_ENABLE_TRACING)
    IFFont* fontPtr = it->second;
#endif

    if (it != m_fonts.end())
    {
        m_fonts.erase(it);
    }

#if defined(AZ_ENABLE_TRACING)
    // Make sure the font being released isn't currently in use by a font family.
    // If it is, the FontFamily will have a dangling pointer and will cause a
    // crash when the FontFamily eventually gets released.
    for (auto reverseMapEntry : m_fontFamilyReverseLookup)
    {
        FontFamily* fontFamily = reverseMapEntry.first;
        AZ_Assert(fontFamily->normal != fontPtr, 
            "The following font is being freed but still in use by a FontFamily: %s",
            fontName);
        AZ_Assert(fontFamily->italic != fontPtr,
            "The following font is being freed but still in use by a FontFamily: %s",
            fontName);
        AZ_Assert(fontFamily->bold != fontPtr,
            "The following font is being freed but still in use by a FontFamily: %s",
            fontName);
        AZ_Assert(fontFamily->boldItalic != fontPtr,
            "The following font is being freed but still in use by a FontFamily: %s",
            fontName);
    }
#endif
}

IFFont* AZ::AtomFont::LoadFont(const char* fontName)
{
    AZStd::string fontNameLower = fontName;
    AZStd::to_lower(fontNameLower.begin(), fontNameLower.end());

    IFFont* font = GetFont(fontNameLower.c_str());
    if (font)
    {
        font->AddRef(); // use existing loaded font
    }
    else
    {
        // attempt to create and load a new font, use the font pathname as the font name
        font = NewFont(fontNameLower.c_str());
        if (!font)
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Error creating a new font named %s.", fontNameLower.c_str());
        }
        else
        {
            // creating font adds one to its refcount so no need for AddRef here
            if (!font->Load(fontNameLower.c_str()))
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Error loading a font from %s.", fontNameLower.c_str());
                font->Release();
                font = nullptr;
            }
        }
    }

    return font;
}

void AZ::AtomFont::ReleaseFontFamily(FontFamily* fontFamily)
{
    // Ensure that Font Family was mapped prior to destruction
    const bool isMapped = m_fontFamilyReverseLookup.find(fontFamily) != m_fontFamilyReverseLookup.end();
    if (!isMapped)
    {
        return;
    }

    // Note that the FontFamily is mapped both by filename and by "family name"
    auto it = m_fontFamilyReverseLookup[fontFamily];
    m_fontFamilies.erase(it);
    AZStd::string familyName(fontFamily->familyName);
    AZStd::to_lower(familyName.begin(), familyName.end());
    m_fontFamilies.erase(familyName.c_str());

    // Reverse lookup is used to avoid needing to store filename path with
    // the font family, so we need to remove that entry also.
    m_fontFamilyReverseLookup.erase(fontFamily);

    SAFE_RELEASE(fontFamily->normal);
    SAFE_RELEASE(fontFamily->bold);
    SAFE_RELEASE(fontFamily->italic);
    SAFE_RELEASE(fontFamily->boldItalic);
}

bool AZ::AtomFont::AddFontFamilyToMaps(const char* fontFamilyFilename, const char* fontFamilyName, FontFamilyPtr fontFamily)
{
    if (!fontFamilyFilename || !fontFamilyName || !fontFamily.get())
    {
        return false;
    }

    // We don't support "updating" mapped values. 
    AZStd::string loweredFilename(PathUtil::MakeGamePath(AZStd::string(fontFamilyFilename)).c_str());
    AZStd::to_lower<AZStd::string::iterator>(loweredFilename.begin(), loweredFilename.end());
    if (m_fontFamilies.find(loweredFilename) != m_fontFamilies.end())
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Couldn't load Font Family '%s': already loaded", fontFamilyFilename);
        return false;
    }

    // Similarly, we don't support Font Family XMLs that have the same font 
    // family name (we assume all Font Family names are unique).
    AZStd::string loweredFontFamilyName(fontFamilyName);
    AZStd::to_lower<AZStd::string::iterator>(loweredFontFamilyName.begin(), loweredFontFamilyName.end());
    if (m_fontFamilies.find(loweredFontFamilyName) != m_fontFamilies.end())
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Couldn't load Font Family '%s': already loaded", fontFamilyName);
        return false;
    }

    // First, insert by filename
    AZStd::pair<AZStd::string, AZStd::weak_ptr<FontFamily>> insertPair(loweredFilename, fontFamily);
    auto iterPosition = m_fontFamilies.insert(insertPair).first;
    m_fontFamilyReverseLookup[fontFamily.get()] = iterPosition;
    
    // Then, by Font Family name
    AZStd::pair<AZStd::string, AZStd::weak_ptr<FontFamily>> nameInsertPair(loweredFontFamilyName, fontFamily);
    m_fontFamilies.insert(nameInsertPair);

    return true;
}

XmlNodeRef AZ::AtomFont::LoadFontFamilyXml(const char* fontFamilyName, AZStd::string& outputDirectory, AZStd::string& outputFullPath)
{
    outputFullPath = fontFamilyName;
    outputDirectory = PathUtil::GetPath(fontFamilyName);
    XmlNodeRef root = SafeLoadXmlFromFile(outputFullPath);

    // When parsing a <font> tag in markup, only the font name is given and 
    // not a path, so we try to build a "best guess" path from the name.
    if (!root)
    {
        AZStd::string fileNoExtension(PathUtil::GetFileName(fontFamilyName));
        AZStd::string fileExtension(PathUtil::GetExt(fontFamilyName));

        if (fileExtension.empty())
        {
            fileExtension = ".fontfamily";
        }

        // Try: "fonts/fontName.fontfamily"
        outputDirectory = AZStd::string("fonts/");
        outputFullPath = outputDirectory + fileNoExtension + fileExtension;
        root = SafeLoadXmlFromFile(outputFullPath);

        // Finally, try: "fonts/fontName/fontName.fontfamily"
        if (!root)
        {
            outputDirectory = AZStd::string("fonts/") + fileNoExtension + "/";
            outputFullPath = outputDirectory + fileNoExtension + fileExtension;
            root = SafeLoadXmlFromFile(outputFullPath);
        }
    }

    return root;
}

void AZ::AtomFont::OnAssetReady(Data::Asset<Data::AssetData> asset)
{
    Data::Asset<RPI::ShaderAsset> shaderAsset = asset;

    AZ::AtomBridge::PerViewportDynamicDraw::Get()->RegisterDynamicDrawContext(
        AZ::Name(AZ::AtomFontDynamicDrawContextName),
        [shaderAsset](RPI::Ptr<RPI::DynamicDrawContext> drawContext)
        {
            AZ_Assert(shaderAsset->IsReady(), "Attempting to register the AtomFont"
                " dynamic draw context before the shader asset is loaded. The shader should be loaded first"
                " to avoid a blocking asset load and potential deadlock, since the DynamicDrawContext lambda"
                " will be executed during scene processing and there may be multiple scenes executing in parallel.");

            Data::Instance<RPI::Shader> shader = RPI::Shader::FindOrCreate(shaderAsset);
            AZ::RPI::ShaderOptionList shaderOptions;
            shaderOptions.push_back(AZ::RPI::ShaderOption(AZ::Name("o_useColorChannels"), AZ::Name("false")));
            shaderOptions.push_back(AZ::RPI::ShaderOption(AZ::Name("o_clamp"), AZ::Name("true")));
            drawContext->InitShaderWithVariant(shader, &shaderOptions);
            drawContext->InitVertexFormat(
                {
                    {"POSITION", RHI::Format::R32G32B32_FLOAT},
                    {"COLOR", RHI::Format::B8G8R8A8_UNORM},
                    {"TEXCOORD0", RHI::Format::R32G32_FLOAT}
                });
            drawContext->EndInit();
        });

    Data::AssetBus::Handler::BusDisconnect();
}

#endif

