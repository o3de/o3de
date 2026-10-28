/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Style.h>

#include <QMargins>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QWidget>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace IndicatorElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        // CheckBox.qss:25-26, RadioButton.qss:24-25 $size.check-indicator.idle.width.Alt1 $size.check-indicator.idle.height
        constexpr QSize IndicatorImageSize(16, 16);
        // CheckBox.qss:27, :80, RadioButton.qss:26 $space.check-indicator.idle.margin.Alt1
        constexpr int IndicatorMargin = 2;
        // CheckBox.qss:77-78 $size.check-indicator.idle.width.Alt2 $size.check-indicator.idle.height
        constexpr QSize ToggleSwitchImageSize(32, 16);
        // CheckBox.qss:95-96, :114-115 $size.check-indicator.unchecked+focus.width $size.check-indicator.unchecked+focus.height
        // $size.check-indicator.checked+focus.width $size.check-indicator.checked+focus.height
        constexpr QSize ToggleSwitchImageSizeFocus(34, 18);
        // CheckBox.qss:98, :117 $space.check-indicator.unchecked+focus $space.check-indicator.checked+focus
        constexpr int ToggleSwitchMarginFocus = 1;
        // RadioButton.qss:31-32 $size.check-indicator.focus.width $size.check-indicator.focus.height
        constexpr QSize RadioButtonImageSizeFocus(18, 18);
        constexpr int RadioButtonMarginFocus = 1;           // RadioButton.qss:33 $space.check-indicator.focus

        constexpr QMargins LabelFocusMargins(3, 2, 3, 2);   // QCommonStyle::subElementRect (SE_CheckBoxFocusRect) $space.check.focus.label

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! QCheckBox::indicator: no focus size of its own
        const IndicatorElement CheckBoxIndicator{
            .m_name = "Global.Indicator.CheckBox",
            .m_imageSize = IndicatorImageSize,
            .m_margin = IndicatorMargin,
            .m_focusImageSize = IndicatorImageSize,
            .m_focusMargin = IndicatorMargin,
        };

        //! QCheckBox.ToggleSwitch::indicator
        const IndicatorElement ToggleSwitchIndicator{
            .m_name = "Global.Indicator.ToggleSwitch",
            .m_imageSize = ToggleSwitchImageSize,
            .m_margin = IndicatorMargin,
            .m_focusImageSize = ToggleSwitchImageSizeFocus,
            .m_focusMargin = ToggleSwitchMarginFocus,
        };

        //! QRadioButton::indicator
        const IndicatorElement RadioButtonIndicator{
            .m_name = "Global.Indicator.RadioButton",
            .m_imageSize = IndicatorImageSize,
            .m_margin = IndicatorMargin,
            .m_focusImageSize = RadioButtonImageSizeFocus,
            .m_focusMargin = RadioButtonMarginFocus,
        };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        ElementRegistry<IndicatorElement>& registry()
        {
            static ElementRegistry<IndicatorElement> rows("_o3de_indicatorElement");
            return rows;
        }

        //! The Global element of a check box or radio button; null for those with a box of their own: the expander (a bare caret,
        //! CheckBox.qss:121-139) and the outliner's toggles (EntityOutliner.qss:37-62)
        const IndicatorElement* globalElement(const QWidget* widget)
        {
            if (widget->inherits("QRadioButton"))
            {
                return &RadioButtonIndicator;
            }
            if (!widget->inherits("QCheckBox") || widget->inherits("AzToolsFramework::EntityOutlinerCheckBox")
                || Style::hasClass(widget, QStringLiteral("Expander")))
            {
                return nullptr;
            }
            return Style::hasClass(widget, QStringLiteral("ToggleSwitch")) ? &ToggleSwitchIndicator : &CheckBoxIndicator;
        }

        //! The element of a check box or radio button: a module's first, then the Global one
        const IndicatorElement* findElement(const QWidget* widget)
        {
            const IndicatorElement* element = registry().find(widget);
            return element ? element : globalElement(widget);
        }
    } // namespace IndicatorElementInternal

    // ----------------------------------------------------------------------------------
    // Geometry
    // ----------------------------------------------------------------------------------

    QSize IndicatorElement::boxSize() const
    {
        return m_imageSize + QSize(2 * m_margin, 2 * m_margin);
    }

    QRect IndicatorElement::imageRect(const QRect& widgetRect, bool focused, Qt::LayoutDirection direction) const
    {
        const QRect box = QStyle::alignedRect(direction, Qt::AlignLeft | Qt::AlignVCenter, boxSize(), widgetRect);
        const int margin = focused ? m_focusMargin : m_margin;
        const QSize image = focused ? m_focusImageSize : m_imageSize;
        return QRect(box.left() + margin, box.top() + margin, image.width(), image.height());
    }

    QRect IndicatorElement::contentsRect(const QRect& widgetRect, int spacing, Qt::LayoutDirection direction) const
    {
        const int offset = boxSize().width() + spacing;
        const QRect logical(widgetRect.left() + offset, widgetRect.top(), widgetRect.width() - offset, widgetRect.height());
        return QStyle::visualRect(direction, widgetRect, logical);
    }

    QSize IndicatorElement::sizeFromContents(const QSize& contents, int spacing) const
    {
        const QSize box = boxSize();
        return QSize(contents.width() + box.width() + spacing, qMax(contents.height(), box.height()));
    }

    QRect IndicatorElement::clickRect(const QStyle* style, const QStyleOptionButton& option, int spacing, const QRect& area) const
    {
        QRect click = QStyle::alignedRect(option.direction, Qt::AlignLeft | Qt::AlignVCenter, boxSize(), area);
        if (!option.text.isEmpty())
        {
            // The label's text with QCommonStyle's focus rectangle around it
            const QRect contents = QStyle::visualRect(option.direction, option.rect, contentsRect(area, spacing, option.direction));
            QRect text = style->itemTextRect(option.fontMetrics, contents,
                Qt::AlignAbsolute | Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                option.state.testFlag(QStyle::State_Enabled), option.text);
            text = text.marginsAdded(IndicatorElementInternal::LabelFocusMargins).intersected(option.rect);
            click |= QStyle::visualRect(option.direction, option.rect, text);
        }
        return click;
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
        // The spacing asked with no option, as the label's position is: a rule's spacing for one state sizes the button only
        const int spacing =
            style->pixelMetric(radio ? QStyle::PM_RadioButtonLabelSpacing : QStyle::PM_CheckBoxLabelSpacing, nullptr, widget);
        const bool focused = option->state.testFlag(QStyle::State_HasFocus) && option->state.testFlag(QStyle::State_Enabled);

        if (!element->m_hidden)
        {
            QStyleOptionButton indicator = *option;
            indicator.rect = element->imageRect(option->rect, focused, option->direction);
            style->drawPrimitive(radio ? QStyle::PE_IndicatorRadioButton : QStyle::PE_IndicatorCheckBox, &indicator, painter, widget);
        }

        // No focus rectangle after the label: `outline: 0` (CheckBox.qss:15, RadioButton.qss:14)
        QStyleOptionButton label = *option;
        label.rect = element->contentsRect(option->rect, spacing, option->direction);
        style->drawControl(radio ? QStyle::CE_RadioButtonLabel : QStyle::CE_CheckBoxLabel, &label, painter, widget);
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void IndicatorElement::registerElement(
        const IndicatorElement* element, const QMetaObject* widgetClass, ElementMatcher matcher, ElementScope scope)
    {
        IndicatorElementInternal::registry().add(element, widgetClass, matcher, scope);
        resolvePolished(widgetClass, [](QWidget* widget)
            {
                resolve(widget);
                widget->updateGeometry();
                widget->update();
            });
    }

    void IndicatorElement::resolve(QWidget* widget)
    {
        using namespace IndicatorElementInternal;
        if (widget)
        {
            // A null element is kept too: sizes and draws read it back without resolving again
            registry().keep(widget, findElement(widget));
        }
    }

    const IndicatorElement* IndicatorElement::forWidget(const QWidget* widget)
    {
        using namespace IndicatorElementInternal;
        if (!widget)
        {
            return nullptr;
        }
        const std::optional<const IndicatorElement*> kept = registry().kept(widget);
        return kept ? *kept : findElement(widget);
    }
} // namespace AzQtComponents
