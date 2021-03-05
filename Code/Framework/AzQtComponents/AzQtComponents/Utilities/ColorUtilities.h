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
#include <QBrush>


namespace AzQtComponents
{
    AZ_QT_COMPONENTS_API QColor AdjustGamma(const QColor& color, qreal gamma);
    AZ_QT_COMPONENTS_API AZ::Color AdjustGamma(const AZ::Color& color, float gamma);

    /**
     * Make a pixmap brush from the alpha-checking pattern with the color we're interested filled over the top
     * This is useful for drawRoundedRect calls because it prevents anti-aliasing and overlapped drawing issues
     * without losing us anti-aliasing of and corner radius
     */
    AZ_QT_COMPONENTS_API QBrush MakeAlphaBrush(const QColor& color, qreal gamma = 1.0f);
    AZ_QT_COMPONENTS_API QBrush MakeAlphaBrush(const AZ::Color& color, float gamma = 1.0f);

    AZ_QT_COMPONENTS_API bool AreClose(const AZ::Color& left, const AZ::Color& right);
};


