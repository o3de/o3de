/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ILocalizationManager.h>
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
    typedef std::vector<string> TLocalizationTagVec;

    constexpr const static size_t LOADING_FIXED_STRING_LENGTH = 2048;
    constexpr const static size_t COMPRESSION_FIXED_BUFFER_LENGTH = 6144;

    CLocalizedStringsManager(ISystem* pSystem);
    virtual ~CLocalizedStringsManager();

    // ILocalizationManager
    const char* LangNameFromPILID(const ILocalizationManager::EPlatformIndependentLanguageID id);
    ILocalizationManager::EPlatformIndependentLanguageID PILIDFromLangName(AZStd::string langName) override;
    ILocalizationManager::EPlatformIndependentLanguageID GetSystemLanguage() override;
    ILocalizationManager::TLocalizationBitfield MaskSystemLanguagesFromSupportedLocalizations(const ILocalizationManager::TLocalizationBitfield systemLanguages);
    ILocalizationManager::TLocalizationBitfield IsLanguageSupported(const ILocalizationManager::EPlatformIndependentLanguageID id);

    const char* GetLanguage() override;
    bool SetLanguage(const char* sLanguage) override;

    int GetLocalizationFormat() const override;
    virtual AZStd::string GetLocalizedSubtitleFilePath(const AZStd::string& localVideoPath, const AZStd::string& subtitleFileExtension) const override;
    virtual AZStd::string GetLocalizedLocXMLFilePath(const AZStd::string& localXmlPath) const override;
    bool InitLocalizationData(const char* sFileName, bool bReload = false);
    bool RequestLoadLocalizationDataByTag(const char* sTag);
    bool LoadLocalizationDataByTag(const char* sTag, bool bReload = false);
    bool ReleaseLocalizationDataByTag(const char* sTag);

    bool LoadAllLocalizationData(bool bReload = false) override;
    bool LoadExcelXmlSpreadsheet(const char* sFileName, bool bReload = false) override;
    void ReloadData() override;
    void FreeData();

    bool LocalizeString_s(const string& sString, string& outLocalizedString, bool bEnglish = false) override;
    bool LocalizeString_ch(const char* sString, string& outLocalizedString, bool bEnglish = false) override;

    void LocalizeAndSubstituteInternal(AZStd::string& locString, const AZStd::vector<AZStd::string>& keys, const AZStd::vector<AZStd::string>& values) override;
    bool LocalizeLabel(const char* sLabel, string& outLocalizedString, bool bEnglish = false) override;
    bool IsLocalizedInfoFound(const char* sKey);
    bool GetLocalizedInfoByKey(const char* sKey, SLocalizedInfoGame& outGameInfo);
    bool GetLocalizedInfoByKey(const char* sKey, SLocalizedSoundInfoGame* pOutSoundInfoGame);
    int  GetLocalizedStringCount();
    bool GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoGame& outGameInfo);
    bool GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoEditor& outEditorInfo);

    bool GetEnglishString(const char* sKey, string& sLocalizedString) override;
    bool GetSubtitle(const char* sKeyOrLabel, string& outSubtitle, bool bForceSubtitle = false) override;

    void FormatStringMessage_List(string& outString, const string& sString, const char** sParams, int nParams) override;
    void FormatStringMessage(string& outString, const string& sString, const char* param1, const char* param2 = 0, const char* param3 = 0, const char* param4 = 0) override;

    void LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, string& outTimeString) override;
    void LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, string& outDateString) override;
    void LocalizeDuration(int seconds, string& outDurationString) override;
    void LocalizeNumber(int number, string& outNumberString) override;
    void LocalizeNumber_Decimal(float number, int decimals, string& outNumberString) override;

    bool ProjectUsesLocalization() const override;
    // ~ILocalizationManager

    // ISystemEventManager
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    // ~ISystemEventManager

    int GetMemoryUsage(ICrySizer* pSizer);

    void GetLoadedTags(TLocalizationTagVec& tagVec);
    void FreeLocalizationData();

private:
    void SetAvailableLocalizationsBitfield(const ILocalizationManager::TLocalizationBitfield availableLocalizations);

    bool LocalizeStringInternal(const char* pStr, size_t len, string& outLocalizedString, bool bEnglish);

    bool DoLoadExcelXmlSpreadsheet(const char* sFileName, uint8 tagID, bool bReload);
    typedef bool(CLocalizedStringsManager::*LoadFunc)(const char*, uint8, bool);
    bool DoLoadAGSXmlDocument(const char* sFileName, uint8 tagID, bool bReload);
    LoadFunc GetLoadFunction() const;

    struct SLocalizedStringEntryEditorExtension
    {
        string  sKey;                                           // Map key text equivalent (without @)
        string  sOriginalActorLine;             // english text
        string  sUtf8TranslatedActorLine;       // localized text
        string  sOriginalText;                      // subtitle. if empty, uses English text
        string  sOriginalCharacterName;     // english character name speaking via XML asset

        unsigned int nRow;                              // Number of row in XML file

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(this, sizeof(*this));

            pSizer->AddObject(sKey);
            pSizer->AddObject(sOriginalActorLine);
            pSizer->AddObject(sUtf8TranslatedActorLine);
            pSizer->AddObject(sOriginalText);
            pSizer->AddObject(sOriginalCharacterName);
        }
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
            string*     psUtf8Uncompressed;
            uint8*      szCompressed;       // Note that no size information is stored. This is for struct size optimization and unfortunately renders the size info inaccurate.
        };

        string sCharacterName;  // character name speaking via XML asset
        trans_text TranslatedText;  // Subtitle of this line

        // audio specific part
        string      sPrototypeSoundEvent;           // associated sound event prototype (radio, ...)
        CryHalf     fVolume;
        CryHalf     fRadioRatio;
        // SoundMoods
        DynArray<SLocalizedAdvancesSoundEntry> SoundMoods;
        // EventParameters
        DynArray<SLocalizedAdvancesSoundEntry> EventParameters;
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
            , pEditorExtension(NULL)
        {
            TranslatedText.psUtf8Uncompressed = NULL;
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

        string GetTranslatedText(const SLanguage* pLanguage) const;

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(this, sizeof(*this));

            pSizer->AddObject(sCharacterName);

            if ((flags & IS_COMPRESSED) == 0 && TranslatedText.psUtf8Uncompressed != NULL)   //Number of bytes stored for compressed text is unknown, which throws this GetMemoryUsage off
            {
                pSizer->AddObject(*TranslatedText.psUtf8Uncompressed);
            }

            pSizer->AddObject(sPrototypeSoundEvent);

            pSizer->AddObject(SoundMoods);
            pSizer->AddObject(EventParameters);

            if (pEditorExtension != NULL)
            {
                pEditorExtension->GetMemoryUsage(pSizer);
            }
        }
    };

    //Keys as CRC32. Strings previously, but these proved too large
    typedef VectorMap<uint32, SLocalizedStringEntry*>   StringsKeyMap;

    struct SLanguage
    {
        typedef std::vector<SLocalizedStringEntry*> TLocalizedStringEntries;
        typedef std::vector<HuffmanCoder*> THuffmanCoders;

        string sLanguage;
        StringsKeyMap m_keysMap;
        TLocalizedStringEntries m_vLocalizedStrings;
        THuffmanCoders m_vEncoders;

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(this, sizeof(*this));
            pSizer->AddObject(sLanguage);
            pSizer->AddObject(m_vLocalizedStrings);
            pSizer->AddObject(m_keysMap);
            pSizer->AddObject(m_vEncoders);
        }
    };

    struct SFileInfo
    {
        bool    bDataStripping;
        uint8 nTagID;
    };

#ifndef _RELEASE
    std::map<string, bool> m_warnedAboutLabels;
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
    void ParseFirstLine(IXmlTableReader* pXmlTableReader, char* nCellIndexToType, std::map<int, string>& SoundMoodIndex, std::map<int, string>& EventParameterIndex);
    void InternalSetCurrentLanguage(SLanguage* pLanguage);
    ISystem* m_pSystem;
    // Pointer to the current language.
    SLanguage* m_pLanguage;

    // all loaded Localization Files
    typedef std::pair<string, SFileInfo> pairFileName;
    typedef std::map<string, SFileInfo> tmapFilenames;
    tmapFilenames m_loadedTables;


    // filenames per tag
    typedef std::vector<string> TStringVec;
    struct STag
    {
        TStringVec  filenames;
        uint8               id;
        bool                loaded;
    };
    typedef std::map<string, STag> TTagFileNames;
    TTagFileNames m_tagFileNames;
    TStringVec m_tagLoadRequests;

    // Array of loaded languages.
    std::vector<SLanguage*> m_languages;

    typedef std::set<string> PrototypeSoundEvents;
    PrototypeSoundEvents m_prototypeEvents;  // this set is purely used for clever string/string assigning to save memory

    struct less_strcmp
    {
        bool operator()(const string& left, const string& right) const
        {
            return strcmp(left.c_str(), right.c_str()) < 0;
        }
    };

    typedef std::set<string, less_strcmp> CharacterNameSet;
    CharacterNameSet m_characterNameSet; // this set is purely used for clever string/string assigning to save memory

    // CVARs
    int m_cvarLocalizationDebug;
    int m_cvarLocalizationEncode;   //Encode/Compress translated text to save memory
    int m_cvarLocalizationFormat;

    //The localizations that are available for this SKU. Used for determining what to show on a language select screen or whether to show one at all
    TLocalizationBitfield m_availableLocalizations;

    //Lock for
    mutable CryCriticalSection m_cs;
    typedef CryAutoCriticalSection AutoLock;
};


