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

        // We want to truncate, not round. toString will round, so we add an extra decimal place to the formatting
        // so we can remove the last value
        QString retValue = locale.toString(value, 'f', (numDecimals > 0) ? numDecimals + 1 : 0);

        // Handle special cases when we have decimals in our value
        if (numDecimals > 0)
        {
            // Truncate the extra digit now, if it's still there
            int decimalPointIndex = retValue.lastIndexOf(decimalPoint);
            if ((decimalPointIndex > 0) && (retValue.size() - (decimalPointIndex + 1)) == (numDecimals + 1))
            {
                retValue.resize(retValue.size() - 1);
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
