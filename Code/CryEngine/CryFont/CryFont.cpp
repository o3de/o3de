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

// Description : CCryFont class.


#include "CryFont_precompiled.h"

#if !defined(USE_NULLFONT_ALWAYS)

#include "CryFont.h"
#include "CryPath.h"
#include "FFont.h"
#include "FontTexture.h"
#include "FontRenderer.h"
#include "ILocalizationManager.h"

#include <AzCore/std/string/conversions.h>
#include <AzFramework/Archive/IArchive.h>

// Static member definitions
const Vec2i CCryFont::defaultGlyphSize = Vec2i(ICryFont::defaultGlyphSizeX, ICryFont::defaultGlyphSizeY);

#if !defined(_RELEASE)
static void DumpFontTexture(IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() != 2)
    {
        return;
    }

    const char* pFontName = pArgs->GetArg(1);

    if (pFontName && *pFontName && *pFontName != '0')
    {
        string fontFile("@devroot@/");
        fontFile += pFontName;
        fontFile += ".bmp";

        CFFont* pFont = (CFFont*) gEnv->pCryFont->GetFont(pFontName);
        if (pFont)
        {
            pFont->GetFontTexture()->WriteToFile(fontFile.c_str());
            gEnv->pLog->LogWithType(IMiniLog::eInputResponse, "Dumped \"%s\" texture to \"%s\"!", pFontName, fontFile.c_str());
        }
    }
}

static void DumpFontNames([[maybe_unused]] IConsoleCmdArgs* pArgs)
{
    string names = gEnv->pCryFont->GetLoadedFontNames();
    gEnv->pLog->LogWithType(IMiniLog::eInputResponse, "Currently loaded fonts: %s", names.c_str());
}

static void ReloadFonts([[maybe_unused]] IConsoleCmdArgs* pArgs)
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

        string m_lang;                      //!< Stores a comma-separated list of languages this collection of fonts applies to.
                                            //!< If this is an empty string, it implies that these set of fonts will be applied
                                            //!< by default (when a language is being used but no fonts in the font family are
                                            //!< mapped to that language).

        string m_fontFilename;              //!< Font used when no styling is applied.
        string m_boldFontFilename;          //!< Bold-styled font
        string m_italicFontFilename;        //!< Italic-styled font
        string m_boldItalicFontFilename;    //!< Bold-italic-styled font
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

        string m_fontFamilyName;                  //!< Value of the "name" font-family tag attribute
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

            string name;

            for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (string(key) == "name")
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

            name.Trim();
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

            string lang;
            for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (string(key) == "lang")
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

            lang.Trim();
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

            string path;
            string tags;

            for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (string(key) == "path")
                    {
                        path = value;
                    }
                    else if (string(key) == "tags")
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

            tags.Trim();
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
    XmlNodeRef SafeLoadXmlFromFile(const string& xmlPath)
    {
        if (gEnv->pCryPak->IsFileExist(xmlPath.c_str()))
        {
            return GetISystem()->LoadXmlFromFile(xmlPath.c_str());
        }

        return XmlNodeRef();
    }

}

CCryFont::CCryFont(ISystem* pSystem)
    : m_pSystem(pSystem)
    , m_fonts()
    , m_rndPropIsRGBA(false)
    , m_rndPropHalfTexelOffset(0.5f)
{
    assert(m_pSystem);

    CryLogAlways("Using FreeType %d.%d.%d", FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH);

    // Persist fonts for application lifetime to prevent unnecessary work
    REGISTER_CVAR(r_persistFontFamilies, r_persistFontFamilies, VF_NULL, "Persist loaded font families for lifetime of application.");

#if !defined(_RELEASE)
    REGISTER_COMMAND("r_DumpFontTexture", DumpFontTexture, 0,
        "Dumps the specified font's texture to a bitmap file\n"
        "Use r_DumpFontTexture to get the loaded font names\n"
        "Usage: r_DumpFontTexture <fontname>");
    REGISTER_COMMAND("r_DumpFontNames", DumpFontNames, 0,
        "Logs a list of fonts currently loaded");
    REGISTER_COMMAND("r_ReloadFonts", ReloadFonts, VF_NULL,
        "Reload all fonts");
#endif
}

CCryFont::~CCryFont()
{
    // Persist fonts for application lifetime to prevent unnecessary work
    m_persistedFontFamilies.clear();

    for (FontMapItor it = m_fonts.begin(), itEnd = m_fonts.end(); it != itEnd; )
    {
        CFFont* pFont = it->second;
        ++it; // iterate as Release() below will remove font from the map
        SAFE_RELEASE(pFont);
    }
}

void CCryFont::Release()
{
    delete this;
}

IFFont* CCryFont::NewFont(const char* pFontName)
{
    string name = pFontName;
    name.MakeLower();

    FontMapItor it = m_fonts.find(CONST_TEMP_STRING(name.c_str()));
    if (it != m_fonts.end())
    {
        return it->second;
    }

    CFFont* pFont = new CFFont(m_pSystem, this, name.c_str());
    m_fonts.insert(FontMapItor::value_type(name, pFont));
    return pFont;
}

IFFont* CCryFont::GetFont(const char* pFontName) const
{
    FontMapConstItor it = m_fonts.find(CONST_TEMP_STRING(string(pFontName).MakeLower()));
    return it != m_fonts.end() ? it->second : 0;
}

FontFamilyPtr CCryFont::LoadFontFamily(const char* pFontFamilyName)
{
    FontFamilyPtr fontFamily(nullptr);
    string fontFamilyPath;
    string fontFamilyFullPath;
    
    XmlNodeRef root = LoadFontFamilyXml(pFontFamilyName, fontFamilyPath, fontFamilyFullPath);

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
                    int searchPos = 0;
                    string langToken;

                    // "lang" font-tag attribute could be comma-separated
                    while (!(langToken = fontTagXml.m_lang.Tokenize(",", searchPos)).empty())
                    {
                        if (langToken.Trim() == currentLanguage)
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
                        [this](FontFamily* pFontFamily)
                    {
                        ReleaseFontFamily(pFontFamily);
                    });

                    // Map the font family name both by path and by name defined
                    // within the Font Family XML itself. This allows font 
                    // families to also be referenced simply by name.
                    if (!AddFontFamilyToMaps(fontFamilyFullPath, xmlData.m_fontFamilyName, fontFamily))
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
        IFFont* pFont = LoadFont(pFontFamilyName);

        if (pFont)
        {
            // Create a font family from a single font by assigning all the
            // font family stylings to the same font
            fontFamily.reset(new FontFamily(),
                [this](FontFamily* pFontFamily)
            {
                ReleaseFontFamily(pFontFamily);
            });

            // Use filepath as familyName so font loading/unloading doesn't break with duplicate file names
            fontFamily->familyName = pFontFamilyName;

            if (!AddFontFamilyToMaps(pFontFamilyName, fontFamily->familyName, fontFamily))
            {
                SAFE_RELEASE(pFont);

                return nullptr;
            }

            // Assign all stylings to the same font
            fontFamily->normal = pFont;
            fontFamily->bold = pFont;
            fontFamily->italic = pFont;
            fontFamily->boldItalic = pFont;

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

FontFamilyPtr CCryFont::GetFontFamily(const char* pFontFamilyName)
{
    FontFamilyPtr fontFamily = nullptr;

    // The given string could either be: a font family name (defined in font
    // family XML), a file path (for regular fonts mapped as font families),
    // or just the filename of a font itself. Fonts are mapped by font family
    // name or by filepath, so attempt lookup using the map first since it's
    // the fastest.
    string loweredName = string(pFontFamilyName).Trim().MakeLower();
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
            string mappedFilenameNoExtension = PathUtil::GetFileName(mappedFontFamilyName.c_str());

            string searchStringFilenameNoExtension = PathUtil::GetFileName(loweredName);

            if (mappedFilenameNoExtension == searchStringFilenameNoExtension)
            {
                fontFamily = FontFamilyPtr(fontFamilyIter.second);
                break;
            }
        }
    }

    return fontFamily;
}

void CCryFont::AddCharsToFontTextures(FontFamilyPtr pFontFamily, const char* pChars, int glyphSizeX, int glyphSizeY)
{
    pFontFamily->normal->AddCharsToFontTexture(pChars, glyphSizeX, glyphSizeY);
    pFontFamily->bold->AddCharsToFontTexture(pChars, glyphSizeX, glyphSizeY);
    pFontFamily->italic->AddCharsToFontTexture(pChars, glyphSizeX, glyphSizeY);
    pFontFamily->boldItalic->AddCharsToFontTexture(pChars, glyphSizeX, glyphSizeY);
}

void CCryFont::SetRendererProperties(IRenderer* pRenderer)
{
    if (pRenderer)
    {
        m_rndPropIsRGBA = (pRenderer->GetFeatures() & RFT_RGBA) != 0;
        m_rndPropHalfTexelOffset = 0.0f;
    }
}

void CCryFont::GetMemoryUsage(ICrySizer* pSizer) const
{
    if (!pSizer->Add(*this))
    {
        return;
    }

    pSizer->AddObject(m_fonts);
}

string CCryFont::GetLoadedFontNames() const
{
    string ret;
    for (FontMapConstItor it = m_fonts.begin(), itEnd = m_fonts.end(); it != itEnd; ++it)
    {
        CFFont* pFont = it->second;
        if (pFont)
        {
            if (!ret.empty())
            {
                ret += ",";
            }
            ret += pFont->GetName();
        }
    }
    return ret;
}

void CCryFont::OnLanguageChanged()
{
    ReloadAllFonts();

    EBUS_EVENT(LanguageChangeNotificationBus, LanguageChanged);
}

void CCryFont::ReloadAllFonts()
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

void CCryFont::UnregisterFont(const char* pFontName)
{
    FontMapItor it = m_fonts.find(CONST_TEMP_STRING(pFontName));

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
            pFontName);
        AZ_Assert(fontFamily->italic != fontPtr,
            "The following font is being freed but still in use by a FontFamily: %s",
            pFontName);
        AZ_Assert(fontFamily->bold != fontPtr,
            "The following font is being freed but still in use by a FontFamily: %s",
            pFontName);
        AZ_Assert(fontFamily->boldItalic != fontPtr,
            "The following font is being freed but still in use by a FontFamily: %s",
            pFontName);
    }
#endif
}

IFFont* CCryFont::LoadFont(const char* pFontName)
{
    string fontName = pFontName;
    fontName.MakeLower();

    IFFont* font = GetFont(fontName);
    if (font)
    {
        font->AddRef(); // use existing loaded font
    }
    else
    {
        // attempt to create and load a new font, use the font pathname as the font name
        font = NewFont(fontName);
        if (!font)
        {
            string errorMsg = "Error creating a new font named ";
            errorMsg += fontName;
            errorMsg += ".";
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, errorMsg.c_str());
        }
        else
        {
            // creating font adds one to its refcount so no need for AddRef here
            if (!font->Load(fontName))
            {
                string errorMsg = "Error loading a font from ";
                errorMsg += fontName;
                errorMsg += ".";
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, errorMsg);
                font->Release();
                font = nullptr;
            }
        }
    }

    return font;
}

void CCryFont::ReleaseFontFamily(FontFamily* pFontFamily)
{
    // Ensure that Font Family was mapped prior to destruction
    const bool isMapped = m_fontFamilyReverseLookup.find(pFontFamily) != m_fontFamilyReverseLookup.end();
    if (!isMapped)
    {
        return;
    }

    // Note that the FontFamily is mapped both by filename and by "family name"
    auto it = m_fontFamilyReverseLookup[pFontFamily];
    m_fontFamilies.erase(it);
    string familyName(pFontFamily->familyName);
    m_fontFamilies.erase(familyName.MakeLower().c_str());

    // Reverse lookup is used to avoid needing to store filename path with
    // the font family, so we need to remove that entry also.
    m_fontFamilyReverseLookup.erase(pFontFamily);

    SAFE_RELEASE(pFontFamily->normal);
    SAFE_RELEASE(pFontFamily->bold);
    SAFE_RELEASE(pFontFamily->italic);
    SAFE_RELEASE(pFontFamily->boldItalic);
}

bool CCryFont::AddFontFamilyToMaps(const char* pFontFamilyFilename, const char* pFontFamilyName, FontFamilyPtr fontFamily)
{
    if (!pFontFamilyFilename || !pFontFamilyName || !fontFamily.get())
    {
        return false;
    }

    // We don't support "updating" mapped values. 
    AZStd::string loweredFilename(PathUtil::MakeGamePath(string(pFontFamilyFilename)).c_str());
    AZStd::to_lower<AZStd::string::iterator>(loweredFilename.begin(), loweredFilename.end());
    if (m_fontFamilies.find(loweredFilename) != m_fontFamilies.end())
    {
        string warnMsg;
        warnMsg.Format("Couldn't load Font Family '%s': already loaded", pFontFamilyFilename);
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, warnMsg.c_str());
        return false;
    }

    // Similarly, we don't support Font Family XMLs that have the same font 
    // family name (we assume all Font Family names are unique).
    AZStd::string loweredFontFamilyName(pFontFamilyName);
    AZStd::to_lower<AZStd::string::iterator>(loweredFontFamilyName.begin(), loweredFontFamilyName.end());
    if (m_fontFamilies.find(loweredFontFamilyName) != m_fontFamilies.end())
    {
        string warnMsg;
        warnMsg.Format("Couldn't load Font Family '%s': already loaded", pFontFamilyName);
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, warnMsg.c_str());
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

XmlNodeRef CCryFont::LoadFontFamilyXml(const char* pFontFamilyName, string& outputDirectory, string& outputFullPath)
{
    outputFullPath = pFontFamilyName;
    outputDirectory = PathUtil::GetPath(pFontFamilyName);
    XmlNodeRef root = SafeLoadXmlFromFile(outputFullPath);

    // When parsing a <font> tag in markup, only the font name is given and 
    // not a path, so we try to build a "best guess" path from the name.
    if (!root)
    {
        string fileNoExtension(PathUtil::GetFileName(pFontFamilyName));
        string fileExtension(PathUtil::GetExt(pFontFamilyName));

        if (fileExtension.empty())
        {
            fileExtension = ".fontfamily";
        }

        // Try: "fonts/fontName.fontfamily"
        outputDirectory = string("fonts/");
        outputFullPath = outputDirectory + fileNoExtension + fileExtension;
        root = SafeLoadXmlFromFile(outputFullPath);

        // Finally, try: "fonts/fontName/fontName.fontfamily"
        if (!root)
        {
            outputDirectory = string("fonts/") + fileNoExtension + "/";
            outputFullPath = outputDirectory + fileNoExtension + fileExtension;
            root = SafeLoadXmlFromFile(outputFullPath);
        }
    }

    return root;
}

#endif

