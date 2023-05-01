/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ILocalizationManager.h>
#include <ISystem.h>
#include <StlUtils.h>
#include <VectorMap.h>
#include <AzCore/std/containers/map.h>

#include "Huffman.h"

//////////////////////////////////////////////////////////////////////////
/*
    Manage Localization Data
*/
class CLocalizedStringsManager
    : public ILocalizationManager
    , public ISystemEventListener
{
public:
    using TLocalizationTagVec = std::vector<AZStd::string>;

    constexpr const static size_t LOADING_FIXED_STRING_LENGTH = 2048;
    constexpr const static size_t COMPRESSION_FIXED_BUFFER_LENGTH = 6144;

    CLocalizedStringsManager(ISystem* pSystem);
    virtual ~CLocalizedStringsManager();

    // ILocalizationManager
    const char* LangNameFromPILID(const ILocalizationManager::EPlatformIndependentLanguageID id) override;
    ILocalizationManager::EPlatformIndependentLanguageID PILIDFromLangName(AZStd::string langName) override;
    ILocalizationManager::EPlatformIndependentLanguageID GetSystemLanguage() override;
    ILocalizationManager::TLocalizationBitfield MaskSystemLanguagesFromSupportedLocalizations(const ILocalizationManager::TLocalizationBitfield systemLanguages) override;
    ILocalizationManager::TLocalizationBitfield IsLanguageSupported(const ILocalizationManager::EPlatformIndependentLanguageID id) override;

    const char* GetLanguage() override;
    bool SetLanguage(const char* sLanguage) override;

    int GetLocalizationFormat() const override;
    AZStd::string GetLocalizedSubtitleFilePath(const AZStd::string& localVideoPath, const AZStd::string& subtitleFileExtension) const override;
    AZStd::string GetLocalizedLocXMLFilePath(const AZStd::string& localXmlPath) const override;
    bool InitLocalizationData(const char* sFileName, bool bReload = false) override;
    bool RequestLoadLocalizationDataByTag(const char* sTag) override;
    bool LoadLocalizationDataByTag(const char* sTag, bool bReload = false) override;
    bool ReleaseLocalizationDataByTag(const char* sTag) override;

    bool LoadAllLocalizationData(bool bReload = false) override;
    bool LoadExcelXmlSpreadsheet(const char* sFileName, bool bReload = false) override;
    void ReloadData() override;
    void FreeData() override;

    bool LocalizeString_s(const AZStd::string& sString, AZStd::string& outLocalizedString, bool bEnglish = false) override;
    bool LocalizeString_ch(const char* sString, AZStd::string& outLocalizedString, bool bEnglish = false) override;

    void LocalizeAndSubstituteInternal(AZStd::string& locString, const AZStd::vector<AZStd::string>& keys, const AZStd::vector<AZStd::string>& values) override;
    bool LocalizeLabel(const char* sLabel, AZStd::string& outLocalizedString, bool bEnglish = false) override;
    bool IsLocalizedInfoFound(const char* sKey) override;
    bool GetLocalizedInfoByKey(const char* sKey, SLocalizedInfoGame& outGameInfo) override;
    bool GetLocalizedInfoByKey(const char* sKey, SLocalizedSoundInfoGame* pOutSoundInfoGame) override;
    int  GetLocalizedStringCount() override;
    bool GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoGame& outGameInfo) override;
    bool GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoEditor& outEditorInfo) override;

    bool GetEnglishString(const char* sKey, AZStd::string& sLocalizedString) override;
    bool GetSubtitle(const char* sKeyOrLabel, AZStd::string& outSubtitle, bool bForceSubtitle = false) override;

    void FormatStringMessage_List(AZStd::string& outString, const AZStd::string& sString, const char** sParams, int nParams) override;
    void FormatStringMessage(AZStd::string& outString, const AZStd::string& sString, const char* param1, const char* param2 = nullptr, const char* param3 = nullptr, const char* param4 = nullptr) override;

    void LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, AZStd::string& outTimeString) override;
    void LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, AZStd::string& outDateString) override;
    void LocalizeDuration(int seconds, AZStd::string& outDurationString) override;
    void LocalizeNumber(int number, AZStd::string& outNumberString) override;
    void LocalizeNumber_Decimal(float number, int decimals, AZStd::string& outNumberString) override;

    bool ProjectUsesLocalization() const override;
    // ~ILocalizationManager

    // ISystemEventManager
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    // ~ISystemEventManager

    void GetLoadedTags(TLocalizationTagVec& tagVec);
    void FreeLocalizationData();

private:
    void SetAvailableLocalizationsBitfield(const ILocalizationManager::TLocalizationBitfield availableLocalizations);

    bool LocalizeStringInternal(const char* pStr, size_t len, AZStd::string& outLocalizedString, bool bEnglish);

    bool DoLoadExcelXmlSpreadsheet(const char* sFileName, uint8 tagID, bool bReload);
    using LoadFunc = bool(CLocalizedStringsManager::*)(const char*, uint8, bool);
    bool DoLoadAGSXmlDocument(const char* sFileName, uint8 tagID, bool bReload);
    LoadFunc GetLoadFunction() const;

    struct SLocalizedStringEntryEditorExtension
    {
        AZStd::string  sKey;                                           // Map key text equivalent (without @)
        AZStd::string  sOriginalActorLine;             // english text
        AZStd::string  sUtf8TranslatedActorLine;       // localized text
        AZStd::string  sOriginalText;                      // subtitle. if empty, uses English text
        AZStd::string  sOriginalCharacterName;     // english character name speaking via XML asset

        unsigned int nRow;                              // Number of row in XML file
    };

    struct SLanguage;

    //#define LOG_DECOMP_TIMES              //If defined, will log decompression times to a file

    struct SLocalizedStringEntry
    {
        //Flags
        enum
        {
            USE_SUBTITLE        = BIT(0),       //should a subtitle displayed for this key?
            IS_DIRECTED_RADIO   = BIT(1), //should the radio receiving hud be displayed?
            IS_INTERCEPTED      = BIT(2), //should the radio receiving hud show the interception display?
            IS_COMPRESSED           = BIT(3),   //Translated text is compressed
        };

        union trans_text
        {
            AZStd::string*     psUtf8Uncompressed;
            uint8*      szCompressed;       // Note that no size information is stored. This is for struct size optimization and unfortunately renders the size info inaccurate.
        };

        AZStd::string sCharacterName;  // character name speaking via XML asset
        trans_text TranslatedText;  // Subtitle of this line

        // audio specific part
        AZStd::string      sPrototypeSoundEvent;           // associated sound event prototype (radio, ...)
        CryHalf     fVolume;
        CryHalf     fRadioRatio;
        // SoundMoods
        AZStd::vector<SLocalizedAdvancesSoundEntry> SoundMoods;
        // EventParameters
        AZStd::vector<SLocalizedAdvancesSoundEntry> EventParameters;
        // ~audio specific part

        // subtitle & radio flags
        uint8       flags;

        // Index of Huffman tree for translated text. -1 = no tree assigned (error)
        int8        huffmanTreeIndex;

        uint8       nTagID;

        // bool    bDependentTranslation; // if the english/localized text contains other localization labels

        //Additional information for Sandbox. Null in game
        SLocalizedStringEntryEditorExtension* pEditorExtension;

        SLocalizedStringEntry()
            : flags(0)
            , huffmanTreeIndex(-1)
            , pEditorExtension(nullptr)
        {
            TranslatedText.psUtf8Uncompressed = nullptr;
        };
        ~SLocalizedStringEntry()
        {
            SAFE_DELETE(pEditorExtension);
            if ((flags & IS_COMPRESSED) == 0)
            {
                SAFE_DELETE(TranslatedText.psUtf8Uncompressed);
            }
            else
            {
                SAFE_DELETE_ARRAY(TranslatedText.szCompressed);
            }
        };

        AZStd::string GetTranslatedText(const SLanguage* pLanguage) const;
    };

    //Keys as CRC32. Strings previously, but these proved too large
    using StringsKeyMap = VectorMap<uint32, SLocalizedStringEntry*>;

    struct SLanguage
    {
        using TLocalizedStringEntries = std::vector<SLocalizedStringEntry*>;
        using THuffmanCoders = std::vector<HuffmanCoder*>;

        AZStd::string sLanguage;
        StringsKeyMap m_keysMap;
        TLocalizedStringEntries m_vLocalizedStrings;
        THuffmanCoders m_vEncoders;
    };

    struct SFileInfo
    {
        bool    bDataStripping;
        uint8 nTagID;
    };

#ifndef _RELEASE
    std::map<AZStd::string, bool> m_warnedAboutLabels;
    bool m_haveWarnedAboutAtLeastOneLabel;

    void LocalizedStringsManagerWarning(const char* label, const char* message);
    void ListAndClearProblemLabels();
#else
    inline void LocalizedStringsManagerWarning(...) {};
    inline void ListAndClearProblemLabels() {};
#endif

    void AddLocalizedString(SLanguage* pLanguage, SLocalizedStringEntry* pEntry, const uint32 keyCRC32);
    void AddControl(int nKey);
    //////////////////////////////////////////////////////////////////////////
    void ParseFirstLine(IXmlTableReader* pXmlTableReader, char* nCellIndexToType, std::map<int, AZStd::string>& SoundMoodIndex, std::map<int, AZStd::string>& EventParameterIndex);
    void InternalSetCurrentLanguage(SLanguage* pLanguage);
    ISystem* m_pSystem;
    // Pointer to the current language.
    SLanguage* m_pLanguage;

    // all loaded Localization Files
    using pairFileName = std::pair<AZStd::string, SFileInfo>;
    using tmapFilenames = std::map<AZStd::string, SFileInfo>;
    tmapFilenames m_loadedTables;


    // filenames per tag
    using TStringVec = std::vector<AZStd::string>;
    struct STag
    {
        TStringVec  filenames;
        uint8               id;
        bool                loaded;
    };
    using TTagFileNames = std::map<AZStd::string, STag>;
    TTagFileNames m_tagFileNames;
    TStringVec m_tagLoadRequests;

    // Array of loaded languages.
    std::vector<SLanguage*> m_languages;

    using PrototypeSoundEvents = std::set<AZStd::string>;
    PrototypeSoundEvents m_prototypeEvents;  // this set is purely used for clever string/string assigning to save memory

    struct less_strcmp
    {
        bool operator()(const AZStd::string& left, const AZStd::string& right) const
        {
            return strcmp(left.c_str(), right.c_str()) < 0;
        }
    };

    using CharacterNameSet = std::set<AZStd::string, less_strcmp>;
    CharacterNameSet m_characterNameSet; // this set is purely used for clever string/string assigning to save memory

    // CVARs
    int m_cvarLocalizationDebug;
    int m_cvarLocalizationEncode;   //Encode/Compress translated text to save memory

    //The localizations that are available for this SKU. Used for determining what to show on a language select screen or whether to show one at all
    TLocalizationBitfield m_availableLocalizations;

    //Lock for
    mutable AZStd::mutex m_cs;
    using AutoLock = AZStd::lock_guard<AZStd::mutex>;
};
