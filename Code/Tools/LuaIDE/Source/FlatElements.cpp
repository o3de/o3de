/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/FlatElements.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/TextElement.h>

#include <QBrush>
#include <QEvent>
#include <QLabel>
#include <QMargins>
#include <QPalette>
#include <QPlainTextEdit>

namespace LUAEditor
{
    // ----------------------------------------------------------------------------------
    // The flattened GUI of LuaIDE
    //
    // LuaIDE sets no style sheet file of its own, so the sites that set a sheet of their own carry their look here: the legacy GUI sets
    // the sheet (StyleManager::setLegacyStyleSheet), the flattened GUI sets none. What the sheets give every widget the same way is not
    // repeated. The settings dialog's label size is carried in LUAEditorSettingsDialog.cpp, which registers it on its own (the dialog
    // is the one that needs it).
    //
    // Carried here:
    //   - the Perforce status label of a document view (LUAEditorMainWindow.cpp:565), a box and a text element on a QLabel;
    //   - the code view's colours, which are the user's syntax style settings: run-time data, handed to the palette (CodeViewPalette).
    // ----------------------------------------------------------------------------------

    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others into one
    //! translation unit, where anonymous namespaces merge.
    namespace FlatElementsInternal
    {
        using AzQtComponents::BoxElement;
        using AzQtComponents::TextElement;

        // ------------------------------------------------------------------------------
        // The Perforce status label
        // ------------------------------------------------------------------------------

        constexpr int PerforceStatusBorderWidth = 1;    // border-width: LUAEditorMainWindow.cpp:565. TOKENIZE: LuaPerforceStatusBorderWidth
        constexpr int PerforceStatusRadius = 8;         // border-radius: LUAEditorMainWindow.cpp:565. TOKENIZE: LuaPerforceStatusRadius

        //! `background: rgba(192,192,192,255); border-style: inset; border-width: 1px; border-color: rgba(100,100,100,255);
        //! border-radius: 8px` (LUAEditorMainWindow.cpp:565) over the margin of every label (`QLabel { margin-top: 1px; margin-bottom:
        //! 1px }`, Text.qss:93-94): an element is the whole definition of its box, so it carries that margin. An inset border has the
        //! colour of the rule on its top and left edges and that colour lightened on its bottom and right edges. The site fills the
        //! label itself (setAutoFillBackground), which the element takes over at polish, as the sheet did.
        const BoxElement PerforceStatusBox{
            .m_name = "LuaIDE.Box.PerforceStatus",
            .m_margin = QMargins(0, AzQtComponents::Foundations::Label::MarginV, 0, AzQtComponents::Foundations::Label::MarginV),
            .m_nativeBorder = false,
            .m_radius = PerforceStatusRadius,
            .m_idle = {
                .m_fill = QBrush(LUAEditorMainWindowFlat::PerforceStatusFill),
                .m_edges = BoxElement::border(
                    PerforceStatusBorderWidth, LUAEditorMainWindowFlat::PerforceStatusBorder, BoxElement::LineStyle::Inset),
            },
        };

        //! `color: black` of the same sheet, over the family and size of the label rule (Text.qss:91-92)
        const TextElement PerforceStatusText{
            .m_name = "LuaIDE.Text.PerforceStatus",
            .m_family = AzQtComponents::Foundations::Label::Family,
            .m_pixelSize = AzQtComponents::Foundations::Label::Size,
            .m_color = LUAEditorMainWindowFlat::PerforceStatusText,
        };

        //! The label the site names (LUAEditorMainWindow.cpp, OnOpenLUAView): the one label of the application that has the sheet
        bool isPerforceStatus(const QWidget* widget)
        {
            return AzQtComponents::Selectors::named(widget, LUAEditorMainWindowFlat::PerforceStatusName);
        }
    } // namespace FlatElementsInternal

    void RegisterFlatElements()
    {
        // A registration lives for the whole run, and the registries do not look for repeats
        static bool registered = false;
        if (registered)
        {
            return;
        }
        registered = true;

        namespace Flat = FlatElementsInternal;
        AzQtComponents::BoxElement::registerElement(&Flat::PerforceStatusBox, &QLabel::staticMetaObject, &Flat::isPerforceStatus);
        AzQtComponents::TextElement::registerElement(&Flat::PerforceStatusText, &QLabel::staticMetaObject, &Flat::isPerforceStatus);
    }

    // ----------------------------------------------------------------------------------
    // The code view's colours
    // ----------------------------------------------------------------------------------

    CodeViewPalette::CodeViewPalette(QPlainTextEdit* view, const CodeViewColors& colors)
        : QObject(view)
        , m_view(view)
        , m_colors(colors)
    {
        view->installEventFilter(this);
    }

    void CodeViewPalette::Apply(QPlainTextEdit* view, const CodeViewColors& colors)
    {
        // The sheet carried each colour as `#rrggbb`: no alpha
        const auto asSheetColor = [](const QColor& color)
        {
            return QColor(color.rgb());
        };
        const CodeViewColors sheetColors{
            .m_focusedBackground = asSheetColor(colors.m_focusedBackground),
            .m_unfocusedBackground = asSheetColor(colors.m_unfocusedBackground),
            .m_readOnlyFocusedBackground = asSheetColor(colors.m_readOnlyFocusedBackground),
            .m_readOnlyUnfocusedBackground = asSheetColor(colors.m_readOnlyUnfocusedBackground),
            .m_selectedText = asSheetColor(colors.m_selectedText),
            .m_selectedBackground = asSheetColor(colors.m_selectedBackground),
        };

        if (CodeViewPalette* palette = Of(view))
        {
            palette->m_colors = sheetColors;
            palette->Update();
        }
        else
        {
            (new CodeViewPalette(view, sheetColors))->Update();
        }
    }

    void CodeViewPalette::Refresh(QPlainTextEdit* view)
    {
        if (CodeViewPalette* palette = Of(view))
        {
            palette->Update();
        }
    }

    bool CodeViewPalette::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == m_view)
        {
            switch (event->type())
            {
            case QEvent::PaletteChange:     // the style writes its own palette at a polish
            case QEvent::FocusIn:
            case QEvent::FocusOut:
                Update();
                break;
            default:
                break;
            }
        }
        return false;
    }

    CodeViewPalette* CodeViewPalette::Of(QPlainTextEdit* view)
    {
        for (QObject* child : view->children())
        {
            if (auto* palette = dynamic_cast<CodeViewPalette*>(child))
            {
                return palette;
            }
        }
        return nullptr;
    }

    void CodeViewPalette::Update()
    {
        // The rules of the sheet, in the order of their cascade: a read only view takes the `[readOnly="true"]` rules over the others
        const bool focused = m_view->hasFocus();
        const QColor& background = m_view->isReadOnly()
            ? (focused ? m_colors.m_readOnlyFocusedBackground : m_colors.m_readOnlyUnfocusedBackground)
            : (focused ? m_colors.m_focusedBackground : m_colors.m_unfocusedBackground);

        QPalette palette = m_view->palette();
        for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            palette.setColor(group, QPalette::Base, background);
            palette.setColor(group, QPalette::Button, background);
            palette.setColor(group, QPalette::Window, background);
            palette.setColor(group, QPalette::Highlight, m_colors.m_selectedBackground);
            palette.setColor(group, QPalette::HighlightedText, m_colors.m_selectedText);
        }

        // Equal colours set nothing, so a palette change that these colours made does not come back here
        if (palette != m_view->palette())
        {
            m_view->setPalette(palette);
        }
    }
} // namespace LUAEditor
