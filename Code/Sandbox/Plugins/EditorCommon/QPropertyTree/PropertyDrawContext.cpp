/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 *
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

// Modifications copyright Amazon.com, Inc. or its affiliates.

#include "EditorCommon_precompiled.h"
#include "PropertyDrawContext.h"
#include <memory>
#include "QPropertyTree.h"
#include "Serialization/Decorators/IconXPM.h"
#include "Unicode.h"
#include <QApplication>
#include <QStyleOption>
#include <QPainter>
#include <QBitmap>

// required to create context for the draw calls
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>

#include <AzQtComponents/Components/StyleManager.h>

#ifndef _MSC_VER
# define _stricmp strcasecmp
#endif

// ---------------------------------------------------------------------------

QColor interpolateColor(const QColor& a, const QColor& b, float k);

IconXPMCache::~IconXPMCache()
{
    flush();
}


void IconXPMCache::flush()
{
    IconToBitmap::iterator it;
    for (it = iconToImageMap_.begin(); it != iconToImageMap_.end(); ++it)
        delete it->second.bitmap;
    iconToImageMap_.clear();
}

struct RGBAImage
{
    int width_;
    int height_;
    std::vector<Color> pixels_;

    RGBAImage() : width_(0), height_(0) {}
};

bool IconXPMCache::parseXPM(RGBAImage* out, const Serialization::IconXPM& icon)
{
    if (icon.lineCount < 3) {
        return false;
    }

    // parse values
    std::vector<Color> pixels;
    int width = 0;
    int height = 0;
    int charsPerPixel = 0;
    int colorCount = 0;
    int hotSpotX = -1;
    int hotSpotY = -1;

    int scanResult = azsscanf(icon.source[0], "%d %d %d %d %d %d", &width, &height, &colorCount, &charsPerPixel, &hotSpotX, &hotSpotY);
    if (scanResult != 4 && scanResult != 6)
        return false;

    if (charsPerPixel > 4)
        return false;

    if (icon.lineCount != 1 + colorCount + height) {
        YASLI_ASSERT(0 && "Wrong line count");
        return false;
    }

    // parse colors
    std::vector<std::pair<int, Color> > colors;
    colors.resize(colorCount);

    for (int colorIndex = 0; colorIndex < colorCount; ++colorIndex) {
        const char* p = icon.source[colorIndex + 1];
        int code = 0;
        for (int charIndex = 0; charIndex < charsPerPixel; ++charIndex) {
            if (*p == '\0')
                return false;
            code = (code << 8) | *p;
            ++p;
        }
        colors[colorIndex].first = code;

        while (*p == '\t' || *p == ' ')
            ++p;

        if (*p == '\0')
            return false;

        if (*p != 'c' && *p != 'g')
            return false;
        ++p;

        while (*p == '\t' || *p == ' ')
            ++p;

        if (*p == '\0')
            return false;

        if (*p == '#') {
            ++p;
            if (strlen(p) == 6) {
                int colorCode;
                if (azsscanf(p, "%x", &colorCode) != 1)
                    return false;
                Color color((colorCode & 0xff0000) >> 16,
                    (colorCode & 0xff00) >> 8,
                    (colorCode & 0xff),
                    255);
                colors[colorIndex].second = color;
            }
        }
        else {
            if (_stricmp(p, "None") == 0)
                colors[colorIndex].second = Color(0, 0, 0, 0);
            else if (_stricmp(p, "Black") == 0)
                colors[colorIndex].second = Color(0, 0, 0, 255);
            else {
                // unknown color
                colors[colorIndex].second = Color(255, 0, 0, 255);
            }
        }
    }

    // parse pixels
    pixels.resize(width * height);
    int pi = 0;
    for (int y = 0; y < height; ++y) {
        const char* p = icon.source[1 + colorCount + y];
        if (strlen(p) != width * charsPerPixel)
            return false;

        for (int x = 0; x < width; ++x) {
            int code = 0;
            for (int i = 0; i < charsPerPixel; ++i) {
                code = (code << 8) | *p;
                ++p;
            }

            for (size_t i = 0; i < size_t(colorCount); ++i)
                if (colors[i].first == code)
                    pixels[pi] = colors[i].second;
            ++pi;
        }
    }

    out->pixels_.swap(pixels);
    out->width_ = width;
    out->height_ = height;
    return true;
}


QImage* IconXPMCache::getImageForIcon(const Serialization::IconXPM& icon)
{
    IconToBitmap::iterator it = iconToImageMap_.find(icon.source);
    if (it != iconToImageMap_.end())
        return it->second.bitmap;

    RGBAImage image;
    if (!parseXPM(&image, icon))
        return 0;

    BitmapCache& cache = iconToImageMap_[icon.source];
    cache.pixels.swap(image.pixels_);
    cache.bitmap = new QImage((unsigned char*)&cache.pixels[0], image.width_, image.height_, QImage::Format_ARGB32);
    return cache.bitmap;
}

// ---------------------------------------------------------------------------

void drawRoundRectangle(QPainter& p, const QRect &_r, unsigned int color, int radius, [[maybe_unused]] int width)
{
    QRect r = _r;
    int dia = 2 * radius;

    p.setPen(QColor(color));
    p.drawRoundedRect(r, dia, dia);
}

void fillRoundRectangle(QPainter& p, const QBrush& brush, const QRect& _r, const QColor& border, int radius)
{
    bool wasAntialisingSet = p.renderHints().testFlag(QPainter::Antialiasing);
    p.setRenderHints(QPainter::Antialiasing, true);

    p.setBrush(brush);
    QPen pen(QBrush(border), 1.0, Qt::SolidLine);
    p.setPen(pen);
    QRectF adjustedRect = _r;
    adjustedRect.adjust(0.5f, 0.5f, -0.5f, -0.5f);
    p.drawRoundedRect(adjustedRect, radius, radius);

    p.setRenderHints(QPainter::Antialiasing, wasAntialisingSet);
}

// ---------------------------------------------------------------------------

void PropertyDrawContext::drawIcon(const QRect& rect, const Serialization::IconXPM& icon) const
{
    QImage* image = tree->_iconCache()->getImageForIcon(icon);
    if (!image)
        return;
    int x = rect.left() + (rect.width() - image->width()) / 2;
    int y = rect.top() + (rect.height() - image->height()) / 2;
    painter->drawImage(x, y, *image);
}

void PropertyDrawContext::drawCheck(const QRect& rect, bool disabled, CheckState checked) const
{
    QStyleOptionButton option;
    if (!disabled)
        option.state |= QStyle::State_Enabled;
    else {
        option.state |= QStyle::State_ReadOnly;
        option.palette.setCurrentColorGroup(QPalette::Disabled);
    }
    if (checked == CHECK_SET)
        option.state |= QStyle::State_On;
    else if (checked == CHECK_IN_BETWEEN)
        option.state |= QStyle::State_NoChange;
    else
        option.state |= QStyle::State_Off;

    // create a widget so that the style sheet has context for its draw calls
    QCheckBox forContext;
    QSize checkboxSize = tree->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, &forContext).size();
    option.rect = QRect(rect.left(), rect.center().y() - checkboxSize.height() / 2, checkboxSize.width(), checkboxSize.height());
    tree->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter, &forContext);
    if (disabled) {
        // With Fusion theme difference between disabled and enabled checkbox is very subtle, let's amplify it
        QColor readOnlyOverlay = tree->backgroundColor();
        readOnlyOverlay.setAlpha(128);
        painter->fillRect(option.rect, QBrush(readOnlyOverlay));
    }
}

void PropertyDrawContext::drawButton(const QRect& rect, const wchar_t* text, int buttonFlags, const QFont* font, const Color* colorOverride) const
{
    QPushButton button;
    button.ensurePolished();
    QStyleOptionButton option;
    option.initFrom(&button);
    if (buttonFlags & BUTTON_DISABLED) {
        option.state |= QStyle::State_ReadOnly;
        option.palette.setCurrentColorGroup(QPalette::Disabled);
    }
    else
        option.state |= QStyle::State_Enabled;
    if (buttonFlags & BUTTON_PRESSED) {
        option.state |= QStyle::State_On;
        option.state |= QStyle::State_Sunken;
    }
    else
        option.state |= QStyle::State_Raised;

    if (buttonFlags & BUTTON_FOCUSED)
        option.state |= QStyle::State_HasFocus;
    option.rect = rect.adjusted(0, 0, -1, -1);

    QWidget* pseudoDrawWidget = &button;

    if (colorOverride) {
        QPalette& palette = option.palette;
        palette.setCurrentColorGroup(QPalette::Normal);
        QColor tintTarget(colorOverride->r, colorOverride->g, colorOverride->b, colorOverride->a);

        QPalette::ColorRole groups[] = { QPalette::Button, QPalette::Light, QPalette::Dark, QPalette::Midlight, QPalette::Mid, QPalette::Shadow };
        for (int i = 0; i < sizeof(groups) / sizeof(groups[0]); ++i)
            palette.setColor(groups[i], interpolateColor(palette.color(groups[i]), tintTarget, 0.11f));

        tree->style()->drawControl(QStyle::CE_PushButtonBevel, &option, painter, pseudoDrawWidget);
    }
    else
    {
        // Previously, a temporary QPushButton widget was used as the drawing aid
        // for this control.  However, our stylesheets didn't seem to affect the
        // QPushButton as intended, which left some of them with incorrect background
        // colors.  It seemed to work to let the tree be the drawing aid, but we should
        // probably revisit this in the future.
        tree->style()->drawControl(QStyle::CE_PushButtonBevel, &option, painter, pseudoDrawWidget);
    }

    QRect textRect;
    if ((buttonFlags & BUTTON_DISABLED) == 0 && buttonFlags & BUTTON_POPUP_ARROW)
    {
        QStyleOption arrowOption;
        arrowOption.rect = QRect(rect.right() - 11, rect.top(), 8, rect.height());
        arrowOption.state |= QStyle::State_Enabled;

        // part of the above context change
        tree->style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrowOption, painter, tree);

        textRect = rect.adjusted(0, 0, -8, 0);
    }
    else
    {
        textRect = rect;
    }

    if (buttonFlags & BUTTON_PRESSED)
    {
        textRect = textRect.adjusted(1, 0, 1, 0);
    }
    if ((buttonFlags & BUTTON_CENTER) == 0)
    {
        textRect.adjust(4, 0, -5, 0);
    }

    QColor textColor;
    if (colorOverride && !(buttonFlags & BUTTON_DISABLED))
    {
        textColor = interpolateColor(tree->palette().color(QPalette::Normal, QPalette::ButtonText), 
                                     QColor(colorOverride->r, colorOverride->g, colorOverride->b, colorOverride->a), 0.4f);
    }
    else
    {
        textColor = tree->palette().color((buttonFlags & BUTTON_DISABLED) ? QPalette::Disabled : QPalette::Normal, QPalette::ButtonText);
    }
    tree->_drawRowValue(*painter, text, font, textRect, textColor, false, (buttonFlags & BUTTON_CENTER) != 0);
}

void PropertyDrawContext::drawButtonWithIcon(const QIcon& icon, const QRect& rect, const wchar_t* text, bool selected, bool pressed, bool focused, bool enabled, bool showButtonFrame, const QFont* font) const
{
    QStyleOptionButton option;
    if (enabled)
        option.state |= QStyle::State_Enabled;
    else
        option.state |= QStyle::State_ReadOnly;
    if (pressed) {
        option.state |= QStyle::State_On;
        option.state |= QStyle::State_Sunken;
    }
    else
        option.state |= QStyle::State_Raised;

    if (focused)
        option.state |= QStyle::State_HasFocus;
    option.rect = rect.adjusted(0, 0, -1, -1);

    // See the comment in the drawButton method above for why we don't use the
    // QPushButton as the drawing aid for this control
    if (showButtonFrame)
        tree->style()->drawControl(QStyle::CE_PushButton, &option, painter, tree);

    int iconSize = 16;
    QRect iconRect(rect.topLeft(), QPoint(rect.left() + iconSize, rect.bottom()));
    QRect textRect;
    if (enabled)
        textRect = rect.adjusted(iconSize, 0, -8, 0);
    else
        textRect = rect.adjusted(iconSize, 0, 0, 0);

    if (pressed)
    {
        textRect.adjust(5, 0, 1, 0);
        iconRect.adjust(4, 0, 4, 0);
    }
    else
    {
        textRect.adjust(4, 0, 0, 0);
        iconRect.adjust(3, 0, 3, 0);
    }
    icon.paint(painter, iconRect);

    QColor textColor = tree->palette().color(enabled ? QPalette::Active : QPalette::Disabled, selected && !showButtonFrame ? QPalette::HighlightedText : QPalette::ButtonText);
    tree->_drawRowValue(*painter, text, font, textRect, textColor, false, false);
}

void PropertyDrawContext::drawValueText(bool highlighted, const wchar_t* text) const
{
    QColor textColor = highlighted ? tree->palette().highlight().color() : tree->palette().buttonText().color();
    QRect textRect(widgetRect.left() + 3, widgetRect.top() + 2, widgetRect.width() - 6, widgetRect.height() - 4);
    tree->_drawRowValue(*painter, text, &tree->font(), textRect, textColor, false, false);
}

void PropertyDrawContext::drawEntry(const wchar_t* text, bool pathEllipsis, bool grayBackground, int trailingOffset) const
{
    QRect rt = widgetRect;
    rt.adjust(0, 0, -trailingOffset, 0);

    // the drawing context requires context so that the style sheet can be used:
    QFrame frameForContext;
    QLineEdit forContext;
#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
    QStyleOptionFrameV2 option;
    option.features = QStyleOptionFrameV2::None;
#else
    QStyleOptionFrame option;
    option.features = QStyleOptionFrame::None;
#endif
    option.state = QStyle::State_Sunken;
    option.lineWidth = tree->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, &frameForContext);
    option.midLineWidth = 0;
    if (!grayBackground)
        option.state |= QStyle::State_Enabled;
    else {
        option.palette.setCurrentColorGroup(QPalette::Disabled);
    }
    if (captured)
        option.state |= QStyle::State_HasFocus;
    option.rect = rt; // option.rect is the rectangle to be drawn on.
    QRect textRect = tree->style()->subElementRect(QStyle::SE_LineEditContents, &option, &forContext);
    if (!textRect.isValid())
    {
        textRect = rt;
        textRect.adjust(3, 1, -3, -2);
    }
    else {
        textRect.adjust(2, 1, -2, -1);
    }

    // make sure the context control is polished (ie, ready for rendering) since we need to use its color palette:
    forContext.ensurePolished();

    // some styles rely on default pens
    painter->setPen(QPen(forContext.palette().color(QPalette::Text)));
    painter->setBrush(QBrush(forContext.palette().color(QPalette::Base)));

    tree->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, painter, &forContext);
    tree->_drawRowValue(*painter, text, &tree->font(), textRect, forContext.palette().color(QPalette::Text), pathEllipsis, false);
    // end amazno changes
}


QFont* propertyTreeDefaultFont()
{
    static QFont font;
    return &font;
}

QFont* propertyTreeDefaultBoldFont()
{
    static QFont font;
    font.setBold(true);
    return &font;
}
