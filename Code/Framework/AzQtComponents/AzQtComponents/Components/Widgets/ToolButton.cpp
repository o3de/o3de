/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManagerInterface.h>
#include <AzQtComponents/Components/Widgets/ToolButton.h>

#include <QPainter>
#include <QSettings>
#include <QSize>
#include <QStyleOptionToolButton>
#include <QToolButton>
#include <QtWidgets/private/qstylehelper_p.h>

namespace AzQtComponents
{
    void ToolButton::initialize()
    {
        auto styleManagerInterface = AZ::Interface<StyleManagerInterface>::Get();
        AZ_Assert(styleManagerInterface, "ToolButton - could not get StyleManagerInterface on ToolButton initialization.");

        if (styleManagerInterface->IsStylePropertyDefined("ButtonIconSize"))
        {
            s_buttonIconSize = styleManagerInterface->GetStylePropertyAsInteger("ButtonIconSize");
        }

        if (styleManagerInterface->IsStylePropertyDefined("ButtonIconMargin"))
        {
            s_defaultButtonMargin = styleManagerInterface->GetStylePropertyAsInteger("ButtonIconMargin");
        }

        if (styleManagerInterface->IsStylePropertyDefined("ButtonIconMenuIndicatorWidth"))
        {
            s_menuIndicatorWidth = styleManagerInterface->GetStylePropertyAsInteger("ButtonIconMenuIndicatorWidth");
        }

        if (styleManagerInterface->IsStylePropertyDefined("ButtonIconCheckedStateBackgroundColor"))
        {
            s_checkedStateBackgroundColor = styleManagerInterface->GetStylePropertyAsColor("ButtonIconCheckedStateBackgroundColor");
        }

        if (styleManagerInterface->IsStylePropertyDefined("ButtonIconMenuIndicatorIcon"))
        {
            s_menuIndicatorIcon = styleManagerInterface->GetStylePropertyAsString("ButtonIconMenuIndicatorIcon");
        }

        // TODO - support QSize
        s_menuIndicatorIconSize = QSize(6, 3);
    }

    bool ToolButton::polish(Style* style, QWidget* widget)
    {
        if (qobject_cast<QToolButton*>(widget))
        {
            style->repolishOnSettingsChange(widget);
            return true;
        }

        return false;
    }

    int ToolButton::buttonIconSize(const Style* style, const QStyleOption* option, const QWidget* widget)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);

        int size = -1;
        if (qobject_cast<const QToolButton*>(widget))
        {
            size = s_buttonIconSize;
        }
        return size;
    }

    int ToolButton::buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);

        int size = -1;
        if (auto toolButton = qobject_cast<const QToolButton*>(widget))
        {
            if (Style::hasClass(toolButton, NoMargins))
            {
                return 0;
            }
            size = s_defaultButtonMargin;
        }
        return size;
    }

    int ToolButton::menuButtonIndicatorWidth(const Style* style, const QStyleOption* option, const QWidget* widget)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);

        int size = -1;
        if (qobject_cast<const QToolButton*>(widget))
        {
            size = s_menuIndicatorWidth;
        }
        return size;
    }

    QSize ToolButton::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentSize, const QWidget* widget)
    {
        auto toolButton = qobject_cast<const QToolButton*>(widget);
        auto buttonOption = qstyleoption_cast<const QStyleOptionToolButton*>(option);
        if (!toolButton || !buttonOption)
        {
            return style->QProxyStyle::sizeFromContents(type, option, contentSize, widget);
        }

        auto size = contentSize;

        const int margin = style->pixelMetric(QStyle::PM_ButtonMargin, option, widget);
        const int menuIndicatorWidth = style->pixelMetric(QStyle::PM_MenuButtonIndicator, option, widget);
        const bool hasMenu = buttonOption->features.testFlag(QStyleOptionToolButton::HasMenu);
        const bool hasPopupMenu = buttonOption->features.testFlag(QStyleOptionToolButton::MenuButtonPopup);

        if (hasMenu && !hasPopupMenu)
        {
            size.rwidth() += menuIndicatorWidth;
        }
        else
        {
            // If we have a menu indicator, don't add the right margin
            size.rwidth() += margin;
        }

        // Always add the left and vertical margins
        size.rwidth() += margin;
        size.rheight() += margin * 2;

        return size;
    }

    QRect ToolButton::subControlRect(const Style* style, const QStyleOptionComplex* option, QStyle::SubControl subControl, const QWidget* widget)
    {
        QRect rect;
        if (auto buttonOption = qstyleoption_cast<const QStyleOptionToolButton *>(option))
        {
            rect = buttonOption->rect;

            const int bm = style->pixelMetric(QStyle::PM_ButtonMargin, option, widget);
            rect.adjust(0, bm, 0, -bm);

            const int menuIndicatorWidth = style->pixelMetric(QStyle::PM_MenuButtonIndicator, option, widget);

            switch (subControl)
            {
            case QStyle::SC_ToolButton:
                if (buttonOption->features & QStyleOptionToolButton::HasMenu)
                {
                    rect.adjust(bm, 0, -(menuIndicatorWidth + bm), 0);
                }
                else
                {
                    rect.adjust(bm, 0, -bm, 0);
                }
                break;

            case QStyle::SC_ToolButtonMenu:
                rect.setLeft(rect.width() - menuIndicatorWidth);
                break;

            default:
                break;
            }
        }
        return rect;
    }

    bool ToolButton::drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget)
    {
        auto toolButton = qobject_cast<const QToolButton*>(widget);
        auto buttonOption = qstyleoption_cast<const QStyleOptionToolButton*>(option);
        if (!toolButton || !buttonOption)
        {
            return false;
        }

        QRect buttonRect = style->subControlRect(QStyle::CC_ToolButton, option, QStyle::SC_ToolButton, widget);
        QRect menuRect = style->subControlRect(QStyle::CC_ToolButton, option, QStyle::SC_ToolButtonMenu, widget);

        painter->save();

        QStyleOptionToolButton label = *buttonOption;

        if (buttonOption->state & QStyle::State_On)
        {
            painter->setPen(Qt::NoPen);
            painter->setBrush(s_checkedStateBackgroundColor);

            const QRect highlightRect = buttonOption->rect;
            painter->drawRect(highlightRect);
        }

        label.rect = buttonRect;

        style->drawControl(QStyle::CE_ToolButtonLabel, &label, painter, widget);

        if (buttonOption->features & QStyleOptionToolButton::HasMenu)
        {
            label.rect = menuRect;
            style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &label, painter, widget);
        }

        painter->restore();

        return true;
    }

    bool ToolButton::drawIndicatorArrowDown(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        auto toolButton = qobject_cast<const QToolButton*>(widget);
        if (!toolButton || !toolButton->menu())
        {
            return false;
        }

        QIcon::Mode mode = option->state & QStyle::State_Enabled
                            ? (option->state & QStyle::State_MouseOver
                                ? QIcon::Active
                                : QIcon::Normal)
                            : QIcon::Disabled;

        const QSize size = s_menuIndicatorIconSize;
        const int arrowWidth = aznumeric_cast<int>(QStyleHelper::dpiScaled(size.width(), QStyleHelper::dpi(option)));
        const int arrowHeight = aznumeric_cast<int>(QStyleHelper::dpiScaled(size.height(), QStyleHelper::dpi(option)));

        const QIcon icon = QIcon(s_menuIndicatorIcon);
        const QSize requestedSize = QSize(arrowWidth, arrowHeight);
        QPixmap originalPixmap;

        if (QWindow* window = widget->window()->windowHandle())
        {
            originalPixmap = icon.pixmap(window, requestedSize);
        }
        else
        {
            originalPixmap = icon.pixmap(requestedSize);
        }

        const auto pixmap = style->generatedIconPixmap(mode, originalPixmap, option);

        const QRect source = pixmap.rect();
        QRect target = QRect(QPoint(0,0), source.size() / pixmap.devicePixelRatio());
        target.moveCenter(option->rect.center());

        painter->drawPixmap(target, pixmap, source);

        return true;
    }

} // namespace AzQtComponents
