/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/Conversions.h>
#include <AzCore/Math/MathUtils.h>
#include <QLocale>

namespace AzQtComponents
{
    namespace Internal
    {
        template <typename FloatType>
        float clamp(FloatType channel)
        {
            return AZ::GetClamp(static_cast<float>(channel), 0.0f, 1.0f);
        };
    }

    using namespace Internal;

    QColor toQColor(const AZ::Color& color)
    {
        return QColor::fromRgbF(clamp(color.GetR()), clamp(color.GetG()), clamp(color.GetB()), clamp(color.GetA()));
    }

    QColor toQColor(float r, float g, float b, float a)
    {
        return QColor::fromRgbF(clamp(r), clamp(g), clamp(b), clamp(a));
    }

    AZ::Color fromQColor(const QColor& color)
    {
        const QColor rgb = color.toRgb();
        return AZ::Color(static_cast<float>(rgb.redF()), static_cast<float>(rgb.greenF()), static_cast<float>(rgb.blueF()), static_cast<float>(rgb.alphaF()));
    }

    QString toString(double value, int numDecimals, const QLocale& locale, bool showGroupSeparator)
    {
        const QChar decimalPoint = locale.decimalPoint();
        const QChar zeroDigit = locale.zeroDigit();
        const int numToStringDecimals = AZStd::max(numDecimals, 20);

        // We want to truncate, not round. toString will round, so we add extra decimal places to the formatting
        // so we can remove the last values
        QString retValue = locale.toString(value, 'f', (numDecimals > 0) ? numToStringDecimals : 0);

        // Handle special cases when we have decimals in our value
        if (numDecimals > 0)
        {
            // Truncate the extra digits now, if they're still there
            int decimalPointIndex = retValue.lastIndexOf(decimalPoint);
            if ((decimalPointIndex > 0) && (retValue.size() - (decimalPointIndex + 1)) == numToStringDecimals)
            {
                retValue.resize(retValue.size() - (numToStringDecimals - numDecimals));
            }

            // Remove trailing zeros, since the locale conversion won't do
            // it for us
            QString trailingZeros = QString("%1+$").arg(zeroDigit);
            retValue.remove(QRegExp(trailingZeros));

            // It's possible we could be left with a decimal point on the end
            // if we stripped the trailing zeros, so if that's the case, then
            // add a zero digit on the end so that it is obvious that this is
            // a float value
            if (retValue.endsWith(decimalPoint))
            {
                retValue.append(zeroDigit);
            }
        }

        // Copied from the QDoubleSpinBox sub-class to handle removing the
        // group separator if necessary
        if (!showGroupSeparator && qAbs(value) >= 1000.0)
        {
            retValue.remove(locale.groupSeparator());
        }

        return retValue;
    }
} // namespace AzQtComponents
