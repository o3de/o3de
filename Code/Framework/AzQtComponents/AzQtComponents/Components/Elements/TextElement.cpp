/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>

#include <AzCore/Debug/Trace.h>

#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QFont>
#include <QLineEdit>
#include <QPalette>
#include <QVariant>
#include <QWidget>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace TextElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        constexpr const char* ToolTipFamily = "Open Sans";      // ToolTip.qss:19 $type.tooltip.idle.family
        constexpr int ToolTipSize = 12;                         // ToolTip.qss:16 $type.tooltip.idle.size
        constexpr const char* PushButtonFamily = "Open Sans";   // PushButton.qss:33 $type.button.idle.family
        constexpr int PushButtonSize = 12;                      // PushButton.qss:34 $type.button.idle.size.Alt1
        constexpr int CheckSize = 12;                           // CheckBox.qss:13, RadioButton.qss:12 $type.check.idle.Alt1
        constexpr const char* MenuBarFamily = "Amazon Ember";   // MenuBar.qss:13 $type.menubar.idle.family
        constexpr int MenuBarSize = 12;                         // MenuBar.qss:14 $type.menubar.idle.size
        constexpr int ProgressBarSize = 1;                      // ProgressBar.qss:16 $type.progress.idle
        constexpr int HeadlineSize = 24;                        // Text.qss:58 $type.label.idle.size.Alt6
        constexpr int TitleSize = 18;                           // Text.qss:67 $type.label.idle.size.Alt3
        constexpr int SubtitleSize = 16;                        // Text.qss:75 $type.label.idle.size.Alt5
        constexpr int HeadingWeight = 250;                      // Text.qss:59, :76 $type.label.idle.weight.Alt3
        constexpr int ButtonWeight = 570;                       // Text.qss:124 $type.label.idle.weight.Alt5
        constexpr int TitleBarTitleSize = 14;                   // TitleBar.qss:34 $type.label.idle.size.Alt2
        constexpr int CardTitleWeight = 700;                    // Card.qss:205 (bold) $type.label.idle.weight.Alt1
        constexpr int PaletteCardTitleWeight = 400;             // ColorPicker.qss:58 (normal) $type.label.idle.weight.Alt2

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! A widget no class rule reaches: '*' over the application font
        const TextElement BaseText{ .m_name = "Global.Text.Base", .m_pixelSize = Foundations::Surface::Size };
        //! Only '*' reaches the widget: family and weight; the size stays the platform's or the code's
        const TextElement AnyText{ .m_name = "Global.Text.Any" };
        //! QLabel, and the .Menu, .Paragraph and .Tooltip tiers
        const TextElement LabelText{
            .m_name = "Global.Text.Label",
            .m_family = Foundations::Label::Family,
            .m_pixelSize = Foundations::Label::Size,
        };
        //! QToolTip (Qt's QTipLabel)
        const TextElement ToolTipText{ .m_name = "Global.Text.ToolTip", .m_family = ToolTipFamily, .m_pixelSize = ToolTipSize };
        //! QPushButton
        const TextElement PushButtonText{
            .m_name = "Global.Text.PushButton",
            .m_family = PushButtonFamily,
            .m_pixelSize = PushButtonSize,
        };
        //! QCheckBox, QRadioButton
        const TextElement CheckText{ .m_name = "Global.Text.Check", .m_pixelSize = CheckSize };
        //! The entry fields: QTextEdit, QPlainTextEdit, QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox
        const TextElement FieldText{
            .m_name = "Global.Text.Field",
            .m_family = Foundations::Field::Family,
            .m_pixelSize = Foundations::Field::Size,
        };
        const TextElement MenuBarText{ .m_name = "Global.Text.MenuBar", .m_family = MenuBarFamily, .m_pixelSize = MenuBarSize };
        //! So that Qt sizes a progress bar for no text: its minimum size hint follows the font
        const TextElement ProgressBarText{ .m_name = "Global.Text.ProgressBar", .m_pixelSize = ProgressBarSize };

        //! The Text.qss tiers
        const TextElement HeadlineText{
            .m_name = "Global.Text.Headline",
            .m_family = Foundations::Label::Family,
            .m_pixelSize = HeadlineSize,
            .m_weight = HeadingWeight,
        };
        const TextElement TitleText{ .m_name = "Global.Text.Title", .m_family = Foundations::Label::Family, .m_pixelSize = TitleSize };
        const TextElement SubtitleText{
            .m_name = "Global.Text.Subtitle",
            .m_family = Foundations::Label::Family,
            .m_pixelSize = SubtitleSize,
            .m_weight = HeadingWeight,
        };
        const TextElement ButtonText{
            .m_name = "Global.Text.Button",
            .m_family = Foundations::Label::Family,
            .m_pixelSize = Foundations::Label::Size,
            .m_weight = ButtonWeight,
        };

        //! `AzQtComponents--TitleBar #title`
        const TextElement TitleBarTitleText{
            .m_name = "Global.Text.TitleBarTitle",
            .m_family = Foundations::Label::Family,
            .m_pixelSize = TitleBarTitleSize,
        };
        //! `AzQtComponents--CardHeader #Title`
        const TextElement CardTitleText{
            .m_name = "Global.Text.CardTitle",
            .m_family = Foundations::Label::Family,
            .m_pixelSize = Foundations::Label::Size,
            .m_weight = CardTitleWeight,
        };
        //! `AzQtComponents--PaletteCardBase AzQtComponents--CardHeader #Title`
        const TextElement PaletteCardTitleText{
            .m_name = "Global.Text.PaletteCardTitle",
            .m_family = Foundations::Label::Family,
            .m_pixelSize = Foundations::Label::Size,
            .m_weight = PaletteCardTitleWeight,
        };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        //! A class name, or a class tag, and the element its rule gives
        struct Rule
        {
            const char* m_key;
            const TextElement* m_element;
        };

        //! The classes a rule gives a size, installed as class fonts whatever the platform supplies, and set over a font code set.
        //! A class comes before its base classes (QTipLabel is a QLabel).
        const Rule ClassRules[] = {
            { "QMenuBar", &MenuBarText },
            { "QProgressBar", &ProgressBarText },
            { "QTipLabel", &ToolTipText },
            { "QLabel", &LabelText },
            { "QPushButton", &PushButtonText },
            { "QCheckBox", &CheckText },
            { "QRadioButton", &CheckText },
            { "QTextEdit", &FieldText },
            { "QPlainTextEdit", &FieldText },
            { "QLineEdit", &FieldText },
            { "QSpinBox", &FieldText },
            { "QDoubleSpinBox", &FieldText },
            { "QComboBox", &FieldText },
        };

        //! Qt's class-font keys of classes no rule gives a size: '*' over the font the platform supplies (Windows: QMenu, QMessageBox,
        //! QStatusBar, QAbstractItemView). The keys Qt reads for itself are left alone, as no rule reaches them: QMdiSubWindowTitleBar,
        //! QDockWidgetTitle, QComboMenuItem, QMenuItem, QTitleBar, QListBox, QComboLineEdit.
        const char* const PlatformClasses[] = { "QMenu", "QMessageBox", "QStatusBar", "QToolButton", "QAbstractItemView", "QListView",
            "QHeaderView", "QSmallFont", "QMiniFont" };

        //! The Text.qss tiers in declaration order: of several tags the later is taken whole (the sheet merged them attribute by
        //! attribute; no widget carries two)
        const Rule TierRules[] = {
            { "Headline", &HeadlineText },
            { "Title", &TitleText },
            { "Subtitle", &SubtitleText },
            { "Menu", &LabelText },
            { "Label", &BaseText },
            { "Paragraph", &LabelText },
            { "Tooltip", &LabelText },
            { "Button", &ButtonText },
        };

        ElementRegistry<TextElement>& registry()
        {
            static ElementRegistry<TextElement> rows("_o3de_textColorElement");
            return rows;
        }

        //! One class font: the element over the platform's font for the class. A class the platform supplies none for gets one of
        //! its own if `always`, never a copy of the application font: Qt tells a class font from it by identity
        //! (QWidgetPrivate::naturalWidgetFont).
        void installClassFont(const char* className, const TextElement& element, bool always)
        {
            QFont font = QApplication::font(className); // the platform's, else the application font
            const bool supplied = !font.isCopyOf(QApplication::font());
            if (!supplied)
            {
                if (!always)
                {
                    return;
                }
                font = QFont(QString::fromUtf8(element.m_family));
            }
            if (element.overlay(font) || !supplied)
            {
                QApplication::setFont(font, className);
            }
        }

        //! Code set font attributes on the widget or on an ancestor: Qt keeps both in the mask of the widget's font, class fonts add
        //! none (QWidgetPrivate::resolveFont)
        bool hasCodeFont(const QWidget* widget)
        {
            return widget->font().resolveMask() != 0;
        }

        //! The element of a widget's class rule, which the sheet set over a font code set (QStyleSheetStyle::updateStyleSheetFont)
        const TextElement* classElement(const QWidget* widget)
        {
            for (const Rule& rule : ClassRules)
            {
                if (widget->inherits(rule.m_key))
                {
                    return rule.m_element;
                }
            }
            return &AnyText;
        }

        const TextElement* tierElement(const QWidget* widget)
        {
            const TextElement* element = nullptr;
            if (widget->property("class").isValid())
            {
                for (const Rule& tier : TierRules)
                {
                    if (Selectors::tagged(widget, tier.m_key))
                    {
                        element = tier.m_element;
                    }
                }
            }
            return element;
        }

        //! The Global element a widget's place or tags give it, or none; the most specific rule first. fromBase: the widget's font is
        //! the untouched base.
        const TextElement* globalElement(const QWidget* widget, bool fromBase)
        {
            if (widget->inherits("QLabel"))
            {
                if (Selectors::named(widget, "title") && Selectors::withinClass(widget, "AzQtComponents::TitleBar"))
                {
                    return &TitleBarTitleText;
                }
                if (Selectors::named(widget, "Title") && Selectors::withinClass(widget, "AzQtComponents::CardHeader"))
                {
                    return Selectors::withinClass(widget, "AzQtComponents::PaletteCardBase") ? &PaletteCardTitleText : &CardTitleText;
                }
                if (const TextElement* element = tierElement(widget))
                {
                    return element;
                }
            }
            // `AzQtComponents--ColorPicker *`: every widget inside a color picker, and the menus and message boxes it opens. A widget
            // that starts from the base has its size already.
            if (!fromBase && Selectors::withinClass(widget, "AzQtComponents::ColorPicker"))
            {
                return &BaseText;
            }
            return nullptr;
        }

        //! A module's element first, then the Global one, then the class rule over a font code set; none: the widget keeps its
        //! class font
        const TextElement* findElement(const QWidget* widget, const QFont& classFont)
        {
            if (const TextElement* element = registry().find(widget))
            {
                return element;
            }
            const bool codeFont = hasCodeFont(widget);
            const bool fromBase = !codeFont && classFont.isCopyOf(QApplication::font("QWidget"));
            if (const TextElement* element = globalElement(widget, fromBase))
            {
                return element;
            }
            return codeFont ? classElement(widget) : nullptr;
        }

        bool hasColor(const TextElement* element)
        {
            return element && (element->m_color.isValid() || element->m_disabledColor.isValid());
        }

        //! Only a widget whose element has a colour keeps it, for applyColor
        void keepColorElement(QWidget* widget, const TextElement* element)
        {
            if (hasColor(element))
            {
                registry().keep(widget, element);
            }
            else
            {
                registry().forget(widget);
            }
        }

        //! The widget inside that the sheet gave the same palette (embeddedWidget, qstylesheetstyle.cpp): the line edit of an
        //! editable combo box or of a spin box, the viewport of a scroll area
        QWidget* embeddedWidget(QWidget* widget)
        {
            if (auto* comboBox = qobject_cast<QComboBox*>(widget))
            {
                return comboBox->isEditable() ? comboBox->lineEdit() : widget;
            }
            if (qobject_cast<QAbstractSpinBox*>(widget))
            {
                return widget->findChild<QLineEdit*>();
            }
            if (auto* scrollArea = qobject_cast<QAbstractScrollArea*>(widget))
            {
                return scrollArea->viewport();
            }
            return widget;
        }

        //! A `color` in one colour group, as QRenderRule::configurePalette sets it over whatever the palette had: the text roles,
        //! the foreground role of the embedded widget, and the placeholder at half the alpha
        void setForeground(QPalette& palette, QPalette::ColorGroup group, const QColor& color, const QWidget* embedded)
        {
            palette.setColor(group, QPalette::ButtonText, color);
            palette.setColor(group, embedded->foregroundRole(), color);
            palette.setColor(group, QPalette::WindowText, color);
            palette.setColor(group, QPalette::Text, color);
            QColor placeholder(color);
            placeholder.setAlpha((color.alpha() + 1) / 2);
            palette.setColor(group, QPalette::PlaceholderText, placeholder);
        }
    } // namespace TextElementInternal

    // ----------------------------------------------------------------------------------
    // Element
    // ----------------------------------------------------------------------------------

    bool TextElement::overlay(QFont& font) const
    {
        const QFont before = font;
        font.setFamily(QString::fromUtf8(m_family));
        if (m_pixelSize > 0)
        {
            font.setPixelSize(m_pixelSize);
        }
        font.setWeight(static_cast<QFont::Weight>(m_weight));
        if (m_italic >= 0)
        {
            font.setItalic(m_italic != 0);
        }
        return font != before;
    }

    // ----------------------------------------------------------------------------------
    // Class fonts
    // ----------------------------------------------------------------------------------

    void TextElement::installClassFonts()
    {
        using namespace TextElementInternal;

        // Qt looks a class font up by the widget's class, then each base class up to QWidget (QApplication::font), and applies it
        // right after the style's polish (QWidget::event, QEvent::Polish)
        installClassFont("QWidget", BaseText, true);
        for (const Rule& rule : ClassRules)
        {
            installClassFont(rule.m_key, *rule.m_element, true);
        }
        for (const char* className : PlatformClasses)
        {
            installClassFont(className, AnyText, false);
        }
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void TextElement::registerElement(
        const TextElement* element, const QMetaObject* widgetClass, ElementMatcher matcher, ElementScope scope)
    {
        TextElementInternal::registry().add(element, widgetClass, matcher, scope);
        resolvePolished(widgetClass, [](QWidget* widget)
            {
                resolve(widget);
                applyColor(widget);
            });
    }

    void TextElement::resolve(QWidget* widget)
    {
        // The class font Qt applies right after this polish
        const QFont classFont = QApplication::font(widget);

        // Tripwire: in the flattened GUI every widget has a class font, the base at least. QApplication::setFont(font) without a
        // class name clears them all, and a widget would take its parent's whole font again.
        AZ_WarningOnce("TextElement", !classFont.isCopyOf(QApplication::font()),
            "The class fonts of the flattened GUI were cleared by QApplication::setFont: call it before QApplication::setStyle.");

        const TextElement* element = TextElementInternal::findElement(widget, classFont);
        TextElementInternal::keepColorElement(widget, element);
        if (!element)
        {
            return;
        }
        // Compared with the font the widget is about to have, not with the one it has now
        QFont font = widget->font().resolve(classFont);
        if (element->overlay(font))
        {
            widget->setFont(font);
        }
    }

    void TextElement::applyColor(QWidget* widget)
    {
        const std::optional<const TextElement*> kept = TextElementInternal::registry().kept(widget);
        if (!kept)
        {
            return;
        }
        const TextElement* element = *kept;
        QWidget* embedded = TextElementInternal::embeddedWidget(widget);
        if (!embedded)
        {
            embedded = widget;                  // a spin box without its line edit yet
        }

        // Each colour group as the sheet built it from the rules of its state, into the widget's palette and the one of the widget
        // inside (QStyleSheetStyle::setPalette): Active and Inactive from the enabled rules, Disabled from :disabled
        const QColor disabled = element->m_disabledColor.isValid() ? element->m_disabledColor : element->m_color;
        QPalette palette = widget->palette();
        if (element->m_color.isValid())
        {
            TextElementInternal::setForeground(palette, QPalette::Active, element->m_color, embedded);
            TextElementInternal::setForeground(palette, QPalette::Inactive, element->m_color, embedded);
        }
        if (disabled.isValid())
        {
            TextElementInternal::setForeground(palette, QPalette::Disabled, disabled, embedded);
        }
        widget->setPalette(palette);
        if (embedded != widget)
        {
            embedded->setPalette(palette);
        }
    }
} // namespace AzQtComponents
