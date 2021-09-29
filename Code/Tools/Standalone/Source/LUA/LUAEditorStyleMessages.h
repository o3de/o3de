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
        AZ_CLASS_ALLOCATOR(SyntaxStyleSettings, AZ::SystemAllocator, 0);

        QColor ToQColor(const AZ::Vector3& color) const
        {
            return QColor::fromRgbF(color.GetX(), color.GetY(), color.GetZ());
        }

        QColor GetTextColor() const { return ToQColor(m_textColor); }
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
        QColor GetKeywordColor() const { return ToQColor(m_keywordColor); }
        QColor GetCommentColor() const { return ToQColor(m_commentColor); }
        QColor GetStringLiteralColor() const { return ToQColor(m_stringLiteralColor); }
        QColor GetNumberColor() const { return ToQColor(m_numberColor); }
        QColor GetLibraryColor() const { return ToQColor(m_libraryColor); }
        QColor GetBracketColor() const { return ToQColor(m_bracketColor); }
        QColor GetUnmatchedBracketColor() const { return ToQColor(m_unmatchedBracketColor); }
        QColor GetFoldingColor() const { return ToQColor(m_foldingColor); }
        QColor GetFoldingSelectedColor() const { return ToQColor(m_foldingCurrentColor); }
        QColor GetFoldingLineColor() const { return ToQColor(m_foldingLineColor); }
        QColor GetFindResultsHeaderColor() const { return ToQColor(m_findResultsHeaderColor); }
        QColor GetFindResultsFileColor() const { return ToQColor(m_findResultsFileColor); }
        QColor GetFindResultsMatchColor() const { return ToQColor(m_findResultsMatchColor); }
        QColor GetFindResultsLineHighlightColor() const { return ToQColor(m_findResultsLineHighlightColor); }
        QFont GetFont() const;
        int GetTabSize() const { return m_tabSize; }
        bool UseSpacesInsteadOfTabs() const { return m_useSpacesInsteadOfTabs; }

        static void Reflect(AZ::ReflectContext* reflection);

    private:
        AZ::Vector3 m_textColor {
            220.0f / 255.0f, 220.0f / 255.0f, 220.0f / 255.0f
        };
        AZ::Vector3 m_textFocusedBackgroundColor {
            60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f
        };
        AZ::Vector3 m_textUnfocusedBackgroundColor {
            60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f
        };
        AZ::Vector3 m_textReadOnlyFocusedBackgroundColor {
            60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f
        };
        AZ::Vector3 m_textReadOnlyUnfocusedBackgroundColor {
            60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f
        };
        AZ::Vector3 m_textSelectedColor {
            60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f
        };
        AZ::Vector3 m_textSelectedBackgroundColor {
            220.0f / 255.0f, 220.0f / 255.0f, 220.0f / 255.0f
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
            25.0f / 255.0f, 25.0f / 255.0f, 25.0f / 255.0f
        };
        AZ::Vector3 m_currentLineOutlineColor {
            128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f
        };
        AZ::Vector3 m_keywordColor {
            160.0f / 255.0f, 160.0f / 255.0f, 255.0f / 255.0f
        };
        AZ::Vector3 m_commentColor {
            130.0f / 255.0f, 160.0f / 255.0f, 130.0f / 255.0f
        };
        AZ::Vector3 m_stringLiteralColor {
            220.0f / 255.0f, 120.0f / 255.0f, 120.0f / 255.0f
        };
        AZ::Vector3 m_numberColor {
            200.0f / 255.0f, 200.0f / 255.0f, 100.0f / 255.0f
        };
        AZ::Vector3 m_libraryColor {
            220.0f / 255.0f, 150.0f / 255.0f, 220.0f / 255.0f
        };
        AZ::Vector3 m_bracketColor {
            80.0f / 255.0f, 190.0f / 255.0f, 190.0f / 255.0f
        };
        AZ::Vector3 m_unmatchedBracketColor {
            80.0f / 255.0f, 130.0f / 255.0f, 130.0f / 255.0f
        };
        AZ::Vector3 m_foldingColor {
            150.0f / 255.0f, 150.0f / 255.0f, 150.0f / 255.0f
        };
        AZ::Vector3 m_foldingCurrentColor {
            255.0f / 255.0f, 50.0f / 255.0f, 50.0f / 255.0f
        };
        AZ::Vector3 m_foldingLineColor {
            0.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f
        };
        AZ::Vector3 m_findResultsHeaderColor {
            255.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f
        };
        AZ::Vector3 m_findResultsFileColor {
            0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f
        };
        AZ::Vector3 m_findResultsMatchColor {
            0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f
        };
        AZ::Vector3 m_findResultsLineHighlightColor {
            160.0f / 255.0f, 160.0f / 255.0f, 164.0f / 255.0f
        };
        AZStd::string m_font {
            "OpenSans"
        };
        int m_fontSize {
            10
        };
        // Number of spaces to make a tab
        int m_tabSize {
            4
        };

        bool m_useSpacesInsteadOfTabs = false;

        void OnColorChange();
        void OnFontChange();
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
