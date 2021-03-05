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

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzCore/Math/Color.h>
#include <QColor>
#include <QString>

class QLocale;

namespace AzQtComponents
{
    AZ_QT_COMPONENTS_API QColor toQColor(const AZ::Color& color);
    AZ_QT_COMPONENTS_API QColor toQColor(float r, float g, float b, float a = 1.0f);

    AZ_QT_COMPONENTS_API AZ::Color fromQColor(const QColor& color);

    AZ_QT_COMPONENTS_API QString toString(double value, int numDecimals, const QLocale& locale, bool showGroupSeparator = false);

    // Maintained for backwards compile compatibility
    inline QColor ToQColor(const AZ::Color& color)
    {
        return toQColor(color);
    }

    // Maintained for backwards compile compatibility
    inline AZ::Color FromQColor(const QColor& color)
    {
        return fromQColor(color);
    }


} // namespace AzQtComponents