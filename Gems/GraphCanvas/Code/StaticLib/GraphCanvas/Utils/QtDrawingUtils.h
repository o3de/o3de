/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QPainter>
#include <QPixmap>

#include <AzCore/std/string/string_view.h>

#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    namespace Styling
    {
        class StyleHelper;
    }

    class QtDrawingUtils
    {
    public:
        static void GenerateGradients(const AZStd::vector< const Styling::StyleHelper* >& stylingHelpers, const QRectF& area, QLinearGradient& penGradient, QLinearGradient& fillGradient);

        static void FillArea(QPainter& painter, const QRectF& area, const Styling::StyleHelper& styleHelper);

        static void CandyStripeArea(QPainter& painter, const QRectF& area, const CandyStripeConfiguration& candyStripeConfiguration);
        static void PatternFillArea(QPainter& painter, const QRectF& area, const QPixmap& pixmap, const PatternFillConfiguration& patternFillConfiguration);
        static void PatternFillArea(QPainter& painter, const QRectF& area, const PatternedFillGenerator& patternedFillGenerator);
    };
}
