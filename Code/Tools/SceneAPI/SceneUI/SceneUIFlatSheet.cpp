/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneUI/SceneUIFlatSheet.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QWidget>

// =======================================================================
// Sheets, Values and Elements
//
// A named namespace, not an anonymous one: a unity build merges the anonymous namespaces of a target's files.
// =======================================================================
namespace SceneUIFlatSheetInternal
{
    using AzQtComponents::BoxElement;
    using AzQtComponents::Foundations::Label::MarginV;
    using AzQtComponents::Selectors::named;
    using AzQtComponents::Selectors::withinClass;
    using AzQtComponents::Selectors::tagged;
    using AzQtComponents::TextElement;

    // ---- The header of a group in the display divider style
    //
    // The header sets its sheet on itself when the group has the "display divider" category (HeaderWidget::UpdateUIForManifestObject).
    // `QFrame, QLabel` reaches the two labels below it (the icon and the name; the delete button is no QFrame). The row of the group
    // has a sheet too (PropertyRowWidget.cpp:66, `QFrame {background-color: #333333; margin-top: 5px; border-top-*-radius: 2px}`),
    // which reaches the same labels. The header's is deeper (a widget's sheet outranks that of its parents, whatever the
    // specificity), so the labels take its margin above (0, in place of the row's 5px) and its font, and the fill and the top corners
    // of the row's sheet, which it leaves alone.

    constexpr const char* DividerHeaderSheet = "QFrame, QLabel {margin-top: 0px; font: bold;}";
    constexpr const char* DividerHeaderTag = "SceneDividerHeader";      // the class tag of the header, which the flattened GUI sets

    const QColor DividerFill(0x33, 0x33, 0x33);     // PropertyRowWidget.cpp:66. TOKENIZE: SceneDividerRowFill
    constexpr int DividerRadius = 2;                // PropertyRowWidget.cpp:66. TOKENIZE: SceneDividerRowRadius
    constexpr int DividerHeaderWeight = 700;        // SceneUIFlatSheet.cpp:45 (font: bold). TOKENIZE: SceneDividerHeaderWeight
    constexpr int DividerHeaderItalic = 0;          // SceneUIFlatSheet.cpp:45 (font: bold). TOKENIZE: SceneDividerHeaderStyle
    constexpr int DividerHeaderMarginTop = 0;       // SceneUIFlatSheet.cpp:45 (margin-top: 0px). TOKENIZE: SceneDividerHeaderMarginTop

    //! The labels below the header: the box of every label (margin above and below, Text.qss:93-94) with the margin above set to 0 by
    //! the header's sheet, over the fill and the top corners of the row's. The corners give the border a style of their own.
    const BoxElement DividerHeaderLabel{
        .m_name = "SceneUI.Box.DividerHeaderLabel",
        .m_margin = QMargins(0, DividerHeaderMarginTop, 0, MarginV),
        .m_nativeBorder = false,
        .m_radii = { .m_topLeft = DividerRadius, .m_topRight = DividerRadius },
        .m_idle = {
            .m_fill = QBrush(DividerFill),
        },
    };

    //! `font: bold` (SceneUIFlatSheet.cpp:45): the shorthand sets the weight to bold and the style to normal (QCss::ValueExtractor::
    //! extractFont), over the family and size of the label rule (Text.qss:91-92)
    const TextElement DividerHeaderLabelText{
        .m_name = "SceneUI.Text.DividerHeaderLabel",
        .m_family = AzQtComponents::Foundations::Label::Family,
        .m_pixelSize = AzQtComponents::Foundations::Label::Size,
        .m_weight = DividerHeaderWeight,
        .m_italic = DividerHeaderItalic,
    };

    //! Any label below a header that carries the tag: the header's labels are its two children (HeaderWidget.ui:26, :48). Their names
    //! first and the parent's tag then, where a walk read the tag of every widget above every label of the Editor
    bool isDividerHeaderLabel(const QWidget* widget)
    {
        const QWidget* parent = widget->parentWidget();
        return (named(widget, "m_icon") || named(widget, "m_nameLabel")) && parent && tagged(parent, DividerHeaderTag);
    }

    // ---- The frame of the buttons of a manifest page
    //
    // The rule names QFrame, so it reaches the frames below the header too; the header has none (its children are push buttons and tool
    // buttons). `border-top: 1px transparent` has no style, which is none: that edge is 0 wide (QCss::ValueExtractor::borderValue,
    // QRenderRule::fixupBorder). The border has a style of its own, and the fill is the universal rule's (BaseStyleSheet.qss:17).

    constexpr const char* PageHeaderSheet =
        "QFrame {border-top: 1px transparent; padding-top: 6px; border-bottom: 1px solid #000000; padding-bottom: 6px}";

    constexpr QMargins PageHeaderPadding(0, 6, 0, 6);               // SceneUIFlatSheet.cpp:89. TOKENIZE: ScenePageHeaderPadding
    constexpr int PageHeaderLineWidth = 1;                          // SceneUIFlatSheet.cpp:89. TOKENIZE: ScenePageHeaderLine
    const QColor PageHeaderLine(0x00, 0x00, 0x00);                  // SceneUIFlatSheet.cpp:89. TOKENIZE: ScenePageHeaderLine

    const BoxElement PageHeader{
        .m_name = "SceneUI.Box.PageHeader",
        .m_padding = PageHeaderPadding,
        .m_nativeBorder = false,
        .m_idle = {
            .m_edges = { .m_bottom = { PageHeaderLineWidth, PageHeaderLine } },
        },
    };

    //! `QFrame#header` below the manifest page
    bool isPageHeader(const QWidget* widget)
    {
        return named(widget, "header") && withinClass(widget, "AZ::SceneAPI::UI::ManifestWidgetPage");
    }

    // ---- The buttons that add an entry
    //
    // A sheet with no selector is a rule for the widget it is set on; here it gives the box a padding, and the application's rules for
    // push buttons (PushButton.qss) stay under it.

    constexpr const char* AddObjectButtonSheet = "padding: 6px;";
    constexpr const char* AddButtonSheet = "padding-left: 12px; padding-right: 12px;";

    constexpr QMargins AddObjectButtonPadding(6, 6, 6, 6);          // SceneUIFlatSheet.cpp:115. TOKENIZE: SceneAddObjectPadding
    constexpr QMargins AddButtonPadding(12, 0, 12, 0);              // SceneUIFlatSheet.cpp:116. TOKENIZE: SceneAddButtonPadding

    const BoxElement AddObjectButton{
        .m_name = "SceneUI.Box.AddObjectButton",
        .m_padding = AddObjectButtonPadding,
    };

    const BoxElement AddButton{
        .m_name = "SceneUI.Box.AddButton",
        .m_padding = AddButtonPadding,
    };

    bool isAddObjectButton(const QWidget* widget)
    {
        return named(widget, "m_addObjectButton") && withinClass(widget, "AZ::SceneAPI::UI::ManifestVectorWidget");
    }

    bool isAddButton(const QWidget* widget)
    {
        return named(widget, "m_addButton") && withinClass(widget, "AZ::SceneAPI::UI::ManifestWidgetPage");
    }
} // namespace SceneUIFlatSheetInternal

// =======================================================================
// Registration and Sites
// =======================================================================
namespace SceneUIFlatSheet
{
    static int s_registerElementsCalls = 0;

    int RegisterElementsCalls()
    {
        return s_registerElementsCalls;
    }

    void RegisterElements()
    {
        ++s_registerElementsCalls;
        static bool registered = false;
        if (registered)
        {
            return;
        }
        registered = true;

        namespace Flat = SceneUIFlatSheetInternal;
        using AzQtComponents::BoxElement;
        using AzQtComponents::TextElement;
        BoxElement::registerElement(&Flat::DividerHeaderLabel, &QLabel::staticMetaObject, &Flat::isDividerHeaderLabel);
        TextElement::registerElement(&Flat::DividerHeaderLabelText, &QLabel::staticMetaObject, &Flat::isDividerHeaderLabel);
        BoxElement::registerElement(&Flat::PageHeader, &QFrame::staticMetaObject, &Flat::isPageHeader);
        BoxElement::registerElement(&Flat::AddObjectButton, &QPushButton::staticMetaObject, &Flat::isAddObjectButton);
        BoxElement::registerElement(&Flat::AddButton, &QPushButton::staticMetaObject, &Flat::isAddButton);
    }

    void StyleDisplayDividerHeader(QWidget* header)
    {
        namespace Flat = SceneUIFlatSheetInternal;
        AzQtComponents::StyleManager::setLegacyStyleSheet(header, QString::fromLatin1(Flat::DividerHeaderSheet));

        // The header is tagged once its group is known, which is after its widgets were made: the labels below it that are polished
        // already are polished again (the others resolve their elements when they are)
        if (AzQtComponents::StyleManager::flatStylingActive() && !AzQtComponents::Selectors::tagged(header, Flat::DividerHeaderTag))
        {
            AzQtComponents::Style::addClass(header, QString::fromLatin1(Flat::DividerHeaderTag));
            for (QWidget* each : header->findChildren<QWidget*>())
            {
                if (each->testAttribute(Qt::WA_WState_Polished))
                {
                    each->style()->unpolish(each);
                    each->style()->polish(each);
                }
            }
        }
    }

    void StyleManifestPageHeader(QFrame* header)
    {
        AzQtComponents::StyleManager::setLegacyStyleSheet(header, QString::fromLatin1(SceneUIFlatSheetInternal::PageHeaderSheet));
    }

    void StyleAddButton(QPushButton* button)
    {
        AzQtComponents::StyleManager::setLegacyStyleSheet(button, QString::fromLatin1(SceneUIFlatSheetInternal::AddButtonSheet));
    }

    void StyleAddObjectButton(QPushButton* button)
    {
        AzQtComponents::StyleManager::setLegacyStyleSheet(button, QString::fromLatin1(SceneUIFlatSheetInternal::AddObjectButtonSheet));
    }
} // namespace SceneUIFlatSheet
