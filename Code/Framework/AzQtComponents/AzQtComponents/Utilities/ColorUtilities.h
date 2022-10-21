/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    AZ_QT_COMPONENTS_API QString MakePropertyDisplayStringInts(const QColor& color, bool includeAlphaChannel);
    AZ_QT_COMPONENTS_API QString MakePropertyDisplayStringFloats(const QColor& color, bool includeAlphaChannel);
};


