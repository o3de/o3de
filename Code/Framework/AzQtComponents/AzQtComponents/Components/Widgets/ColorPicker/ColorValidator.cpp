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

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorValidator.h>
#include <AzCore/Math/MathUtils.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>

namespace AzQtComponents
{
    bool RGBColorValidator::isValid(const Internal::ColorController* controller)
    {
        const float fullAlpha = 1.0f;
        return qFuzzyCompare(controller->alpha(), fullAlpha);
    }

    void RGBColorValidator::adjust(Internal::ColorController* controller)
    {
        const float fullAlpha = 1.0f;

        controller->setAlpha(fullAlpha);
    }

    void RGBColorValidator::warn()
    {
        Q_EMIT colorWarning(QStringLiteral("The selected color has an alpha setting other than 255/1.0 and will be used with the alpha channel set to 255/1.0"));
    }

    bool RGBALowRangeValidator::isValid(const Internal::ColorController* controller)
    {
        const float maxChannelValue = 1.0f;
        return (controller->red() <= maxChannelValue) && (controller->green() <= maxChannelValue) && (controller->blue() <= maxChannelValue) && (controller->alpha() <= maxChannelValue);
    }

    void RGBALowRangeValidator::adjust(Internal::ColorController* controller)
    {
        const float maxChannelValue = 1.0f;
        AZ::Color adjusted = controller->color();

        if (adjusted.GetR() > maxChannelValue)
        {
            adjusted.SetR(maxChannelValue);
        }
        if (adjusted.GetG() > maxChannelValue)
        {
            adjusted.SetG(maxChannelValue);
        }
        if (adjusted.GetB() > maxChannelValue)
        {
            adjusted.SetB(maxChannelValue);
        }
        if (adjusted.GetA() > maxChannelValue)
        {
            adjusted.SetA(maxChannelValue);
        }

        controller->setColor(adjusted);
    }

    void RGBALowRangeValidator::warn()
    {
        Q_EMIT colorWarning(QStringLiteral("The selected color is in the high dynamic range and will be clamped so that each channel is between 0 and 1"));
    }

    HueSaturationValidator::HueSaturationValidator(float defaultV, QObject* parent)
        : ColorValidator(parent)
        , m_defaultV(defaultV)
    {

    }

    bool HueSaturationValidator::isValid(const Internal::ColorController* controller)
    {
        return qFuzzyCompare(controller->value(), m_defaultV);
    }

    void HueSaturationValidator::adjust(Internal::ColorController* controller)
    {
        controller->setValue(m_defaultV);
    }

    void HueSaturationValidator::warn()
    {
        Q_EMIT colorWarning(QStringLiteral("The selected color has an invalid 'value' channel. It will be clamped to the default value of %1").arg(m_defaultV));
    }
} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_ColorValidator.cpp"