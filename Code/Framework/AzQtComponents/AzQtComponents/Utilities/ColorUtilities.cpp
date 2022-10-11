/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzQtComponents/Components/Style.h>
#include <cmath>
#include <QPainter>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/MathUtils.h>
#include <assert.h>

namespace AzQtComponents
{
    QColor AdjustGamma(const QColor& color, qreal gamma)
    {
        const QColor rgb = color.toRgb();
        const float r = aznumeric_cast<float>(std::pow(rgb.redF(), 1.0 / gamma));
        const float g = aznumeric_cast<float>(std::pow(rgb.greenF(), 1.0 / gamma));
        const float b = aznumeric_cast<float>(std::pow(rgb.blueF(), 1.0 / gamma));
        const float a = aznumeric_cast<float>(rgb.alphaF());
        return toQColor(r, g, b, a);
    }

    AZ::Color AdjustGamma(const AZ::Color& color, float gamma)
    {
        const float r = std::pow(color.GetR(), 1.0f / gamma);
        const float g = std::pow(color.GetG(), 1.0f / gamma);
        const float b = std::pow(color.GetB(), 1.0f / gamma);
        const float a = color.GetA();
        return AZ::Color(r, g, b, a);
    }

    QBrush MakeAlphaBrush(const QColor& color, qreal gamma)
    {
        QColor adjusted = gamma == 1.0f ? color : AdjustGamma(color, gamma);

        QPixmap alpha = Style::cachedPixmap(QStringLiteral(":/stylesheet/img/UI20/alpha-background.png"));
        QPainter filler(&alpha);
        filler.fillRect(alpha.rect(), adjusted);

        return QBrush(alpha);
    }

    QBrush MakeAlphaBrush(const AZ::Color& color, float gamma)
    {
        return MakeAlphaBrush(ToQColor(color), gamma);
    }

    bool AreClose(const AZ::Color& left, const AZ::Color& right)
    {
        // the two values can't be off more than a single 8 bit rounded unit
        const float tolerance = 1.0f / 255.0f;

        return left.IsClose(right, tolerance);
    }


    QString MakePropertyDisplayStringInts(const QColor& color, bool includeAlphaChannel)
    {
        int R, G, B, A;
        color.getRgb(&R, &G, &B, &A);
        auto colorStr =
            includeAlphaChannel
            ? QStringLiteral("%1,%2,%3,%4").arg(R).arg(G).arg(B).arg(A)
            : QStringLiteral("%1,%2,%3").arg(R).arg(G).arg(B)
            ;
        return colorStr;
    }

    QString MakePropertyDisplayStringFloats(const QColor& color, bool includeAlphaChannel)
    {
        static const int FieldWidth = 0;
        static const char Format = 'f';
        static const int Precision = 3;

        qreal R, G, B, A;
        color.getRgbF(&R, &G, &B, &A);
        auto colorStr =
            includeAlphaChannel
            ? QStringLiteral("%1, %2, %3, %4").arg(R, FieldWidth, Format, Precision).arg(G, FieldWidth, Format, Precision).arg(B, FieldWidth, Format, Precision).arg(A, FieldWidth, Format, Precision)
            : QStringLiteral("%1, %2, %3").arg(R, FieldWidth, Format, Precision).arg(G, FieldWidth, Format, Precision).arg(B, FieldWidth, Format, Precision)
            ;
        return colorStr;
    }

} // namespace AzQtComponents



