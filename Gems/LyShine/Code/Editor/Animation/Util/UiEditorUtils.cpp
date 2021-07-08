/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiCanvasEditor_precompiled.h"
//#include "CustomizeKeyboardPage.h"

#include <Editor/Resource.h>

#include <QColor>

//////////////////////////////////////////////////////////////////////////
void HeapCheck::Check([[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
}

#include <QCursor>
#include <QPixmap>

QCursor CMFCUtils::LoadCursor(UINT nIDResource, int hotX, int hotY)
{
    QString path;
    switch (nIDResource)
    {
    case IDC_ARRBLCK:
        path = QStringLiteral("cur00003.cur");
        break;
    case IDC_ARRBLCKCROSS:
        path = QStringLiteral("cur00004.cur");
        break;
    case IDC_ARRWHITE:
        path = QStringLiteral("cur00005.cur");
        break;
    case IDC_ARROW_ADDKEY:
        path = QStringLiteral("arr_addkey.cur");
        break;
    case IDC_LEFTRIGHT:
        path = QStringLiteral("leftright.cur");
        break;
    case IDC_POINTER_OBJHIT:
        path = QStringLiteral("pointerHit.cur");
        break;
    case IDC_POINTER_OBJECT_ROTATE:
        path = QStringLiteral("object_rotate.cur");
        break;
    default:
        AZ_Assert(0, "Calling LoadCursor with an unknown cursor type");
        return QCursor();
    }
    path = QStringLiteral(":/cursors/res/") + path;
    QPixmap pm(path);
    if (!pm.isNull() && (hotX < 0 || hotX < 0))
    {
        QFile f(path);
        f.open(QFile::ReadOnly);
        QDataStream stream(&f);
        stream.setByteOrder(QDataStream::LittleEndian);
        f.read(10);
        quint16 x;
        stream >> x;
        hotX = x;
        stream >> x;
        hotY = x;
    }
    return QCursor(pm, hotX, hotY);
}

//////////////////////////////////////////////////////////////////////////-
QString TrimTrailingZeros(QString str)
{
    if (str.contains('.'))
    {
        for (int p = str.size() - 1; p >= 0; --p)
        {
            if (str.at(p) == '.')
            {
                return str.left(p);
            }
            else if (str.at(p) != '0')
            {
                return str.left(p + 1);
            }
        }
        return QString("0");
    }
    return str;
}

//////////////////////////////////////////////////////////////////////////
// This function is supposed to format float in user-friendly way,
// omitting the exponent notation.
//
// Why not using printf? Its formatting rules has following drawbacks:
//  %g   - will use exponent for small numbers;
//  %.Nf - doesn't allow to control total amount of significant numbers,
//         which exposes limited precision during binary-to-decimal fraction
//         conversion.
//////////////////////////////////////////////////////////////////////////
void FormatFloatForUI(QString& str, int significantDigits, double value)
{
    str = TrimTrailingZeros(QString::number(value, 'f', significantDigits));
    return;
}
//////////////////////////////////////////////////////////////////////////-

//////////////////////////////////////////////////////////////////////////
QColor ColorLinearToGamma(ColorF col)
{
    float r = clamp_tpl(col.r, 0.0f, 1.0f);
    float g = clamp_tpl(col.g, 0.0f, 1.0f);
    float b = clamp_tpl(col.b, 0.0f, 1.0f);

    r = (float)(r <= 0.0031308 ? (12.92 * r) : (1.055 * pow((double)r, 1.0 / 2.4) - 0.055));
    g = (float)(g <= 0.0031308 ? (12.92 * g) : (1.055 * pow((double)g, 1.0 / 2.4) - 0.055));
    b = (float)(b <= 0.0031308 ? (12.92 * b) : (1.055 * pow((double)b, 1.0 / 2.4) - 0.055));

    return QColor(FtoI(r * 255.0f), FtoI(g * 255.0f), FtoI(b * 255.0f));
}

//////////////////////////////////////////////////////////////////////////
ColorF ColorGammaToLinear(const QColor& col)
{
    float r = (float)col.red() / 255.0f;
    float g = (float)col.green() / 255.0f;
    float b = (float)col.blue() / 255.0f;

    return ColorF((float)(r <= 0.04045 ? (r / 12.92) : pow(((double)r + 0.055) / 1.055, 2.4)),
        (float)(g <= 0.04045 ? (g / 12.92) : pow(((double)g + 0.055) / 1.055, 2.4)),
        (float)(b <= 0.04045 ? (b / 12.92) : pow(((double)b + 0.055) / 1.055, 2.4)));
}
