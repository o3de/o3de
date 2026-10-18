/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/Debug/Trace.h>

#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QFont>
#include <QLineEdit>
#include <QPalette>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <QWidget>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this
    //! file with others into one translation unit, where anonymous namespaces merge (TabElement.cpp has a
    //! globalElement of its own, and ProgressBar and TitleBar are classes of this library).
    namespace TextElementInternal
    {
        // ------------------------------------------------------------------------------
        // Global elements. Each value cites the declaration that grants it, as file:line of
        // the legacy stylesheets in Components/Widgets/ (or of the code that sets it).
        // ------------------------------------------------------------------------------

        //! The base: what a widget no class font and no rule reaches renders. '*' (BaseStyleSheet.qss:19-20)
        //! over the application font, Open Sans 12px (StyleManager.cpp:220-222). TOKENIZE: BodyTextSize
        const TextElement Base{ .m_name = "Global.Text.Base", .m_pixelSize = 12 };
        //! Only '*' reaches the widget: family and weight. Its size stays (the platform's class font, or what
        //! code set).
        const TextElement Any{ .m_name = "Global.Text.Any" };
        //! A rule sets 12px: Text.qss:92 (.Label, QLabel), ToolTip.qss:16 (QToolTip, Qt's QTipLabel),
        //! PushButton.qss:34, CheckBox.qss:13, RadioButton.qss:12, ComboBox.qss:18, BaseStyleSheet.qss:82
        //! (QTextEdit, QPlainTextEdit, QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox), ColorPicker.qss:16-18
        //! (every widget inside a color picker) and :56-58 (a palette card's title, not bold).
        //! TOKENIZE: BodyTextSize
        const TextElement Body{ .m_name = "Global.Text.Body", .m_pixelSize = 12 };
        const TextElement MenuBar{ .m_name = "Global.Text.MenuBar", .m_family = "Amazon Ember", .m_pixelSize = 12 }; // MenuBar.qss:13-14
        //! ProgressBar.qss:16, so that Qt sizes the bar for no text (its minimum size hint follows the font)
        const TextElement ProgressBar{ .m_name = "Global.Text.ProgressBar", .m_pixelSize = 1 };

        // ---- Text.qss tiers. TOKENIZE: HeadlineTextSize, TitleTextSize, SubtitleTextSize, LightTextWeight, ButtonTextWeight
        const TextElement Headline{ .m_name = "Global.Text.Headline", .m_pixelSize = 24, .m_weight = 250 };   // Text.qss:58-59
        const TextElement Title{ .m_name = "Global.Text.Title", .m_pixelSize = 18 };                          // Text.qss:67
        const TextElement Subtitle{ .m_name = "Global.Text.Subtitle", .m_pixelSize = 16, .m_weight = 250 };   // Text.qss:75-76
        const TextElement Button{ .m_name = "Global.Text.Button", .m_pixelSize = 12, .m_weight = 570 };       // Text.qss:123-124

        // ---- named labels of AzQtComponents widgets
        const TextElement TitleBarTitle{ .m_name = "Global.Text.TitleBarTitle", .m_pixelSize = 14 };          // TitleBar.qss:31-34
        //! Card.qss:202-205 (bold) over the QLabel's 12px (Text.qss:92). TOKENIZE: CardTitleFontWeight
        const TextElement CardTitle{ .m_name = "Global.Text.CardTitle", .m_pixelSize = 12, .m_weight = 700 };

        // ------------------------------------------------------------------------------
        // Class fonts. Qt looks one up for a widget by its class, then each superclass up
        // to QWidget (qapplication.cpp:1307-1316), and applies it right after the style's
        // polish (qwidget.cpp:9107-9111).
        // ------------------------------------------------------------------------------

        struct ClassFont
        {
            const char* m_className;
            const TextElement* m_element;
        };

        //! Installed whatever the platform supplies: a rule reaches every widget of these classes.
        const ClassFont OwnClassFonts[] = {
            { "QWidget", &Base },               // every class without a class font of its own
            { "QMenuBar", &MenuBar },
            { "QProgressBar", &ProgressBar },
        };

        //! Qt's class-font keys that name widget classes (qapplication.cpp:556-608), overlaid where the platform
        //! supplies a font (Windows: QMenu, QMessageBox, QStatusBar, QAbstractItemView and QTipLabel, at 9pt).
        //! The keys Qt code reads for itself are left alone, as no rule reaches them: QMdiSubWindowTitleBar
        //! (QMessageBox sizes its title with it, StyleManager.cpp:340-345), QDockWidgetTitle (qdockwidget.cpp:620),
        //! QComboMenuItem (qcombobox.cpp:317), QMenuItem, QTitleBar, QListBox and QComboLineEdit.
        const ClassFont PlatformClassFonts[] = {
            { "QMenu", &Any },
            { "QMessageBox", &Any },
            { "QStatusBar", &Any },
            { "QToolButton", &Any },
            { "QAbstractItemView", &Any },
            { "QListView", &Any },
            { "QHeaderView", &Any },
            { "QSmallFont", &Any },             // macOS small and mini controls (qapplication.cpp:1299-1306)
            { "QMiniFont", &Any },
            { "QLabel", &Body },
            { "QTipLabel", &Body },
            { "QPushButton", &Body },
            { "QCheckBox", &Body },
            { "QRadioButton", &Body },
        };

        //! The class fonts modules registered, installed again whenever installClassFonts runs
        QVector<ClassFont>& moduleClassFonts()
        {
            static QVector<ClassFont> fonts;
            return fonts;
        }

        //! One class font: the element over the platform's font for the class, which keeps what the element
        //! leaves unset. A class the platform supplies no font for gets one of its own if 'always': never a copy
        //! of the application font, as Qt tells a class font from it by identity (qwidget.cpp:4758-4766).
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

        // ------------------------------------------------------------------------------
        // Widget elements
        // ------------------------------------------------------------------------------

        //! Code set font attributes on the widget or on an ancestor (QWidget::setFont, a .ui font): Qt keeps
        //! both in the mask of the widget's font (qwidget.cpp:4841-4846). Class fonts add none (qwidget.cpp:4779).
        bool hasCodeFont(const QWidget* widget)
        {
            return widget->font().resolveMask() != 0;
        }

        //! The classes a rule gives a size (see Body; a QTipLabel is a QLabel). Not QAbstractSpinBox: no rule
        //! reaches QDateTimeEdit.
        const char* const SizeRuleClasses[] = { "QLabel", "QPushButton", "QCheckBox", "QRadioButton", "QComboBox",
            "QTextEdit", "QPlainTextEdit", "QLineEdit", "QSpinBox", "QDoubleSpinBox" };

        //! Native renders the rules over a font code set, attribute by attribute (qstylesheetstyle.cpp:6455-6457):
        //! the element of the widget's class rules, over its font.
        const TextElement* classElement(const QWidget* widget)
        {
            if (widget->inherits("QMenuBar"))
            {
                return &MenuBar;
            }
            if (widget->inherits("QProgressBar"))
            {
                return &ProgressBar;
            }
            for (const char* className : SizeRuleClasses)
            {
                if (widget->inherits(className))
                {
                    return &Body;
                }
            }
            return &Any;
        }

        //! Text.qss tiers in declaration order, .Tier being [class~=Tier] (qcssparser.cpp:2780-2786). Of several tags
        //! the later declaration is taken whole; Native merges them attribute by attribute (no m0 widget carries
        //! two). .Menu, .Label, .Paragraph and .Tooltip are 12px (Text.qss:84, 92, 100, 115).
        struct Tier
        {
            const char* m_tag;
            const TextElement* m_element;
        };
        const Tier Tiers[] = {
            { "Headline", &Headline },
            { "Title", &Title },
            { "Subtitle", &Subtitle },
            { "Menu", &Body },
            { "Label", &Body },
            { "Paragraph", &Body },
            { "Tooltip", &Body },
            { "Button", &Button },
        };

        const TextElement* tierElement(const QWidget* widget)
        {
            const QVariant tags = widget->property("class");
            if (!tags.isValid())
            {
                return nullptr;
            }
            // Whole words, compared with case (qcssparser.cpp:2168-2180)
            const QStringList words = tags.toString().split(QLatin1Char(' '), Qt::SkipEmptyParts);
            const TextElement* element = nullptr;
            for (const Tier& tier : Tiers)
            {
                if (words.contains(QLatin1String(tier.m_tag), Qt::CaseSensitive))
                {
                    element = tier.m_element;
                }
            }
            return element;
        }

        //! The Global element a widget's place or tags give it, or null; the most specific declaration first.
        //! fromBase: the widget's font is the untouched base.
        const TextElement* globalElement(const QWidget* widget, bool fromBase)
        {
            if (widget->inherits("QLabel"))
            {
                // AzQtComponents--TitleBar #title (TitleBar.qss:31-34)
                if (TextElement::named(widget, "title") && TextElement::withinClass(widget, "AzQtComponents::TitleBar"))
                {
                    return &TitleBarTitle;
                }
                // AzQtComponents--CardHeader #Title (Card.qss:202-205); not bold on a palette card
                // (AzQtComponents--PaletteCardBase AzQtComponents--CardHeader #Title, ColorPicker.qss:56-58)
                if (TextElement::named(widget, "Title") && TextElement::withinClass(widget, "AzQtComponents::CardHeader"))
                {
                    return TextElement::withinClass(widget, "AzQtComponents::PaletteCardBase") ? &Body : &CardTitle;
                }
                if (const TextElement* element = tierElement(widget))
                {
                    return element;
                }
            }
            // AzQtComponents--ColorPicker * (ColorPicker.qss:16-18): every widget inside a color picker, the menu
            // and message boxes it opens too (those UNVERIFIED: no capture shows them). A widget that starts from
            // the base has the 12px already, so only the others look for the ancestor.
            if (!fromBase && TextElement::withinClass(widget, "AzQtComponents::ColorPicker"))
            {
                return &Body;
            }
            return nullptr;
        }

        struct Row
        {
            const TextElement* m_element;
            const QMetaObject* m_widgetClass;
            TextElement::Matcher m_matcher;
        };

        //! The widget elements modules registered, checked before the Global ones
        QVector<Row>& moduleRows()
        {
            static QVector<Row> rows;
            return rows;
        }

        //! Modules first, then the Global elements, then the class rules over a font code set. A widget nothing
        //! matches keeps the font Qt gives it: its class font, or the base.
        const TextElement* resolve(const QWidget* widget, const QFont& classFont)
        {
            for (const Row& row : moduleRows())
            {
                if (widget->metaObject()->inherits(row.m_widgetClass) && row.m_matcher(widget))
                {
                    return row.m_element;
                }
            }
            const bool codeFont = hasCodeFont(widget);
            const bool fromBase = !codeFont && classFont.isCopyOf(QApplication::font("QWidget"));
            if (const TextElement* element = globalElement(widget, fromBase))
            {
                return element;
            }
            return codeFont ? classElement(widget) : nullptr;
        }

        // ------------------------------------------------------------------------------
        // Colour
        // ------------------------------------------------------------------------------

        //! The name of the dynamic property that keeps the element whose colour the polish applies last. Only a widget
        //! whose element has a colour carries it.
        constexpr const char* ColorElementProperty = "_o3de_textColorElement";

        bool hasColor(const TextElement* element)
        {
            return element && (element->m_color.isValid() || element->m_disabledColor.isValid());
        }

        void keepColorElement(QWidget* widget, const TextElement* element)
        {
            if (hasColor(element))
            {
                widget->setProperty(ColorElementProperty, QVariant::fromValue(reinterpret_cast<quintptr>(element)));
            }
            else if (widget->property(ColorElementProperty).isValid())
            {
                widget->setProperty(ColorElementProperty, QVariant());
            }
        }

        //! The widget inside that the sheet gave the same palette (QStyleSheetStyle's embeddedWidget,
        //! qstylesheetstyle.cpp:2440-2462): the line edit of an editable combo box or of a spin box, the viewport
        //! of a scroll area
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

        //! A `color` in one colour group, as QRenderRule::configurePalette set it (qstylesheetstyle.cpp:1454-1464): the
        //! text roles, the foreground role of the embedded widget, and the placeholder at half the alpha. Whatever the
        //! palette had there before (setDefault, qstylesheetstyle.cpp:1424-1432).
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
        for (const TextElementInternal::ClassFont& classFont : TextElementInternal::OwnClassFonts)
        {
            TextElementInternal::installClassFont(classFont.m_className, *classFont.m_element, true);
        }
        for (const TextElementInternal::ClassFont& classFont : TextElementInternal::PlatformClassFonts)
        {
            TextElementInternal::installClassFont(classFont.m_className, *classFont.m_element, false);
        }
        for (const TextElementInternal::ClassFont& classFont : TextElementInternal::moduleClassFonts())
        {
            TextElementInternal::installClassFont(classFont.m_className, *classFont.m_element, true);
        }
    }

    void TextElement::registerClassFont(const TextElement* element, const char* className)
    {
        TextElementInternal::moduleClassFonts().append({ className, element });
        if (StyleManager::stylesheetsDisabled())
        {
            TextElementInternal::installClassFont(className, *element, true);
        }
    }

    // ----------------------------------------------------------------------------------
    // Widget elements
    // ----------------------------------------------------------------------------------

    void TextElement::registerElement(const TextElement* element, const QMetaObject* widgetClass, Matcher matcher)
    {
        TextElementInternal::moduleRows().append({ element, widgetClass, matcher });
        if (!StyleManager::stylesheetsDisabled())
        {
            return;
        }

        // A module builds some windows before it sets up its styles: resolve the widgets polished before this call
        const QWidgetList widgets = QApplication::allWidgets();
        for (QWidget* widget : widgets)
        {
            if (widget->testAttribute(Qt::WA_WState_Polished) && widget->metaObject()->inherits(widgetClass))
            {
                apply(widget);
                applyColor(widget);
            }
        }
    }

    void TextElement::apply(QWidget* widget)
    {
        // The class font Qt applies right after this polish: the base for most widgets
        const QFont classFont = QApplication::font(widget);

        // Tripwire: while the flattened GUI runs, every widget has a class font (at least the base). A
        // QApplication::setFont(font) without a class name clears them all (qapplication.cpp:1362-1365), and
        // a widget would take its parent's whole font again.
        AZ_WarningOnce("TextElement", !classFont.isCopyOf(QApplication::font()),
            "The class fonts of the flattened GUI were cleared by QApplication::setFont: call it before QApplication::setStyle.");

        const TextElement* element = TextElementInternal::resolve(widget, classFont);
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
        const QVariant kept = widget->property(TextElementInternal::ColorElementProperty);
        if (!kept.isValid())
        {
            return;
        }
        const TextElement* element = reinterpret_cast<const TextElement*>(kept.value<quintptr>());
        QWidget* embedded = TextElementInternal::embeddedWidget(widget);
        if (!embedded)
        {
            embedded = widget;                  // a spin box without its line edit yet
        }

        // Each colour group as the sheet built it from the rules of its state, into the widget's own palette, and the
        // same palette to the widget inside (QStyleSheetStyle::setPalette, qstylesheetstyle.cpp:2680-2727): Active and
        // Inactive from the enabled rules, Disabled from :disabled
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

    // ----------------------------------------------------------------------------------
    // Selectors
    // ----------------------------------------------------------------------------------

    bool TextElement::named(const QWidget* widget, const char* objectName)
    {
        return widget->objectName() == QLatin1String(objectName);
    }

    bool TextElement::childOf(const QWidget* widget, const char* parentObjectName)
    {
        const QWidget* parent = widget->parentWidget();
        return parent && named(parent, parentObjectName);
    }

    bool TextElement::within(const QWidget* widget, const char* ancestorObjectName)
    {
        for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
        {
            if (named(ancestor, ancestorObjectName))
            {
                return true;
            }
        }
        return false;
    }

    bool TextElement::withinClass(const QWidget* widget, const char* ancestorClass)
    {
        for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
        {
            if (ancestor->inherits(ancestorClass))
            {
                return true;
            }
        }
        return false;
    }

    bool TextElement::tagged(const QWidget* widget, const char* token)
    {
        // .token is [class~=token]: a whole word of the class property, compared with case (qcssparser.cpp:2168-2180;
        // Style::hasClass compares without case)
        const QVariant tags = widget->property("class");
        return tags.isValid()
            && tags.toString().split(QLatin1Char(' '), Qt::SkipEmptyParts).contains(QLatin1String(token), Qt::CaseSensitive);
    }
} // namespace AzQtComponents
