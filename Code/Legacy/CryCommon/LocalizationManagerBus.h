/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Localization Utility classes/functions.
//////////////////////////////////////////////////////////////////////////

template<typename... Args>
AZStd::vector<AZStd::string> MakeLocKeyList(Args... args)
{
    return {args...};
}

// Helper functions for Localization and data-string substitution
struct LocalizationHelpers
{
    // Returns the string version of any type convertible by std::to_string()
    // otherwise throws a compile error.
    template <typename T>
    static AZStd::string DataToString(T t);

    // Need a function to specifically handle std::string, since std::to_string() doesn't handle it.
    static inline AZStd::string DataToString(AZStd::string str);

    // Need a function to specifically handle const char*, since std::to_string() doesn't handle it.
    static inline AZStd::string DataToString(const char* str);

    // Base case of the recursive function to convert a data list to strings
    template<typename T>
    static void ConvertValuesToStrings(AZStd::vector<AZStd::string>& values, T t);

    // Recursive function to convert a data list to strings
    template<typename T, typename... Args>
    static void ConvertValuesToStrings(AZStd::vector<AZStd::string>& values, T t, Args... args);

    // Check if a key string is in a list of substitution strings
    static inline bool IsKeyInList(const AZStd::vector<AZStd::string>& keys, const AZStd::string& target, int& index);
};

//////////////////////////////////////////////////////////////////////////
// Localization manager bus interface.
//////////////////////////////////////////////////////////////////////////

// Summary:
//      Interface to the Localization Manager.
class LocalizationManagerRequests
    : public AZ::EBusTraits
{
public:
    virtual bool SetLanguage(const char* sLanguage) = 0;
    virtual const char* GetLanguage() = 0;

    virtual int GetLocalizationFormat() const = 0;

    // Get Localized subtitle file for current language
    // Summary:
    //  Provides the asset path of a video subtitle file based on the input video path name. Input localVideoPath
    //  should contain the game-specific path after the current language folder. 
    //  ex:
    //      input: localVideoPath = "/VideoSubtitleSrt/100/101/101VT_01.bnk"
    //      output: return "Localization/en-US/VideoSubtitleSrt/100/101/101VT_01.srt"
    //
    //  NOTE: System expects that video file has the same name as the subtitle file (other than the file extension).
    virtual AZStd::string GetLocalizedSubtitleFilePath(const AZStd::string& localVideoPath, const AZStd::string& subtitleFileExtension) const = 0;

    // Get the given xml for the current language
    // Summary:
    //  Provides the asset path of a localization xml file given the local path (e.g. the path starting from
    //  your language folder).
    virtual AZStd::string GetLocalizedLocXMLFilePath(const AZStd::string& localXmlPath) const = 0;

    virtual bool LoadExcelXmlSpreadsheet(const char* sFileName, bool bReload = false) = 0;
    virtual void ReloadData() = 0;

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
    virtual bool LocalizeString_ch(const char* sString, AZStd::string& outLocalizedString, bool bEnglish = false) = 0;

    // Summary:
    //   Same as LocalizeString( const char* sString, AZStd::string& outLocalizedString, bool bEnglish=false )
    //   but at the moment this is faster.
    virtual bool LocalizeString_s(const AZStd::string& sString, AZStd::string& outLocalizedString, bool bEnglish = false) = 0;

    // Set up system for passing in placeholder data for localized strings
    // Summary:
    //   Parse localized string and substitute data in for each key that is surrounded by curly braces
    //   ie. {player_name}
    virtual void LocalizeAndSubstituteInternal(AZStd::string& locString, const AZStd::vector<AZStd::string>& keys, const AZStd::vector<AZStd::string>& values) = 0;

    // Summary:
    //   Parse localized string and substitute data in for each key that is surrounded by curly braces. Number of arguments 
    //   after 'const AZStd::vector<AZStd::string>& keys' should be equal to the number of strings in 'keys'.
    //   ex:
    //        float distance = GetWinDistance();
    //        AZStd::string winState = IsPlayerFirstPlace() ? "won" : "lost";
    //        LocalizationManagerRequests::Broadcast(&LocalizationManagerRequests::Events::LocalizeAndSubstitute<float, AZStd::string>
    //                                          , "@QUICKRESULTS_DISTANCEDIFFERENCE, outLocalizedString
    //                                          , MakeLocKeyString("race_result", "distance_ahead")
    //                                          , winState, distance);
    //
    //  where "@QUICKRESULTS_DISTANCEDIFFERENCE" would be localized to "You {race_result} by {distance_ahead} meters!" and then 
    //  "{race_result}" would be replaced by 'winState" and "{distance_ahead}" would be replaced by the 'distance' argument as a string.
    template<typename T, typename... Args>
    void LocalizeAndSubstitute(const AZStd::string& locString, AZStd::string& outLocalizedString, const AZStd::vector<AZStd::string>& keys, T t, Args... args);

    // Summary:
    //   Return the localized version corresponding to a label.
    // Description:
    //   A label has to start with '@' sign.
    // Parameters:
    //   sLabel              - Label to be translated, must start with '@' sign.
    //   outLocalizedString  - Localized version of the label.
    //   bEnglish            - if true, returns the always present English version of the label.
    // Returns:
    //   True if localization was successful, false otherwise.
    virtual bool LocalizeLabel(const char* sLabel, AZStd::string& outLocalizedString, bool bEnglish = false) = 0;

    // Summary:
    //   Return number of localization entries.
    virtual int  GetLocalizedStringCount() = 0;

    // Summary:
    //   Get the english localization info structure corresponding to a key.
    // Parameters:
    //   sKey         - Key to be looked up. Key = Label without '@' sign.
    //   sLocalizedString - Corresponding english language string.
    // Returns:
    //   True if successful, false otherwise (key not found).
    virtual bool GetEnglishString(const char* sKey, AZStd::string& sLocalizedString) = 0;

    // Summary:
    //   Get Subtitle for Key or Label .
    // Parameters:
    //   sKeyOrLabel    - Key or Label to be used for subtitle lookup. Key = Label without '@' sign.
    //   outSubtitle    - Subtitle (untouched if Key/Label not found).
    //   bForceSubtitle - If true, get subtitle (sLocalized or sEnglish) even if not specified in Data file.
    // Returns:
    //   True if subtitle found (and outSubtitle filled in), false otherwise.
    virtual bool GetSubtitle(const char* sKeyOrLabel, AZStd::string& outSubtitle, bool bForceSubtitle = false) = 0;

    // Description:
    //      These methods format outString depending on sString with ordered arguments
    //      FormatStringMessage(outString, "This is %2 and this is %1", "second", "first");
    // Arguments:
    //      outString - This is first and this is second.
    virtual void FormatStringMessage_List(AZStd::string& outString, const AZStd::string& sString, const char** sParams, int nParams) = 0;
    virtual void FormatStringMessage(AZStd::string& outString, const AZStd::string& sString, const char* param1, const char* param2 = 0, const char* param3 = 0, const char* param4 = 0) = 0;

    virtual void LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, AZStd::string& outTimeString) = 0;
    virtual void LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, AZStd::string& outDateString) = 0;
    virtual void LocalizeDuration(int seconds, AZStd::string& outDurationString) = 0;
    virtual void LocalizeNumber(int number, AZStd::string& outNumberString) = 0;
    virtual void LocalizeNumber_Decimal(float number, int decimals, AZStd::string& outNumberString) = 0;

    // Summary:
    //   Returns true if the project has localization configured for use, false otherwise.
    virtual bool ProjectUsesLocalization() const = 0;
    // </interfuscator:shuffle>

};
using LocalizationManagerRequestBus = AZ::EBus<LocalizationManagerRequests>;

// Summary:
//      Simple bus that notifies listeners that the language (g_language) has changed.
class LanguageChangeNotification
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

    virtual ~LanguageChangeNotification() = default;

    virtual void LanguageChanged() = 0;
};

using LanguageChangeNotificationBus = AZ::EBus<LanguageChangeNotification>;

// Set up system for passing in placeholder data for localized strings
#include "LocalizationManagerBus.inl"

