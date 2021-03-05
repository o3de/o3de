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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "ColorUtils.h"

// Qt
#include <QColor>

//////////////////////////////////////////////////////////////////////////
QColor ColorLinearToGamma(ColorF col)
{
    float r = clamp_tpl(col.r, 0.0f, 1.0f);
    float g = clamp_tpl(col.g, 0.0f, 1.0f);
    float b = clamp_tpl(col.b, 0.0f, 1.0f);
    float a = clamp_tpl(col.a, 0.0f, 1.0f);

    r = (float)(r <= 0.0031308 ? (12.92 * r) : (1.055 * pow((double)r, 1.0 / 2.4) - 0.055));
    g = (float)(g <= 0.0031308 ? (12.92 * g) : (1.055 * pow((double)g, 1.0 / 2.4) - 0.055));
    b = (float)(b <= 0.0031308 ? (12.92 * b) : (1.055 * pow((double)b, 1.0 / 2.4) - 0.055));

    return QColor(FtoI(r * 255.0f), FtoI(g * 255.0f), FtoI(b * 255.0f), FtoI(a * 255.0f));
}

//////////////////////////////////////////////////////////////////////////
ColorF ColorGammaToLinear(const QColor& col)
{
    float r = (float)col.red() / 255.0f;
    float g = (float)col.green() / 255.0f;
    float b = (float)col.blue() / 255.0f;
    float a = (float)col.alpha() / 255.0f;

    return ColorF((float)(r <= 0.04045 ? (r / 12.92) : pow(((double)r + 0.055) / 1.055, 2.4)),
        (float)(g <= 0.04045 ? (g / 12.92) : pow(((double)g + 0.055) / 1.055, 2.4)),
        (float)(b <= 0.04045 ? (b / 12.92) : pow(((double)b + 0.055) / 1.055, 2.4)), a);
}

QColor ColorToQColor(uint32 color)
{
    return QColor::fromRgbF((float)GetRValue(color) / 255.0f,
        (float)GetGValue(color) / 255.0f,
        (float)GetBValue(color) / 255.0f);
}
