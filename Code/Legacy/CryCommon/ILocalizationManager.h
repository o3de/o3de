/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_ILOCALIZATIONMANAGER_H
#define CRYINCLUDE_CRYCOMMON_ILOCALIZATIONMANAGER_H
#pragma once

#include "LocalizationManagerBus.h"
//#include <platform.h> // Needed for LARGE_INTEGER (for consoles).

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////

class XmlNodeRef;

//////////////////////////////////////////////////////////////////////////
// Localized strings manager interface.
//////////////////////////////////////////////////////////////////////////

// Localization Info structure
struct SLocalizedInfoGame
{
    SLocalizedInfoGame ()
        : szCharacterName(nullptr)
        , bUseSubtitle(false)
    {
    }

    const char* szCharacterName;
    AZStd::string sUtf8TranslatedText;

    bool bUseSubtitle;
};

struct SLocalizedAdvancesSoundEntry
{
    AZStd::string sName;
    float       fValue;
};

// Localization Sound Info structure, containing sound related parameters.
struct SLocalizedSoundInfoGame
    : public SLocalizedInfoGame
{
    SLocalizedSoundInfoGame()
        : sSoundEvent(nullptr)
        , fVolume(0.f)
        , fRadioRatio (0.f)
        , bIsDirectRadio(false)
        , bIsIntercepted(false)
        , nNumSoundMoods(0)
        , pSoundMoods (nullptr)
        , nNumEventParameters(0)
        , pEventParameters(nullptr)
    {
    }

    const char* sSoundEvent;
    float fVolume;
    float fRadioRatio;
    bool    bIsDirectRadio;
    bool    bIsIntercepted;

    // SoundMoods.
    size_t         nNumSoundMoods;
    SLocalizedAdvancesSoundEntry* pSoundMoods;

    // EventParameters.
    size_t         nNumEventParameters;
    SLocalizedAdvancesSoundEntry* pEventParameters;
};

// Localization Sound Info structure, containing sound related parameters.
struct SLocalizedInfoEditor
    : public SLocalizedInfoGame
{
    SLocalizedInfoEditor()
        : sKey(nullptr)
        , sOriginalCharacterName(nullptr)
        , sOriginalActorLine(nullptr)
        , sUtf8TranslatedActorLine(nullptr)
        , nRow(0)
    {
    }

    const char* sKey;
    const char* sOriginalCharacterName;
    const char* sOriginalActorLine;
    const char* sUtf8TranslatedActorLine;
    unsigned int nRow;
};

// Summary:
//      Interface to the Localization Manager.
struct ILocalizationManager
    : public LocalizationManagerRequestBus::Handler
{
    //Platform independent language IDs. These are used to map the platform specific language codes to localization pakfiles
    //Please ensure that each entry in this enum has a corresponding entry in the PLATFORM_INDEPENDENT_LANGUAGE_NAMES array which is defined in LocalizedStringManager.cpp currently.
    enum EPlatformIndependentLanguageID
    {
        ePILID_English_US,
        ePILID_English_GB,
        ePILID_German_DE,
        ePILID_Russian_RU,
        ePILID_Polish_PL,
        ePILID_Turkish_TR,
        ePILID_Spanish_ES,
        ePILID_Spanish_MX,
        ePILID_French_FR,
        ePILID_French_CA,
        ePILID_Italian_IT,
        ePILID_Portugese_PT,
        ePILID_Portugese_BR,
        ePILID_Japanese_JP,
        ePILID_Korean_KR,
        ePILID_Chinese_T,
        ePILID_Chinese_S,
        ePILID_Dutch_NL,
        ePILID_Finnish_FI,
        ePILID_Swedish_SE,
        ePILID_Czech_CZ,
        ePILID_Norwegian_NO,
        ePILID_Arabic_SA,
        ePILID_Danish_DK,
        ePILID_MAX_OR_INVALID,   //Not a language, denotes the maximum number of languages or an unknown language
    };

    using TLocalizationBitfield = uint32;

    // <interfuscator:shuffle>
    virtual ~ILocalizationManager()= default;
    virtual const char* LangNameFromPILID(const ILocalizationManager::EPlatformIndependentLanguageID id) = 0;
    virtual ILocalizationManager::EPlatformIndependentLanguageID PILIDFromLangName(AZStd::string langName) = 0;
    virtual ILocalizationManager::EPlatformIndependentLanguageID GetSystemLanguage() { return ILocalizationManager::EPlatformIndependentLanguageID::ePILID_English_US; }
    virtual ILocalizationManager::TLocalizationBitfield MaskSystemLanguagesFromSupportedLocalizations(const ILocalizationManager::TLocalizationBitfield systemLanguages) = 0;
    virtual ILocalizationManager::TLocalizationBitfield IsLanguageSupported(const ILocalizationManager::EPlatformIndependentLanguageID id) = 0;
    bool SetLanguage([[maybe_unused]] const char* sLanguage) override { return false; }
    const char* GetLanguage() override { return nullptr; }

    int GetLocalizationFormat() const override { return -1; }
    AZStd::string GetLocalizedSubtitleFilePath([[maybe_unused]] const AZStd::string& localVideoPath, [[maybe_unused]] const AZStd::string& subtitleFileExtension) const override { return ""; }
    AZStd::string GetLocalizedLocXMLFilePath([[maybe_unused]] const AZStd::string& localXmlPath) const override { return ""; }
    // load the descriptor file with tag information
    virtual bool InitLocalizationData(const char* sFileName, bool bReload = false) = 0;
    // request to load loca data by tag. Actual loading will happen during next level load begin event.
    virtual bool RequestLoadLocalizationDataByTag(const char* sTag) = 0;
    // direct load of loca data by tag
    virtual bool LoadLocalizationDataByTag(const char* sTag, bool bReload = false) = 0;
    virtual bool ReleaseLocalizationDataByTag(const char* sTag) = 0;

    virtual bool LoadAllLocalizationData(bool bReload = false) = 0;
    bool LoadExcelXmlSpreadsheet([[maybe_unused]] const char* sFileName, [[maybe_unused]] bool bReload = false) override { return false; }
    void ReloadData() override {};

    // Summary:
    //   Free localization data.
    virtual void FreeData() = 0;

    // Summary:
    //   Translate a string into the currently selected language.
    // Description:
    //   Processes the input string and translates all labels contained into the currently selected language.
    // Parameters:
    //   sString             - String to be translated.
    //   outLocalizedString  - Translated version of the string.
    //   bEnglish            - if true, translates the string into the always present English language.
    // Returns:
    //   true if localization was successful, false otherwise
    bool LocalizeString_ch([[maybe_unused]] const char* sString, [[maybe_unused]] AZStd::string& outLocalizedString, [[maybe_unused]] bool bEnglish = false) override { return false; }

    // Summary:
    //   Same as LocalizeString( const char* sString, AZStd::string& outLocalizedString, bool bEnglish=false )
    //   but at the moment this is faster.
    bool LocalizeString_s([[maybe_unused]] const AZStd::string& sString, [[maybe_unused]] AZStd::string& outLocalizedString, [[maybe_unused]] bool bEnglish = false) override { return false; }

    // Summary:
    void LocalizeAndSubstituteInternal([[maybe_unused]] AZStd::string& locString, [[maybe_unused]] const AZStd::vector<AZStd::string>& keys, [[maybe_unused]] const AZStd::vector<AZStd::string>& values) override {}
    //   Return the localized version corresponding to a label.
    // Description:
    //   A label has to start with '@' sign.
    // Parameters:
    //   sLabel              - Label to be translated, must start with '@' sign.
    //   outLocalizedString  - Localized version of the label.
    //   bEnglish            - if true, returns the always present English version of the label.
    // Returns:
    //   True if localization was successful, false otherwise.
    bool LocalizeLabel([[maybe_unused]] const char* sLabel, [[maybe_unused]] AZStd::string& outLocalizedString, [[maybe_unused]] bool bEnglish = false) override { return false; }
    virtual bool IsLocalizedInfoFound([[maybe_unused]] const char* sKey) { return false; }

    // Summary:
    //   Get localization info structure corresponding to a key (key=label without the '@' sign).
    // Parameters:
    //   sKey    - Key to be looked up. Key = Label without '@' sign.
    //   outGameInfo - Reference to localization info structure to be filled in.
    //  Returns:
    //    True if info for key was found, false otherwise.
    virtual bool GetLocalizedInfoByKey(const char* sKey, SLocalizedInfoGame& outGameInfo) = 0;

    // Summary:
    //   Get the sound localization info structure corresponding to a key.
    // Parameters:
    //   sKey         - Key to be looked up. Key = Label without '@' sign.
    //   outSoundInfo - reference to sound info structure to be filled in
    //                                  pSoundMoods requires nNumSoundMoods-times allocated memory
    //                                  on return nNumSoundMoods will hold how many SoundsMood entries are needed
    //                                  pEventParameters requires nNumEventParameters-times allocated memory
    //                                  on return nNumEventParameters will hold how many EventParameter entries are needed
    //                                  Passing 0 in the Num fields will make the query ignore checking for allocated memory
    // Returns:
    //   True if successful, false otherwise (key not found, or not enough memory provided to write additional info).
    virtual bool GetLocalizedInfoByKey(const char* sKey, SLocalizedSoundInfoGame* pOutSoundInfo) = 0;

    // Summary:
    //   Return number of localization entries.
    int  GetLocalizedStringCount() override { return -1; }

    // Summary:
    //   Get the localization info structure at index nIndex.
    // Parameters:
    //   nIndex  - Index.
    //   outEditorInfo - Reference to localization info structure to be filled in.
    // Returns:
    //   True if successful, false otherwise (out of bounds).
    virtual bool GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoEditor& outEditorInfo) = 0;

    // Summary:
    //   Get the localization info structure at index nIndex.
    // Parameters:
    //   nIndex  - Index.
    //   outGameInfo - Reference to localization info structure to be filled in.
    // Returns:
    //   True if successful, false otherwise (out of bounds).
    virtual bool GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoGame& outGameInfo) = 0;

    // Summary:
    //   Get the english localization info structure corresponding to a key.
    // Parameters:
    //   sKey         - Key to be looked up. Key = Label without '@' sign.
    //   sLocalizedString - Corresponding english language string.
    // Returns:
    //   True if successful, false otherwise (key not found).
    bool GetEnglishString([[maybe_unused]] const char* sKey, [[maybe_unused]] AZStd::string& sLocalizedString) override { return false; }

    // Summary:
    //   Get Subtitle for Key or Label .
    // Parameters:
    //   sKeyOrLabel    - Key or Label to be used for subtitle lookup. Key = Label without '@' sign.
    //   outSubtitle    - Subtitle (untouched if Key/Label not found).
    //   bForceSubtitle - If true, get subtitle (sLocalized or sEnglish) even if not specified in Data file.
    // Returns:
    //   True if subtitle found (and outSubtitle filled in), false otherwise.
    bool GetSubtitle([[maybe_unused]] const char* sKeyOrLabel, [[maybe_unused]] AZStd::string& outSubtitle, [[maybe_unused]] bool bForceSubtitle = false) override { return false; }

    // Description:
    //      These methods format outString depending on sString with ordered arguments
    //      FormatStringMessage(outString, "This is %2 and this is %1", "second", "first");
    // Arguments:
    //      outString - This is first and this is second.
    void FormatStringMessage_List([[maybe_unused]] AZStd::string& outString, [[maybe_unused]] const AZStd::string& sString, [[maybe_unused]] const char** sParams, [[maybe_unused]] int nParams) override {}
    void FormatStringMessage([[maybe_unused]] AZStd::string& outString, [[maybe_unused]] const AZStd::string& sString, [[maybe_unused]] const char* param1, [[maybe_unused]] const char* param2 = nullptr, [[maybe_unused]] const char* param3 = nullptr, [[maybe_unused]] const char* param4 = nullptr) override {}

    void LocalizeTime([[maybe_unused]] time_t t, [[maybe_unused]] bool bMakeLocalTime, [[maybe_unused]] bool bShowSeconds, [[maybe_unused]] AZStd::string& outTimeString) override {}
    void LocalizeDate([[maybe_unused]] time_t t, [[maybe_unused]] bool bMakeLocalTime, [[maybe_unused]] bool bShort, [[maybe_unused]] bool bIncludeWeekday, [[maybe_unused]] AZStd::string& outDateString) override {}
    void LocalizeDuration([[maybe_unused]] int seconds, [[maybe_unused]] AZStd::string& outDurationString) override {}
    void LocalizeNumber([[maybe_unused]] int number, [[maybe_unused]] AZStd::string& outNumberString) override {}
    void LocalizeNumber_Decimal([[maybe_unused]] float number, [[maybe_unused]] int decimals, [[maybe_unused]] AZStd::string& outNumberString) override {}

    // Summary:
    //   Returns true if the project has localization configured for use, false otherwise.
    bool ProjectUsesLocalization() const override { return false; }
    // </interfuscator:shuffle>

    static ILINE TLocalizationBitfield LocalizationBitfieldFromPILID(EPlatformIndependentLanguageID pilid)
    {
        assert(pilid >= 0 && pilid < ePILID_MAX_OR_INVALID);
        return (1 << pilid);
    }
};

// Summary:
//      Simple bus that notifies listeners that the language (g_language) has changed.




#endif // CRYINCLUDE_CRYCOMMON_ILOCALIZATIONMANAGER_H
