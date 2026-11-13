/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QColor>
#include <QObject>

class QEvent;
class QPlainTextEdit;

namespace LUAEditor
{
    // ----------------------------------------------------------------------------------
    // The values of the Perforce status label that the legacy GUI's sheet (LUAEditorMainWindow.cpp) and the flattened GUI's elements
    // (FlatElements.cpp) both read, so that each one has a single source: the flattened system owns it. A named namespace, not an
    // anonymous one: a unity build joins the files of a target into one translation unit, where anonymous namespaces merge.
    // ----------------------------------------------------------------------------------
    namespace LUAEditorMainWindowFlat
    {
        //! The object name the site gives the label, which the label's rows match first
        inline constexpr const char* PerforceStatusName = "luaPerforceStatus";

        inline constexpr QColor PerforceStatusFill(192, 192, 192, 255);   // LUAEditorMainWindow.cpp:565. TOKENIZE: LuaPerforceStatusFill
        inline constexpr QColor PerforceStatusText(0, 0, 0);              // LUAEditorMainWindow.cpp:565. TOKENIZE: LuaPerforceStatusText
        inline constexpr QColor PerforceStatusBorder(100, 100, 100, 255); // LUAEditorMainWindow.cpp:565. TOKENIZE: LuaPerforceStatusBorder
    } // namespace LUAEditorMainWindowFlat

    //! Registers the elements of the flattened GUI of LuaIDE (the Perforce status label of a document view). LuaIDE sets no style sheet
    //! file of its own for StyleManager::registerFlatSheet to hang them on, so the main window calls this once, where the module first
    //! needs them; the legacy GUI never calls it. A second call registers nothing.
    void RegisterFlatElements();

    //! The colours of the code view that the user's syntax style settings give (LUAEditorView.cpp, CreateStyleSheet), as the settings
    //! hold them: the legacy GUI's sheet carried each as `#rrggbb` (QColor::name), which drops the alpha, and so does the hand-over
    struct CodeViewColors
    {
        QColor m_focusedBackground;             //!< `QPlainTextEdit:focus { background-color }`, the sheet's %1
        QColor m_unfocusedBackground;           //!< `QPlainTextEdit:!focus`, %2
        QColor m_readOnlyFocusedBackground;     //!< `QPlainTextEdit[readOnly="true"]:focus`, %3
        QColor m_readOnlyUnfocusedBackground;   //!< `QPlainTextEdit[readOnly="true"]:!focus`, %4
        QColor m_selectedText;                  //!< `selection-color` of the four rules, %5
        QColor m_selectedBackground;            //!< `selection-background-color` of the four rules, %6
    };

    //! Flattened GUI only: the code view's colours on its palette, which is how the flattened GUI hands run-time colours over (the
    //! same as the palette of the find results list, FindResults in LUAEditorFindResults.cpp). The legacy GUI gives the view the colours
    //! with a sheet of four rules (LUAEditorView.cpp, CreateStyleSheet); the flattened GUI sets none.
    //!
    //! `background-color` gives the palette's Base, Button and Window and the selection colours its Highlight and HighlightedText
    //! (QRenderRule::configurePalette), in every colour group. The background follows the state the sheet's rules named: the focus, and
    //! whether the view is read only (the later `[readOnly="true"]` rules outrank `:focus` and `:!focus`). Style gives every text edit
    //! the field's own palette when it polishes the widget (Style.cpp, applyFlattenedTextEntryPalette), over whatever the code set
    //! before, so the colours are put over it again whenever the palette changes and the focus moves.
    //!
    //! The view's read-only state has no event that Qt sends for it from every class (LUAViewWidget::SetReadonly polishes the view
    //! again), so SetReadonly calls Refresh once it has.
    //!
    //! One object holds the colours of a view, a child of the view: Apply makes it the first time and gives it the new colours after.
    class CodeViewPalette
        : public QObject
    {
    public:
        //! The view has these colours from now on: put on its palette at once, and kept there
        static void Apply(QPlainTextEdit* view, const CodeViewColors& colors);

        //! The view's palette is made from its state again (it was read only, or no longer is). Nothing for a view with no colours.
        static void Refresh(QPlainTextEdit* view);

        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        CodeViewPalette(QPlainTextEdit* view, const CodeViewColors& colors);

        static CodeViewPalette* Of(QPlainTextEdit* view);
        void Update();

        QPlainTextEdit* m_view;
        CodeViewColors m_colors;
    };
} // namespace LUAEditor
