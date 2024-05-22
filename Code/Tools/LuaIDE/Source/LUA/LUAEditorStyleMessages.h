/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <QColor>
#include <QFont>

#pragma once

namespace AZ { class ReflectContext; }

namespace LUAEditor
{
    class SyntaxStyleSettings
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(SyntaxStyleSettings, "{9C5A2A16-1855-4074-AA06-FC58A6A789D7}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(SyntaxStyleSettings, AZ::SystemAllocator);

        SyntaxStyleSettings();

        QColor ToQColor(const AZ::Vector3& color) const
        {
            return QColor::fromRgbF(color.GetX(), color.GetY(), color.GetZ());
        }

        QColor GetTextColor() const { return ToQColor(m_textColor); }
        QColor GetLineNumberColor() const { return ToQColor(m_lineNumberColor); }
        QColor GetTextFocusedBackgroundColor() const { return ToQColor(m_textFocusedBackgroundColor); }
        QColor GetTextUnfocusedBackgroundColor() const { return ToQColor(m_textUnfocusedBackgroundColor); }
        QColor GetTextReadOnlyFocusedBackgroundColor() const { return ToQColor(m_textReadOnlyFocusedBackgroundColor); }
        QColor GetTextReadOnlyUnfocusedBackgroundColor() const { return ToQColor(m_textReadOnlyUnfocusedBackgroundColor); }
        QColor GetTextSelectedColor() const { return ToQColor(m_textSelectedColor); }
        QColor GetTextSelectedBackgroundColor() const { return ToQColor(m_textSelectedBackgroundColor); }
        QColor GetTextWhitespaceColor() const { return ToQColor(m_textWhitespaceColor); }
        QColor GetBreakpointFocusedBackgroundColor() const { return ToQColor(m_breakpointFocusedBackgroundColor); }
        QColor GetBreakpointUnfocusedBackgroundColor() const { return ToQColor(m_breakpointUnfocusedBackgroundColor); }
        QColor GetFoldingFocusedBackgroundColor() const { return ToQColor(m_foldingFocusedBackgroundColor); }
        QColor GetFoldingUnfocusedBackgroundColor() const { return ToQColor(m_foldingUnfocusedBackgroundColor); }
        QColor GetCurrentIdentifierColor() const { return ToQColor(m_currentIdentifierColor); }
        QColor GetCurrentLineOutlineColor() const { return ToQColor(m_currentLineOutlineColor); }
        QColor GetSpecialCharacterColor() const { return ToQColor(m_specialCharacterColor); }
        QColor GetKeywordColor() const { return ToQColor(m_keywordColor); }
        QColor GetSpecialKeywordColor() const { return ToQColor(m_specialKeywordColor); }
        QColor GetCommentColor() const { return ToQColor(m_commentColor); }
        QColor GetStringLiteralColor() const { return ToQColor(m_stringLiteralColor); }
        QColor GetNumberColor() const { return ToQColor(m_numberColor); }
        QColor GetLibraryColor() const { return ToQColor(m_libraryColor); }
        QColor GetMethodColor() const { return ToQColor(m_methodColor); }
        QColor GetBracketColor() const { return ToQColor(m_bracketColor); }
        QColor GetSelectedBracketColor() const { return ToQColor(m_selectedBracketColor); }
        QColor GetUnmatchedBracketColor() const { return ToQColor(m_unmatchedBracketColor); }
        QColor GetFoldingColor() const { return ToQColor(m_foldingColor); }
        QColor GetFoldingSelectedColor() const { return ToQColor(m_foldingCurrentColor); }
        QColor GetFoldingLineColor() const { return ToQColor(m_foldingLineColor); }
        QColor GetFindResultsHeaderColor() const { return ToQColor(m_findResultsHeaderColor); }
        QColor GetFindResultsFileColor() const { return ToQColor(m_findResultsFileColor); }
        QColor GetFindResultsMatchColor() const { return ToQColor(m_findResultsMatchColor); }
        QColor GetFindResultsLineHighlightColor() const { return ToQColor(m_findResultsLineHighlightColor); }
        const QFont& GetFont() const { return m_font; }
        bool GetNoAntiAliasing() const { return m_noAntialiasing; }
        int GetTabSize() const { return m_tabSize; }
        bool UseSpacesInsteadOfTabs() const { return m_useSpacesInsteadOfTabs; }

        void SetZoomPercent(float zoom);

        static void Reflect(AZ::ReflectContext* reflection);

    private:
        AZ::Vector3 m_textColor {
            156.0f / 255.0f, 220.0f / 255.0f, 254.0f / 255.0f
        };
        AZ::Vector3 m_lineNumberColor {
            200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f
        };
        AZ::Vector3 m_textFocusedBackgroundColor {
            31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f
        };
        AZ::Vector3 m_textUnfocusedBackgroundColor {
            31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f
        };
        AZ::Vector3 m_textReadOnlyFocusedBackgroundColor {
            60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f
        };
        AZ::Vector3 m_textReadOnlyUnfocusedBackgroundColor {
            60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f
        };
        AZ::Vector3 m_textSelectedColor {
            225.0f / 255.0f, 225.0f / 255.0f, 225.0f / 255.0f
        };
        AZ::Vector3 m_textSelectedBackgroundColor {
            55.0f / 255.0f, 90.0f / 255.0f, 125.0f / 255.0f
        };
        AZ::Vector3 m_textWhitespaceColor{
            100.0f / 255.0f, 100.0f / 255.0f, 100.0f / 255.0f
        };
        AZ::Vector3 m_breakpointFocusedBackgroundColor {
            80.0f / 255.0f, 80.0f / 255.0f, 80.0f / 255.0f
        };
        AZ::Vector3 m_breakpointUnfocusedBackgroundColor {
            80.0f / 255.0f, 80.0f / 255.0f, 80.0f / 255.0f
        };
        AZ::Vector3 m_foldingFocusedBackgroundColor {
            70.0f / 255.0f, 70.0f / 255.0f, 70.0f / 255.0f
        };
        AZ::Vector3 m_foldingUnfocusedBackgroundColor {
            70.0f / 255.0f, 70.0f / 255.0f, 70.0f / 255.0f
        };
        AZ::Vector3 m_currentIdentifierColor {
            68.0f / 255.0f, 68.0f / 255.0f, 68.0f / 255.0f
        };
        AZ::Vector3 m_currentLineOutlineColor {
            61.0f / 255.0f, 61.0f / 255.0f, 61.0f / 255.0f
        };
        AZ::Vector3 m_specialCharacterColor {
            204.0f / 255.0f, 204.0f / 255.0f, 204.0f / 255.0f
        };
        AZ::Vector3 m_keywordColor {
            213.0f / 255.0f, 134.0f / 255.0f, 192.0f / 255.0f
        };
        AZ::Vector3 m_specialKeywordColor {
            63.0f / 255.0f, 156.0f / 255.0f, 214.0f / 255.0f
        };
        AZ::Vector3 m_commentColor {
            106.0f / 255.0f, 153.0f / 255.0f, 85.0f / 255.0f
        };
        AZ::Vector3 m_stringLiteralColor {
            206.0f / 255.0f, 145.0f / 255.0f, 117.0f / 255.0f
        };
        AZ::Vector3 m_numberColor {
            181.0f / 255.0f, 203.0f / 255.0f, 164.0f / 255.0f
        };
        AZ::Vector3 m_libraryColor {
            78.0f / 255.0f, 201.0f / 255.0f, 176.0f / 255.0f
        };
        AZ::Vector3 m_methodColor {
            220.0f / 255.0f, 220.0f / 255.0f, 170.0f / 255.0f
        };
        AZ::Vector3 m_bracketColor {
            255.0f / 255.0f, 215.0f / 255.0f, 0.0f / 255.0f
        };
        AZ::Vector3 m_selectedBracketColor {
            219.0f / 255.0f, 29.0f / 255.0f, 133.0f / 255.0f
        };
        AZ::Vector3 m_unmatchedBracketColor {
            219.0f / 255.0f, 29.0f / 255.0f, 133.0f / 255.0f
        };
        AZ::Vector3 m_foldingColor {
            150.0f / 255.0f, 150.0f / 255.0f, 150.0f / 255.0f
        };
        AZ::Vector3 m_foldingCurrentColor {
            240.0f / 255.0f, 240.0f / 255.0f, 240.0f / 255.0f
        };
        AZ::Vector3 m_foldingLineColor {
            150.0f / 255.0f, 150.0f / 255.0f, 150.0f / 255.0f
        };
        AZ::Vector3 m_findResultsHeaderColor {
            255.0f / 255.0f, 220.0f / 255.0f, 20.0f / 255.0f
        };
        AZ::Vector3 m_findResultsFileColor {
            105.0f / 255.0f, 220.0f / 255.0f, 53.0f / 255.0f
        };
        AZ::Vector3 m_findResultsMatchColor {
            255.0f / 255.0f, 220.0f / 255.0f, 20.0f / 255.0f
        };
        AZ::Vector3 m_findResultsLineHighlightColor {
            160.0f / 255.0f, 160.0f / 255.0f, 164.0f / 255.0f
        };
        QFont m_font;
        AZStd::string m_fontFamily { "Consolas" };
        int m_fontSize {
            14
        };
        bool m_noAntialiasing {
            false
        };
        // Number of spaces to make a tab
        int m_tabSize {
            4
        };
        float m_zoomPercent = 100.f;

        bool m_useSpacesInsteadOfTabs = false;

        void OnColorChange();
        void OnFontChange();

        // We use this class to ensure that the font is updated when the instance of it got de-serialized.
        class SerializationEvents : public AZ::SerializeContext::IEventHandler
        {
            void OnReadEnd(void* classPtr) override
            {
                if(auto* settings = static_cast<SyntaxStyleSettings*>(classPtr))
                {
                    settings->OnFontChange();
                }
            }
        };
    };

    class HighlightedWords
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single;

        typedef AZ::EBus<HighlightedWords> Bus;
        typedef Bus::Handler Handler;

        typedef AZStd::unordered_set<AZStd::string> LUAKeywordsType;
        virtual const LUAKeywordsType* GetLUAKeywords() = 0;
        virtual const LUAKeywordsType* GetLUALibraryFunctions() = 0;
    };

    class HighlightedWordNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        typedef AZ::EBus<HighlightedWordNotifications> Bus;
        typedef Bus::Handler Handler;

        virtual void LUALibraryFunctionsUpdated() = 0;
    };
}
