/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
 // Modifications copyright Amazon.com, Inc. or its affiliates.

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYDRAWCONTEXT_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYDRAWCONTEXT_H
#pragma once


#include <map>
#include <vector>
#include <QRect>
#include "Color.h"
#include "EditorCommonAPI.h"

class QPainter;
class QImage;
class QBrush;
class QRect;
class QIcon;
class QColor;
class QFont;
struct RGBAImage;
namespace Serialization { struct IconXPM; }
struct Color;

struct IconXPMCache
{
    void initialize();
    void finalize();
    void flush();

    ~IconXPMCache();

    QImage* getImageForIcon(const Serialization::IconXPM& icon);
private:
    struct BitmapCache {
        std::vector<Color> pixels;
        QImage* bitmap;
    };

    static bool parseXPM(RGBAImage* out, const Serialization::IconXPM& xpm);
    typedef std::map<const char* const*, BitmapCache> IconToBitmap;
    IconToBitmap iconToImageMap_;
};


void fillRoundRectangle(QPainter& p, const QBrush& brush, const QRect& r, const QColor& borderColor, int radius);
void drawRoundRectangle(QPainter& p, const QRect &_r, unsigned int color, int radius, int width);


enum CheckState {
    CHECK_SET,
    CHECK_NOT_SET,
    CHECK_IN_BETWEEN
};

enum {
    BUTTON_POPUP_ARROW = 1 << 0,
    BUTTON_DISABLED = 1 << 1,
    BUTTON_FOCUSED = 1 << 2,
    BUTTON_PRESSED = 1 << 3,
    BUTTON_CENTER = 1 << 4
};

class QPropertyTree;
struct EDITOR_COMMON_API PropertyDrawContext {
    const QPropertyTree* tree;
    QPainter* painter;
    QRect widgetRect;
    QRect lineRect;
    bool captured;
    bool m_pressed;

    void drawIcon(const QRect& rect, const Serialization::IconXPM& icon) const;
    void drawCheck(const QRect& rect, bool disabled, CheckState checked) const;
    void drawButton(const QRect& rect, const wchar_t* text, int buttonFlags, const QFont* font, const Color* optionalColorOverride = 0) const;
    void drawButtonWithIcon(const QIcon& icon, const QRect& rect, const wchar_t* text, bool selected, bool pressed, bool focused, bool enabled, bool showButtonFrame, const QFont* font) const;
    void drawValueText(bool highlighted, const wchar_t* text) const;
    void drawEntry(const wchar_t* text, bool pathEllipsis, bool grayBackground, int trailingOffset) const;

    PropertyDrawContext()
    : tree(0)
    , painter(0)
    , captured(false)
    , m_pressed(false)
    {
    }
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYDRAWCONTEXT_H
