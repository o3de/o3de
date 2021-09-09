/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"

#include "LocalizedStringManager.h"

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define LOCALIZEDSTRINGMANAGER_CPP_SECTION_1 1
#endif

#include <ISystem.h>
#include "System.h" // to access InitLocalization()
#include <CryPath.h>
#include <IConsole.h>
#include <IFont.h>
#include <locale.h>
#include <time.h>

#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Math/Crc.h>

#define MAX_CELL_COUNT 32

// CVAR names
#if !defined(_RELEASE)
const char c_sys_localization_debug[] = "sys_localization_debug";
const char c_sys_localization_encode[] = "sys_localization_encode";
#endif // !defined(_RELEASE)

#define LOC_WINDOW "Localization"
const char c_sys_localization_format[] = "sys_localization_format";

enum ELocalizedXmlColumns
{
    ELOCALIZED_COLUMN_SKIP  = 0,
    ELOCALIZED_COLUMN_KEY,
    ELOCALIZED_COLUMN_AUDIOFILE,
    ELOCALIZED_COLUMN_CHARACTER_NAME,
    ELOCALIZED_COLUMN_SUBTITLE_TEXT,
    ELOCALIZED_COLUMN_ACTOR_LINE,
    ELOCALIZED_COLUMN_USE_SUBTITLE,
    ELOCALIZED_COLUMN_VOLUME,
    ELOCALIZED_COLUMN_SOUNDEVENT,
    ELOCALIZED_COLUMN_RADIO_RATIO,
    ELOCALIZED_COLUMN_EVENTPARAMETER,
    ELOCALIZED_COLUMN_SOUNDMOOD,
    ELOCALIZED_COLUMN_IS_DIRECT_RADIO,
    // legacy names
    ELOCALIZED_COLUMN_LEGACY_PERSON,
    ELOCALIZED_COLUMN_LEGACY_CHARACTERNAME,
    ELOCALIZED_COLUMN_LEGACY_TRANSLATED_CHARACTERNAME,
    ELOCALIZED_COLUMN_LEGACY_ENGLISH_DIALOGUE,
    ELOCALIZED_COLUMN_LEGACY_TRANSLATION,
    ELOCALIZED_COLUMN_LEGACY_YOUR_TRANSLATION,
    ELOCALIZED_COLUMN_LEGACY_ENGLISH_SUBTITLE,
    ELOCALIZED_COLUMN_LEGACY_TRANSLATED_SUBTITLE,
    ELOCALIZED_COLUMN_LEGACY_ORIGINAL_CHARACTER_NAME,
    ELOCALIZED_COLUMN_LEGACY_TRANSLATED_CHARACTER_NAME,
    ELOCALIZED_COLUMN_LEGACY_ORIGINAL_TEXT,
    ELOCALIZED_COLUMN_LEGACY_TRANSLATED_TEXT,
    ELOCALIZED_COLUMN_LEGACY_ORIGINAL_ACTOR_LINE,
    ELOCALIZED_COLUMN_LEGACY_TRANSLATED_ACTOR_LINE,
    ELOCALIZED_COLUMN_LAST,
};

// The order must match to the order of the ELocalizedXmlColumns
static const char* sLocalizedColumnNames[] =
{
    // everyhing read by the file will be convert to lower cases
    "skip",
    "key",
    "audio_filename",
    "character name",
    "subtitle text",
    "actor line",
    "use subtitle",
    "volume",
    "prototype event",
    "radio ratio",
    "eventparameter",
    "soundmood",
    "is direct radio",
    // legacy names
    "person",
    "character name",
    "translated character name",
    "english dialogue",
    "translation",
    "your translation",
    "english subtitle",
    "translated subtitle",
    "original character name",
    "translated character name",
    "original text",
    "translated text",
    "original actor line",
    "translated actor line",
};

//Please ensure that this array matches the contents of EPlatformIndependentLanguageID in ILocalizationManager.h
static const char* PLATFORM_INDEPENDENT_LANGUAGE_NAMES[ ILocalizationManager::ePILID_MAX_OR_INVALID ] =
{
    "en-US",  // English (USA)
    "en-GB",  // English (UK)
    "de-DE",  // German
    "ru-RU",  // Russian (Russia)
    "pl-PL",  // Polish
    "tr-TR",  // Turkish
    "es-ES",  // Spanish (Spain)
    "es-MX",  // Spanish (Mexico)
    "fr-FR",  // French (France)
    "fr-CA",  // French (Canada)
    "it-IT",  // Italian
    "pt-PT",  // Portugese (Portugal)
    "pt-BR",  // Portugese (Brazil)
    "ja-JP",  // Japanese
    "ko-KR",  // Korean
    "zh-CHT", // Traditional Chinese
    "zh-CHS", // Simplified Chinese
    "nl-NL",  // Dutch (The Netherlands)
    "fi-FI",  // Finnish
    "sv-SE",  // Swedish
    "cs-CZ",  // Czech
    "no-NO",  // Norwegian
    "ar-SA",   // Arabic (Saudi Arabia)
    "da-DK"   // Danish (Denmark)
};

//////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void ReloadDialogData([[maybe_unused]] IConsoleCmdArgs* pArgs)
{
    LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::ReloadData);
    //CSystem *pSystem = (CSystem*) gEnv->pSystem;
    //pSystem->InitLocalization();
    //pSystem->OpenBasicPaks();
}
#endif //#if !defined(_RELEASE)

//////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void TestFormatMessage ([[maybe_unused]] IConsoleCmdArgs* pArgs)
{
    AZStd::string fmt1 ("abc %1 def % gh%2i %");
    AZStd::string fmt2 ("abc %[action:abc] %2 def % gh%1i %1");
    AZStd::string out1, out2;
    LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::FormatStringMessage, out1, fmt1, "first", "second", "third", nullptr);
    CryLogAlways("%s", out1.c_str());
    LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::FormatStringMessage, out2, fmt2, "second", nullptr, nullptr, nullptr);
    CryLogAlways("%s", out2.c_str());
}
#endif //#if !defined(_RELEASE)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
CLocalizedStringsManager::CLocalizedStringsManager(ISystem* pSystem)
    : m_cvarLocalizationDebug(0)
    , m_cvarLocalizationEncode(1)
    , m_availableLocalizations(0)
{
    m_pSystem = pSystem;
    m_pSystem->GetISystemEventDispatcher()->RegisterListener(this);

    m_languages.reserve(4);
    m_pLanguage = 0;

#if !defined(_RELEASE)
    m_haveWarnedAboutAtLeastOneLabel = false;

    REGISTER_COMMAND("ReloadDialogData", ReloadDialogData, VF_NULL,
        "Reloads all localization dependent XML sheets for the currently set language.");

    REGISTER_COMMAND("_TestFormatMessage", TestFormatMessage, VF_NULL, "");

    REGISTER_CVAR2(c_sys_localization_debug, &m_cvarLocalizationDebug, m_cvarLocalizationDebug, VF_CHEAT,
        "Toggles debugging of the Localization Manager.\n"
        "Usage: sys_localization_debug [0..3]\n"
        "1: outputs warnings\n"
        "2: outputs extended information and warnings\n"
        "3: outputs CRC32 hashes and strings to help detect clashes\n"
        "Default is 0 (off).");

    REGISTER_CVAR2(c_sys_localization_encode, &m_cvarLocalizationEncode, m_cvarLocalizationEncode, VF_REQUIRE_APP_RESTART,
        "Toggles encoding of translated text to save memory. REQUIRES RESTART.\n"
        "Usage: sys_localization_encode [0..1]\n"
        "0: No encoding, store as wide strings\n"
        "1: Huffman encode translated text, saves approx 30% with a small runtime performance cost\n"
        "Default is 1.");
#endif //#if !defined(_RELEASE)

    REGISTER_CVAR2(c_sys_localization_format, &m_cvarLocalizationFormat, 1, VF_NULL,
        "Usage: sys_localization_format [0..1]\n"
        "    0: O3DE Legacy Localization (Excel 2003)\n"
        "    1: AGS XML\n"
        "Default is 1 (AGS Xml)");
    //Check that someone hasn't added a language ID without a language name
    assert(PLATFORM_INDEPENDENT_LANGUAGE_NAMES[ ILocalizationManager::ePILID_MAX_OR_INVALID - 1 ] != 0);

    // Populate available languages by scanning the localization directory for paks
    // Default to US English if language is not supported
    AZStd::string sPath;
    const AZStd::string sLocalizationFolder(PathUtil::GetLocalizationFolder());
    ILocalizationManager::TLocalizationBitfield availableLanguages = 0;

    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    // test language name against supported languages
    for (int i = 0; i < ILocalizationManager::ePILID_MAX_OR_INVALID; i++)
    {
        AZStd::string sCurrentLanguage = LangNameFromPILID((ILocalizationManager::EPlatformIndependentLanguageID)i);
        sPath = sLocalizationFolder.c_str() + sCurrentLanguage;
        AZStd::to_lower(sPath.begin(), sPath.end());
        if (fileIO && fileIO->IsDirectory(sPath.c_str()))
        {
            availableLanguages |= ILocalizationManager::LocalizationBitfieldFromPILID((ILocalizationManager::EPlatformIndependentLanguageID)i);
            if (m_cvarLocalizationDebug >= 2)
            {
                AZ_TracePrintf("Localization", "Detected language support for %s (id %d)", sCurrentLanguage.c_str(), i);
            }
        }
    }

    AZ_Warning("Localization", !(m_cvarLocalizationFormat == 0 && availableLanguages == 0 && ProjectUsesLocalization()), "No localization files found!");

    SetAvailableLocalizationsBitfield(availableLanguages);
    LocalizationManagerRequestBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////
CLocalizedStringsManager::~CLocalizedStringsManager()
{
    FreeData();
    LocalizationManagerRequestBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::GetLoadedTags(TLocalizationTagVec& tagVec)
{
    TTagFileNames::const_iterator end = m_tagFileNames.end();
    for (TTagFileNames::const_iterator it = m_tagFileNames.begin(); it != end; ++it)
    {
        if (it->second.loaded)
        {
            tagVec.push_back(it->first);
        }
    }
}

//////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FreeLocalizationData()
{
    AutoLock lock(m_cs);    //Make sure to lock, as this is a modifying operation
    ListAndClearProblemLabels();

    for (uint32 i = 0; i < m_languages.size(); i++)
    {
        if (m_cvarLocalizationEncode == 1)
        {
            auto pLanguage = m_languages[i];
            for (uint8 iEncoder = 0; iEncoder < pLanguage->m_vEncoders.size(); iEncoder++)
            {
                SAFE_DELETE(pLanguage->m_vEncoders[iEncoder]);
            }
        }
        std::for_each(m_languages[i]->m_vLocalizedStrings.begin(), m_languages[i]->m_vLocalizedStrings.end(), stl::container_object_deleter());
        m_languages[i]->m_keysMap.clear();
        m_languages[i]->m_vLocalizedStrings.clear();
    }
    m_loadedTables.clear();
}

//////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FreeData()
{
    FreeLocalizationData();

    for (uint32 i = 0; i < m_languages.size(); i++)
    {
        delete m_languages[i];
    }
    m_languages.resize(0);
    m_loadedTables.clear();

    m_pLanguage = 0;
}

//////////////////////////////////////////////////////////////////////////
const char* CLocalizedStringsManager::LangNameFromPILID(const ILocalizationManager::EPlatformIndependentLanguageID id)
{
    assert(id >= 0 && id < ILocalizationManager::ePILID_MAX_OR_INVALID);
    return PLATFORM_INDEPENDENT_LANGUAGE_NAMES[ id ];
}

//////////////////////////////////////////////////////////////////////////
ILocalizationManager::EPlatformIndependentLanguageID CLocalizedStringsManager::PILIDFromLangName(AZStd::string langName)
{
    for (int i = 0; i < ILocalizationManager::ePILID_MAX_OR_INVALID; i++)
    {
        if (!_stricmp(langName.c_str(), PLATFORM_INDEPENDENT_LANGUAGE_NAMES[i]))
        {
            return (ILocalizationManager::EPlatformIndependentLanguageID)i;
        }
    }
    return ILocalizationManager::ePILID_MAX_OR_INVALID;
}

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION LOCALIZEDSTRINGMANAGER_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(LocalizedStringManager_cpp)
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#endif // AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
//////////////////////////////////////////////////////////////////////////
ILocalizationManager::EPlatformIndependentLanguageID CLocalizedStringsManager::GetSystemLanguage()
{

    return ILocalizationManager::EPlatformIndependentLanguageID::ePILID_English_US;
}
#endif // defined(AZ_RESTRICTED_PLATFORM)

//Uses bitwise operations to compare the localizations we provide in this SKU and the languages that the platform supports.
//Returns !0 if we provide more localizations than are available as system languages
ILocalizationManager::TLocalizationBitfield CLocalizedStringsManager::MaskSystemLanguagesFromSupportedLocalizations(const ILocalizationManager::TLocalizationBitfield systemLanguages)
{
    return (~systemLanguages) & m_availableLocalizations;
}

//Returns !0 if the language is supported.
ILocalizationManager::TLocalizationBitfield CLocalizedStringsManager::IsLanguageSupported(const ILocalizationManager::EPlatformIndependentLanguageID id)
{
    return m_availableLocalizations & (1 << id);
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::SetAvailableLocalizationsBitfield(const ILocalizationManager::TLocalizationBitfield availableLocalizations)
{
    m_availableLocalizations = availableLocalizations;
}

//////////////////////////////////////////////////////////////////////
const char* CLocalizedStringsManager::GetLanguage()
{
    if (m_pLanguage == 0)
    {
        return "";
    }
    return m_pLanguage->sLanguage.c_str();
}

//////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::SetLanguage(const char* sLanguage)
{
    if (m_cvarLocalizationDebug >= 2)
    {
        CryLog("<Localization> Set language to %s", sLanguage);
    }

    // Check if already language loaded.
    for (uint32 i = 0; i < m_languages.size(); i++)
    {
        if (_stricmp(sLanguage, m_languages[i]->sLanguage.c_str()) == 0)
        {
            InternalSetCurrentLanguage(m_languages[i]);
            return true;
        }
    }

    SLanguage* pLanguage = new SLanguage;
    m_languages.push_back(pLanguage);

    if (m_cvarLocalizationDebug >= 2)
    {
        CryLog("<Localization> Insert new language to %s", sLanguage);
    }

    pLanguage->sLanguage = sLanguage;
    InternalSetCurrentLanguage(pLanguage);

    //-------------------------------------------------------------------------------------------------
    // input localization
    //-------------------------------------------------------------------------------------------------
    // keyboard
    for (int i = 0; i <= 0x80; i++)
    {
        AddControl(i);
    }

    // mouse
    for (int i = 1; i <= 0x0f; i++)
    {
        AddControl(i * 0x10000);
    }

    return (true);
}

//////////////////////////////////////////////////////////////////////////
int CLocalizedStringsManager::GetLocalizationFormat() const
{
    return m_cvarLocalizationFormat;
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CLocalizedStringsManager::GetLocalizedSubtitleFilePath(const AZStd::string& localVideoPath, const AZStd::string& subtitleFileExtension) const
{
    AZStd::string sLocalizationFolder(PathUtil::GetLocalizationFolder().c_str());
    size_t backSlashIdx = sLocalizationFolder.find_first_of("\\", 0);
    if (backSlashIdx != AZStd::string::npos)
    {
        sLocalizationFolder.replace(backSlashIdx, 2, "/");
    }
    AZStd::string filePath(m_pLanguage->sLanguage.c_str());
    filePath = sLocalizationFolder.c_str() + filePath + "/" + localVideoPath;
    return filePath.substr(0, filePath.find_last_of('.')).append(subtitleFileExtension);
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CLocalizedStringsManager::GetLocalizedLocXMLFilePath(const AZStd::string & localXmlPath) const
{
    AZStd::string sLocalizationFolder(PathUtil::GetLocalizationFolder().c_str());
    size_t backSlashIdx = sLocalizationFolder.find_first_of("\\", 0);
    if (backSlashIdx != AZStd::string::npos)
    {
        sLocalizationFolder.replace(backSlashIdx, 2, "/");
    }
    const AZStd::string& filePath = AZStd::string::format("%s%s/%s", sLocalizationFolder.c_str(), m_pLanguage->sLanguage.c_str(), localXmlPath.c_str());
    return filePath.substr(0, filePath.find_last_of('.')).append(".loc.xml");
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::AddControl([[maybe_unused]] int nKey)
{
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::ParseFirstLine(IXmlTableReader* pXmlTableReader, char* nCellIndexToType, std::map<int, AZStd::string>& SoundMoodIndex, std::map<int, AZStd::string>& EventParameterIndex)
{
    AZStd::string sCellContent;

    for (;; )
    {
        int nCellIndex = 0;
        const char* pContent = 0;
        size_t contentSize = 0;
        if (!pXmlTableReader->ReadCell(nCellIndex, pContent, contentSize))
        {
            break;
        }

        if (nCellIndex >= MAX_CELL_COUNT)
        {
            break;
        }

        if (contentSize <= 0)
        {
            continue;
        }

        sCellContent.assign(pContent, contentSize);
        AZStd::to_lower(sCellContent.begin(), sCellContent.end());

        for (int i = 0; i < sizeof(sLocalizedColumnNames) / sizeof(sLocalizedColumnNames[0]); ++i)
        {
            const char* pFind = strstr(sCellContent.c_str(), sLocalizedColumnNames[i]);
            if (pFind != 0)
            {
                nCellIndexToType[nCellIndex] = static_cast<char>(i);

                // find SoundMood
                if (i == ELOCALIZED_COLUMN_SOUNDMOOD)
                {
                    const char* pSoundMoodName = pFind + strlen(sLocalizedColumnNames[i]) + 1;
                    int nSoundMoodNameLength = static_cast<int>(sCellContent.length() - strlen(sLocalizedColumnNames[i]) - 1);
                    if (nSoundMoodNameLength > 0)
                    {
                        SoundMoodIndex[nCellIndex] = pSoundMoodName;
                    }
                }

                // find EventParameter
                if (i == ELOCALIZED_COLUMN_EVENTPARAMETER)
                {
                    const char* pParameterName = pFind + strlen(sLocalizedColumnNames[i]) + 1;
                    int nParameterNameLength = static_cast<int>(sCellContent.length() - strlen(sLocalizedColumnNames[i]) - 1);
                    if (nParameterNameLength > 0)
                    {
                        EventParameterIndex[nCellIndex] = pParameterName;
                    }
                }

                break;
            }
            // HACK until all columns are renamed to "Translation"
            //if (_stricmp(sCellContent, "Your Translation") == 0)
            //{
            //  nCellIndexToType[nCellIndex] = ELOCALIZED_COLUMN_TRANSLATED_ACTOR_LINE;
            //  break;
            //}
        }
    }
}

// copy characters to lower-case 0-terminated buffer
static void CopyLowercase(char* dst, size_t dstSize, const char* src, size_t srcCount)
{
    if (dstSize > 0)
    {
        if (srcCount > dstSize - 1)
        {
            srcCount = dstSize - 1;
        }
        while (srcCount--)
        {
            const char c = *src++;
            *dst++ = (c <= 'Z' && c >= 'A') ? c + ('a' - 'A') : c;
        }
        *dst = '\0';
    }
}

//////////////////////////////////////////////////////////////////////////
static void ReplaceEndOfLine(AZStd::fixed_string<CLocalizedStringsManager::LOADING_FIXED_STRING_LENGTH>& s)
{
    const AZStd::string oldSubstr("\\n");
    const AZStd::string newSubstr(" \n");
    size_t pos = 0;
    for (;; )
    {
        pos = s.find(oldSubstr, pos);
        if (pos == AZStd::fixed_string<CLocalizedStringsManager::LOADING_FIXED_STRING_LENGTH>::npos)
        {
            return;
        }
        s.replace(pos, oldSubstr.length(), newSubstr);
    }
}

//////////////////////////////////////////////////////////////////////////

void CLocalizedStringsManager::OnSystemEvent(
    ESystemEvent eEvent, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
    // might want to add an event which tells us that we are loading the main menu
    // so everything can be unloaded and init files reloaded so safe some memory
    switch (eEvent)
    {
    case ESYSTEM_EVENT_LEVEL_LOAD_START:
    {
        // This event is here not of interest while we're in the Editor.
        if (!gEnv->IsEditor())
        {
            if (m_cvarLocalizationDebug >= 2)
            {
                CryLog("<Localization> Loading Requested Tags");
            }

            for (TStringVec::iterator it = m_tagLoadRequests.begin(); it != m_tagLoadRequests.end(); ++it)
            {
                LoadLocalizationDataByTag(it->c_str());
            }
        }

        m_tagLoadRequests.clear();
        break;
    }
    case ESYSTEM_EVENT_EDITOR_ON_INIT:
    {
        // Load all tags after the Editor has finished initialization.
        for (TTagFileNames::iterator it = m_tagFileNames.begin(); it != m_tagFileNames.end(); ++it)
        {
            LoadLocalizationDataByTag(it->first.c_str());
        }

        break;
    }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::InitLocalizationData(
    const char* sFileName, [[maybe_unused]] bool bReload)
{
    XmlNodeRef root = m_pSystem->LoadXmlFromFile(sFileName);

    if (!root)
    {
        CryLog("Loading Localization File %s failed!", sFileName);
        return false;
    }

    for (int i = 0; i < root->getChildCount(); i++)
    {
        XmlNodeRef typeNode = root->getChild(i);
        AZStd::string sType = typeNode->getTag();

        // tags should be unique
        if (m_tagFileNames.find(sType) != m_tagFileNames.end())
        {
            continue;
        }

        TStringVec vEntries;
        for (int j = 0; j < typeNode->getChildCount(); j++)
        {
            XmlNodeRef entry = typeNode->getChild(j);
            if (!entry->isTag("entry"))
            {
                continue;
            }

            vEntries.push_back(entry->getContent());
        }

        CRY_ASSERT(m_tagFileNames.size() < 255);

        uint8 curNumTags = static_cast<uint8>(m_tagFileNames.size());

        m_tagFileNames[sType].filenames = vEntries;
        m_tagFileNames[sType].id                = curNumTags + 1;
        m_tagFileNames[sType].loaded        = false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::RequestLoadLocalizationDataByTag(const char* sTag)
{
    TTagFileNames::iterator it = m_tagFileNames.find(sTag);
    if (it == m_tagFileNames.end())
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[LocError] RequestLoadLocalizationDataByTag - Localization tag '%s' not found", sTag);
        return false;
    }

    if (m_cvarLocalizationDebug >= 2)
    {
        CryLog("<Localization> RequestLoadLocalizationDataByTag %s", sTag);
    }

    m_tagLoadRequests.push_back(sTag);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LoadLocalizationDataByTag(
    const char* sTag, bool bReload)
{
    TTagFileNames::iterator it = m_tagFileNames.find(sTag);
    if (it == m_tagFileNames.end())
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[LocError] LoadLocalizationDataByTag - Localization tag '%s' not found", sTag);
        return false;
    }

    if (it->second.loaded)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[LocError] LoadLocalizationDataByTag - Already loaded tag '%s'", sTag);
        return true;
    }

    bool bResult = true;

    stack_string const sLocalizationFolder(PathUtil::GetLocalizationFolder());

    LoadFunc loadFunction = GetLoadFunction();
    TStringVec& vEntries = it->second.filenames;
    for (TStringVec::iterator it2 = vEntries.begin(); it2 != vEntries.end(); ++it2)
    {
        //Only load files of the correct type for the configured format
        if ((m_cvarLocalizationFormat == 0 && strstr(it2->c_str(), ".xml")) || (m_cvarLocalizationFormat == 1 && strstr(it2->c_str(), ".agsxml")))
        {
            bResult &= (this->*loadFunction)(it2->c_str(), it->second.id, bReload);
        }
    }

    if (m_cvarLocalizationDebug >= 2)
    {
        CryLog("<Localization> LoadLocalizationDataByTag %s with result %d", sTag, bResult);
    }

    it->second.loaded = true;

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::ReleaseLocalizationDataByTag(
    const char* sTag)
{
    INDENT_LOG_DURING_SCOPE(true, "Releasing localization data with the tag '%s'", sTag);
    ListAndClearProblemLabels();

    TTagFileNames::iterator it = m_tagFileNames.find(sTag);
    if (it == m_tagFileNames.end())
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[LocError] ReleaseLocalizationDataByTag - Localization tag '%s' not found", sTag);
        return false;
    }

    if (it->second.loaded == false)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[LocError] ReleaseLocalizationDataByTag - tag '%s' not loaded", sTag);
        return false;
    }

    const uint8 nTagID = it->second.id;

    tmapFilenames newLoadedTables;
    for (tmapFilenames::iterator iter = m_loadedTables.begin(); iter != m_loadedTables.end(); iter++)
    {
        if (iter->second.nTagID != nTagID)
        {
            newLoadedTables[iter->first] = iter->second;
        }
    }
    m_loadedTables = newLoadedTables;

    if (m_pLanguage)
    {
        //LARGE_INTEGER liStart;
        //QueryPerformanceCounter(&liStart);
        AutoLock lock(m_cs);    //Make sure to lock, as this is a modifying operation

        bool bMapEntryErased = false;
        //First, remove entries from the map
        for (StringsKeyMap::iterator keyMapIt = m_pLanguage->m_keysMap.begin(); keyMapIt != m_pLanguage->m_keysMap.end(); )
        {
            if (keyMapIt->second->nTagID == nTagID)
            {
                //VECTORMAP ONLY
                keyMapIt = m_pLanguage->m_keysMap.erase(keyMapIt);
                bMapEntryErased = true;
            }
            else
            {
                keyMapIt++;
            }
        }

        if (bMapEntryErased == true)
        {
            StringsKeyMap newMap = m_pLanguage->m_keysMap;
            m_pLanguage->m_keysMap.clearAndFreeMemory();
            m_pLanguage->m_keysMap = newMap;
        }

        bool bVecEntryErased = false;
        //Then remove the entries in the storage vector
        const int32 numEntries = static_cast<int32>(m_pLanguage->m_vLocalizedStrings.size());
        for (int32 i = numEntries - 1; i >= 0; i--)
        {
            SLocalizedStringEntry* entry = m_pLanguage->m_vLocalizedStrings[i];
            PREFAST_ASSUME(entry);
            if (entry->nTagID == nTagID)
            {
                if (m_cvarLocalizationEncode == 1)
                {
                    if (entry->huffmanTreeIndex != -1)
                    {
                        HuffmanCoder* pCoder = m_pLanguage->m_vEncoders[entry->huffmanTreeIndex];
                        if (pCoder != NULL)
                        {
                            pCoder->DecRef();
                            if (pCoder->RefCount() == 0)
                            {
                                if (m_cvarLocalizationDebug >= 2)
                                {
                                    CryLog("<Localization> Releasing coder %u as it no longer has associated strings", entry->huffmanTreeIndex);
                                }
                                //This coding table no longer needed, it has no more associated strings
                                SAFE_DELETE(m_pLanguage->m_vEncoders[entry->huffmanTreeIndex]);
                            }
                        }
                    }
                }
                bVecEntryErased = true;
                delete(entry);
                m_pLanguage->m_vLocalizedStrings.erase(m_pLanguage->m_vLocalizedStrings.begin() + i);
            }
        }

        //Shrink the vector if necessary
        if (bVecEntryErased == true)
        {
            SLanguage::TLocalizedStringEntries newVec = m_pLanguage->m_vLocalizedStrings;
            m_pLanguage->m_vLocalizedStrings.clear();
            m_pLanguage->m_vLocalizedStrings = newVec;
        }
    }

    if (m_cvarLocalizationDebug >= 2)
    {
        CryLog("<Localization> ReleaseLocalizationDataByTag %s", sTag);
    }

    it->second.loaded = false;

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LoadAllLocalizationData(bool bReload)
{
    for (TTagFileNames::iterator it = m_tagFileNames.begin(); it != m_tagFileNames.end(); ++it)
    {
        if(!LoadLocalizationDataByTag(it->first.c_str(), bReload))
            return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LoadExcelXmlSpreadsheet(const char* sFileName, bool bReload)
{
    LoadFunc loadFunction = GetLoadFunction();
    return (this->*loadFunction)(sFileName, 0, bReload);
}

enum class YesNoType
{
    Yes,
    No,
    Invalid
};

// parse the yes/no string
/*!
\param szString any of the following strings: yes, enable, true, 1, no, disable, false, 0
\return YesNoType::Yes if szString is yes/enable/true/1, YesNoType::No if szString is no, disable, false, 0 and YesNoType::Invalid if the string is not one of the expected values.
*/
inline YesNoType ToYesNoType(const char* szString)
{
    if (!_stricmp(szString, "yes")
        || !_stricmp(szString, "enable")
        || !_stricmp(szString, "true")
        || !_stricmp(szString, "1"))
    {
        return YesNoType::Yes;
    }
    if (!_stricmp(szString, "no")
        || !_stricmp(szString, "disable")
        || !_stricmp(szString, "false")
        || !_stricmp(szString, "0"))
    {
        return YesNoType::No;
    }
    return YesNoType::Invalid;
}

//////////////////////////////////////////////////////////////////////
// Loads a string-table from a Excel XML Spreadsheet file.
bool CLocalizedStringsManager::DoLoadExcelXmlSpreadsheet(const char* sFileName, uint8 nTagID, bool bReload)
{
    if (!m_pLanguage)
    {
        return false;
    }

    //check if this table has already been loaded
    if (!bReload)
    {
        if (m_loadedTables.find(AZStd::string(sFileName)) != m_loadedTables.end())
        {
            return (true);
        }
    }

    ListAndClearProblemLabels();

    IXmlTableReader* const pXmlTableReader = m_pSystem->GetXmlUtils()->CreateXmlTableReader();
    if (!pXmlTableReader)
    {
        CryLog("Loading Localization File %s failed (XML system failure)!", sFileName);
        return false;
    }

    XmlNodeRef root;
    AZStd::string sPath;
    {
        const AZStd::string sLocalizationFolder(PathUtil::GetLocalizationRoot());
        const AZStd::string& languageFolder = m_pLanguage->sLanguage;
        sPath = sLocalizationFolder.c_str() + languageFolder + PathUtil::GetSlash() + sFileName;
        root = m_pSystem->LoadXmlFromFile(sPath.c_str());
        if (!root)
        {
            CryLog("Loading Localization File %s failed!", sPath.c_str());
            pXmlTableReader->Release();
            return false;
        }
    }

    // bug search, re-export to a file to compare it
    //string sReExport = sFileName;
    //sReExport += ".re";
    //root->saveToFile(sReExport.c_str());

    CryLog("Loading Localization File %s", sFileName);
    INDENT_LOG_DURING_SCOPE();

    //Create a huffman coding table for these strings - if they're going to be encoded or compressed
    HuffmanCoder* pEncoder = NULL;
    uint8 iEncoder = 0;
    size_t startOfStringsToCompress = 0;
    if (m_cvarLocalizationEncode == 1)
    {
        {
            for (iEncoder = 0; iEncoder < m_pLanguage->m_vEncoders.size(); iEncoder++)
            {
                if (m_pLanguage->m_vEncoders[iEncoder] == NULL)
                {
                    m_pLanguage->m_vEncoders[iEncoder] = pEncoder = new HuffmanCoder();
                    break;
                }
            }
            if (iEncoder == m_pLanguage->m_vEncoders.size())
            {
                pEncoder = new HuffmanCoder();
                m_pLanguage->m_vEncoders.push_back(pEncoder);
            }
            //Make a note of the current end of the loc strings array, as encoding is done in two passes.
            //One pass to build the code table, another to apply it
            pEncoder->Init();
        }
        startOfStringsToCompress = m_pLanguage->m_vLocalizedStrings.size();
    }

    {
        if (!pXmlTableReader->Begin(root))
        {
            CryLog("Loading Localization File %s failed! The file is in an unsupported format.", sPath.c_str());
            pXmlTableReader->Release();
            return false;
        }
    }

    int rowCount = pXmlTableReader->GetEstimatedRowCount();
    {
        AutoLock lock(m_cs);    //Make sure to lock, as this is a modifying operation
        m_pLanguage->m_vLocalizedStrings.reserve(m_pLanguage->m_vLocalizedStrings.size() + rowCount);
    }
    {
        AutoLock lock(m_cs);    //Make sure to lock, as this is a modifying operation
        //VectorMap only, not applicable to std::map
        m_pLanguage->m_keysMap.reserve(m_pLanguage->m_keysMap.size() + rowCount);
    }

    {
        pairFileName sNewFile;
        sNewFile.first = sFileName;
        sNewFile.second.bDataStripping = false; // this is off for now
        sNewFile.second.nTagID = nTagID;
        m_loadedTables.insert(sNewFile);
    }

    // Cell Index
    char nCellIndexToType[MAX_CELL_COUNT];
    memset(nCellIndexToType, 0, sizeof(nCellIndexToType));

    // SoundMood Index
    std::map<int, AZStd::string> SoundMoodIndex;

    // EventParameter Index
    std::map<int, AZStd::string> EventParameterIndex;

    bool bFirstRow = true;

    AZStd::fixed_string<LOADING_FIXED_STRING_LENGTH> sTmp;

    // lower case event name
    char szLowerCaseEvent[128];
    // lower case key
    char szLowerCaseKey[1024];
    // key CRC
    uint32 keyCRC;

    size_t nMemSize = 0;

    for (;; )
    {
        int nRowIndex = -1;
        {
            if (!pXmlTableReader->ReadRow(nRowIndex))
            {
                break;
            }
        }

        if (bFirstRow)
        {
            bFirstRow = false;
            ParseFirstLine(pXmlTableReader, nCellIndexToType, SoundMoodIndex, EventParameterIndex);
            // Skip first row, it contains description only.
            continue;
        }

        bool bValidKey = false;
        bool bValidTranslatedText = false;
        bool bValidTranslatedCharacterName = false;
        bool bValidTranslatedActorLine = false;
        bool bUseSubtitle = true;
        bool bIsDirectRadio = false;
        bool bIsIntercepted = false;

        struct CConstCharArray
        {
            const char* ptr;
            size_t count;

            CConstCharArray()
            {
                clear();
            }
            void clear()
            {
                ptr = "";
                count = 0;
            }
            bool empty() const
            {
                return count == 0;
            }
        };

        CConstCharArray sKeyString;
        CConstCharArray sCharacterName;
        CConstCharArray sTranslatedCharacterName; // Legacy, to be removed some day...
        CConstCharArray sSubtitleText;
        CConstCharArray sTranslatedText; // Legacy, to be removed some day...
        CConstCharArray sActorLine;
        CConstCharArray sTranslatedActorLine; // Legacy, to be removed some day...
        CConstCharArray sSoundEvent;

        float fVolume = 1.0f;
        float fRadioRatio = 1.0f;
        float fEventParameterValue = 0.0f;
        float fSoundMoodValue = 0.0f;

        int nItems = 0;
        std::map<int, float> SoundMoodValues;
        std::map<int, float> EventParameterValues;

        for (;; )
        {
            int nCellIndex = -1;
            CConstCharArray cell;

            {
                if (!pXmlTableReader->ReadCell(nCellIndex, cell.ptr, cell.count))
                {
                    break;
                }
            }

            if (nCellIndex >= MAX_CELL_COUNT)
            {
                break;
            }

            // skip empty cells
            if (cell.count <= 0)
            {
                continue;
            }

            const char nCellType = nCellIndexToType[nCellIndex];

            switch (nCellType)
            {
            case ELOCALIZED_COLUMN_SKIP:
                break;
            case ELOCALIZED_COLUMN_KEY:
                sKeyString = cell;
                bValidKey = true;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_AUDIOFILE:
                sKeyString = cell;
                bValidKey = true;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_CHARACTER_NAME:
                sCharacterName = cell;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_SUBTITLE_TEXT:
                sSubtitleText = cell;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_ACTOR_LINE:
                sActorLine = cell;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_USE_SUBTITLE:
                sTmp.assign(cell.ptr, cell.count);
                bUseSubtitle = ToYesNoType(sTmp.c_str()) == YesNoType::No ? false : true; // favor yes (yes and invalid -> yes)
                break;
            case ELOCALIZED_COLUMN_VOLUME:
                sTmp.assign(cell.ptr, cell.count);
                fVolume = (float)atof(sTmp.c_str());
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_SOUNDEVENT:
                sSoundEvent = cell;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_RADIO_RATIO:
                sTmp.assign(cell.ptr, cell.count);
                fRadioRatio = (float)atof(sTmp.c_str());
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_EVENTPARAMETER:
                sTmp.assign(cell.ptr, cell.count);
                fEventParameterValue = (float)atof(sTmp.c_str());
                {
                    EventParameterValues[nCellIndex] = fEventParameterValue;
                }
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_SOUNDMOOD:
                sTmp.assign(cell.ptr, cell.count);
                fSoundMoodValue = (float)atof(sTmp.c_str());
                {
                    SoundMoodValues[nCellIndex] = fSoundMoodValue;
                }
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_IS_DIRECT_RADIO:
                sTmp.assign(cell.ptr, cell.count);
                if (!_stricmp(sTmp.c_str(), "intercept"))
                {
                    bIsIntercepted = true;
                }
                bIsDirectRadio = bIsIntercepted || (ToYesNoType(sTmp.c_str()) == YesNoType::Yes ? true : false); // favor no (no and invalid -> no)
                ++nItems;
                break;
            // legacy names
            case ELOCALIZED_COLUMN_LEGACY_PERSON:
                // old file often only have content in this column
                if (!cell.empty())
                {
                    sCharacterName = cell;
                    sTranslatedCharacterName = cell;
                    bValidTranslatedCharacterName = true;
                }
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_CHARACTERNAME:
                sCharacterName = cell;
                sTranslatedCharacterName = cell;
                bValidTranslatedCharacterName = true;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_TRANSLATED_CHARACTERNAME:
                sTranslatedCharacterName = cell;
                bValidTranslatedCharacterName = true;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_ENGLISH_DIALOGUE:
                // old file often only have content in this column
                sActorLine = cell;
                sSubtitleText = cell;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_TRANSLATION:
                sTranslatedActorLine = cell;
                sTranslatedText = cell;
                bValidTranslatedText = true;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_YOUR_TRANSLATION:
                sTranslatedActorLine = cell;
                sTranslatedText = cell;
                bValidTranslatedText = true;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_ENGLISH_SUBTITLE:
                sSubtitleText = cell;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_TRANSLATED_SUBTITLE:
                sTranslatedText = cell;
                sTranslatedActorLine = cell;
                bValidTranslatedText = true;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_ORIGINAL_CHARACTER_NAME:
                sCharacterName = cell;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_TRANSLATED_CHARACTER_NAME:
                sTranslatedCharacterName = cell;
                bValidTranslatedCharacterName = true;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_ORIGINAL_TEXT:
                sSubtitleText = cell;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_TRANSLATED_TEXT:
                sTranslatedText = cell;
                bValidTranslatedText = true;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_ORIGINAL_ACTOR_LINE:
                sActorLine = cell;
                ++nItems;
                break;
            case ELOCALIZED_COLUMN_LEGACY_TRANSLATED_ACTOR_LINE:
                sTranslatedActorLine = cell;
                bValidTranslatedActorLine = true;
                ++nItems;
                break;
            }
        }

        if (!bValidKey)
        {
            continue;
        }

        if (!bValidTranslatedText)
        {
            // if this is a dialog entry with a soundevent and with subtitles then a warning should be issued
            if (m_cvarLocalizationDebug && !sSoundEvent.empty() && bUseSubtitle)
            {
                sTmp.assign(sKeyString.ptr, sKeyString.count);
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[LocError] Key '%s' in file <%s> has no translated text", sTmp.c_str(), sFileName);
            }

            // use translated actor line entry if available before falling back to original entry
            if (!sTranslatedActorLine.empty())
            {
                sTranslatedText = sTranslatedActorLine;
            }
            else
            {
                sTranslatedText = sSubtitleText;
            }
        }

        if (!bValidTranslatedActorLine)
        {
            // if this is a dialog entry with a soundevent then a warning should be issued
            if (m_cvarLocalizationDebug && !sSoundEvent.empty())
            {
                sTmp.assign(sKeyString.ptr, sKeyString.count);
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[LocError] Key '%s' in file <%s> has no translated actor line", sTmp.c_str(), sFileName);
            }

            // use translated text entry if available before falling back to original entry
            if (!sTranslatedText.empty())
            {
                sTranslatedActorLine = sTranslatedText;
            }
            else
            {
                sTranslatedActorLine = sSubtitleText;
            }
        }

        if (!sSoundEvent.empty() && !bValidTranslatedCharacterName)
        {
            if (m_cvarLocalizationDebug)
            {
                sTmp.assign(sKeyString.ptr, sKeyString.count);
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[LocError] Key '%s' in file <%s> has no translated character name", sTmp.c_str(), sFileName);
            }

            sTranslatedCharacterName = sCharacterName;
        }

        if (nItems == 1) // skip lines which contain just one item in the key
        {
            continue;
        }

        // reject to store text if line was marked with no subtitles in game mode
        if (!gEnv->IsEditor())
        {
            if (!bUseSubtitle)
            {
                sSubtitleText.clear();
                sTranslatedText.clear();
            }
        }

        // Skip @ character in the key string.
        if (!sKeyString.empty() && sKeyString.ptr[0] == '@')
        {
            sKeyString.ptr++;
            sKeyString.count--;
        }

        {
            CopyLowercase(szLowerCaseEvent, sizeof(szLowerCaseEvent), sSoundEvent.ptr, sSoundEvent.count);
            CopyLowercase(szLowerCaseKey, sizeof(szLowerCaseKey), sKeyString.ptr, sKeyString.count);
        }

        //Compute the CRC32 of the key
        keyCRC = AZ::Crc32(szLowerCaseKey);
        if (m_cvarLocalizationDebug >= 3)
        {
            CryLogAlways("<Localization dupe/clash detection> CRC32: 0x%8X, Key: %s", keyCRC, szLowerCaseKey);
        }

        if (m_pLanguage->m_keysMap.find(keyCRC) != m_pLanguage->m_keysMap.end())
        {
            sTmp.assign(sKeyString.ptr, sKeyString.count);
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[LocError] Localized String '%s' Already Loaded for Language %s OR there is a CRC hash clash", sTmp.c_str(), m_pLanguage->sLanguage.c_str());
            continue;
        }

        SLocalizedStringEntry* pEntry = new SLocalizedStringEntry();
        pEntry->flags = 0;

        if (bUseSubtitle == true)
        {
            pEntry->flags |= SLocalizedStringEntry::USE_SUBTITLE;
        }
        pEntry->nTagID = nTagID;

        if (gEnv->IsEditor())
        {
            pEntry->pEditorExtension = new SLocalizedStringEntryEditorExtension();

            pEntry->pEditorExtension->sKey = szLowerCaseKey;

            pEntry->pEditorExtension->nRow = nRowIndex;

            if (!sActorLine.empty())
            {
                sTmp.assign(sActorLine.ptr, sActorLine.count);
                ReplaceEndOfLine(sTmp);
                pEntry->pEditorExtension->sOriginalActorLine.assign(sTmp.c_str());
            }
            if (!sTranslatedActorLine.empty())
            {
                sTmp.assign(sTranslatedActorLine.ptr, sTranslatedActorLine.count);
                ReplaceEndOfLine(sTmp);
                pEntry->pEditorExtension->sUtf8TranslatedActorLine.append(sTmp.c_str());
            }
            if (bUseSubtitle && !sSubtitleText.empty())
            {
                sTmp.assign(sSubtitleText.ptr, sSubtitleText.count);
                ReplaceEndOfLine(sTmp);
                pEntry->pEditorExtension->sOriginalText.assign(sTmp.c_str());
            }
            // only use the translated character name
            {
                pEntry->pEditorExtension->sOriginalCharacterName.assign(sCharacterName.ptr, sCharacterName.count);
            }
        }

        if (bUseSubtitle && !sTranslatedText.empty())
        {
            sTmp.assign(sTranslatedText.ptr, sTranslatedText.count);
            ReplaceEndOfLine(sTmp);
            if (m_cvarLocalizationEncode == 1)
            {
                pEncoder->Update((const uint8*)(sTmp.c_str()), sTmp.length());
                //CryLogAlways("%u Storing %s (%u)", m_pLanguage->m_vLocalizedStrings.size(), sTmp.c_str(), sTmp.length());
                pEntry->TranslatedText.szCompressed = new uint8[sTmp.length() + 1];
                pEntry->flags |= SLocalizedStringEntry::IS_COMPRESSED;
                //Store the raw string. It'll be compressed later
                memcpy(pEntry->TranslatedText.szCompressed, sTmp.c_str(), sTmp.length());
                pEntry->TranslatedText.szCompressed[sTmp.length()] = '\0';  //Null terminate
            }
            else
            {
                pEntry->TranslatedText.psUtf8Uncompressed = new AZStd::string(sTmp.c_str(), sTmp.c_str() + sTmp.length());
            }
        }

        // the following is used to cleverly assign strings
        // we store all known string into the m_prototypeEvents set and assign known entries from there
        // the CryString makes sure, that only the ref-count is increment on assignment
        if (*szLowerCaseEvent)
        {
            PrototypeSoundEvents::iterator it = m_prototypeEvents.find(AZStd::string(szLowerCaseEvent));
            if (it != m_prototypeEvents.end())
            {
                pEntry->sPrototypeSoundEvent = *it;
            }
            else
            {
                pEntry->sPrototypeSoundEvent = szLowerCaseEvent;
                m_prototypeEvents.insert(pEntry->sPrototypeSoundEvent);
            }
        }

        const CConstCharArray sWho = sTranslatedCharacterName.empty() ? sCharacterName : sTranslatedCharacterName;
        if (!sWho.empty())
        {
            sTmp.assign(sWho.ptr, sWho.count);
            ReplaceEndOfLine(sTmp);
            AZStd::replace(sTmp.begin(), sTmp.end(), ' ', '_');
            AZStd::string tmp;
            {
                tmp = sTmp.c_str();
            }
            CharacterNameSet::iterator it = m_characterNameSet.find(tmp);
            if (it != m_characterNameSet.end())
            {
                pEntry->sCharacterName = *it;
            }
            else
            {
                pEntry->sCharacterName = tmp;
                m_characterNameSet.insert(pEntry->sCharacterName);
            }
        }

        pEntry->fVolume = CryConvertFloatToHalf(fVolume);

        // SoundMood Entries
        {
            pEntry->SoundMoods.resize(static_cast<int>(SoundMoodValues.size()));
            if (SoundMoodValues.size() > 0)
            {
                std::map<int, float>::const_iterator itEnd = SoundMoodValues.end();
                int nSoundMoodCount = 0;
                for (std::map<int, float>::const_iterator it = SoundMoodValues.begin(); it != itEnd; ++it)
                {
                    pEntry->SoundMoods[nSoundMoodCount].fValue = (*it).second;
                    pEntry->SoundMoods[nSoundMoodCount].sName = SoundMoodIndex[(*it).first];
                    ++nSoundMoodCount;
                }
            }
        }

        // EventParameter Entries
        {
            pEntry->EventParameters.resize(static_cast<int>(EventParameterValues.size()));
            if (EventParameterValues.size() > 0)
            {
                std::map<int, float>::const_iterator itEnd = EventParameterValues.end();
                int nEventParameterCount = 0;
                for (std::map<int, float>::const_iterator it = EventParameterValues.begin(); it != itEnd; ++it)
                {
                    pEntry->EventParameters[nEventParameterCount].fValue = (*it).second;
                    pEntry->EventParameters[nEventParameterCount].sName = EventParameterIndex[(*it).first];
                    ++nEventParameterCount;
                }
            }
        }

        pEntry->fRadioRatio = CryConvertFloatToHalf(fRadioRatio);

        if (bIsDirectRadio == true)
        {
            pEntry->flags |= SLocalizedStringEntry::IS_DIRECTED_RADIO;
        }
        if (bIsIntercepted == true)
        {
            pEntry->flags |= SLocalizedStringEntry::IS_INTERCEPTED;
        }

        nMemSize += sizeof(*pEntry) + pEntry->sCharacterName.length() * sizeof(char);
        if (m_cvarLocalizationEncode == 0)
        {
            //Note that this isn't accurate if we're using encoding/compression to shrink the string as the encoding step hasn't happened yet
            if (pEntry->TranslatedText.psUtf8Uncompressed)
            {
                nMemSize += pEntry->TranslatedText.psUtf8Uncompressed->length() * sizeof(char);
            }
        }
        if (pEntry->pEditorExtension != NULL)
        {
            nMemSize += pEntry->pEditorExtension->sKey.length()
                + pEntry->pEditorExtension->sOriginalActorLine.length()
                + pEntry->pEditorExtension->sUtf8TranslatedActorLine.length() * sizeof(char)
                + pEntry->pEditorExtension->sOriginalText.length()
                + pEntry->pEditorExtension->sOriginalCharacterName.length();
        }


        // Compression Preparation
        //unsigned int nSourceSize = pEntry->swTranslatedText.length()*sizeof(wchar_t);
        //if (nSourceSize)
        //  int zResult = Compress(pDest, nDestLen, pEntry->swTranslatedText.c_str(), nSourceSize);

        AddLocalizedString(m_pLanguage, pEntry, keyCRC);
    }

    if (m_cvarLocalizationEncode == 1)
    {
        pEncoder->Finalize();

        {
            uint8 compressionBuffer[COMPRESSION_FIXED_BUFFER_LENGTH];
            //uint8 decompressionBuffer[COMPRESSION_FIXED_BUFFER_LENGTH];
            size_t uncompressedTotal = 0, compressedTotal = 0;
            for (size_t stringToCompress = startOfStringsToCompress; stringToCompress < m_pLanguage->m_vLocalizedStrings.size(); stringToCompress++)
            {
                SLocalizedStringEntry* pStringToCompress = m_pLanguage->m_vLocalizedStrings[stringToCompress];
                if (pStringToCompress->TranslatedText.szCompressed != NULL)
                {
                    size_t compBufSize = COMPRESSION_FIXED_BUFFER_LENGTH;
                    memset(compressionBuffer, 0, COMPRESSION_FIXED_BUFFER_LENGTH);
                    //CryLogAlways("%u Compressing %s (%p)", stringToCompress, pStringToCompress->szCompressedTranslatedText, pStringToCompress->szCompressedTranslatedText);
                    size_t inputStringLength = strlen((const char*)(pStringToCompress->TranslatedText.szCompressed));
                    pEncoder->CompressInput(pStringToCompress->TranslatedText.szCompressed, inputStringLength, compressionBuffer, &compBufSize);
                    compressionBuffer[compBufSize] = 0;
                    pStringToCompress->huffmanTreeIndex = iEncoder;
                    pEncoder->AddRef();
                    //CryLogAlways("Compressed %s (%u) to %s (%u)", pStringToCompress->szCompressedTranslatedText, strlen((const char*)pStringToCompress->szCompressedTranslatedText), compressionBuffer, compBufSize);
                    uncompressedTotal += inputStringLength;
                    compressedTotal += compBufSize;

                    uint8* szCompressedString = new uint8[compBufSize];
                    SAFE_DELETE_ARRAY(pStringToCompress->TranslatedText.szCompressed);

                    memcpy(szCompressedString, compressionBuffer, compBufSize);
                    pStringToCompress->TranslatedText.szCompressed = szCompressedString;

                    //Testing code
                    //memset( decompressionBuffer, 0, COMPRESSION_FIXED_BUFFER_LENGTH );
                    //size_t decompBufSize = pEncoder->UncompressInput(compressionBuffer, COMPRESSION_FIXED_BUFFER_LENGTH, decompressionBuffer, COMPRESSION_FIXED_BUFFER_LENGTH);
                    //CryLogAlways("Decompressed %s (%u) to %s (%u)", compressionBuffer, compBufSize, decompressionBuffer, decompBufSize);
                }
            }
            //CryLogAlways("[LOC PROFILING] %s, %u, Uncompressed %u, Compressed %u", sFileName, m_pLanguage->m_vLocalizedStrings.size() - startOfStringsToCompress, uncompressedTotal, compressedTotal);
        }
    }

    pXmlTableReader->Release();

    return true;
}

bool CLocalizedStringsManager::DoLoadAGSXmlDocument(const char* sFileName, uint8 nTagID, bool bReload)
{
    if (!sFileName)
    {
        return false;
    }
    if (!m_pLanguage)
    {
        return false;
    }
    if (!bReload)
    {
        if (m_loadedTables.find(AZStd::string(sFileName)) != m_loadedTables.end())
        {
            return true;
        }
    }
    ListAndClearProblemLabels();
    XmlNodeRef root;
    AZStd::string sPath;
    {
        const AZStd::string sLocalizationFolder(PathUtil::GetLocalizationRoot());
        const AZStd::string& languageFolder = m_pLanguage->sLanguage;
        sPath = sLocalizationFolder.c_str() + languageFolder + PathUtil::GetSlash() + sFileName;
        root = m_pSystem->LoadXmlFromFile(sPath.c_str());
        if (!root)
        {
            AZ_TracePrintf(LOC_WINDOW, "Loading Localization File %s failed!", sPath.c_str());
            return false;
        }
    }
    AZ_TracePrintf(LOC_WINDOW, "Loading Localization File %s", sPath.c_str());
    HuffmanCoder* pEncoder = nullptr;
    uint8 iEncoder = 0;
    size_t startOfStringsToCompress = 0;
    if (m_cvarLocalizationEncode == 1)
    {
        {
            for (iEncoder = 0; iEncoder < m_pLanguage->m_vEncoders.size(); iEncoder++)
            {
                if (m_pLanguage->m_vEncoders[iEncoder] == nullptr)
                {
                    pEncoder = new HuffmanCoder();
                    m_pLanguage->m_vEncoders[iEncoder] = pEncoder;
                    break;
                }
            }
            if (iEncoder == m_pLanguage->m_vEncoders.size())
            {
                pEncoder = new HuffmanCoder();
                m_pLanguage->m_vEncoders.push_back(pEncoder);
            }
            pEncoder->Init();
        }
        startOfStringsToCompress = m_pLanguage->m_vLocalizedStrings.size();
    }
    int rowCount = 0;
    {
        AutoLock lock(m_cs);    // Make sure to lock, as this is a modifying operation
        rowCount = root->getChildCount();
        {
            m_pLanguage->m_vLocalizedStrings.reserve(m_pLanguage->m_vLocalizedStrings.size() + rowCount);
        }
        m_pLanguage->m_keysMap.reserve(m_pLanguage->m_keysMap.size() + rowCount);
    }
    {
        pairFileName sNewFile;
        sNewFile.first = sFileName;
        sNewFile.second.bDataStripping = false; // this is off for now
        sNewFile.second.nTagID = nTagID;
        m_loadedTables.insert(sNewFile);
    }
    const char* key = nullptr;
    AZStd::string keyString;
    AZStd::string lowerKey;
    AZStd::string textValue;
    uint32 keyCRC=0;
    for (int i = 0; i < rowCount; ++i)
    {
        XmlNodeRef childNode = root->getChild(i);
        if (azstricmp(childNode->getTag(), "string") || !childNode->getAttr("key", &key))
        {
            continue;
        }
        keyString = key;
        textValue = childNode->getContent();
        if (textValue.empty())
        {
            continue;
        }
        AzFramework::StringFunc::Replace(textValue, "\\n", " \n");
        if (keyString[0] == '@')
        {
            AzFramework::StringFunc::LChop(keyString, 1);
        }
        lowerKey = keyString;
        AZStd::to_lower(lowerKey.begin(), lowerKey.end());
        keyCRC = AZ::Crc32(lowerKey);
        if (m_cvarLocalizationDebug >= 3)
        {
            CryLogAlways("<Localization dupe/clash detection> CRC32: 0%8X, Key: %s", keyCRC, lowerKey.c_str());
        }
        if (m_pLanguage->m_keysMap.find(keyCRC) != m_pLanguage->m_keysMap.end())
        {
            AZ_Warning(LOC_WINDOW, false, "Localized String '%s' Already Loaded for Language %s OR there is a CRC hash clash", keyString.c_str(), m_pLanguage->sLanguage.c_str());
            continue;
        }
        SLocalizedStringEntry* pEntry = new SLocalizedStringEntry;
        pEntry->flags = SLocalizedStringEntry::USE_SUBTITLE;
        pEntry->nTagID = nTagID;
        if (gEnv->IsEditor())
        {
            pEntry->pEditorExtension = new SLocalizedStringEntryEditorExtension();
            pEntry->pEditorExtension->sKey = lowerKey.c_str();
            pEntry->pEditorExtension->nRow = i;
            {
                pEntry->pEditorExtension->sUtf8TranslatedActorLine.append(textValue.c_str());
            }
            {
                pEntry->pEditorExtension->sOriginalText.assign(textValue.c_str());
            }
        }
        {
            const char* textString = textValue.c_str();
            size_t textLength = textValue.length();
            if (m_cvarLocalizationEncode == 1)
            {
                pEncoder->Update((const uint8*)(textString), textLength);
                pEntry->TranslatedText.szCompressed = new uint8[textLength + 1];
                pEntry->flags |= SLocalizedStringEntry::IS_COMPRESSED;
                memcpy(pEntry->TranslatedText.szCompressed, textString, textLength);
                pEntry->TranslatedText.szCompressed[textLength] = '\0';  //Null terminate
            }
            else
            {
                pEntry->TranslatedText.psUtf8Uncompressed = new AZStd::string(textString, textString + textLength);
            }
        }
        {
            AddLocalizedString(m_pLanguage, pEntry, keyCRC);
        }
    }
    if (m_cvarLocalizationEncode == 1)
    {
        {
            pEncoder->Finalize();
        }
        {
            uint8 compressionBuffer[COMPRESSION_FIXED_BUFFER_LENGTH] = {};
            size_t uncompressedTotal = 0, compressedTotal = 0;
            for (size_t stringToCompress = startOfStringsToCompress; stringToCompress < m_pLanguage->m_vLocalizedStrings.size(); stringToCompress++)
            {
                SLocalizedStringEntry* pStringToCompress = m_pLanguage->m_vLocalizedStrings[stringToCompress];
                if (pStringToCompress->TranslatedText.szCompressed != nullptr)
                {
                    size_t compBufSize = COMPRESSION_FIXED_BUFFER_LENGTH;
                    memset(compressionBuffer, 0, COMPRESSION_FIXED_BUFFER_LENGTH);
                    size_t inputStringLength = strnlen((const char*)(pStringToCompress->TranslatedText.szCompressed), COMPRESSION_FIXED_BUFFER_LENGTH);
                    pEncoder->CompressInput(pStringToCompress->TranslatedText.szCompressed, inputStringLength, compressionBuffer, &compBufSize);
                    compressionBuffer[compBufSize] = 0;
                    pStringToCompress->huffmanTreeIndex = iEncoder;
                    pEncoder->AddRef();
                    uncompressedTotal += inputStringLength;
                    compressedTotal += compBufSize;
                    uint8* szCompressedString = new uint8[compBufSize];
                    SAFE_DELETE_ARRAY(pStringToCompress->TranslatedText.szCompressed);
                    memcpy(szCompressedString, compressionBuffer, compBufSize);
                    pStringToCompress->TranslatedText.szCompressed = szCompressedString;
                }
            }
        }
    }
    return true;
}
//////////////////////////////////////////////////////////////////////////
CLocalizedStringsManager::LoadFunc CLocalizedStringsManager::GetLoadFunction() const
{
    CRY_ASSERT_MESSAGE(gEnv && gEnv->pConsole, "System environment or console missing!");
    if (gEnv && gEnv->pConsole)
    {
        if(m_cvarLocalizationFormat == 1)
        {
            return &CLocalizedStringsManager::DoLoadAGSXmlDocument;
        }
    }
    return &CLocalizedStringsManager::DoLoadExcelXmlSpreadsheet;
}
void CLocalizedStringsManager::ReloadData()
{
    tmapFilenames temp = m_loadedTables;

    LoadFunc loadFunction = GetLoadFunction();
    FreeLocalizationData();
    for (tmapFilenames::iterator it = temp.begin(); it != temp.end(); it++)
    {
        (this->*loadFunction)((*it).first.c_str(), (*it).second.nTagID, true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::AddLocalizedString(SLanguage* pLanguage, SLocalizedStringEntry* pEntry, const uint32 keyCRC32)
{
    pLanguage->m_vLocalizedStrings.push_back(pEntry);
    [[maybe_unused]] int nId = (int)pLanguage->m_vLocalizedStrings.size() - 1;
    pLanguage->m_keysMap[keyCRC32] = pEntry;

    if (m_cvarLocalizationDebug >= 2)
    {
        CryLog("<Localization> Add new string <%u> with ID %d to <%s>", keyCRC32, nId, pLanguage->sLanguage.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LocalizeString_ch(const char* sString, AZStd::string& outLocalizedString, bool bEnglish)
{
    return LocalizeStringInternal(sString, strlen(sString), outLocalizedString, bEnglish);
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LocalizeString_s(const AZStd::string& sString, AZStd::string& outLocalizedString, bool bEnglish)
{
    return LocalizeStringInternal(sString.c_str(), sString.length(), outLocalizedString, bEnglish);
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LocalizeStringInternal(const char* pStr, size_t len, AZStd::string& outLocalizedString, bool bEnglish)
{
    assert (m_pLanguage);
    if (m_pLanguage == 0)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "LocalizeString: No language set.");
        outLocalizedString.assign(pStr, pStr + len);
        return false;
    }

    // note: we don't write directly to outLocalizedString, in case it aliases pStr
    AZStd::string out;

    // scan the string
    const char* pPos = pStr;
    const char* pEnd = pStr + len;
    while (true)
    {
        const char* pLabel = strchr(pPos, '@');
        if (!pLabel)
        {
            break;
        }
        // found an occurrence

        // we have skipped a few characters, so copy them over
        if (pLabel != pPos)
        {
            out.append(pPos, pLabel);
        }

        // Search label for first occurence of any label terminating character
        const char* pLabelEnd = strpbrk(pLabel, " !\"#$%&\'()*+,./:;<=>\?[\\]^`{|}~\n\t\r");
        if (!pLabelEnd)
        {
            pLabelEnd = pEnd;
        }

        // localize token
        AZStd::string token(pLabel, pLabelEnd);
        AZStd::string sLocalizedToken;
        if (bEnglish)
        {
            GetEnglishString(token.c_str(), sLocalizedToken);
        }
        else
        {
            LocalizeLabel(token.c_str(), sLocalizedToken);
        }
        out.append(sLocalizedToken);
        pPos = pLabelEnd;
    }
    out.append(pPos, pEnd);
    out.swap(outLocalizedString);
    return true;
}

void CLocalizedStringsManager::LocalizeAndSubstituteInternal(AZStd::string& locString, const AZStd::vector<AZStd::string>& keys, const AZStd::vector<AZStd::string>& values)
{
    AZStd::string outString;
    LocalizeString_ch(locString.c_str(), outString);
    locString = outString .c_str();
    if (values.size() != keys.size())
    {
        AZ_Warning("game", false, "Localization Error: LocalizeAndSubstitute was given %u keys and %u values to replace. These numbers must be equal.", keys.size(), values.size());
        return;
    }
    size_t startIndex = locString.find('{');
    size_t endIndex = locString.find('}', startIndex);
    while (startIndex != AZStd::string::npos && endIndex != AZStd::string::npos)
    {
        const size_t subLength = endIndex - startIndex - 1;
        AZStd::string substituteOut = locString.substr(startIndex + 1, subLength).c_str();
        int index = 0;
        if (LocalizationHelpers::IsKeyInList(keys, substituteOut, index))
        {
            const char* value = values[index].c_str();
            locString.replace(startIndex, subLength + 2, value);
            startIndex += strlen(value);
        }
        else
        {
            AZ_Warning("game", false, "Localization Error: Localized string '%s' contains a key '%s' that is not mapped to a data element.", locString.c_str(), substituteOut.c_str());
            startIndex += substituteOut.length();
        }
        startIndex = locString.find_first_of('{', startIndex);
        endIndex = locString.find_first_of('}', startIndex);
    }
}
#if defined(LOG_DECOMP_TIMES)
static double g_fSecondsPerTick = 0.0;
static FILE* pDecompLog = NULL;
// engine independent game timer since gEnv/pSystem isn't available yet
static void LogDecompTimer(__int64 nTotalTicks, __int64 nDecompTicks, __int64 nAllocTicks)
{
    if (g_fSecondsPerTick == 0.0)
    {
        __int64 nPerfFreq;
        QueryPerformanceFrequency((LARGE_INTEGER*)&nPerfFreq);
        g_fSecondsPerTick = 1.0 / (double)nPerfFreq;
    }

    if (!pDecompLog)
    {
        char szFilenameBuffer[MAX_PATH];
        time_t rawTime;
        time(&rawTime);
        struct tm* pTimeInfo = localtime(&rawTime);

        CreateDirectory("TestResults\\", 0);
        strftime(szFilenameBuffer, sizeof(szFilenameBuffer), "TestResults\\Decomp_%Y_%m_%d-%H_%M_%S.csv", pTimeInfo);
        pDecompLog = fopen(szFilenameBuffer, "wb");
        fprintf(pDecompLog, "Total,Decomp,Alloc\n");
    }
    float nTotalMillis = float( g_fSecondsPerTick * 1000.0 * nTotalTicks );
    float nDecompMillis = float( g_fSecondsPerTick * 1000.0 * nDecompTicks );
    float nAllocMillis = float( g_fSecondsPerTick * 1000.0 * nAllocTicks );
    fprintf(pDecompLog, "%f,%f,%f\n", nTotalMillis, nDecompMillis, nAllocMillis);
    fflush(pDecompLog);
}
#endif

AZStd::string CLocalizedStringsManager::SLocalizedStringEntry::GetTranslatedText(const SLanguage* pLanguage) const
{
    if ((flags & IS_COMPRESSED) != 0)
    {
#if defined(LOG_DECOMP_TIMES)
        __int64 nTotalTicks, nDecompTicks, nAllocTicks;
        nTotalTicks = CryGetTicks();
#endif  //LOG_DECOMP_TIMES

        AZStd::string outputString;
        if (TranslatedText.szCompressed != NULL)
        {
            uint8 decompressionBuffer[COMPRESSION_FIXED_BUFFER_LENGTH];
            HuffmanCoder* pEncoder = pLanguage->m_vEncoders[huffmanTreeIndex];

#if defined(LOG_DECOMP_TIMES)
            nDecompTicks = CryGetTicks();
#endif  //LOG_DECOMP_TIMES

            //We don't actually know how much memory was allocated for this string, but the maximum compression buffer size is known
            size_t decompBufSize = pEncoder->UncompressInput(TranslatedText.szCompressed, COMPRESSION_FIXED_BUFFER_LENGTH, decompressionBuffer, COMPRESSION_FIXED_BUFFER_LENGTH);
            assert(decompBufSize < COMPRESSION_FIXED_BUFFER_LENGTH && "Buffer overflow");

#if defined(LOG_DECOMP_TIMES)
            nDecompTicks = CryGetTicks() - nDecompTicks;
#endif  //LOG_DECOMP_TIMES

#if !defined(NDEBUG)
            size_t len = strnlen((const char*)decompressionBuffer, COMPRESSION_FIXED_BUFFER_LENGTH);
            assert(len < COMPRESSION_FIXED_BUFFER_LENGTH && "Buffer not null-terminated");
#endif

#if defined(LOG_DECOMP_TIMES)
            nAllocTicks = CryGetTicks();
#endif  //LOG_DECOMP_TIMES

            outputString.assign((const char*)decompressionBuffer, (const char*)decompressionBuffer + decompBufSize);

#if defined(LOG_DECOMP_TIMES)
            nAllocTicks = CryGetTicks() - nAllocTicks;
            nTotalTicks = CryGetTicks() - nTotalTicks;
            LogDecompTimer(nTotalTicks, nDecompTicks, nAllocTicks);
#endif  //LOG_DECOMP_TIMES
        }
        return outputString;
    }
    else
    {
        if (TranslatedText.psUtf8Uncompressed != NULL)
        {
            return *TranslatedText.psUtf8Uncompressed;
        }
        else
        {
            AZStd::string emptyOutputString;
            return emptyOutputString;
        }
    }
}

#ifndef _RELEASE
//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::LocalizedStringsManagerWarning(const char* label, const char* message)
{
    if (!m_warnedAboutLabels[label])
    {
        CryWarning (VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Failed to localize label '%s' - %s", label, message);
        m_warnedAboutLabels[label] = true;
        m_haveWarnedAboutAtLeastOneLabel = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::ListAndClearProblemLabels()
{
    if (m_haveWarnedAboutAtLeastOneLabel)
    {
        CryLog ("These labels caused localization problems:");
        INDENT_LOG_DURING_SCOPE();

        for (std::map<AZStd::string, bool>::iterator iter = m_warnedAboutLabels.begin(); iter != m_warnedAboutLabels.end(); iter++)
        {
            CryLog ("%s", iter->first.c_str());
        }

        m_warnedAboutLabels.clear();
        m_haveWarnedAboutAtLeastOneLabel = false;
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LocalizeLabel(const char* sLabel, AZStd::string& outLocalString, bool bEnglish)
{
    assert(sLabel);
    if (!m_pLanguage || !sLabel)
    {
        return false;
    }

    // Label sign.
    if (sLabel[0] == '@')
    {
        uint32 labelCRC32 = AZ::Crc32(sLabel + 1);   // skip @ character.
        {
            AutoLock lock(m_cs);    //Lock here, to prevent strings etc being modified underneath this lookup
            SLocalizedStringEntry* entry = stl::find_in_map(m_pLanguage->m_keysMap, labelCRC32, NULL);

            if (entry != NULL)
            {
                AZStd::string translatedText = entry->GetTranslatedText(m_pLanguage);
                if ((bEnglish || translatedText.empty()) && entry->pEditorExtension != NULL)
                {
                    //assert(!"No Localization Text available!");
                    outLocalString.assign(entry->pEditorExtension->sOriginalText);
                    return true;
                }
                else
                {
                    outLocalString.swap(translatedText);
                }
                return true;
            }
            else
            {
                LocalizedStringsManagerWarning(sLabel, "entry not found in string table");
            }
        }
    }
    else
    {
        LocalizedStringsManagerWarning(sLabel, "must start with @ symbol");
    }

    outLocalString.assign(sLabel);

    return false;
}


//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetEnglishString(const char* sKey, AZStd::string& sLocalizedString)
{
    assert(sKey);
    if (!m_pLanguage || !sKey)
    {
        return false;
    }

    // Label sign.
    if (sKey[0] == '@')
    {
        uint32 keyCRC32 = AZ::Crc32(sKey + 1);
        {
            AutoLock lock(m_cs); // Lock here, to prevent strings etc being modified underneath this lookup
            SLocalizedStringEntry* entry = stl::find_in_map(m_pLanguage->m_keysMap, keyCRC32, NULL); // skip @ character.
            if (entry != NULL && entry->pEditorExtension != NULL)
            {
                sLocalizedString = entry->pEditorExtension->sOriginalText;
                return true;
            }
            else
            {
                keyCRC32 = AZ::Crc32(sKey);
                entry = stl::find_in_map(m_pLanguage->m_keysMap, keyCRC32, NULL);
                if (entry != NULL && entry->pEditorExtension != NULL)
                {
                    sLocalizedString = entry->pEditorExtension->sOriginalText;
                    return true;
                }
                else
                {
                    // CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Localized string for Label <%s> not found", sKey );
                    sLocalizedString = sKey;
                    return false;
                }
            }
        }
    }
    else
    {
        // CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Not a valid localized string Label <%s>, must start with @ symbol", sKey
        // );
    }

    sLocalizedString = sKey;
    return false;
}

bool CLocalizedStringsManager::IsLocalizedInfoFound(const char* sKey)
{
    if (!m_pLanguage)
    {
        return false;
    }
    uint32 keyCRC32 = AZ::Crc32(sKey);
    {
        AutoLock lock(m_cs);    //Lock here, to prevent strings etc being modified underneath this lookup
        const SLocalizedStringEntry* entry = stl::find_in_map(m_pLanguage->m_keysMap, keyCRC32, NULL);
        return (entry != NULL);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetLocalizedInfoByKey(const char* sKey, SLocalizedInfoGame& outGameInfo)
{
    if (!m_pLanguage)
    {
        return false;
    }

    uint32 keyCRC32 = AZ::Crc32(sKey);
    {
        AutoLock lock(m_cs);    //Lock here, to prevent strings etc being modified underneath this lookup
        const SLocalizedStringEntry* entry = stl::find_in_map(m_pLanguage->m_keysMap, keyCRC32, NULL);
        if (entry != NULL)
        {
            outGameInfo.szCharacterName = entry->sCharacterName.c_str();
            outGameInfo.sUtf8TranslatedText = entry->GetTranslatedText(m_pLanguage);

            outGameInfo.bUseSubtitle = (entry->flags & SLocalizedStringEntry::USE_SUBTITLE);

            return true;
        }
        else
        {
            return false;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetLocalizedInfoByKey(const char* sKey, SLocalizedSoundInfoGame* pOutSoundInfo)
{
    assert(sKey);
    if (!m_pLanguage || !sKey || !pOutSoundInfo)
    {
        return false;
    }

    bool bResult = false;

    uint32 keyCRC32 = AZ::Crc32(sKey);
    {
        AutoLock lock(m_cs);    //Lock here, to prevent strings etc being modified underneath this lookup
        const SLocalizedStringEntry* pEntry = stl::find_in_map(m_pLanguage->m_keysMap, keyCRC32, NULL);
        if (pEntry != NULL)
        {
            bResult = true;

            pOutSoundInfo->szCharacterName = pEntry->sCharacterName.c_str();
            pOutSoundInfo->sUtf8TranslatedText = pEntry->GetTranslatedText(m_pLanguage);

            //pOutSoundInfo->sOriginalActorLine = pEntry->sOriginalActorLine.c_str();
            //pOutSoundInfo->sTranslatedActorLine = pEntry->swTranslatedActorLine.c_str();
            //pOutSoundInfo->sOriginalText = pEntry->sOriginalText;
            // original Character

            pOutSoundInfo->sSoundEvent = pEntry->sPrototypeSoundEvent.c_str();
            pOutSoundInfo->fVolume = CryConvertHalfToFloat(pEntry->fVolume);
            pOutSoundInfo->fRadioRatio = CryConvertHalfToFloat(pEntry->fRadioRatio);
            pOutSoundInfo->bUseSubtitle = (pEntry->flags & SLocalizedStringEntry::USE_SUBTITLE) != 0;
            pOutSoundInfo->bIsDirectRadio = (pEntry->flags & SLocalizedStringEntry::IS_DIRECTED_RADIO) != 0;
            pOutSoundInfo->bIsIntercepted = (pEntry->flags & SLocalizedStringEntry::IS_INTERCEPTED) != 0;

            //SoundMoods
            if (pOutSoundInfo->nNumSoundMoods >= pEntry->SoundMoods.size())
            {
                // enough space to copy data
                int i = 0;
                for (; i < pEntry->SoundMoods.size(); ++i)
                {
                    pOutSoundInfo->pSoundMoods[i].sName = pEntry->SoundMoods[i].sName;
                    pOutSoundInfo->pSoundMoods[i].fValue = pEntry->SoundMoods[i].fValue;
                }
                // if mode is available fill it with default
                for (; i < pOutSoundInfo->nNumSoundMoods; ++i)
                {
                    pOutSoundInfo->pSoundMoods[i].sName = "";
                    pOutSoundInfo->pSoundMoods[i].fValue = 0.0f;
                }
                pOutSoundInfo->nNumSoundMoods = pEntry->SoundMoods.size();
            }
            else
            {
                // not enough memory, say what is needed
                pOutSoundInfo->nNumSoundMoods = pEntry->SoundMoods.size();
                bResult = (pOutSoundInfo->pSoundMoods == NULL); // only report error if memory was provided but is too small
            }

            //EventParameters
            if (pOutSoundInfo->nNumEventParameters >= pEntry->EventParameters.size())
            {
                // enough space to copy data
                int i = 0;
                for (; i < pEntry->EventParameters.size(); ++i)
                {
                    pOutSoundInfo->pEventParameters[i].sName = pEntry->EventParameters[i].sName;
                    pOutSoundInfo->pEventParameters[i].fValue = pEntry->EventParameters[i].fValue;
                }
                // if mode is available fill it with default
                for (; i < pOutSoundInfo->nNumEventParameters; ++i)
                {
                    pOutSoundInfo->pEventParameters[i].sName = "";
                    pOutSoundInfo->pEventParameters[i].fValue = 0.0f;
                }
                pOutSoundInfo->nNumEventParameters = pEntry->EventParameters.size();
            }
            else
            {
                // not enough memory, say what is needed
                pOutSoundInfo->nNumEventParameters = pEntry->EventParameters.size();
                bResult = (pOutSoundInfo->pSoundMoods == NULL); // only report error if memory was provided but is too small
            }
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
int CLocalizedStringsManager::GetLocalizedStringCount()
{
    if (!m_pLanguage)
    {
        return 0;
    }
    return static_cast<int>(m_pLanguage->m_vLocalizedStrings.size());
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoGame& outGameInfo)
{
    if (!m_pLanguage)
    {
        return false;
    }
    const std::vector<SLocalizedStringEntry*>& entryVec = m_pLanguage->m_vLocalizedStrings;
    if (nIndex < 0 || nIndex >= (int)entryVec.size())
    {
        return false;
    }
    const SLocalizedStringEntry* pEntry = entryVec[nIndex];

    outGameInfo.szCharacterName = pEntry->sCharacterName.c_str();
    outGameInfo.sUtf8TranslatedText = pEntry->GetTranslatedText(m_pLanguage);

    outGameInfo.bUseSubtitle = (pEntry->flags & SLocalizedStringEntry::USE_SUBTITLE);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoEditor& outEditorInfo)
{
    if (!m_pLanguage)
    {
        return false;
    }
    const std::vector<SLocalizedStringEntry*>& entryVec = m_pLanguage->m_vLocalizedStrings;
    if (nIndex < 0 || nIndex >= (int)entryVec.size())
    {
        return false;
    }
    const SLocalizedStringEntry* pEntry = entryVec[nIndex];
    outEditorInfo.szCharacterName = pEntry->sCharacterName.c_str();
    outEditorInfo.sUtf8TranslatedText = pEntry->GetTranslatedText(m_pLanguage);

    assert(pEntry->pEditorExtension != NULL);

    outEditorInfo.sKey = pEntry->pEditorExtension->sKey.c_str();

    outEditorInfo.sOriginalActorLine = pEntry->pEditorExtension->sOriginalActorLine.c_str();
    outEditorInfo.sUtf8TranslatedActorLine = pEntry->pEditorExtension->sUtf8TranslatedActorLine.c_str();

    //outEditorInfo.sOriginalText = pEntry->sOriginalText;
    outEditorInfo.sOriginalCharacterName = pEntry->pEditorExtension->sOriginalCharacterName.c_str();

    outEditorInfo.nRow = pEntry->pEditorExtension->nRow;
    outEditorInfo.bUseSubtitle = (pEntry->flags & SLocalizedStringEntry::USE_SUBTITLE);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetSubtitle(const char* sKeyOrLabel, AZStd::string& outSubtitle, bool bForceSubtitle)
{
    assert(sKeyOrLabel);
    if (!m_pLanguage || !sKeyOrLabel || !*sKeyOrLabel)
    {
        return false;
    }
    if (*sKeyOrLabel == '@')
    {
        ++sKeyOrLabel;
    }

    uint32 keyCRC32 = AZ::Crc32(sKeyOrLabel);
    {
        AutoLock lock(m_cs);    //Lock here, to prevent strings etc being modified underneath this lookup
        const SLocalizedStringEntry* pEntry = stl::find_in_map(m_pLanguage->m_keysMap, keyCRC32, NULL);
        if (pEntry != NULL)
        {
            if ((pEntry->flags & SLocalizedStringEntry::USE_SUBTITLE) == false && !bForceSubtitle)
            {
                return false;
            }

            // TODO verify that no fallback is needed

            outSubtitle = pEntry->GetTranslatedText(m_pLanguage);

            if (outSubtitle.empty() == true)
            {
                if (pEntry->pEditorExtension != NULL && pEntry->pEditorExtension->sUtf8TranslatedActorLine.empty() == false)
                {
                    outSubtitle = pEntry->pEditorExtension->sUtf8TranslatedActorLine;
                }
                else if (pEntry->pEditorExtension != NULL && pEntry->pEditorExtension->sOriginalText.empty() == false)
                {
                    outSubtitle = pEntry->pEditorExtension->sOriginalText;
                }
                else if (pEntry->pEditorExtension != NULL && pEntry->pEditorExtension->sOriginalActorLine.empty() == false)
                {
                    outSubtitle = pEntry->pEditorExtension->sOriginalActorLine;
                }
            }
            return true;
        }
        return false;
    }
}

void InternalFormatStringMessage(AZStd::string& outString, const AZStd::string& sString, const char** sParams, int nParams)
{
    static const char token = '%';
    static const char tokens1[2] = { token, '\0' };
    static const char tokens2[3] = { token, token, '\0' };

    int maxArgUsed = 0;
    size_t lastPos = 0;
    size_t curPos = 0;
    const int sourceLen = static_cast<int>(sString.length());
    while (true)
    {
        auto foundPos = sString.find(token, curPos);
        if (foundPos != AZStd::string::npos)
        {
            if (foundPos + 1 < sourceLen)
            {
                const int nArg = sString[foundPos + 1] - '1';
                if (nArg >= 0 && nArg <= 9)
                {
                    if (nArg < nParams)
                    {
                        outString.append(sString, lastPos, foundPos - lastPos);
                        outString.append(sParams[nArg]);
                        curPos = foundPos + 2;
                        lastPos = curPos;
                        maxArgUsed = std::max(maxArgUsed, nArg);
                    }
                    else
                    {
                        AZStd::string tmp(sString);
                        AZ::StringFunc::Replace(tmp, tokens1, tokens2);
                        if constexpr (sizeof(*tmp.c_str()) == sizeof(char))
                        {
                            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Parameter for argument %d is missing. [%s]", nArg + 1, (const char*)tmp.c_str());
                        }
                        else
                        {
                            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Parameter for argument %d is missing. [%S]", nArg + 1, (const wchar_t*)tmp.c_str());
                        }
                        curPos = foundPos + 1;
                    }
                }
                else
                {
                    curPos = foundPos + 1;
                }
            }
            else
            {
                curPos = foundPos + 1;
            }
        }
        else
        {
            outString.append(sString, lastPos, sourceLen - lastPos);
            break;
        }
    }
}

void InternalFormatStringMessage(AZStd::string& outString, const AZStd::string& sString, const char* param1, const char* param2 = 0, const char* param3 = 0, const char* param4 = 0)
{
    static const int MAX_PARAMS = 4;
    const char* params[MAX_PARAMS] = { param1, param2, param3, param4 };
    int nParams = 0;
    while (nParams < MAX_PARAMS && params[nParams])
    {
        ++nParams;
    }
    InternalFormatStringMessage(outString, sString, params, nParams);
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FormatStringMessage_List(AZStd::string& outString, const AZStd::string& sString, const char** sParams, int nParams)
{
    InternalFormatStringMessage(outString, sString, sParams, nParams);
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FormatStringMessage(AZStd::string& outString, const AZStd::string& sString, const char* param1, const char* param2, const char* param3, const char* param4)
{
    InternalFormatStringMessage(outString, sString, param1, param2, param3, param4);
}

//////////////////////////////////////////////////////////////////////////

#if defined (WIN32) || defined(WIN64)
namespace
{
    struct LanguageID
    {
        const char*   language;
        unsigned long lcID;
    };

    LanguageID languageIDArray[] =
    {
        { "en-US", 0x0409 },  // English (USA)
        { "en-GB", 0x0809 },  // English (UK)
        { "de-DE", 0x0407 },  // German
        { "ru-RU", 0x0419 },  // Russian (Russia)
        { "pl-PL", 0x0415 },  // Polish
        { "tr-TR", 0x041f },  // Turkish
        { "es-ES", 0x0c0a },  // Spanish (Spain)
        { "es-MX", 0x080a },  // Spanish (Mexico)
        { "fr-FR", 0x040c },  // French (France)
        { "fr-CA", 0x0c0c },  // French (Canada)
        { "it-IT", 0x0410 },  // Italian
        { "pt-PT", 0x0816 },  // Portugese (Portugal)
        { "pt-BR", 0x0416 },  // Portugese (Brazil)
        { "ja-JP", 0x0411 },  // Japanese
        { "ko-KR", 0x0412 },  // Korean
        { "zh-CHT", 0x0804 }, // Traditional Chinese
        { "zh-CHS", 0x0804 }, // Simplified Chinese
        { "nl-NL", 0x0413 },  // Dutch (The Netherlands)
        { "fi-FI", 0x040b },  // Finnish
        { "sv-SE", 0x041d },  // Swedish
        { "cs-CZ", 0x0405 },  // Czech
        { "no-NO", 0x0414 },  // Norwegian (Norway)
        { "ar-SA", 0x0401 },  // Arabic (Saudi Arabia)
        { "da-DK", 0x0406 },  // Danish (Denmark)
    };

    const size_t numLanguagesIDs = sizeof(languageIDArray) / sizeof(languageIDArray[0]);

    LanguageID GetLanguageID(const char* language)
    {
        // default is English (US)
        const LanguageID defaultLanguage = { "en-US", 0x0409 };
        for (int i = 0; i < numLanguagesIDs; ++i)
        {
            if (_stricmp(language, languageIDArray[i].language) == 0)
            {
                return languageIDArray[i];
            }
        }
        return defaultLanguage;
    }

    LanguageID g_currentLanguageID = { 0, 0 };
};
#endif

void CLocalizedStringsManager::InternalSetCurrentLanguage(CLocalizedStringsManager::SLanguage* pLanguage)
{
    m_pLanguage = pLanguage;
#if defined (WIN32) || defined(WIN64)
    if (m_pLanguage != 0)
    {
        g_currentLanguageID = GetLanguageID(m_pLanguage->sLanguage.c_str());
    }
    else
    {
        g_currentLanguageID.language = 0;
        g_currentLanguageID.lcID = 0;
    }
#endif
    // TODO: on Linux based systems we should now set the locale
    // Enabling this on windows something seems to get corrupted...
    // on Windows we always use Windows Platform Functions, which use the lcid
#if 0
    if (g_currentLanguageID.language)
    {
        const char* currentLocale = setlocale(LC_ALL, 0);
        if (0 == setlocale(LC_ALL, g_currentLanguageID.language))
        {
            setlocale(LC_ALL, currentLocale);
        }
    }
    else
    {
        setlocale(LC_ALL, "C");
    }
#endif
    ReloadData();
    if (gEnv->pCryFont)
    {
        gEnv->pCryFont->OnLanguageChanged();
    }
}

void CLocalizedStringsManager::LocalizeDuration(int seconds, AZStd::string& outDurationString)
{
    int s = seconds;
    int d, h, m;
    d  = s / 86400;
    s -= d * 86400;
    h = s / 3600;
    s -= h * 3600;
    m = s / 60;
    s = s - m * 60;
    AZStd::string str;
    if (d > 1)
    {
        str = AZStd::string::format("%d @ui_days %02d:%02d:%02d", d, h, m, s);
    }
    else if (d > 0)
    {
        str = AZStd::string::format("%d @ui_day %02d:%02d:%02d", d, h, m, s);
    }
    else if (h > 0)
    {
        str = AZStd::string::format("%02d:%02d:%02d", h, m, s);
    }
    else
    {
        str = AZStd::string::format("%02d:%02d", m, s);
    }
    LocalizeString_s(str, outDurationString);
}

void CLocalizedStringsManager::LocalizeNumber(int number, AZStd::string& outNumberString)
{
    if (number == 0)
    {
        outNumberString.assign("0");
        return;
    }

    outNumberString.assign("");

    int n = abs(number);
    AZStd::string separator;
    AZStd::fixed_string<64> tmp;
    LocalizeString_ch("@ui_thousand_separator", separator);
    while (n > 0)
    {
        int a = n / 1000;
        int b = n - (a * 1000);
        if (a > 0)
        {
            tmp = AZStd::string::format("%s%03d%s", separator.c_str(), b, tmp.c_str());
        }
        else
        {
            tmp = AZStd::string::format("%d%s", b, tmp.c_str());
        }
        n = a;
    }

    if (number < 0)
    {
        tmp = AZStd::string::format("-%s", tmp.c_str());
    }

    outNumberString.assign(tmp.c_str());
}

void CLocalizedStringsManager::LocalizeNumber_Decimal(float number, int decimals, AZStd::string& outNumberString)
{
    if (number == 0.0f)
    {
        AZStd::fixed_string<64> tmp;
        tmp = AZStd::fixed_string<64>::format("%.*f", decimals, number);
        outNumberString.assign(tmp.c_str());
        return;
    }

    outNumberString.assign("");

    AZStd::string commaSeparator;
    LocalizeString_ch("@ui_decimal_separator", commaSeparator);
    float f = number > 0.0f ? number : -number;
    int d = (int)f;

    AZStd::string intPart;
    LocalizeNumber(d, intPart);

    float decimalsOnly = f - (float)d;

    int decimalsAsInt = aznumeric_cast<int>(int_round(decimalsOnly * pow(10.0f, decimals)));

    AZStd::fixed_string<64> tmp;
    tmp = AZStd::fixed_string<64>::format("%s%s%0*d", intPart.c_str(), commaSeparator.c_str(), decimals, decimalsAsInt);

    outNumberString.assign(tmp.c_str());
}

bool CLocalizedStringsManager::ProjectUsesLocalization() const
{
    ICVar* pCVar = gEnv->pConsole->GetCVar("sys_localization_folder");
    CRY_ASSERT_MESSAGE(pCVar,
        "Console variable 'sys_localization_folder' not defined! "
        "This was previously defined at startup in CSystem::CreateSystemVars.");

    // If game.cfg didn't provide a sys_localization_folder, we'll assume that
    // the project doesn't want to use localization features.
    return (pCVar->GetFlags() & VF_WASINCONFIG) != 0;
}

#if defined (WIN32) || defined(WIN64)
namespace
{
    void UnixTimeToFileTime(time_t unixtime, FILETIME* filetime)
    {
        LONGLONG longlong = Int32x32To64(unixtime, 10000000) + 116444736000000000;
        filetime->dwLowDateTime = (DWORD) longlong;
        filetime->dwHighDateTime = (DWORD)(longlong >> 32);
    }

    void UnixTimeToSystemTime(time_t unixtime, SYSTEMTIME* systemtime)
    {
        FILETIME filetime;
        UnixTimeToFileTime(unixtime, &filetime);
        FileTimeToSystemTime(&filetime, systemtime);
    }

    time_t UnixTimeFromFileTime(const FILETIME* filetime)
    {
        LONGLONG longlong = filetime->dwHighDateTime;
        longlong <<= 32;
        longlong |= filetime->dwLowDateTime;
        longlong -= 116444736000000000;
        return longlong / 10000000;
    }

    time_t UnixTimeFromSystemTime(const SYSTEMTIME* systemtime)
    {
        // convert systemtime to filetime
        FILETIME filetime;
        SystemTimeToFileTime(systemtime, &filetime);
        // convert filetime to unixtime
        time_t unixtime = UnixTimeFromFileTime(&filetime);
        return unixtime;
    }
};

void CLocalizedStringsManager::LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, AZStd::string& outTimeString)
{
    if (bMakeLocalTime)
    {
        struct tm thetime;
        localtime_s(&thetime, &t);
        t = gEnv->pTimer->DateToSecondsUTC(thetime);
    }
    outTimeString.clear();
    LCID lcID = g_currentLanguageID.lcID ? g_currentLanguageID.lcID : LOCALE_USER_DEFAULT;
    DWORD flags = bShowSeconds == false ? TIME_NOSECONDS : 0;
    SYSTEMTIME systemTime;
    UnixTimeToSystemTime(t, &systemTime);
    int len = ::GetTimeFormatW(lcID, flags, &systemTime, 0, 0, 0);
    if (len > 0)
    {
        // len includes terminating null!
        AZStd::fixed_wstring<256> tmpString;
        tmpString.resize(len);
        ::GetTimeFormatW(lcID, flags, &systemTime, 0, (wchar_t*) tmpString.c_str(), len);
        AZStd::to_string(outTimeString, tmpString.data());
    }
}

void CLocalizedStringsManager::LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, AZStd::string& outDateString)
{
    if (bMakeLocalTime)
    {
        struct tm thetime;
        localtime_s(&thetime, &t);
        t = gEnv->pTimer->DateToSecondsUTC(thetime);
    }
    outDateString.resize(0);
    LCID lcID = g_currentLanguageID.lcID ? g_currentLanguageID.lcID : LOCALE_USER_DEFAULT;
    SYSTEMTIME systemTime;
    UnixTimeToSystemTime(t, &systemTime);

    // len includes terminating null!
    AZStd::fixed_wstring<256> tmpString;

    if (bIncludeWeekday)
    {
        // Get name of day
        int len = ::GetDateFormatW(lcID, 0, &systemTime, L"ddd", 0, 0);
        if (len > 0)
        {
            // len includes terminating null!
            tmpString.resize(len);
            ::GetDateFormatW(lcID, 0, &systemTime, L"ddd", (wchar_t*) tmpString.c_str(), len);
            AZStd::string utf8;
            AZStd::to_string(utf8, tmpString.data());
            outDateString.append(utf8);
            outDateString.append(" ");
        }
    }
    DWORD flags = bShort ? DATE_SHORTDATE : DATE_LONGDATE;
    int len = ::GetDateFormatW(lcID, flags, &systemTime, 0, 0, 0);
    if (len > 0)
    {
        // len includes terminating null!
        tmpString.resize(len);
        ::GetDateFormatW(lcID, flags, &systemTime, 0, (wchar_t*) tmpString.c_str(), len);
        AZStd::string utf8;
        AZStd::to_string(utf8, tmpString.data());
        outDateString.append(utf8);
    }
}

#else // #if defined (WIN32) || defined(WIN64)

void CLocalizedStringsManager::LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, AZStd::string& outTimeString)
{
    struct tm theTime;
    if (bMakeLocalTime)
    {
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
        localtime_s(&theTime, &t);
#else
        theTime = *localtime(&t);
#endif
    }
    else
    {
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
        gmtime_s(&theTime, &t);
#else
        theTime = *gmtime(&t);
#endif
    }

    wchar_t buf[256];
    const size_t bufSize = sizeof(buf) / sizeof(buf[0]);
    wcsftime(buf, bufSize, bShowSeconds ? L"%#X" : L"%X", &theTime);
    buf[bufSize - 1] = 0;
    AZStd::to_string(outTimeString, buf);
}

void CLocalizedStringsManager::LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, AZStd::string& outDateString)
{
    struct tm theTime;
    if (bMakeLocalTime)
    {
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
        localtime_s(&theTime, &t);
#else
        theTime = *localtime(&t);
#endif
    }
    else
    {
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
        gmtime_s(&theTime, &t);
#else
        theTime = *gmtime(&t);
#endif
    }

    wchar_t buf[256];
    const size_t bufSize = sizeof(buf) / sizeof(buf[0]);
    const wchar_t* format = bShort ? (bIncludeWeekday ? L"%a %x" : L"%x") : L"%#x"; // long format always contains Weekday name
    wcsftime(buf, bufSize, format, &theTime);
    buf[bufSize - 1] = 0;
    AZStd::to_string(outDateString, buf);
}



#endif


