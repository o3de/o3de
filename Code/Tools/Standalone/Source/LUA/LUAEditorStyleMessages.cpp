/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorStyleMessages.h"
#include <AzCore/Serialization/EditContext.h>
#include "LUAEditorViewMessages.h"

namespace LUAEditor
{
    QFont SyntaxStyleSettings::GetFont() const
    {
        QFont result;
        result.setFamily(m_font.c_str());
        result.setFixedPitch(true);
        result.setStyleHint(QFont::Monospace);
        result.setPointSize(m_fontSize);

        return result;
    }

    void SyntaxStyleSettings::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<SyntaxStyleSettings, AZ::UserSettings >()
                ->Version(6)
                ->Field("m_font", &SyntaxStyleSettings::m_font)
                ->Field("m_fontSize", &SyntaxStyleSettings::m_fontSize)
                ->Field("m_tabSize", &SyntaxStyleSettings::m_tabSize)
                ->Field("m_useSpacesInsteadOfTabs", &SyntaxStyleSettings::m_useSpacesInsteadOfTabs)

                ->Field("m_textColor", &SyntaxStyleSettings::m_textColor)
                ->Field("m_textSelectedColor", &SyntaxStyleSettings::m_textSelectedColor)
                ->Field("m_textSelectedBackgroundColor", &SyntaxStyleSettings::m_textSelectedBackgroundColor)

                ->Field("m_textFocusedBackgroundColor", &SyntaxStyleSettings::m_textFocusedBackgroundColor)
                ->Field("m_textUnfocusedBackgroundColor", &SyntaxStyleSettings::m_textUnfocusedBackgroundColor)
                ->Field("m_textReadOnlyFocusedBackgroundColor", &SyntaxStyleSettings::m_textReadOnlyFocusedBackgroundColor)
                ->Field("m_textReadOnlyUnfocusedBackgroundColor", &SyntaxStyleSettings::m_textReadOnlyUnfocusedBackgroundColor)

                ->Field("m_textWhitespaceColor", &SyntaxStyleSettings::m_textWhitespaceColor)

                ->Field("m_breakpointFocusedBackgroundColor", &SyntaxStyleSettings::m_breakpointFocusedBackgroundColor)
                ->Field("m_breakpointUnocusedBackgroundColor", &SyntaxStyleSettings::m_breakpointUnfocusedBackgroundColor)

                ->Field("m_foldingFocusedBackgroundColor", &SyntaxStyleSettings::m_foldingFocusedBackgroundColor)
                ->Field("m_foldingUnfocusedBackgroundColor", &SyntaxStyleSettings::m_foldingUnfocusedBackgroundColor)

                ->Field("m_currentIdentifierColor", &SyntaxStyleSettings::m_currentIdentifierColor)
                ->Field("m_currentLineOutlineColor", &SyntaxStyleSettings::m_currentLineOutlineColor)
                ->Field("m_keywordColor", &SyntaxStyleSettings::m_keywordColor)
                ->Field("m_commentColor", &SyntaxStyleSettings::m_commentColor)
                ->Field("m_stringLiteralColor", &SyntaxStyleSettings::m_stringLiteralColor)
                ->Field("m_numberColor", &SyntaxStyleSettings::m_numberColor)
                ->Field("m_libraryColor", &SyntaxStyleSettings::m_libraryColor)
                ->Field("m_bracketColor", &SyntaxStyleSettings::m_bracketColor)
                ->Field("m_unmatchedBracketColor", &SyntaxStyleSettings::m_unmatchedBracketColor)

                ->Field("m_foldingColor", &SyntaxStyleSettings::m_foldingColor)
                ->Field("m_foldingCurrentColor", &SyntaxStyleSettings::m_foldingCurrentColor)
                ->Field("m_foldingLineColor", &SyntaxStyleSettings::m_foldingLineColor)

                ->Field("m_findResultsHeaderColor", &SyntaxStyleSettings::m_findResultsHeaderColor)
                ->Field("m_findResultsFileColor", &SyntaxStyleSettings::m_findResultsFileColor)
                ->Field("m_findResultsMatchColor", &SyntaxStyleSettings::m_findResultsMatchColor)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SyntaxStyleSettings>("Syntax Colors", "Customize the Lua IDE syntax and interface colors.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Font")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SyntaxStyleSettings::m_font, "Font", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnFontChange)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SyntaxStyleSettings::m_fontSize, "Size", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnFontChange)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SyntaxStyleSettings::m_tabSize, "Tab Size", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnFontChange)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SyntaxStyleSettings::m_useSpacesInsteadOfTabs, "Use Spaces instead of Tabs", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnFontChange)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Editing")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Text")

                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_textColor, "Default", "Default text color")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_textSelectedColor, "Selected Text", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_textSelectedBackgroundColor, "Selected Text Background", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_textFocusedBackgroundColor, "Focused Background Color", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_textUnfocusedBackgroundColor, "Unfocused Background Color", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_textReadOnlyFocusedBackgroundColor, "Read Only Focused Background", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_textReadOnlyUnfocusedBackgroundColor, "Read Only Background", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_textWhitespaceColor, "Whitespace Color", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Interface")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_breakpointFocusedBackgroundColor, "Focused Breakpoint Background", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_breakpointUnfocusedBackgroundColor, "Non Focused Breakpoint Background", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_foldingFocusedBackgroundColor, "Folding Focused Background", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_foldingUnfocusedBackgroundColor, "Folding Non Focused Back", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_currentLineOutlineColor, "Line Outline", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "LUA Syntax")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_currentIdentifierColor, "Current Identifier", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_keywordColor, "Keyword", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_commentColor, "Comment", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_stringLiteralColor, "String Literal", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_numberColor, "Number", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_libraryColor, "Library", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_bracketColor, "Bracket", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_unmatchedBracketColor, "Unmatched Bracket", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_foldingColor, "Folding", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_foldingCurrentColor, "Folding Current", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_foldingLineColor, "Folding Line", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Find Results")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_findResultsHeaderColor, "Header", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_findResultsFileColor, "File", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SyntaxStyleSettings::m_findResultsMatchColor, "Match", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SyntaxStyleSettings::OnColorChange)
                ;
            }
        }
    }

    void SyntaxStyleSettings::OnColorChange()
    {
        EBUS_EVENT(LUAEditorMainWindowMessages::Bus, Repaint);
    }

    void SyntaxStyleSettings::OnFontChange()
    {
        EBUS_EVENT(LUAEditorMainWindowMessages::Bus, Repaint);
    }
}
