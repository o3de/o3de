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

bool PushButton::polish(Style* style, QWidget* widget, const PushButton::Config& config)
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
        widget->setMaximumSize(config.smallIcon.width + (hasMenu ? config.smallIcon.arrowWidth : 0), config.smallIcon.frame.height);

        style->repolishOnSettingsChange(widget);

        return true;
    }
    else if (pushButton != nullptr)
    {
        widget->setMaximumHeight(config.defaultFrame.height);
        return true;
    }

    return false;
}

int PushButton::buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const PushButton::Config& config)
{
    Q_UNUSED(option);

    if (style->hasClass(widget, g_smallIconClass))
    {
        return config.smallIcon.frame.margin;
    }

    int margin = -1;
    const QPushButton* pushButton = qobject_cast<const QPushButton*>(widget);
    if (pushButton && !pushButton->isFlat())
    {
        margin = config.defaultFrame.margin;
    }

    return margin;
}

QSize PushButton::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const PushButton::Config& config)
{
    QSize sz = style->QProxyStyle::sizeFromContents(type, option, size, widget);

    const QPushButton* pushButton = qobject_cast<const QPushButton*>(widget);
    if (style->hasClass(widget, g_smallIconClass))
    {
        sz.setHeight(config.smallIcon.frame.height);
    }
    else if (pushButton && !pushButton->isFlat())
    {
        sz.setHeight(config.defaultFrame.height);
    }

    return sz;
}

int PushButton::menuButtonIndicatorWidth(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(option);

    int size = -1;
    if (qobject_cast<const QPushButton*>(widget))
    {
        size = config.dropdownButton.menuIndicatorWidth;
    }
    return size;
}

bool PushButton::drawPushButtonBevel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const PushButton::Config& config)
{
    Q_UNUSED(style);

    const auto* button = qobject_cast<const QPushButton*>(widget);
    if (!button || button->isFlat())
    {
        return false;
    }

    const auto* buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
    const bool isDisabled = !(option->state & QStyle::State_Enabled);
    Border border = isDisabled ? config.disabledBorder : config.defaultBorder;

    if (!button->isFlat())
    {
        QRectF r = option->rect.adjusted(0, 0, -1, -1);

        bool isSmallIconButton = style->hasClass(widget, g_smallIconClass);

        QColor gradientStartColor;
        QColor gradientEndColor;

        const bool isPrimary = (style->hasClass(widget, g_primaryClass));

        selectColors(option, isPrimary ? config.primary : config.secondary, isDisabled, gradientStartColor, gradientEndColor);

        if (option->state & QStyle::State_HasFocus)
        {
            border = config.focusedBorder;
        }

        float radius = isSmallIconButton ? aznumeric_cast<float>(config.smallIcon.frame.radius) : aznumeric_cast<float>(config.defaultFrame.radius);
        drawFilledFrame(painter, r, gradientStartColor, gradientEndColor, border, radius);
    }

    if (buttonOption->features & QStyleOptionButton::HasMenu)
    {
        const int mbi = style->pixelMetric(QStyle::PM_MenuButtonIndicator, buttonOption, widget);
        QRect arrowRect(0, 0, mbi, mbi);
        QRect contentRect = option->rect.adjusted(0, 0, -1, -1); // As above
        arrowRect.moveCenter(contentRect.center());
        arrowRect.moveRight(contentRect.right() - (config.dropdownButton.menuIndicatorPadding + border.thickness));

        QStyleOptionButton downArrow = *buttonOption;
        downArrow.rect = arrowRect;
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &downArrow, painter, widget);
    }

    return true;
}

bool PushButton::drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const PushButton::Config& config)
{
    if (style->hasClass(widget, g_smallIconClass))
    {
        drawSmallIconButton(style, option, painter, widget, config);
        return true;
    }

    const bool drawFrame = false;
    drawSmallIconButton(style, option, painter, widget, config, drawFrame);
    return true;
}

bool PushButton::drawIndicatorArrowDown(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
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

    const auto pixmap = style->generatedIconPixmap(mode, config.dropdownButton.indicatorArrowDown, option);
    painter->drawPixmap(option->rect, pixmap, pixmap.rect());

    return true;
}

bool PushButton::drawPushButtonFocusRect(const Style* /*style*/, const QStyleOption* /*option*/, QPainter* /*painter*/, const QWidget* widget, const PushButton::Config& /*config*/)
{
    const auto* button = qobject_cast<const QPushButton*>(widget);
    if (!button)
    {
        return false;
    }

    // no frame; handled when the bevel is drawn

    return true;
}

QPixmap PushButton::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option, const Config& config)
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
            color = config.iconButton.disabledColor;
            break;
        }
        case QIcon::Active:
        {
            color = config.iconButton.activeColor;
            break;
        }
        case QIcon::Selected:
        {
            color = config.iconButton.selectedColor;
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

static void ReadFrame(QSettings& settings, const QString& name, PushButton::Frame& frame)
{
    settings.beginGroup(name);
    ConfigHelpers::read<int>(settings, QStringLiteral("Height"), frame.height);
    ConfigHelpers::read<int>(settings, QStringLiteral("Radius"), frame.radius);
    ConfigHelpers::read<int>(settings, QStringLiteral("Margin"), frame.margin);
    settings.endGroup();
}

static void ReadGradient(QSettings& settings, const QString& name, PushButton::Gradient& gradient)
{
    settings.beginGroup(name);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("Start"), gradient.start);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("End"), gradient.end);
    settings.endGroup();
}

static void ReadButtonColorSet(QSettings& settings, const QString& name, PushButton::ColorSet& colorSet)
{
    settings.beginGroup(name);
    ReadGradient(settings, QStringLiteral("Disabled"), colorSet.disabled);
    ReadGradient(settings, QStringLiteral("Sunken"), colorSet.sunken);
    ReadGradient(settings, QStringLiteral("Hovered"), colorSet.hovered);
    ReadGradient(settings, QStringLiteral("Normal"), colorSet.normal);
    settings.endGroup();
}

static void ReadBorder(QSettings& settings, const QString& name, PushButton::Border& border)
{
    settings.beginGroup(name);
    ConfigHelpers::read<int>(settings, QStringLiteral("Thickness"), border.thickness);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("Color"), border.color);
    settings.endGroup();
}

static void ReadSmallIcon(QSettings& settings, const QString& name, PushButton::SmallIcon& smallIcon)
{
    settings.beginGroup(name);
    ReadFrame(settings, "Frame", smallIcon.frame);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("EnabledArrowColor"), smallIcon.enabledArrowColor);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("DisabledArrowColor"), smallIcon.disabledArrowColor);
    ConfigHelpers::read<int>(settings, QStringLiteral("Width"), smallIcon.width);
    ConfigHelpers::read<int>(settings, QStringLiteral("ArrowWidth"), smallIcon.arrowWidth);
    settings.endGroup();
}

static void ReadIconButton(QSettings& settings, const QString& name, PushButton::IconButton& iconButton)
{
    ConfigHelpers::GroupGuard group(&settings, name);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("ActiveColor"), iconButton.activeColor);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("DisabledColor"), iconButton.disabledColor);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("SelectedColor"), iconButton.selectedColor);
}

static void ReadDropdownButton(QSettings& settings, const QString& name, PushButton::DropdownButton& dropdownButton)
{
    ConfigHelpers::GroupGuard group(&settings, name);
    ConfigHelpers::read<QPixmap>(settings, QStringLiteral("IndicatorArrowDown"), dropdownButton.indicatorArrowDown);
    ConfigHelpers::read<int>(settings, QStringLiteral("MenuIndicatorWidth"), dropdownButton.menuIndicatorWidth);
    ConfigHelpers::read<int>(settings, QStringLiteral("MenuIndicatorPadding"), dropdownButton.menuIndicatorPadding);
}

PushButton::Config PushButton::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();

    ReadButtonColorSet(settings, QStringLiteral("PrimaryColorSet"), config.primary);
    ReadButtonColorSet(settings, QStringLiteral("SecondaryColorSet"), config.secondary);

    ReadBorder(settings, QStringLiteral("Border"), config.defaultBorder);
    ReadBorder(settings, QStringLiteral("DisabledBorder"), config.disabledBorder);
    ReadBorder(settings, QStringLiteral("FocusedBorder"), config.focusedBorder);
    ReadFrame(settings, QStringLiteral("DefaultFrame"), config.defaultFrame);
    ReadSmallIcon(settings, QStringLiteral("SmallIcon"), config.smallIcon);
    ReadIconButton(settings, QStringLiteral("IconButton"), config.iconButton);
    ReadDropdownButton(settings, QStringLiteral("DropdownButton"), config.dropdownButton);

    return config;
}

PushButton::Config PushButton::defaultConfig()
{
    Config config;

    config.primary.disabled.start = QColor("#978DAA");
    config.primary.disabled.end = QColor("#978DAA");

    config.primary.sunken.start = QColor("#4D2F7B");
    config.primary.sunken.end = QColor("#4D2F7B");

    config.primary.hovered.start = QColor("#9F7BD7");
    config.primary.hovered.end = QColor("#8B6EBA");

    config.primary.normal.start = QColor("#8156CF");
    config.primary.normal.end = QColor("#6441A4");

    config.secondary.disabled.start = QColor("#666666");
    config.secondary.disabled.end = QColor("#666666");

    config.secondary.sunken.start = QColor("#444444");
    config.secondary.sunken.end = QColor("#444444");

    config.secondary.hovered.start = QColor("#888888");
    config.secondary.hovered.end = QColor("#777777");

    config.secondary.normal.start = QColor("#777777");
    config.secondary.normal.end = QColor("#666666");

    config.defaultBorder.thickness = 1;
    config.defaultBorder.color = QColor("#000000");

    config.disabledBorder.thickness = config.defaultBorder.thickness;
    config.disabledBorder.color = QColor("#222222");

    config.focusedBorder.thickness = 2;
    config.focusedBorder.color = QColor("#C8ABFF");

    config.defaultFrame.height = 32;
    config.defaultFrame.radius = 4;
    config.defaultFrame.margin = 4;

    config.smallIcon.frame.height = 24;
    config.smallIcon.frame.radius = 2;
    config.smallIcon.frame.margin = 4;
    config.smallIcon.enabledArrowColor = QColor("#FFFFFF");
    config.smallIcon.disabledArrowColor = QColor("#BBBBBB");
    config.smallIcon.width = 24;
    config.smallIcon.arrowWidth = 10;

    config.iconButton.activeColor = QColor("#94D2FF");
    config.iconButton.disabledColor = QColor("#999999");
    config.iconButton.selectedColor = QColor("#FFFFFF");

    config.dropdownButton.indicatorArrowDown = QPixmap(QStringLiteral(":/stylesheet/img/UI20/dropdown-button-arrow.svg"));
    config.dropdownButton.menuIndicatorWidth = 16;
    config.dropdownButton.menuIndicatorPadding = 4;

    return config;
}

void PushButton::drawSmallIconButton(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const PushButton::Config& config, bool drawFrame)
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
            drawSmallIconFrame(style, option, totalArea, painter, config);
        }

        drawSmallIconLabel(style, buttonOption, bflags, buttonArea, painter, widget, config);

        drawSmallIconArrow(style, buttonOption, mflags, menuArea, painter, widget, config);
    }
}

void PushButton::drawSmallIconFrame(const Style* style, const QStyleOption* option, const QRect& frame, QPainter* painter, const PushButton::Config& config)
{
    Q_UNUSED(style);

    bool isDisabled = !(option->state & QStyle::State_Enabled);

    PushButton::Border border = isDisabled ? config.disabledBorder : config.defaultBorder;

    if (option->state & QStyle::State_HasFocus)
    {
        border = config.focusedBorder;
    }

    QColor gradientStartColor;
    QColor gradientEndColor;

    selectColors(option, config.secondary, isDisabled, gradientStartColor, gradientEndColor);

    float radius = aznumeric_cast<float>(config.smallIcon.frame.radius);
    drawFilledFrame(painter, frame, gradientStartColor, gradientEndColor, border, radius);
}

void PushButton::drawSmallIconLabel(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& buttonArea, QPainter* painter, const QWidget* widget, const PushButton::Config& config)
{
    Q_UNUSED(config);

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

void PushButton::drawSmallIconArrow(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& inputArea, QPainter* painter, const QWidget* widget, const PushButton::Config& config)
{
    Q_UNUSED(config);
    Q_UNUSED(widget);
    Q_UNUSED(style);

    // non-const version of area rect
    QRect menuArea = inputArea;

    bool isDisabled = !(state & QStyle::State_Enabled);
    QColor color = isDisabled ? config.smallIcon.disabledArrowColor : config.smallIcon.enabledArrowColor;

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

void PushButton::drawFilledFrame(QPainter* painter, const QRectF& rect, const QColor& gradientStartColor, const QColor& gradientEndColor, const PushButton::Border& border, float radius)
{
    painter->save();

    QPainterPath path;
    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen(border.color, border.thickness);
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

void PushButton::selectColors(const QStyleOption* option, const PushButton::ColorSet& colorSet, bool isDisabled, QColor& gradientStartColor, QColor& gradientEndColor)
{
    if (isDisabled)
    {
        gradientStartColor = colorSet.disabled.start;
        gradientEndColor = colorSet.disabled.end;
    }
    else if (option->state & QStyle::State_Sunken)
    {
        gradientStartColor = colorSet.sunken.start;
        gradientEndColor = colorSet.sunken.end;
    }
    else if (option->state & QStyle::State_MouseOver)
    {
        gradientStartColor = colorSet.hovered.start;
        gradientEndColor = colorSet.hovered.end;
    }
    else
    {
        gradientStartColor = colorSet.normal.start;
        gradientEndColor = colorSet.normal.end;
    }
}

} // namespace AzQtComponents
