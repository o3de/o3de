/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>

#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManagerInterface.h>
#include <AzQtComponents/Components/Widgets/PushButton.h>

#include <QDialogButtonBox>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>
#include <QPushButton>
#include <QSettings>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolButton>
#include <QVariant>

#include <QtWidgets/private/qstyle_p.h>
#include <QtWidgets/private/qstylehelper_p.h>

namespace AzQtComponents
{

static QString g_primaryClass = QStringLiteral("Primary");
static QString g_smallIconClass = QStringLiteral("SmallIcon");

void PushButton::applyPrimaryStyle(QPushButton* button)
{
    button->setDefault(true);
}

void PushButton::applySmallIconStyle(QToolButton* button)
{
    Style::addClass(button, g_smallIconClass);
}

template <typename Button>
bool buttonHasMenu(Button* button)
{
    if (button != nullptr)
    {
        return button->menu();
    }

    return false;
}

void PushButton::initialize()
{
    auto styleManagerInterface = AZ::Interface<StyleManagerInterface>::Get();
    AZ_Assert(styleManagerInterface, "ToolButton - could not get StyleManagerInterface on ToolButton initialization.");

    if (styleManagerInterface->IsStylePropertyDefined("ButtonFrameHeight"))
    {
        s_buttonFrameHeight = styleManagerInterface->GetStylePropertyAsInteger("ButtonFrameHeight");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonFrameRadius"))
    {
        s_buttonFrameRadius = styleManagerInterface->GetStylePropertyAsInteger("ButtonFrameRadius");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonFrameMargin"))
    {
        s_buttonFrameMargin = styleManagerInterface->GetStylePropertyAsInteger("ButtonFrameMargin");
    }

    if (styleManagerInterface->IsStylePropertyDefined("ButtonFrameHeight"))
    {
        s_buttonFrameHeight = styleManagerInterface->GetStylePropertyAsInteger("ButtonFrameHeight");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonFrameRadius"))
    {
        s_buttonFrameRadius = styleManagerInterface->GetStylePropertyAsInteger("ButtonFrameRadius");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonFrameMargin"))
    {
        s_buttonFrameMargin = styleManagerInterface->GetStylePropertyAsInteger("ButtonFrameMargin");
    }

    if (styleManagerInterface->IsStylePropertyDefined("ButtonSmallIconArrowColor"))
    {
        s_buttonSmallIconEnabledArrowColor = styleManagerInterface->GetStylePropertyAsColor("ButtonSmallIconArrowColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonSmallIconArrowDisabledColor"))
    {
        s_buttonSmallIconDisabledArrowColor = styleManagerInterface->GetStylePropertyAsColor("ButtonSmallIconArrowDisabledColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonSmallIconWidth"))
    {
        s_buttonSmallIconWidth = styleManagerInterface->GetStylePropertyAsInteger("ButtonSmallIconWidth");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonSmallIconArrowWidth"))
    {
        s_buttonSmallIconArrowWidth = styleManagerInterface->GetStylePropertyAsInteger("ButtonSmallIconArrowWidth");
    }

    if (styleManagerInterface->IsStylePropertyDefined("ButtonPrimaryDisabledBackgroundGradientStartColor"))
    {
        s_buttonPrimaryColorDisabledStart = styleManagerInterface->GetStylePropertyAsColor("ButtonPrimaryDisabledBackgroundGradientStartColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonPrimaryDisabledBackgroundGradientEndColor"))
    {
        s_buttonPrimaryColorDisabledEnd = styleManagerInterface->GetStylePropertyAsColor("ButtonPrimaryDisabledBackgroundGradientEndColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonPrimarySunkenBackgroundGradientStartColor"))
    {
        s_buttonPrimaryColorSunkenStart = styleManagerInterface->GetStylePropertyAsColor("ButtonPrimarySunkenBackgroundGradientStartColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonPrimarySunkenBackgroundGradientEndColor"))
    {
        s_buttonPrimaryColorSunkenEnd = styleManagerInterface->GetStylePropertyAsColor("ButtonPrimarySunkenBackgroundGradientEndColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonPrimaryHoveredBackgroundGradientStartColor"))
    {
        s_buttonPrimaryColorHoveredStart = styleManagerInterface->GetStylePropertyAsColor("ButtonPrimaryHoveredBackgroundGradientStartColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonPrimaryHoveredBackgroundGradientEndColor"))
    {
        s_buttonPrimaryColorHoveredEnd = styleManagerInterface->GetStylePropertyAsColor("ButtonPrimaryHoveredBackgroundGradientEndColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonPrimaryBackgroundGradientStartColor"))
    {
        s_buttonPrimaryColorNormalStart = styleManagerInterface->GetStylePropertyAsColor("ButtonPrimaryBackgroundGradientStartColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonPrimaryBackgroundGradientEndColor"))
    {
        s_buttonPrimaryColorNormalEnd = styleManagerInterface->GetStylePropertyAsColor("ButtonPrimaryBackgroundGradientEndColor");
    }

    if (styleManagerInterface->IsStylePropertyDefined("ButtonDisabledBackgroundGradientStartColor"))
    {
        s_buttonColorDisabledStart = styleManagerInterface->GetStylePropertyAsColor("ButtonDisabledBackgroundGradientStartColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonDisabledBackgroundGradientEndColor"))
    {
        s_buttonColorDisabledEnd = styleManagerInterface->GetStylePropertyAsColor("ButtonDisabledBackgroundGradientEndColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonSunkenBackgroundGradientStartColor"))
    {
        s_buttonColorSunkenStart = styleManagerInterface->GetStylePropertyAsColor("ButtonSunkenBackgroundGradientStartColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonSunkenBackgroundGradientEndColor"))
    {
        s_buttonColorSunkenEnd = styleManagerInterface->GetStylePropertyAsColor("ButtonSunkenBackgroundGradientEndColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonHoveredBackgroundGradientStartColor"))
    {
        s_buttonColorHoveredStart = styleManagerInterface->GetStylePropertyAsColor("ButtonHoveredBackgroundGradientStartColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonHoveredBackgroundGradientEndColor"))
    {
        s_buttonColorHoveredEnd = styleManagerInterface->GetStylePropertyAsColor("ButtonHoveredBackgroundGradientEndColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonBackgroundGradientStartColor"))
    {
        s_buttonColorNormalStart = styleManagerInterface->GetStylePropertyAsColor("ButtonBackgroundGradientStartColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonBackgroundGradientEndColor"))
    {
        s_buttonColorNormalEnd = styleManagerInterface->GetStylePropertyAsColor("ButtonBackgroundGradientEndColor");
    }

    if (styleManagerInterface->IsStylePropertyDefined("ButtonBorderThickness"))
    {
        s_buttonBorderThickness = styleManagerInterface->GetStylePropertyAsInteger("ButtonBorderThickness");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonBorderColor"))
    {
        s_buttonBorderColor = styleManagerInterface->GetStylePropertyAsColor("ButtonBorderColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonBorderDisabledThickness"))
    {
        s_buttonBorderDisabledThickness = styleManagerInterface->GetStylePropertyAsInteger("ButtonBorderDisabledThickness");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonBorderDisabledColor"))
    {
        s_buttonBorderDisabledColor = styleManagerInterface->GetStylePropertyAsColor("ButtonBorderDisabledColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonBorderFocusedThickness"))
    {
        s_buttonBorderFocusedThickness = styleManagerInterface->GetStylePropertyAsInteger("ButtonBorderFocusedThickness");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonBorderFocusedColor"))
    {
        s_buttonBorderFocusedColor = styleManagerInterface->GetStylePropertyAsColor("ButtonBorderFocusedColor");
    }

    if (styleManagerInterface->IsStylePropertyDefined("ButtonIconColor"))
    {
        s_buttonIconColor = styleManagerInterface->GetStylePropertyAsColor("ButtonIconColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonIconActiveColor"))
    {
        s_buttonIconActiveColor = styleManagerInterface->GetStylePropertyAsColor("ButtonIconActiveColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonIconDisabledColor"))
    {
        s_buttonIconDisabledColor = styleManagerInterface->GetStylePropertyAsColor("ButtonIconDisabledColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonIconSelectedColor"))
    {
        s_buttonIconSelectedColor = styleManagerInterface->GetStylePropertyAsColor("ButtonIconSelectedColor");
    }

    if (styleManagerInterface->IsStylePropertyDefined("ButtonDropdownIndicatorArrowDown"))
    {
        s_buttonDropdownIndicatorArrowDown = styleManagerInterface->GetStylePropertyAsString("ButtonDropdownIndicatorArrowDown");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonDropdownMenuIndicatorWidth"))
    {
        s_buttonDropdownMenuIndicatorWidth = styleManagerInterface->GetStylePropertyAsInteger("ButtonDropdownMenuIndicatorWidth");
    }
    if (styleManagerInterface->IsStylePropertyDefined("ButtonDropdownMenuIndicatorPadding"))
    {
        s_buttonDropdownMenuIndicatorPadding = styleManagerInterface->GetStylePropertyAsInteger("ButtonDropdownMenuIndicatorPadding");
    }
}

bool PushButton::polish(Style* style, QWidget* widget)
{
    QToolButton* toolButton = qobject_cast<QToolButton*>(widget);
    QPushButton* pushButton = qobject_cast<QPushButton*>(widget);

    if (pushButton != nullptr)
    {
        // Edge cases for dialog box buttons.
        QDialogButtonBox* dialogButtonBox = qobject_cast<QDialogButtonBox*>(widget->parent());

        // Detect if this is the first button in a dialog box.
        bool isFirstInDialog = (dialogButtonBox != nullptr) &&
            (dialogButtonBox->nextInFocusChain() == pushButton);

        // Detect if other buttons in the dialog box have been marked as default.
        bool otherButtonInDialogIsDefault = false;
        if (dialogButtonBox != nullptr)
        {
            for (auto dialogButton : dialogButtonBox->buttons())
            {
                QPushButton* dialogPushButton = qobject_cast<QPushButton*>(dialogButton);
                if (dialogPushButton && dialogPushButton->isDefault())
                {
                    otherButtonInDialogIsDefault = true;
                }
            }
        }

        // For dialogs, highlight the first button if no default button has been specified.
        if (pushButton->isDefault() || (isFirstInDialog && !otherButtonInDialogIsDefault))
        {
            AzQtComponents::Style::addClass(pushButton, g_primaryClass);
        }
        else
        {
            AzQtComponents::Style::removeClass(pushButton, g_primaryClass);
        }
    }

    if ((style->hasClass(widget, g_smallIconClass) && (toolButton != nullptr)))
    {
        widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        bool hasMenu = buttonHasMenu(toolButton) || buttonHasMenu(pushButton);
        widget->setMaximumSize(s_buttonSmallIconWidth + (hasMenu ? s_buttonSmallIconArrowWidth : 0), s_buttonFrameHeight);

        style->repolishOnSettingsChange(widget);

        return true;
    }
    else if (pushButton != nullptr)
    {
        widget->setMaximumHeight(s_buttonFrameHeight);
        return true;
    }

    return false;
}

int PushButton::buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget)
{
    Q_UNUSED(option);

    if (style->hasClass(widget, g_smallIconClass))
    {
        return s_buttonFrameMargin;
    }

    int margin = -1;
    const QPushButton* pushButton = qobject_cast<const QPushButton*>(widget);
    if (pushButton && !pushButton->isFlat())
    {
        margin = s_buttonFrameMargin;
    }

    return margin;
}

QSize PushButton::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget)
{
    QSize sz = style->QProxyStyle::sizeFromContents(type, option, size, widget);

    const QPushButton* pushButton = qobject_cast<const QPushButton*>(widget);
    if (style->hasClass(widget, g_smallIconClass))
    {
        sz.setHeight(s_buttonFrameHeight);
    }
    else if (pushButton && !pushButton->isFlat())
    {
        sz.setHeight(s_buttonFrameHeight);
    }

    return sz;
}

int PushButton::menuButtonIndicatorWidth(const Style* style, const QStyleOption* option, const QWidget* widget)
{
    Q_UNUSED(style);
    Q_UNUSED(option);

    int size = -1;
    if (qobject_cast<const QPushButton*>(widget))
    {
        size = s_buttonDropdownMenuIndicatorWidth;
    }
    return size;
}

bool PushButton::drawPushButtonBevel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget)
{
    Q_UNUSED(style);

    const auto* button = qobject_cast<const QPushButton*>(widget);
    if (!button || button->isFlat())
    {
        return false;
    }

    const auto* buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
    const bool isDisabled = !(option->state & QStyle::State_Enabled);
    QColor borderColor = isDisabled ? s_buttonBorderDisabledColor : s_buttonBorderColor;
    int borderThickness = isDisabled ? s_buttonBorderDisabledThickness : s_buttonBorderThickness;

    if (!button->isFlat())
    {
        QRectF r = option->rect.adjusted(0, 0, -1, -1);

        QColor gradientStartColor;
        QColor gradientEndColor;

        const bool isPrimary = (style->hasClass(widget, g_primaryClass));
        selectColors(option, isPrimary, isDisabled, gradientStartColor, gradientEndColor);

        if (option->state & QStyle::State_HasFocus)
        {
            borderColor = s_buttonBorderFocusedColor;
            borderThickness = s_buttonBorderFocusedThickness;
        }

        float radius = s_buttonFrameRadius;
        drawFilledFrame(painter, r, gradientStartColor, gradientEndColor, borderColor, borderThickness, radius);
    }

    if (buttonOption->features & QStyleOptionButton::HasMenu)
    {
        const int mbi = style->pixelMetric(QStyle::PM_MenuButtonIndicator, buttonOption, widget);
        QRect arrowRect(0, 0, mbi, mbi);
        QRect contentRect = option->rect.adjusted(0, 0, -1, -1); // As above
        arrowRect.moveCenter(contentRect.center());
        arrowRect.moveRight(contentRect.right() - (s_buttonDropdownMenuIndicatorPadding + borderThickness));

        QStyleOptionButton downArrow = *buttonOption;
        downArrow.rect = arrowRect;
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &downArrow, painter, widget);
    }

    return true;
}

bool PushButton::drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget)
{
    if (style->hasClass(widget, g_smallIconClass))
    {
        drawSmallIconButton(style, option, painter, widget);
        return true;
    }

    const bool drawFrame = false;
    drawSmallIconButton(style, option, painter, widget, drawFrame);
    return true;
}

bool PushButton::drawIndicatorArrowDown(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget)
{
    auto pushButton = qobject_cast<const QPushButton*>(widget);
    if (!pushButton)
    {
        return false;
    }

    // If it's got the small icon class, arrow drawing is already done in drawToolButton
    if (style->hasClass(widget, g_smallIconClass))
    {
        return true;
    }

    QIcon::Mode mode = option->state & QStyle::State_Enabled
                        ? QIcon::Normal
                        : QIcon::Disabled;

    const auto pixmap = style->generatedIconPixmap(mode, s_buttonDropdownIndicatorArrowDown, option);
    painter->drawPixmap(option->rect, pixmap, pixmap.rect());

    return true;
}

bool PushButton::drawPushButtonFocusRect(const Style* /*style*/, const QStyleOption* /*option*/, QPainter* /*painter*/, const QWidget* widget)
{
    const auto* button = qobject_cast<const QPushButton*>(widget);
    if (!button)
    {
        return false;
    }

    // no frame; handled when the bevel is drawn

    return true;
}

QPixmap PushButton::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option)
{
    Q_UNUSED(option);

    if (iconMode == QIcon::Normal)
    {
        return pixmap;
    }

    QColor color;
    switch(iconMode)
    {
        case QIcon::Disabled:
        {
            color = s_buttonIconDisabledColor;
            break;
        }
        case QIcon::Active:
        {
            color = s_buttonIconActiveColor;
            break;
        }
        case QIcon::Selected:
        {
            color = s_buttonIconSelectedColor;
            break;
        }
        default:
        {
            return pixmap;
        }
    }

    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y=0; y < image.height(); ++y)
    {
        auto scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));

        for (int x=0; x < image.width(); ++x)
        {
            QRgb pixel = *scanLine;
            color.setAlpha(qAlpha(pixel));
            *scanLine = color.rgba();
            ++scanLine;
        }
    }

    return QPixmap::fromImage(image);
}

void PushButton::drawSmallIconButton(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, bool drawFrame)
{
    // Draws the icon and the arrow drop down together, as per the spec, which calls for no separating borders
    // between the icon and the arrow.
    // There aren't enough controls exposed via CSS to do this in a stylesheet.

    if (const QStyleOptionToolButton* buttonOption = qstyleoption_cast<const QStyleOptionToolButton*>(option))
    {
        QRect buttonArea = style->subControlRect(QStyle::CC_ToolButton, buttonOption, QStyle::SC_ToolButton, widget);
        QRect menuArea = style->subControlRect(QStyle::CC_ToolButton, buttonOption, QStyle::SC_ToolButtonMenu, widget);
        QRect totalArea = buttonArea;

        int menuButtonIndicator = style->pixelMetric(QStyle::PM_MenuButtonIndicator, buttonOption, widget);

        if (!(buttonOption->subControls & QStyle::SC_ToolButtonMenu) && (buttonOption->features & QStyleOptionToolButton::HasMenu))
        {
            // replicating what the Qt Fusion style does
            QRect ir = buttonOption->rect;
            menuArea = QRect(ir.right() + 5 - menuButtonIndicator, ir.y() + ir.height() - menuButtonIndicator + 4, menuButtonIndicator - 6, menuButtonIndicator - 6);
        }

        QStyle::State bflags = buttonOption->state & ~QStyle::State_Sunken;

        if (bflags & QStyle::State_AutoRaise)
        {
            if (!(bflags & QStyle::State_MouseOver) || !(bflags & QStyle::State_Enabled))
            {
                bflags &= ~QStyle::State_Raised;
            }
        }

        QStyle::State mflags = bflags;
        if (buttonOption->state & QStyle::State_Sunken)
        {
            if (buttonOption->activeSubControls & QStyle::SC_ToolButton)
            {
                bflags |= QStyle::State_Sunken;
            }

            mflags |= QStyle::State_Sunken;
        }

        // check if we need to deal with the menu at all
        if ((buttonOption->subControls & QStyle::SC_ToolButtonMenu) || (buttonOption->features & QStyleOptionToolButton::HasMenu))
        {
            totalArea = buttonArea.united(menuArea);
        }

        if (drawFrame)
        {
            drawSmallIconFrame(style, option, totalArea, painter);
        }

        drawSmallIconLabel(style, buttonOption, bflags, buttonArea, painter, widget);

        drawSmallIconArrow(style, buttonOption, mflags, menuArea, painter, widget);
    }
}

void PushButton::drawSmallIconFrame(const Style* style, const QStyleOption* option, const QRect& frame, QPainter* painter)
{
    Q_UNUSED(style);

    bool isDisabled = !(option->state & QStyle::State_Enabled);

    QColor borderColor = isDisabled ? s_buttonBorderDisabledColor : s_buttonBorderColor;
    int borderThickness = isDisabled ? s_buttonBorderDisabledThickness : s_buttonBorderThickness;

    if (option->state & QStyle::State_HasFocus)
    {
        borderColor = s_buttonBorderFocusedColor;
        borderThickness = s_buttonBorderFocusedThickness;
    }

    QColor gradientStartColor;
    QColor gradientEndColor;

    selectColors(option, false, isDisabled, gradientStartColor, gradientEndColor);

    float radius = aznumeric_cast<float>(s_buttonFrameRadius);
    drawFilledFrame(painter, frame, gradientStartColor, gradientEndColor, borderColor, borderThickness, radius);
}

void PushButton::drawSmallIconLabel(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& buttonArea, QPainter* painter, const QWidget* widget)
{
    QStyleOptionToolButton label = *buttonOption;
    label.state = state;
    int fw = style->pixelMetric(QStyle::PM_DefaultFrameWidth, buttonOption, widget);
    label.rect = buttonArea.adjusted(fw, fw, -fw, -fw);
    style->drawControl(QStyle::CE_ToolButtonLabel, &label, painter, widget);
}

static QRect defaultArrowRect(const QStyleOptionToolButton* buttonOption)
{
    static const QRect rect {0, 0, qRound(QStyleHelper::dpiScaled(14, QStyleHelper::dpi(buttonOption))), qRound(QStyleHelper::dpiScaled(8, QStyleHelper::dpi(buttonOption)))};
    return rect;
}

static QPixmap initializeDownArrowPixmap(const QColor& arrowColor, const QStyleOptionToolButton* buttonOption, const QRect& rect, Qt::ArrowType type = Qt::DownArrow)
{
    const int arrowWidth = aznumeric_cast<int>(QStyleHelper::dpiScaled(14, QStyleHelper::dpi(buttonOption)));
    const int arrowHeight = aznumeric_cast<int>(QStyleHelper::dpiScaled(8, QStyleHelper::dpi(buttonOption)));

    const int arrowMax = qMin(arrowHeight, arrowWidth);
    const int rectMax = qMin(rect.height(), rect.width());
    const int size = qMin(arrowMax, rectMax);

    QPixmap cachePixmap;
    cachePixmap = styleCachePixmap(rect.size());
    cachePixmap.fill(Qt::transparent);
    QPainter cachePainter(&cachePixmap);

    QRectF arrowRect;
    arrowRect.setWidth(size);
    arrowRect.setHeight(arrowHeight * size / arrowWidth);
    if (type == Qt::LeftArrow || type == Qt::RightArrow)
        arrowRect = arrowRect.transposed();
    arrowRect.moveTo((rect.width() - arrowRect.width()) / 2.0,
                     (rect.height() - arrowRect.height()) / 2.0);

    QPolygonF triangle;
    triangle.reserve(3);
    switch (type) {
        case Qt::DownArrow:
            triangle << arrowRect.topLeft() << arrowRect.topRight() << QPointF(arrowRect.center().x(), arrowRect.bottom());
            break;
        case Qt::RightArrow:
            triangle << arrowRect.topLeft() << arrowRect.bottomLeft() << QPointF(arrowRect.right(), arrowRect.center().y());
            break;
        case Qt::LeftArrow:
            triangle << arrowRect.topRight() << arrowRect.bottomRight() << QPointF(arrowRect.left(), arrowRect.center().y());
            break;
        default:
            triangle << arrowRect.bottomLeft() << arrowRect.bottomRight() << QPointF(arrowRect.center().x(), arrowRect.top());
            break;
    }

    cachePainter.setPen(Qt::NoPen);
    cachePainter.setBrush(arrowColor);
    cachePainter.setRenderHint(QPainter::Antialiasing);
    cachePainter.drawPolygon(triangle);

    return cachePixmap;
}

static void drawArrow(const Style* style, const QStyleOptionToolButton* buttonOption, QPainter* painter, const QRect& rect, const QColor& arrowColor)
{
    Q_UNUSED(style);

    QString downArrowCacheKey = QStringLiteral("AzQtComponents::PushButton::DownArrow%1").arg(arrowColor.rgba());
    QPixmap arrowPixmap;
    
    if (!QPixmapCache::find(downArrowCacheKey, &arrowPixmap))
    {
        arrowPixmap = initializeDownArrowPixmap(arrowColor, buttonOption, defaultArrowRect(buttonOption));
        QPixmapCache::insert(downArrowCacheKey, arrowPixmap);
    }

    QRect arrowRect;
    int imageMax = qMin(arrowPixmap.height(), arrowPixmap.width());
    int rectMax = qMin(rect.height(), rect.width());
    int size = qMin(imageMax, rectMax);

    arrowRect.setWidth(size);
    arrowRect.setHeight(size);
    if (arrowPixmap.width() > arrowPixmap.height())
    {
        arrowRect.setHeight(arrowPixmap.height() * size / arrowPixmap.width());
    }
    else
    {
        arrowRect.setWidth(arrowPixmap.width() * size / arrowPixmap.height());
    }

    arrowRect.moveTopLeft(rect.center() - arrowRect.center());

    // force the arrow to the bottom
    arrowRect.moveTop(rect.bottom() - arrowRect.height() - 1);
    arrowRect.moveLeft(rect.left());

    QRectF offsetArrowRect = arrowRect;

    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawPixmap(offsetArrowRect, arrowPixmap, QRectF(QPointF(0, 0), arrowPixmap.size()));
    painter->restore();
}

void PushButton::drawSmallIconArrow(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& inputArea, QPainter* painter, const QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(style);

    // non-const version of area rect
    QRect menuArea = inputArea;

    bool isDisabled = !(state & QStyle::State_Enabled);
    QColor color = isDisabled ? s_buttonSmallIconDisabledArrowColor : s_buttonSmallIconEnabledArrowColor;

    bool paintArrow = false;
    if (buttonOption->subControls & QStyle::SC_ToolButtonMenu)
    {
        if (state & (QStyle::State_Sunken | QStyle::State_On | QStyle::State_Raised))
        {
            paintArrow = true;
        }
    }
    else if (buttonOption->features & QStyleOptionToolButton::HasMenu)
    {
        paintArrow = true;
    }

    if (paintArrow)
    {
        drawArrow(style, buttonOption, painter, menuArea, color);
    }
}

void PushButton::drawFilledFrame(
    QPainter* painter,
    const QRectF& rect,
    const QColor& gradientStartColor,
    const QColor& gradientEndColor,
    const QColor& borderColor,
    const int borderThickness,
    float radius)
{
    painter->save();

    QPainterPath path;
    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen(borderColor, borderThickness);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    // offset the frame so that it smudges and anti-aliases a bit better
    path.addRoundedRect(rect.adjusted(0.5, 0.5, -0.5f, -0.5f), radius, radius);

    QLinearGradient background(rect.topLeft(), rect.bottomLeft());
    background.setColorAt(0, gradientStartColor);
    background.setColorAt(1, gradientEndColor);
    painter->fillPath(path, background);
    painter->drawPath(path);
    painter->restore();
}

void PushButton::selectColors(
    const QStyleOption* option, bool isPrimary, bool isDisabled, QColor& gradientStartColor, QColor& gradientEndColor)
{
    if (isPrimary)
    {
        if (isDisabled)
        {
            gradientStartColor = s_buttonPrimaryColorDisabledStart;
            gradientEndColor = s_buttonPrimaryColorDisabledEnd;
        }
        else if (option->state & QStyle::State_Sunken)
        {
            gradientStartColor = s_buttonPrimaryColorSunkenStart;
            gradientEndColor = s_buttonPrimaryColorSunkenEnd;
        }
        else if (option->state & QStyle::State_MouseOver)
        {
            gradientStartColor = s_buttonPrimaryColorHoveredStart;
            gradientEndColor = s_buttonPrimaryColorHoveredEnd;
        }
        else
        {
            gradientStartColor = s_buttonPrimaryColorNormalStart;
            gradientEndColor = s_buttonPrimaryColorNormalEnd;
        }
    }
    else
    {
        if (isDisabled)
        {
            gradientStartColor = s_buttonColorDisabledStart;
            gradientEndColor = s_buttonColorDisabledEnd;
        }
        else if (option->state & QStyle::State_Sunken)
        {
            gradientStartColor = s_buttonColorSunkenStart;
            gradientEndColor = s_buttonColorSunkenEnd;
        }
        else if (option->state & QStyle::State_MouseOver)
        {
            gradientStartColor = s_buttonColorHoveredStart;
            gradientEndColor = s_buttonColorHoveredEnd;
        }
        else
        {
            gradientStartColor = s_buttonColorNormalStart;
            gradientEndColor = s_buttonColorNormalEnd;
        }
    }
}

} // namespace AzQtComponents
