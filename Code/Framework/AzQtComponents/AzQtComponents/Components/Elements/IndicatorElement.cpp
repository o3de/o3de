/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QVariant>
#include <QVector>
#include <QWidget>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file
    //! with others into one translation unit, where anonymous namespaces merge (CheckBox and RadioButton are
    //! classes of this library).
    namespace IndicatorElementInternal
    {
        // ------------------------------------------------------------------------------
        // Global elements. Each value cites the declaration that grants it, as file:line of the
        // legacy stylesheets in Components/Widgets/.
        // ------------------------------------------------------------------------------

        //! QCheckBox::indicator: 16x16 with a 2px margin, 20x20 with it. A focus rule changes only its image.
        //! The focus values are UNVERIFIED: no capture holds a focused check box.
        //! TOKENIZE: CheckBoxImageSize, CheckBoxIndicatorMargin
        const IndicatorElement CheckBoxIndicator{
            .m_name = "Global.CheckBox.Box",
            .m_imageSize = QSize(16, 16),                   // CheckBox.qss:25-26
            .m_margin = 2,                                  // CheckBox.qss:27
            .m_focusImageSize = QSize(16, 16),              // no :focus size in CheckBox.qss: the same box
            .m_focusMargin = 2,
        };

        //! QCheckBox.ToggleSwitch::indicator: 32x16 with a 2px margin, 36x20; focused 34x18 with a 1px margin,
        //! 36x20 as well. The focus values are UNVERIFIED: no capture holds a focused toggle switch.
        //! TOKENIZE: ToggleSwitchImageSize, ToggleSwitchFocusImageSize
        const IndicatorElement ToggleSwitchIndicator{
            .m_name = "Global.CheckBox.ToggleSwitch",
            .m_imageSize = QSize(32, 16),                   // CheckBox.qss:77-78
            .m_margin = 2,                                  // CheckBox.qss:80
            .m_focusImageSize = QSize(34, 18),              // CheckBox.qss:95-96 (checked: :114-115)
            .m_focusMargin = 1,                             // CheckBox.qss:98 (checked: :117)
        };

        //! QRadioButton::indicator: 16x16 with a 2px margin, 20x20; focused 18x18 with a 1px margin, 20x20 as well.
        //! The focus values are UNVERIFIED: no capture holds a focused radio button.
        //! TOKENIZE: RadioButtonImageSize, RadioButtonFocusImageSize
        const IndicatorElement RadioButtonIndicator{
            .m_name = "Global.RadioButton.Box",
            .m_imageSize = QSize(16, 16),                   // RadioButton.qss:24-25
            .m_margin = 2,                                  // RadioButton.qss:26
            .m_focusImageSize = QSize(18, 18),              // RadioButton.qss:31-32
            .m_focusMargin = 1,                             // RadioButton.qss:33
        };

        constexpr const char* ElementProperty = "_o3de_indicatorElement";

        //! The element of a check box or radio button; null for those that have a box of their own
        const IndicatorElement* globalElement(const QWidget* widget)
        {
            if (widget->inherits("QRadioButton"))
            {
                return &RadioButtonIndicator;
            }
            // The expander is a bare 16px caret (CheckBox.qss:121-139); the outliner's visibility and lock toggles
            // are another sheet's (EntityOutliner.qss:37-62)
            if (!widget->inherits("QCheckBox") || widget->inherits("AzToolsFramework::EntityOutlinerCheckBox")
                || Style::hasClass(widget, QStringLiteral("Expander")))
            {
                return nullptr;
            }
            return Style::hasClass(widget, QStringLiteral("ToggleSwitch")) ? &ToggleSwitchIndicator : &CheckBoxIndicator;
        }

        struct Row
        {
            const IndicatorElement* m_element;
            const QMetaObject* m_widgetClass;
            IndicatorElement::Matcher m_matcher;
        };

        //! The elements modules registered, checked before the Global ones, in the order they were registered
        QVector<Row>& moduleRows()
        {
            static QVector<Row> rows;
            return rows;
        }

        //! The element of a check box or radio button: a module's, else the Global one of its class (null for the widgets that have a box of their own)
        const IndicatorElement* findElement(const QWidget* widget)
        {
            for (const Row& row : moduleRows())
            {
                if (widget->metaObject()->inherits(row.m_widgetClass) && row.m_matcher(widget))
                {
                    return row.m_element;
                }
            }
            return globalElement(widget);
        }
    } // namespace IndicatorElementInternal

    // ----------------------------------------------------------------------------------
    // Use
    // ----------------------------------------------------------------------------------

    QSize IndicatorElement::boxSize() const
    {
        return m_imageSize + QSize(2 * m_margin, 2 * m_margin);
    }

    QRect IndicatorElement::imageRect(const QRect& widgetRect, bool focused, Qt::LayoutDirection direction) const
    {
        // qstylesheetstyle.cpp:6184-6191: the box aligned left and centered vertically (defaultPosition, :2218-2224)
        const QRect box = QStyle::alignedRect(direction, Qt::AlignLeft | Qt::AlignVCenter, boxSize(), widgetRect);
        const int margin = focused ? m_focusMargin : m_margin;
        const QSize image = focused ? m_focusImageSize : m_imageSize;
        return QRect(box.left() + margin, box.top() + margin, image.width(), image.height());
    }

    QRect IndicatorElement::contentsRect(const QRect& widgetRect, int spacing, Qt::LayoutDirection direction) const
    {
        // qstylesheetstyle.cpp:6193-6207: after the box and the label spacing
        const int offset = boxSize().width() + spacing;
        const QRect logical(widgetRect.left() + offset, widgetRect.top(), widgetRect.width() - offset, widgetRect.height());
        return QStyle::visualRect(direction, widgetRect, logical);
    }

    QSize IndicatorElement::sizeFromContents(const QSize& contents, int spacing) const
    {
        // qstylesheetstyle.cpp:5397-5411
        const QSize box = boxSize();
        return QSize(contents.width() + box.width() + spacing, qMax(contents.height(), box.height()));
    }

    QRect IndicatorElement::clickRect(const QStyle* style, const QStyleOptionButton& option, int spacing) const
    {
        return clickRect(style, option, spacing, option.rect);
    }

    QRect IndicatorElement::clickRect(const QStyle* style, const QStyleOptionButton& option, int spacing, const QRect& area) const
    {
        QRect click = QStyle::alignedRect(option.direction, Qt::AlignLeft | Qt::AlignVCenter, boxSize(), area);
        if (!option.text.isEmpty())
        {
            // QCommonStyle's focus rectangle, laid on the contents above (qcommonstyle.cpp:2487-2515, 2660-2663)
            const QRect contents = QStyle::visualRect(option.direction, option.rect, contentsRect(area, spacing, option.direction));
            QRect text = style->itemTextRect(option.fontMetrics, contents,
                Qt::AlignAbsolute | Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                option.state.testFlag(QStyle::State_Enabled), option.text);
            text = text.adjusted(-3, -2, 3, 2).intersected(option.rect);
            click |= QStyle::visualRect(option.direction, option.rect, text);
        }
        return click;
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    const IndicatorElement* IndicatorElement::resolve(const QWidget* widget)
    {
        if (!widget)
        {
            return nullptr;
        }
        const IndicatorElement* element = IndicatorElementInternal::findElement(widget);
        // Kept on the widget: sizes and draws read it back without resolving again (a null element is kept as well)
        const_cast<QWidget*>(widget)->setProperty(IndicatorElementInternal::ElementProperty, QVariant::fromValue(reinterpret_cast<quintptr>(element)));
        return element;
    }

    const IndicatorElement* IndicatorElement::forWidget(const QWidget* widget)
    {
        if (!widget)
        {
            return nullptr;
        }
        const QVariant stored = widget->property(IndicatorElementInternal::ElementProperty);
        return stored.isValid() ? reinterpret_cast<const IndicatorElement*>(stored.value<quintptr>()) : resolve(widget);
    }

    void IndicatorElement::registerElement(const IndicatorElement* element, const QMetaObject* widgetClass, Matcher matcher)
    {
        IndicatorElementInternal::moduleRows().append({ element, widgetClass, matcher });
        if (!StyleManager::stylesheetsDisabled())
        {
            return;
        }

        // A module builds some windows before it sets up its styles: a button that has an element kept (it was sized or drawn, or polished
        // and asked) is resolved again. One that has none yet resolves with the module's rows when it is first asked.
        const QWidgetList widgets = QApplication::allWidgets();
        for (QWidget* widget : widgets)
        {
            if (widget->metaObject()->inherits(widgetClass) && widget->property(IndicatorElementInternal::ElementProperty).isValid())
            {
                resolve(widget);
                widget->updateGeometry();
                widget->update();
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // Drawing
    // ----------------------------------------------------------------------------------

    bool IndicatorElement::draw(const QStyle* style, const QStyleOptionButton* option, QPainter* painter, const QWidget* widget)
    {
        const IndicatorElement* element = forWidget(widget);
        if (!element || !option)
        {
            return false;
        }

        const bool radio = widget->inherits("QRadioButton");
        // The label sits where SE_CheckBoxContents and SE_RadioButtonContents put it, which ask the spacing with no option
        // (qstylesheetstyle.cpp:6200): a rule's spacing for a state (`:checked { spacing }`) sizes the button but does not move its label
        const int spacing = style->pixelMetric(radio ? QStyle::PM_RadioButtonLabelSpacing : QStyle::PM_CheckBoxLabelSpacing, nullptr, widget);
        // An enabled widget with the focus draws its focus image: the larger one of a radio button or toggle switch
        const bool focused = option->state.testFlag(QStyle::State_HasFocus) && option->state.testFlag(QStyle::State_Enabled);

        // A hidden indicator has its box (the label starts after it) and no image in it
        if (!element->m_hidden)
        {
            QStyleOptionButton indicator = *option;
            indicator.rect = element->imageRect(option->rect, focused, option->direction);
            style->drawPrimitive(radio ? QStyle::PE_IndicatorRadioButton : QStyle::PE_IndicatorCheckBox, &indicator, painter, widget);
        }

        // No focus rectangle after the label: CheckBox.qss:15 and RadioButton.qss:14 set `outline: 0`
        QStyleOptionButton label = *option;
        label.rect = element->contentsRect(option->rect, spacing, option->direction);
        style->drawControl(radio ? QStyle::CE_RadioButtonLabel : QStyle::CE_CheckBoxLabel, &label, painter, widget);
        return true;
    }
} // namespace AzQtComponents
